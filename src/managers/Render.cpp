#include "Render.h"

#include "managers/Resources.h"

#include "systems.h"
#include "components.h"
#include "shaders/main.h"
#include "shaders/render_target_to_screen.h"

#include "RenderTools/Pipeline.h"

#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>

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

		class RenderState
		{
			e_Pass m_currentPass{ e_Pass_Count };
			e_PassGlue m_currentPassGlue{ e_PassGlue_Count };
			e_Renderer m_currentRenderer{ e_Renderer_Count };

		public:
			std::array<Pass, e_Pass_Count> m_passes{};
			std::array<PassGlue, e_PassGlue_Count> m_passGlues{};
			std::array<Renderer, e_Renderer_Count> m_renderers{};

			sg_pass_action m_defaultPassAction{};

			void NextPass(e_Pass _pass)
			{
				if (m_currentPass < e_Pass_Count)
				{
					sg_end_pass();
				}

				ASSERT(_pass < e_Pass_Count);

				if (_pass == e_DefaultPass)
				{
					sg_begin_default_pass(m_defaultPassAction, sapp_width(), sapp_height());
					m_currentPass = _pass;
				}
				else
				{
					if (m_passes[_pass].Begin())
					{
						m_currentPass = _pass;
					}
					else
					{
						m_currentPass = e_Pass_Count;
					}

					ASSERT(m_currentPass == _pass);
				}
			}

			void SetPassGlue(e_PassGlue _passGlue)
			{
				ASSERT(_passGlue < e_PassGlue_Count);
				ASSERT(m_currentPass < e_Pass_Count);
				ASSERT(m_currentRenderer < e_Renderer_Count);

				if (m_passGlues[_passGlue].Set(m_currentPass, m_currentRenderer))
				{
					m_currentPassGlue = _passGlue;
				}
				else
				{
					m_currentPassGlue = e_PassGlue_Count;
				}

				ASSERT(m_currentPassGlue == _passGlue);
			}

			void SetBinding(sg_bindings const& _binds)
			{
				ASSERT(m_currentPass < e_Pass_Count);
				ASSERT(m_currentRenderer < e_Renderer_Count);
				ASSERT(m_renderers[m_currentRenderer].CanUseGeneralBindings());

				sg_apply_bindings(_binds);
				m_currentPassGlue = e_PassGlue_Count;
			}

			void SetRenderer(e_Renderer _renderer)
			{
				ASSERT(_renderer < e_Renderer_Count);
				ASSERT(m_currentPass < e_Pass_Count);

				if (m_renderers[_renderer].Activate(m_currentPass))
				{
					m_currentRenderer = _renderer;
				}
				else
				{
					m_currentRenderer = e_Renderer_Count;
				}

				ASSERT(m_currentRenderer == _renderer);
			}

			void Draw(int numElements, int baseElement = 0, int numInstances = 1)
			{
				ASSERT(m_currentPass < e_Pass_Count);
				ASSERT(m_currentRenderer < e_Renderer_Count);

				sg_draw(baseElement, numElements, numInstances);
			}

			void Commit()
			{
				if (m_currentPass < e_Pass_Count)
				{
					sg_end_pass();
				}
				sg_commit();
				m_currentPass = e_Pass_Count;
				m_currentPassGlue = e_PassGlue_Count;
				m_currentRenderer = e_Renderer_Count;
			}

		} state;

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
		}

		//--------------------------------------------------------------------------------
		void InitBuffers()
		{
			// Make passes
			state.m_passes[Pass_MainTarget] = Pass{ RENDER_AREA_WIDTH, RENDER_AREA_HEIGHT, true, 1, "smol" };
			sg_pass_action smolPassAction{};
			sg_color_attachment_action colourAttachAction{
				.action = SG_ACTION_CLEAR,
				.value = { 0.0f, 0.0f, 0.0f, 1.0f, },
			};
			smolPassAction.colors[0] = colourAttachAction;

			state.m_passes[Pass_MainTarget].SetPassAction(smolPassAction);

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
			sg_bindings binds{};
			binds.vertex_buffers[0] = sg_make_buffer(smolBufferDesc);
			binds.fs_images[SLOT_render_target_to_screen_tex] = state.m_passes[Pass_MainTarget].GetColourImage(0);
			state.m_passGlues[PassGlue_MainTarget_To_Screen] = PassGlue{ binds };
			state.m_passGlues[PassGlue_MainTarget_To_Screen].AddValidPass(Pass_RenderToScreen);
			state.m_passGlues[PassGlue_MainTarget_To_Screen].AddValidRenderer(Renderer_TargetToScreen);
		}

		//--------------------------------------------------------------------------------
		void InitShaders()
		{
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

			state.m_renderers[Renderer_Main] = Renderer{ sg_make_pipeline(mainPipeDesc) };
			state.m_renderers[Renderer_Main].AllowGeneralBindings();
			state.m_renderers[Renderer_Main].AddValidPass(Pass_MainTarget);

			sg_layout_desc targetToScreenLayoutDesc{};
			targetToScreenLayoutDesc.attrs[ATTR_render_target_to_screen_vs_aPos].format = SG_VERTEXFORMAT_FLOAT3;
			targetToScreenLayoutDesc.attrs[ATTR_render_target_to_screen_vs_aTexCoord].format = SG_VERTEXFORMAT_FLOAT2;
			targetToScreenLayoutDesc.buffers[0].stride = sizeof(fVec3) + sizeof(fVec2);
			targetToScreenLayoutDesc.attrs[ATTR_render_target_to_screen_vs_aPos].offset = 0;
			targetToScreenLayoutDesc.attrs[ATTR_render_target_to_screen_vs_aTexCoord].offset = sizeof(fVec3);

			sg_pipeline_desc targetToScreenDesc{
				.shader = sg_make_shader(render_target_to_screen_sg_shader_desc(sg_query_backend())),
				.layout = targetToScreenLayoutDesc,
				.depth = {
					.compare = SG_COMPAREFUNC_LESS_EQUAL,
					.write_enabled = true,
				},
				.index_type = SG_INDEXTYPE_NONE,
				.cull_mode = SG_CULLMODE_BACK,
				.label = "smol-pipeline",
			};

			state.m_renderers[Renderer_TargetToScreen] = Renderer{ sg_make_pipeline(targetToScreenDesc) };
			state.m_renderers[Renderer_TargetToScreen].AddValidPass(Pass_RenderToScreen);

			sg_pass_action passAction{};
			sg_color_attachment_action colourAttachAction{};
			colourAttachAction.action = SG_ACTION_CLEAR;
			colourAttachAction.value = { 0.0f, 0.0f, 0.0f, 1.0f, };
			passAction.colors[0] = colourAttachAction;
			state.m_defaultPassAction = passAction;
		}

		//--------------------------------------------------------------------------------
		void InitPipeline()
		{
			InitBuffers();
			InitShaders();
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
			state.NextPass(Pass_MainTarget);
		}

		//--------------------------------------------------------------------------------
		void Render
		(
			Core::Render::FrameData const& _rfd
		)
		{
			// RENDER_PASSES
			state.SetRenderer(Renderer_Main);
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
					state.SetBinding(mesh.m_bindings);
					sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_main_material, SG_RANGE_REF(mesh.m_material));
					state.Draw(mesh.NumToDraw());
				}

			}
			//end foreach

			// temporary light cleanup
			frameScene.lights.Reset();
			frameScene.models.clear();

			// DEFAULT_PASS_END
			Core::Render::TextAndGLDebug::Render();

			// end of the main drawing pass
			// begin of the screen drawing pass
			state.NextPass(Pass_RenderToScreen);
			state.SetRenderer(Renderer_TargetToScreen);
			state.SetPassGlue(PassGlue_MainTarget_To_Screen);

			render_target_to_screen_vs_params_t aspectData{
				.aspectMult = (4.0f / 3.0f) * (_rfd.contextWindow.fH / _rfd.contextWindow.fW),
			};
			sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_render_target_to_screen_vs_params, SG_RANGE_REF(aspectData));
			state.Draw(6);

			Core::Render::DImGui::Render(); // imgui last, always a debug.

			// RENDER_END
			state.Commit();
		}

		//--------------------------------------------------------------------------------
		void Cleanup()
		{
			for (auto& pass : state.m_passes)
			{
				pass = Pass{};
			}
			for (auto& passGlue : state.m_passGlues)
			{
				passGlue = PassGlue{};
			}
			for (auto& renderer : state.m_renderers)
			{
				renderer = Renderer{};
			}
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