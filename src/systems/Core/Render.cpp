#include "Render.h"

#include "SystemOrdering.h"
#include "components.h"

#include "managers/Resources.h"

#include "HandmadeMath.h"
#include "shaders/main.h"

#define SOKOL_IMPL
#if DEBUG_TOOLS
#define SOKOL_TRACE_HOOKS
#endif
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_time.h>

struct
{
	sg_pipeline mainPipeline;
} renderState;

struct
{
	hmm_mat4 proj{ HMM_Perspective(60.0f, 640.0f / 480.0f, 0.01f, 10.0f) };
	hmm_mat4 view{ HMM_LookAt(HMM_Vec3(1.4f, 1.5f, 4.0f), HMM_Vec3(0.0f, 0.0f, 0.0f), HMM_Vec3(0.0f, 1.0f, 0.0f)) };
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
			mainLayoutDesc.attrs[ATTR_main_vs_aNormal].offset = sizeof(fVec3Data);
			mainLayoutDesc.attrs[ATTR_main_vs_aTexCoord].offset = sizeof(fVec3Data) + sizeof(fVec3Data);
#else
			mainLayoutDesc.attrs[ATTR_main_vs_aPos].buffer_index = 0;
			mainLayoutDesc.attrs[ATTR_main_vs_aNormal].buffer_index = 1;
			mainLayoutDesc.attrs[ATTR_main_vs_aTexCoord].buffer_index = 2;
#endif

			sg_pipeline_desc mainPipeDesc{};
			mainPipeDesc.layout = mainLayoutDesc;
			mainPipeDesc.shader = sg_make_shader(main_sg_shader_desc());
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
				cameraState.proj = HMM_Perspective(60.0f, _rfd.fW / _rfd.fH, 0.01f, 1000.0f);

				hmm_mat4 const cameraMat = HMM_Mat4FromfTrans(_t.T());
				cameraState.view = HMM_InverseNoScale(cameraMat);
			});

			ecs::make_system<ecs::opts::group<Sys::DEFAULT_PASS>, ecs::opts::not_parallel>([](Core::MT_Only&, Core::Render::FrameData const& _fd, Core::Render::Model const& _model, Core::Transform const& _t)
			{
				if (_model.m_drawDefaultPass)
				{
					main_vs_params_t vs_params;
					fTrans const worldTransform = _t.CalculateWorldTransform();
					hmm_mat4 const modelMat = HMM_Mat4FromfTrans(worldTransform);
					vs_params.view_model = cameraState.view * modelMat;
					vs_params.normal = HMM_Transpose(HMM_InverseNoScale(vs_params.view_model));
					vs_params.projection = cameraState.proj;


					main_fs_params_t fs_params;
					fs_params.lightColor = fVec3Data(1.0f, 1.0f, 1.0f);
					hmm_vec3 lightPos = HMM_Vec3(1.2f, 1.0f, 2.0f);
					lightPos = (cameraState.view * HMM_Vec4v(lightPos, 1.0f)).XYZ;
					fs_params.lightPos = fVec3Data(lightPos.X, lightPos.Y, lightPos.Z);

					Resource::ModelData const& model = Resource::GetModel(_model.m_modelID);

					sg_apply_pipeline(renderState.mainPipeline);
					sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_main_vs_params, &vs_params, sizeof(vs_params));
					sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_main_fs_params, &fs_params, sizeof(fs_params));

					for (Resource::MeshData const& mesh : model.m_meshes)
					{
						sg_apply_bindings(mesh.m_bindings);
						sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_main_material, &mesh.m_material, sizeof(Resource::MaterialData));
						sg_draw(0, mesh.NumToDraw(), 1);
					}
				}
			});
		}
		void Cleanup()
		{

		}
	}
}