#include "Render.h"

#include "managers/Resources.h"

#include "systems.h"
#include "components.h"
#include "shaders/main.h"
#include "shaders/render_target_to_screen.h"

#define SOKOL_IMPL
#if DEBUG_TOOLS
#define SOKOL_TRACE_HOOKS
#endif
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_time.h>

// classes
namespace Core
{
	namespace Render
	{
		static constexpr int MAX_LIGHTS = 16;

		class LightsState
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

			struct
			{
				sg_image colTarget{};
				sg_image depthTarget{};
				sg_pass pass{};
				sg_pipeline targetToScreen{};
				sg_bindings binds{};
			} smol;

		} renderState;

		struct ModelToDraw
		{
			Resource::ModelID model{};
			fTrans transform{};
		};

		struct FrameScene
		{
			LightsState lights{};
			CameraState camera{};
			std::vector<ModelToDraw> models{};

		};

		FrameScene frameScene{};
		std::mutex frameSceneMutex{};
	}
}

// functions
namespace Core
{
	namespace Render
	{
		//--------------------------------------------------------------------------------
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

			sg_pipeline_desc mainPipeDesc{
				.shader = sg_make_shader(main_sg_shader_desc(sg_query_backend())),
				.layout = mainLayoutDesc,
				.depth = {
					.compare = SG_COMPAREFUNC_LESS_EQUAL,
					.write_enabled = true,
				},
				.index_type = SG_INDEXTYPE_UINT32,
				.cull_mode = SG_CULLMODE_BACK,
				.label = "main-pipeline",
			};

			renderState.mainPipeline = sg_make_pipeline(&mainPipeDesc);

			{
				sg_image_desc smolRenderTargetDesc{
					.type = SG_IMAGETYPE_2D,
					.render_target = true,
					.width = RENDER_AREA_WIDTH,
					.height = RENDER_AREA_HEIGHT,
					.label = "smol-render",
				};
				renderState.smol.colTarget = sg_make_image(smolRenderTargetDesc);

				sg_image_desc smolDepthTargetDesc{
					.type = SG_IMAGETYPE_2D,
					.render_target = true,
					.width = RENDER_AREA_WIDTH,
					.height = RENDER_AREA_HEIGHT,
					.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
					.label = "smol-depth",
				};
				renderState.smol.depthTarget = sg_make_image(smolDepthTargetDesc);

				sg_pass_desc smolPassDesc{
					.depth_stencil_attachment = {.image = renderState.smol.depthTarget, .mip_level = 0 },
					.label = "smol-pass",
				};
				smolPassDesc.color_attachments[0] = { .image = renderState.smol.colTarget, .mip_level = 0 },
					renderState.smol.pass = sg_make_pass(smolPassDesc);

				sg_layout_desc smolLayoutDesc{};
				smolLayoutDesc.attrs[ATTR_render_target_to_screen_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3;
				smolLayoutDesc.attrs[ATTR_render_target_to_screen_vs_aTexCoord].format = SG_VERTEXFORMAT_FLOAT2;
				smolLayoutDesc.buffers[0].stride = sizeof(fVec3) + sizeof(fVec2);
				smolLayoutDesc.attrs[ATTR_render_target_to_screen_vs_aPos].offset = 0;
				smolLayoutDesc.attrs[ATTR_render_target_to_screen_vs_aTexCoord].offset = sizeof(fVec3);

				sg_pipeline_desc smolPipeDesc{
					.shader = sg_make_shader(render_target_to_screen_sg_shader_desc(sg_query_backend())),
					.layout = smolLayoutDesc,
					.depth = {
						.compare = SG_COMPAREFUNC_LESS_EQUAL,
						.write_enabled = true,
					},
					.index_type = SG_INDEXTYPE_NONE,
					.cull_mode = SG_CULLMODE_BACK,
					.label = "smol-pipeline",
				};

				renderState.smol.targetToScreen = sg_make_pipeline(smolPipeDesc);

				float smolBuf[] = {
					-1.0f, -1.0f, 0.0f,		0, 1,
					-1.0f,  1.0f, 0.0f,		0, 0,
					 1.0f,  1.0f, 0.0f,		1, 0,

					 1.0f,  1.0f, 0.0f,		1, 0,
					 1.0f, -1.0f, 0.0f,		1, 1,
					-1.0f, -1.0f, 0.0f,		0, 1,
				};

				sg_buffer_desc smolBufferDesc{
					.type = SG_BUFFERTYPE_VERTEXBUFFER,
					.data = SG_RANGE(smolBuf),
					.label = "smol-buffer",
				};
				renderState.smol.binds.vertex_buffers[0] = sg_make_buffer(smolBufferDesc);
				renderState.smol.binds.fs_images[SLOT_render_target_to_screen_tex] = renderState.smol.colTarget;
			}

		}

		//--------------------------------------------------------------------------------
		void StartPass
		(
			Core::Render::FrameData const& _rfd,
			Core::Render::Camera const& _cam,
			Core::Transform const& _t
		)
		{
			// DEFAULT_PASS_START
			frameScene.camera.proj = glm::perspective(glm::radians(_cam.m_povY), _rfd.renderArea.fW / _rfd.renderArea.fH, 0.01f, 1000.0f);

			fTrans const cameraTrans = _t.CalculateWorldTransform();
			fMat4 const cameraMat = glm::lookAt(cameraTrans.m_origin, cameraTrans.m_origin + cameraTrans.forward(), fVec3(0.0f, 1.0f, 0.0f));
			frameScene.camera.view = cameraMat;

			// DEFAULT_PASS_START
			sg_pass_action pass_action{};
			sg_color_attachment_action color_attach_action{};
			color_attach_action.action = SG_ACTION_CLEAR;
			color_attach_action.value = { 0.0f, 0.0f, 0.0f, 1.0f, };
			pass_action.colors[0] = color_attach_action;

			sg_begin_pass(renderState.smol.pass, pass_action);

		}

		//--------------------------------------------------------------------------------
		void Render
		(
			Core::Render::FrameData const& _rfd
		)
		{
			// RENDER_PASSES
			sg_apply_pipeline(renderState.mainPipeline);
			sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_main_lights, SG_RANGE_REF(frameScene.lights.shader_LightData()));

			main_vs_params_t vs_params;
			vs_params.projection = frameScene.camera.proj;
			for (ModelToDraw const& mtd : frameScene.models)
			{
				fMat4 const modelMat = mtd.transform.GetRenderMatrix();
				vs_params.view_model = frameScene.camera.view * modelMat;
				vs_params.normal = glm::transpose(glm::inverse(vs_params.view_model));

				Resource::ModelData const& model = Resource::GetModel(mtd.model);

				sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_main_vs_params, SG_RANGE_REF(vs_params));

				for (Resource::MeshData const& mesh : model.m_meshes)
				{
					sg_apply_bindings(mesh.m_bindings);
					sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_main_material, SG_RANGE_REF(mesh.m_material));
					sg_draw(0, mesh.NumToDraw(), 1);
				}

			}
			//end foreach

			// temporary light cleanup
			frameScene.lights.Reset();
			frameScene.models.clear();

			// DEFAULT_PASS_END
			Core::Render::TextAndGLDebug::Render();

			sg_end_pass();

			// end of the main drawing pass
			// begin of the screen drawing pass

			sg_pass_action pass_action{};
			sg_color_attachment_action color_attach_action{};
			color_attach_action.action = SG_ACTION_CLEAR;
			color_attach_action.value = { 0.0f, 0.0f, 0.0f, 1.0f, };
			pass_action.colors[0] = color_attach_action;

			sg_begin_default_pass(pass_action, sapp_width(), sapp_height());

			sg_apply_pipeline(renderState.smol.targetToScreen);
			sg_apply_bindings(renderState.smol.binds);
			render_target_to_screen_vs_params_t aspectData{
				.aspectMult = (4.0f / 3.0f) * (_rfd.contextWindow.fH / _rfd.contextWindow.fW),
			};
			sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_render_target_to_screen_vs_params, SG_RANGE_REF(aspectData));
			sg_draw(0, 6, 1);

			Core::Render::DImGui::Render(); // imgui last, always a debug.
			sg_end_pass();

			// RENDER_END
			sg_commit();
		}

		//--------------------------------------------------------------------------------
		void Cleanup()
		{
			sg_shutdown();
		}

		//--------------------------------------------------------------------------------
		CameraState const& GetCameraState()
		{
			return frameScene.camera;
		}

		//--------------------------------------------------------------------------------
		LightSetter AddLightToScene()
		{
			std::scoped_lock lock(frameSceneMutex);
			return frameScene.lights.AddLight();
		}

		//--------------------------------------------------------------------------------
		void AddAmbientLightToScene(fVec3 const& _col)
		{
			std::scoped_lock lock(frameSceneMutex);
			frameScene.lights.ambientLight += _col;
		}

		//--------------------------------------------------------------------------------
		void AddModelToScene
		(
			Core::Resource::ModelID const& _model,
			fTrans const& _worldTrans
		)
		{
			std::scoped_lock lock(frameSceneMutex);
			frameScene.models.push_back({ _model, _worldTrans });
		}
	}
}