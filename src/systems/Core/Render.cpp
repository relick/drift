#include "Render.h"

#include "SystemOrdering.h"
#include "components.h"

#include "managers/Resources.h"

#include "shaders/main.h"

#define SOKOL_IMPL
#if DEBUG_TOOLS
#define SOKOL_TRACE_HOOKS
#endif
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_time.h>

static constexpr int MAX_LIGHTS = 16;

struct LightSetter
{
	fVec4& Col;
	fVec4& Pos;
	fVec4& Att;
	fVec4& Dir;
	fVec4& Cut;
};

class PassData
{
	main_lights_t lightData{};
	usize numLights{ 0 };

public:
	fVec3& ambientLight{ lightData.ambient };
	LightSetter AddLight()
	{
		usize thisLightI = numLights;
		if (numLights < MAX_LIGHTS)
		{
			numLights++;
			lightData.numLights = static_cast<float>(numLights);
		}
		else
		{
			ASSERT(false); // run out of lights!!
		}

		return LightSetter{ lightData.Col[thisLightI], lightData.Pos[thisLightI], lightData.Att[thisLightI], lightData.Dir[thisLightI], lightData.Cut[thisLightI], };
	}
	void Reset()
	{
		numLights = 0;
		lightData.ambient = fVec3(0.0f, 0.0f, 0.0f);
	}
	main_lights_t const& shader_LightData() { return lightData; }
};

struct
{
	sg_pipeline mainPipeline{};
	PassData defaultPass{};
} renderState;

struct
{
	fMat4 proj{ glm::perspective(glm::radians(60.0f), 640.0f / 480.0f, 0.01f, 10.0f) };
	fMat4 view{ glm::lookAt(fVec3(1.4f, 1.5f, 4.0f), fVec3(0.0f, 0.0f, 0.0f), fVec3(0.0f, 1.0f, 0.0f)) };
} cameraState;

namespace Core
{
	namespace Render
	{
		void Init()
		{
			sg_desc gfxDesc{};
			gfxDesc.context = sapp_sgcontext();
			gfxDesc.buffer_pool_size = 512; // buff it up? // could go muuuuuch higher
			sg_setup(&gfxDesc);

			/* create main pipeline object */

			// deinterleaved data in separate buffers.
			sg_layout_desc mainLayoutDesc{};
			mainLayoutDesc.attrs[ATTR_main_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3;
			mainLayoutDesc.attrs[ATTR_main_vs_aNormal].format = SG_VERTEXFORMAT_FLOAT3;
			mainLayoutDesc.attrs[ATTR_main_vs_aTexCoord].format = SG_VERTEXFORMAT_FLOAT2;
#if USE_INTERLEAVED
			mainLayoutDesc.buffers[0].stride = sizeof(Resource::VertexData);
			mainLayoutDesc.attrs[ATTR_main_vs_aPos].offset = 0;
			mainLayoutDesc.attrs[ATTR_main_vs_aNormal].offset = sizeof(fVec3);
			mainLayoutDesc.attrs[ATTR_main_vs_aTexCoord].offset = sizeof(fVec3) + sizeof(fVec3);
#else
			mainLayoutDesc.attrs[ATTR_main_vs_aPos].buffer_index = 0;
			mainLayoutDesc.attrs[ATTR_main_vs_aNormal].buffer_index = 1;
			mainLayoutDesc.attrs[ATTR_main_vs_aTexCoord].buffer_index = 2;
#endif

			sg_buffer_desc uboDesc{};
			uboDesc.type = SG_BUFFERTYPE_INDEXBUFFER;

			sg_pipeline_desc mainPipeDesc{};
			mainPipeDesc.layout = mainLayoutDesc;
			sg_shader_desc mainShaderDesc{};
			{
				mainShaderDesc = *main_sg_shader_desc();
			}
			mainPipeDesc.shader = sg_make_shader(mainShaderDesc);
			mainPipeDesc.index_type = SG_INDEXTYPE_UINT32;
			mainPipeDesc.depth_stencil = {
				.depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL,
				.depth_write_enabled = true,
			};
			mainPipeDesc.rasterizer.cull_mode = SG_CULLMODE_BACK;
			mainPipeDesc.label = "main-pipeline";

			renderState.mainPipeline = sg_make_pipeline(&mainPipeDesc);
		}

		void Setup()
		{
			ecs::make_system<ecs::opts::group<Sys::DEFAULT_PASS_START>>([](Core::MT_Only&, Core::Render::FrameData const& _rfd, Core::Render::Camera const& _cam, Core::Transform const& _t, Core::Render::DefaultPass_Tag)
			{
				cameraState.proj = glm::perspective(glm::radians(_cam.m_povY), _rfd.fW / _rfd.fH, 0.01f, 1000.0f);

				fTrans const cameraTrans = _t.CalculateWorldTransform();
				fMat4 const cameraMat = glm::lookAt(cameraTrans.m_origin, cameraTrans.m_origin + cameraTrans.forward(), fVec3(0.0f, 1.0f, 0.0f));
				cameraState.view = cameraMat;
			});

			ecs::make_system<ecs::opts::group<Sys::RENDER_ADD>, ecs::opts::not_parallel>([](Core::MT_Only&, Core::Render::FrameData const& _fd, Core::Render::Light const& _light, Core::Transform const& _t)
			{
				switch (_light.m_type)
				{
				case Light::Type::Ambient:
				{
					renderState.defaultPass.ambientLight += (_light.m_colour * _light.m_intensity);
					break;
				}
				case Light::Type::Directional:
				{
					LightSetter lightSetter = renderState.defaultPass.AddLight();
					lightSetter.Col = fVec4(_light.m_colour, _light.m_intensity);

					fTrans const worldT = _t.CalculateWorldTransform();
					fMat3 const& camBasis = worldT.m_basis;
					fVec3 const& front = camBasis[2]; // get z, is front vec/light dir.
					fVec3 const lightDir = glm::normalize(fVec3((cameraState.view * fVec4(front, 0.0f)).xyz));
					lightSetter.Pos = fVec4(-lightDir, 0.0f);
					break;
				}
				case Light::Type::Point:
				{
					LightSetter lightSetter = renderState.defaultPass.AddLight();
					lightSetter.Col = fVec4(_light.m_colour, _light.m_intensity);

					fTrans const worldT = _t.CalculateWorldTransform();
					fVec3 const lightPos = (cameraState.view * fVec4(worldT.m_origin, 1.0f)).xyz;
					lightSetter.Pos = fVec4(lightPos, 1.0f);

					// set dir and cutoff values that allow for omnidirectional lighting.
					lightSetter.Dir = fVec4(0.0f, 0.0f, 0.0f, 0.0f);
					lightSetter.Cut = fVec4(0.0f, -1.1f, 0.0f, 0.0f); // all cosine values are greater than this.

					lightSetter.Att = fVec4(_light.m_attenuation, 0.0f);
					break;
				}
				case Light::Type::Spotlight:
				{
					LightSetter lightSetter = renderState.defaultPass.AddLight();
					lightSetter.Col = fVec4(_light.m_colour, _light.m_intensity);

					fTrans const worldT = _t.CalculateWorldTransform();
					fVec3 const lightPos = (cameraState.view * fVec4(worldT.m_origin, 1.0f)).xyz;
					lightSetter.Pos = fVec4(lightPos, 1.0f);

					fMat3 const& camBasis = worldT.m_basis;
					fVec3 const& front = camBasis[2]; // get z, is front vec/light dir.
					fVec3 const lightDir = glm::normalize(fVec3((cameraState.view * fVec4(front, 0.0f)).xyz));
					lightSetter.Dir = fVec4(-lightDir, 0.0f);

					lightSetter.Cut = fVec4(_light.m_cutoffAngle, _light.m_outerCutoffAngle, 0.0f, 0.0f);

					lightSetter.Att = fVec4(_light.m_attenuation, 0.0f);
					break;
				}
				}
			});

			ecs::make_system<ecs::opts::group<Sys::RENDER_PASSES>, ecs::opts::not_parallel>([](Core::MT_Only&, Core::Render::FrameData const& _fd, Core::Render::Model const& _model, Core::Transform const& _t)
			{
				if (_model.m_drawDefaultPass)
				{
					main_vs_params_t vs_params;
					fTrans const worldTransform = _t.CalculateWorldTransform();
					fMat4 const modelMat = worldTransform.GetRenderMatrix();
					vs_params.view_model = cameraState.view * modelMat;
					vs_params.normal = glm::transpose(glm::inverse(vs_params.view_model));
					vs_params.projection = cameraState.proj;

					Resource::ModelData const& model = Resource::GetModel(_model.m_modelID);

					sg_apply_pipeline(renderState.mainPipeline);
					sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_main_vs_params, &vs_params, sizeof(vs_params));
					sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_main_lights, &renderState.defaultPass.shader_LightData(), sizeof(main_lights_t));

					for (Resource::MeshData const& mesh : model.m_meshes)
					{
						sg_apply_bindings(mesh.m_bindings);
						sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_main_material, &mesh.m_material, sizeof(Resource::MaterialData));
						sg_draw(0, mesh.NumToDraw(), 1);
					}
				}
			});

			// temporary light cleanup
			ecs::make_system<ecs::opts::group<Sys::DEFAULT_PASS_END>, ecs::opts::not_parallel>([](Core::MT_Only&, Core::Render::DefaultPass_Tag)
			{
				renderState.defaultPass.Reset();
			});
		}
		void Cleanup()
		{

		}
	}
}