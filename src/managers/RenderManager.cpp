#include "RenderManager.h"

#include "managers/ResourceManager.h"

#include "systems.h"
#include "components.h"
#include "shaders/main.h"
#include "shaders/render_target_to_screen.h"
#include "shaders/depth_only.h"
#include "shaders/skybox.h"

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
			fVec3 directionalDir{};
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
					kaError("ran out of lights");
				}

				return LightSetter{ lightData.Col[thisLightI], lightData.Pos[thisLightI], lightData.Att[thisLightI], lightData.Dir[thisLightI], lightData.Cut[thisLightI], };
			}
			void Reset()
			{
				numLights = 0;
				lightData.ambient = {};
				directionalDir = {};
			}
			main_lights_t const& shader_LightData() { return lightData; }
		};

		class RenderStateFiller;

		class RenderState
		{
			friend RenderStateFiller;

			e_Pass m_currentPass{ e_Pass_Count };
			e_PassGlue m_currentPassGlue{ e_PassGlue_Count };
			e_Renderer m_currentRenderer{ e_Renderer_Count };
			int m_storedBindingNumToDraw{ 0 };

			std::array<std::optional<Pass>, e_Pass_Count> m_passes{};
			std::array<std::optional<PassGlue>, e_PassGlue_Count> m_passGlues{};
			std::array<std::optional<Renderer>, e_Renderer_Count> m_renderers{};

			sg_pass_action m_defaultPassAction{};

		public:
			void NextPass(e_Pass _pass)
			{
				if (m_currentPass < e_Pass_Count)
				{
					sg_end_pass();
				}

				kaAssert(_pass < e_Pass_Count);

				if (_pass == e_DefaultPass)
				{
					sg_begin_default_pass(m_defaultPassAction, sapp_width(), sapp_height());
					m_currentPass = _pass;
				}
				else
				{
					kaAssert(m_passes[_pass].has_value());
					if (m_passes[_pass]->Begin())
					{
						m_currentPass = _pass;
					}
					else
					{
						m_currentPass = e_Pass_Count;
					}

					kaAssert(m_currentPass == _pass);
				}
			}

			void SetPassGlue(e_PassGlue _passGlue)
			{
				kaAssert(_passGlue < e_PassGlue_Count);
				kaAssert(m_currentPass < e_Pass_Count);
				kaAssert(m_currentRenderer < e_Renderer_Count);

				kaAssert(m_passGlues[_passGlue].has_value());
				if (m_passGlues[_passGlue]->Set(m_currentPass, m_currentRenderer))
				{
					m_currentPassGlue = _passGlue;
				}
				else
				{
					m_currentPassGlue = e_PassGlue_Count;
				}
				m_storedBindingNumToDraw = 0;

				kaAssert(m_currentPassGlue == _passGlue);
			}

			void SetBinding(sg_bindings const& _binds, int _numToDraw)
			{
				kaAssert(m_currentPass < e_Pass_Count);
				kaAssert(m_currentRenderer < e_Renderer_Count);
				kaAssert(m_renderers[m_currentRenderer]->CanUseGeneralBindings());

				sg_apply_bindings(_binds);
				m_currentPassGlue = e_PassGlue_Count;
				m_storedBindingNumToDraw = _numToDraw;
			}

			void SetRenderer(e_Renderer _renderer)
			{
				kaAssert(_renderer < e_Renderer_Count);
				kaAssert(m_currentPass < e_Pass_Count);

				kaAssert(m_renderers[_renderer].has_value());
				if (m_renderers[_renderer]->Activate(m_currentPass))
				{
					m_currentRenderer = _renderer;
				}
				else
				{
					m_currentRenderer = e_Renderer_Count;
				}

				kaAssert(m_currentRenderer == _renderer);
			}

			void Draw(int numElements, int baseElement, int numInstances)
			{
				kaAssert(m_currentPass < e_Pass_Count);
				kaAssert(m_currentRenderer < e_Renderer_Count);
				kaAssert(numElements > 0);

				sg_draw(baseElement, numElements, numInstances);
			}

			void Draw(int baseElement = 0, int numInstances = 1)
			{
				if (m_currentPassGlue < e_PassGlue_Count)
				{
					Draw(m_passGlues[m_currentPassGlue]->NumToDraw(), baseElement, numInstances);
				}
				else
				{
					Draw(m_storedBindingNumToDraw, baseElement, numInstances);
				}
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

			void Cleanup()
			{
				for (auto& pass : m_passes)
				{
					pass = {};
				}
				for (auto& passGlue : m_passGlues)
				{
					passGlue = {};
				}
				for (auto& renderer : m_renderers)
				{
					renderer = {};
				}
			}
		} state;

		class RenderStateFiller
		{
			RenderState& m_state;
		public:
			RenderStateFiller(RenderState& _state)
				: m_state{ _state }
			{}

			std::optional<Pass>& Pass(e_Pass _pass) { return m_state.m_passes[_pass]; }
			std::optional<PassGlue>& PassGlue(e_PassGlue _passGlue) { return m_state.m_passGlues[_passGlue]; }
			std::optional<Renderer>& Renderer(e_Renderer _renderer) { return m_state.m_renderers[_renderer]; }
			void SetDefaultPassAction(sg_pass_action const& _action) { m_state.m_defaultPassAction = _action; }
		};

		struct ModelToDraw
		{
			Resource::ModelID m_model{};
			fTrans m_transform{};

			ModelToDraw(Resource::ModelID _model, fTrans const& _trans)
				: m_model{ _model }
				, m_transform{ _trans }
			{}
		};

		struct ModelScratchData
		{
			Resource::ModelData const& m_model;
			fMat4 const m_renderMatrix;

			ModelScratchData(Resource::ModelData const& _model, fMat4 const& _renderMatrix)
				: m_model{ _model }
				, m_renderMatrix{ _renderMatrix }
			{}
		};

		struct SpriteToDraw
		{
			Resource::SpriteID m_sprite{};
			fTrans m_transform{};

			SpriteToDraw(Resource::SpriteID _sprite, fTrans const& _trans)
				: m_sprite{ _sprite }
				, m_transform{ _trans }
			{}
		};

		struct FrameScene
		{
			LightsState lights{};
			CameraState camera{};
			std::vector<ModelToDraw> models;
			std::vector<ModelScratchData> modelScratchData;
			std::vector<SpriteToDraw> sprites;
			Resource::TextureID skybox{};
			sg_bindings skyboxBinds{};
			std::mutex lightsMutex{};
			std::mutex modelsMutex{};
			std::mutex spritesMutex{};
		};

		FrameScene frameScene{};
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

		sg_image shadowMap{};

		//--------------------------------------------------------------------------------
		void InitBuffers
		(
			int _mainRenderWidth,
			int _mainRenderHeight,
			RenderStateFiller& io_state
		)
		{
			// Main target
			{
				io_state.Pass(Pass_MainTarget) = Pass{ _mainRenderWidth, _mainRenderHeight, { .use_depth = true }, 1, "mainTarget" };
				sg_pass_action passAction{};
				sg_color_attachment_action colourAttachAction{
					.action = SG_ACTION_CLEAR,
					.value = { 0.0f, 0.0f, 0.0f, 1.0f, },
				};
				passAction.colors[0] = colourAttachAction;
				io_state.Pass(Pass_MainTarget)->SetPassAction(passAction);
			}

			// Directional light
			{
				// directional lighting depth buffer
				io_state.Pass(Pass_DirectionalLight) = Pass{ 2048, 2048, {.use_depth = true, .sampled = true, .linear_filter = true,}, 0, "directionalLight" };
				sg_pass_action passAction{
					.depth = {
						.action = SG_ACTION_CLEAR,
						.value = 1.0f,
					},
				};
				io_state.Pass(Pass_DirectionalLight)->SetPassAction(passAction);
				shadowMap = io_state.Pass(Pass_DirectionalLight)->GetDepthImage();
			}

			// Target to screen glue
			{
				// in triangle-strip form
				float rectangleWithUV[] = {
					-1.0f,  1.0f, 0.0f,		0, 0,
					 1.0f,  1.0f, 0.0f,		1, 0,
					-1.0f, -1.0f, 0.0f,		0, 1,
					 1.0f, -1.0f, 0.0f,		1, 1,
				};

				sg_buffer_desc rectangleWithUVBufferDesc{
					.type = SG_BUFFERTYPE_VERTEXBUFFER,
					.data = SG_RANGE(rectangleWithUV),
					.label = "rectangleWithUV-buffer",
				};
				sg_bindings binds{};
				binds.vertex_buffers[0] = sg_make_buffer(rectangleWithUVBufferDesc);
				binds.fs_images[SLOT_render_target_to_screen_tex] = io_state.Pass(Pass_MainTarget)->GetColourImage(0);
				io_state.PassGlue(PassGlue_MainTarget_To_Screen) = PassGlue{ binds, 4 };
				io_state.PassGlue(PassGlue_MainTarget_To_Screen)->AddValidPass(Pass_RenderToScreen);
				io_state.PassGlue(PassGlue_MainTarget_To_Screen)->AddValidRenderer(Renderer_TargetToScreen);
			}

			{
				// in triangle-strip form
				float skyboxCube[] = {
					 1.0f,  1.0f, -1.0f, // front-top-right
					-1.0f,  1.0f, -1.0f, // front-top-left
					 1.0f,  1.0f,  1.0f, // back-top-right
					-1.0f,  1.0f,  1.0f, // back-top-left
					-1.0f, -1.0f,  1.0f, // back-bottom-left
					-1.0f,  1.0f, -1.0f, // front-top-left
					-1.0f, -1.0f, -1.0f, // front-bottom-left
					 1.0f,  1.0f, -1.0f, // front-top-right
					 1.0f, -1.0f, -1.0f, // front-bottom-right
					 1.0f,  1.0f,  1.0f, // back-top-right
					 1.0f, -1.0f,  1.0f, // back-bottom-right
					-1.0f, -1.0f,  1.0f, // back-bottom-left
					 1.0f, -1.0f, -1.0f, // front-bottom-right
					-1.0f, -1.0f, -1.0f, // front-bottom-left
				};

				sg_buffer_desc skyboxCubeDesc{
					.type = SG_BUFFERTYPE_VERTEXBUFFER,
					.data = SG_RANGE(skyboxCube),
					.label = "skyboxCube-buffer",
				};
				frameScene.skyboxBinds.vertex_buffers[0] = sg_make_buffer(skyboxCubeDesc);
			}
		}

		//--------------------------------------------------------------------------------
		void InitShaders
		(
			RenderStateFiller& io_state
		)
		{
			// default pass action: clear to black
			sg_pass_action passAction{};
			sg_color_attachment_action colourAttachAction{};
			colourAttachAction.action = SG_ACTION_CLEAR;
			colourAttachAction.value = { 0.0f, 0.0f, 0.0f, 1.0f, };
			passAction.colors[0] = colourAttachAction;
			io_state.SetDefaultPassAction(passAction);

			// main renderer, used for colour image, done last
			{
				sg_layout_desc mainLayoutDesc{};
				mainLayoutDesc.attrs[ATTR_main_vs_aPos] = {
					.offset = 0,
					.format = SG_VERTEXFORMAT_FLOAT3,
				};
				mainLayoutDesc.attrs[ATTR_main_vs_aNormal] = {
					.offset = sizeof(fVec3),
					.format = SG_VERTEXFORMAT_FLOAT3,
				};
				mainLayoutDesc.attrs[ATTR_main_vs_aTexCoord] = {
					.offset = sizeof(fVec3) + sizeof(fVec3),
					.format = SG_VERTEXFORMAT_FLOAT2,
				};
				mainLayoutDesc.attrs[ATTR_main_vs_aTangent] = {
					.offset = sizeof(fVec3) + sizeof(fVec3) + sizeof(fVec2),
					.format = SG_VERTEXFORMAT_FLOAT3,
				};
				mainLayoutDesc.buffers[0].stride = sizeof(Resource::VertexData);

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

				io_state.Renderer(Renderer_Main) = Renderer{ sg_make_pipeline(mainPipeDesc) };
				io_state.Renderer(Renderer_Main)->AllowGeneralBindings();
				io_state.Renderer(Renderer_Main)->AddValidPass(Pass_MainTarget);
			}

			// target to screen renderer, all it does is put a texture on the screen
			{
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
						.compare = SG_COMPAREFUNC_LESS,
						.write_enabled = true,
					},
					.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
					.index_type = SG_INDEXTYPE_NONE,
					.cull_mode = SG_CULLMODE_BACK,
					.label = "target-to-screen-pipeline",
				};

				io_state.Renderer(Renderer_TargetToScreen) = Renderer{ sg_make_pipeline(targetToScreenDesc) };
				io_state.Renderer(Renderer_TargetToScreen)->AddValidPass(Pass_RenderToScreen);
			}

			// depth only renderer, used for fast shadow mapping
			{
				sg_layout_desc depthOnlyLayoutDesc{};
				depthOnlyLayoutDesc.attrs[ATTR_depth_only_vs_aPos] = {
					.offset = 0,
					.format = SG_VERTEXFORMAT_FLOAT3,
				};
				depthOnlyLayoutDesc.buffers[0].stride = sizeof(Resource::VertexData);

				sg_pipeline_desc depthOnlyDesc{
					.shader = sg_make_shader(depth_only_sg_shader_desc(sg_query_backend())),
					.layout = depthOnlyLayoutDesc,
					.depth = {
						.pixel_format = SG_PIXELFORMAT_DEPTH_STENCIL,
						.compare = SG_COMPAREFUNC_LESS,
						.write_enabled = true,
					},
					.no_color = true,
					.index_type = SG_INDEXTYPE_UINT32,
					.cull_mode = SG_CULLMODE_FRONT,
					.label = "depth-only-pipeline",
				};

				io_state.Renderer(Renderer_DepthOnly) = Renderer{ sg_make_pipeline(depthOnlyDesc) };
				io_state.Renderer(Renderer_DepthOnly)->AllowGeneralBindings();
				io_state.Renderer(Renderer_DepthOnly)->AddValidPass(Pass_DirectionalLight);
			}

			// skybox renderer
			{
				sg_layout_desc skyboxLayoutDesc{};
				skyboxLayoutDesc.attrs[ATTR_skybox_vs_aPos] = {
					.offset = 0,
					.format = SG_VERTEXFORMAT_FLOAT3,
				};
				skyboxLayoutDesc.buffers[0].stride = sizeof(fVec3);

				sg_pipeline_desc skyboxDesc{
					.shader = sg_make_shader(skybox_sg_shader_desc(sg_query_backend())),
					.layout = skyboxLayoutDesc,
					.depth = {
						.compare = SG_COMPAREFUNC_LESS_EQUAL,
						.write_enabled = true,
					},
					.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
					.cull_mode = SG_CULLMODE_BACK,
					.label = "skybox-pipeline",
				};

				io_state.Renderer(Renderer_Skybox) = Renderer{ sg_make_pipeline(skyboxDesc) };
				io_state.Renderer(Renderer_Skybox)->AddValidPass(Pass_MainTarget);
				io_state.Renderer(Renderer_Skybox)->AllowGeneralBindings();
			}
		}

		//--------------------------------------------------------------------------------
		void SetupPipeline
		(
			int _mainRenderWidth,
			int _mainRenderHeight
		)
		{
			RenderStateFiller filler{ state };
			InitBuffers(_mainRenderWidth, _mainRenderHeight, filler);
			InitShaders(filler);
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
			frameScene.camera.pos = cameraTrans.m_origin;
			fMat4 const cameraMat = glm::lookAt(cameraTrans.m_origin, cameraTrans.m_origin + cameraTrans.forward(), fVec3(0.0f, 1.0f, 0.0f));
			frameScene.camera.view = cameraMat;

			// DEFAULT_PASS_START
			state.NextPass(Pass_MainTarget);
		}

		//--------------------------------------------------------------------------------
		template<typename T_ModelVisitor, typename T_MeshVisitor>
		void RenderMainScene(T_ModelVisitor const& _fnModelVisitor, T_MeshVisitor const& _fnMeshVisitor)
		{
			for (ModelScratchData const& mtd : frameScene.modelScratchData)
			{
				_fnModelVisitor(mtd.m_renderMatrix, mtd.m_model);

				for (Resource::MeshData const& mesh : mtd.m_model.m_meshes)
				{
					_fnMeshVisitor(mesh);

					state.Draw();
				}

			}
		}

		//--------------------------------------------------------------------------------
		fMat4 GetDirectionalLightOrthoMat(float _bounds, float _nearPlane, float _farPlane)
		{
			// d3d needs off-centre, gl needs usual.
#if SOKOL_D3D11
			return glm::orthoRH_ZO(-_bounds, _bounds, -_bounds, _bounds, _nearPlane, _farPlane);
#else
			return glm::orthoRH_NO(-_bounds, _bounds, -_bounds, _bounds, _nearPlane, _farPlane);
#endif	
		}

		//--------------------------------------------------------------------------------
		void Render
		(
			Core::Render::FrameData const& _rfd
		)
		{
			frameScene.modelScratchData.clear();
			frameScene.modelScratchData.reserve(frameScene.models.size());
			for (ModelToDraw const& mtd : frameScene.models)
			{
				frameScene.modelScratchData.emplace_back(Resource::GetModel(mtd.m_model), mtd.m_transform.GetRenderMatrix());
			}

			// RENDER_PASSES
			state.NextPass(Pass_DirectionalLight);
			state.SetRenderer(Renderer_DepthOnly);

			fMat4 const lightProj = GetDirectionalLightOrthoMat(10.0f, 1.0f, 50.0f);
			fVec3 const lightPos = frameScene.camera.pos - (frameScene.lights.directionalDir * 25.0f);
			fMat4 const lightView = glm::lookAt(lightPos, lightPos + frameScene.lights.directionalDir, fVec3(0.0f, 1.0f, 0.0f));
			fMat4 const lightSpace = lightProj * lightView;

			auto fnLightModelVisitor = [&lightSpace](fMat4 const& _modelMatrix, Resource::ModelData const& _model)
			{
				depth_only_vs_params_t vs_params = {
					.projViewModel = lightSpace * _modelMatrix,
				};

				sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_depth_only_vs_params, SG_RANGE_REF(vs_params));
			};

			auto fnLightMeshVisitor = [](Resource::MeshData const& _mesh)
			{
				sg_bindings bufOnlyBinds = _mesh.m_bindings;
				std::memset(&bufOnlyBinds.vs_images, 0, sizeof(bufOnlyBinds.vs_images));
				std::memset(&bufOnlyBinds.fs_images, 0, sizeof(bufOnlyBinds.fs_images));
				state.SetBinding(bufOnlyBinds, _mesh.NumToDraw());
			};

			RenderMainScene(fnLightModelVisitor, fnLightMeshVisitor);

			state.NextPass(Pass_MainTarget);
			state.SetRenderer(Renderer_Main);
			sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_main_lights, SG_RANGE_REF(frameScene.lights.shader_LightData()));

			main_vs_params_t vs_params = {
				.projection = frameScene.camera.proj,
			};
			sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_main_vs_params, SG_RANGE_REF(vs_params));

			auto fnMainModelVisitor = [&lightSpace](fMat4 const& _modelMatrix, Resource::ModelData const& _model)
			{
				main_model_params_t model_params = {
					.viewModel = frameScene.camera.view * _modelMatrix,
					.normal = glm::transpose(glm::inverse(model_params.viewModel)),
					.lightSpace = lightSpace * _modelMatrix,
				};

				sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_main_model_params, SG_RANGE_REF(model_params));
			};

			auto fnMainMeshVisitor = [](Resource::MeshData const& _mesh)
			{
				sg_bindings addShadowBinds = _mesh.m_bindings;
				addShadowBinds.fs_images[SLOT_main_directionalShadowMap] = shadowMap;
				state.SetBinding(addShadowBinds, _mesh.NumToDraw());
				sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_main_material, SG_RANGE_REF(_mesh.m_material));
			};

			RenderMainScene(fnMainModelVisitor, fnMainMeshVisitor);

			// render skybox (if exists)
			if (frameScene.skybox.IsValid())
			{
				state.SetRenderer(Renderer_Skybox);
				skybox_vs_params_t vs_params{
					.untranslated_projView = frameScene.camera.proj * fMat4(fMat3(frameScene.camera.view)),
				};
				sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_skybox_vs_params, SG_RANGE_REF(vs_params));

				skybox_fs_params_t fs_params{
					.sunDir = frameScene.lights.directionalDir,
				};
				sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_skybox_fs_params, SG_RANGE_REF(fs_params));

				Resource::TextureData const& skyboxTex = Resource::GetTexture(frameScene.skybox);
				frameScene.skyboxBinds.fs_images[SLOT_skybox_skybox] = skyboxTex.m_texID;

				state.SetBinding(frameScene.skyboxBinds, 14);
				state.Draw();

				frameScene.skybox = Resource::TextureID{};
			}

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
			state.Draw();

			Core::Render::DImGui::Render(); // imgui last, always a debug.

			// RENDER_END
			state.Commit();
		}

		//--------------------------------------------------------------------------------
		void Cleanup()
		{
			state.Cleanup();
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
			std::scoped_lock lock(frameScene.lightsMutex);
			return frameScene.lights.AddLight();
		}

		//--------------------------------------------------------------------------------
		void AddAmbientLightToScene(fVec3 const& _col)
		{
			std::scoped_lock lock(frameScene.lightsMutex);
			frameScene.lights.ambientLight += _col;
		}

		//--------------------------------------------------------------------------------
		void SetDirectionalLightDir(fVec3 const& _dir)
		{
			std::scoped_lock lock(frameScene.lightsMutex);
			frameScene.lights.directionalDir = _dir;
		}

		//--------------------------------------------------------------------------------
		void AddSpriteToScene
		(
			Core::Resource::SpriteID _sprite,
			fTrans const& _screenTrans
		)
		{
			std::scoped_lock lock(frameScene.spritesMutex);
			frameScene.sprites.emplace_back(_sprite, _screenTrans);
		}

		//--------------------------------------------------------------------------------
		void AddModelToScene
		(
			Core::Resource::ModelID _model,
			fTrans const& _worldTrans
		)
		{
			std::scoped_lock lock(frameScene.modelsMutex);
			frameScene.models.emplace_back(_model, _worldTrans);
		}

		//--------------------------------------------------------------------------------
		void AddSkyboxToScene
		(
			Core::Resource::TextureID _skybox
		)
		{
			kaAssert(!frameScene.skybox.IsValid(), "only one skybox allowed at a time!");
			frameScene.skybox = _skybox;
		}
	}
}