#include "RenderManager.h"

#include "managers/ResourceManager.h"

#include "common/Mutex.h"
#include "systems.h"
#include "components.h"
#include "shaders/main.h"
#include "shaders/render_target_to_screen.h"
#include "shaders/depth_only.h"
#include "shaders/skybox.h"
#include "shaders/sprites.h"

#include "RenderTools/Pipeline.h"

#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>

#include <functional>

// classes
namespace Core
{
	namespace Render
	{
		static constexpr int32 MAX_LIGHTS = 16;

		class LightsState
		{
			main_lights_t lightData{};
			usize numLights{ 0 };

		public:
			Vec3& ambientLight{ lightData.ambient };
			Vec3 directionalDir{};
			LightSetter AddLight()
			{
				usize thisLightI = numLights;
				if (numLights < MAX_LIGHTS)
				{
					numLights++;
					lightData.numLights = static_cast< Vec1 >(numLights);
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

			bool m_mainCameraIsSet{ false }; // only draw 3D when true.

		public:
			void NextPass(e_Pass _pass)
			{
				if (m_currentPass == _pass)
				{
					return;
				}

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

			void Draw(int numInstances = 1)
			{
				constexpr int s_baseElement = 0; // best to use buffer offsets in the bindings instead

				if (m_currentPassGlue < e_PassGlue_Count)
				{
					Draw(m_passGlues[m_currentPassGlue]->NumToDraw(), s_baseElement, numInstances);
				}
				else
				{
					Draw(m_storedBindingNumToDraw, s_baseElement, numInstances);
				}
			}

			void MainCameraSet() { m_mainCameraIsSet = true; }
			bool IsMainCameraSet() const { return m_mainCameraIsSet; }

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
				m_mainCameraIsSet = false;
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
		};
		static RenderState g_renderState;

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
			Trans m_transform{};

			ModelToDraw(Resource::ModelID _model, Trans const& _trans)
				: m_model{ _model }
				, m_transform{ _trans }
			{}
		};

		struct ModelScratchData
		{
			Resource::ModelData const& m_model;
			Mat4 const m_renderMatrix;

			ModelScratchData(Resource::ModelData const& _model, Mat4 const& _renderMatrix)
				: m_model{ _model }
				, m_renderMatrix{ _renderMatrix }
			{}
		};

		struct SpriteToDraw
		{
			Resource::SpriteID m_sprite{};
			Trans2D m_transform{};

			SpriteToDraw(Resource::SpriteID _sprite, Trans2D const& _trans)
				: m_sprite{ _sprite }
				, m_transform{ _trans }
			{}
		};

		struct SpriteScratchData
		{
			std::reference_wrapper<Resource::SpriteData const> m_sprite;
			Trans2D m_transform;

			SpriteScratchData(Resource::SpriteData const& _sprite, Trans2D const& _transform)
				: m_sprite{ _sprite }
				, m_transform{ _transform }
			{}
		};

		struct SpriteBufferData
		{
			Vec3 m_position;
			Vec2 m_scale;
			Vec1 m_rotation;
			Vec2 m_topLeftUV;
			Vec2 m_UVDims;
			Vec2 m_spriteDims;

			SpriteBufferData(Vec3 _position, Vec2 _scale, Vec1 _rotation, Vec2 _topLeftUV, Vec2 _uvDims, Vec2 _spriteDims)
				: m_position(_position)
				, m_scale(_scale)
				, m_rotation(_rotation)
				, m_topLeftUV(_topLeftUV)
				, m_UVDims(_uvDims)
				, m_spriteDims(_spriteDims)
			{}
		};

		constexpr usize g_maxSpritesPerFrame = 128;

		struct FrameScene
		{
			Mutex< LightsState > lights{};
			Resource::TextureSampleID directionalShadowMap{};
			CameraState camera{};
			Mutex< std::vector<ModelToDraw> > models;
			std::vector<ModelScratchData> modelScratchData;
			Mutex< std::vector<SpriteToDraw> > sprites;
			std::vector<SpriteScratchData> spriteScratchData;
			std::vector<SpriteBufferData> spriteBufferData;
			sg_bindings spriteBinds{};
			sg_buffer spriteBuffer{};
			Resource::TextureSampleID skybox{};
			sg_bindings skyboxBinds{};
		};

		static FrameScene g_frameScene{};
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
		static void InitBuffers
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
				g_frameScene.directionalShadowMap = io_state.Pass(Pass_DirectionalLight)->GetDepthImage();
			}

			// Target to screen glue
			{
				// in triangle-strip form
				auto rectangleWithUV = MakeArray< Vec1 >(
					-1.0f,  1.0f,	0, 0,
					 1.0f,  1.0f,	1, 0,
					-1.0f, -1.0f,	0, 1,
					 1.0f, -1.0f,	1, 1
				);

				sg_buffer_desc rectangleWithUVBufferDesc{
					.type = SG_BUFFERTYPE_VERTEXBUFFER,
					.data = SG_RANGE(rectangleWithUV),
					.label = "rectangleWithUV-buffer",
				};
				sg_bindings binds{};
				binds.vertex_buffers[0] = sg_make_buffer(rectangleWithUVBufferDesc);
				binds.fs_images[SLOT_render_target_to_screen_tex] = io_state.Pass(Pass_MainTarget)->GetColourImage(0).GetValue();
				io_state.PassGlue(PassGlue_MainTarget_To_Screen) = PassGlue{ binds, 4 };
				io_state.PassGlue(PassGlue_MainTarget_To_Screen)->AddValidPass(Pass_RenderToScreen);
				io_state.PassGlue(PassGlue_MainTarget_To_Screen)->AddValidRenderer(Renderer_TargetToScreen);
			}

			{
				// in triangle-strip form
				auto skyboxCube = MakeArray< Vec1 >(
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
					-1.0f, -1.0f, -1.0f // front-bottom-left
				);

				sg_buffer_desc skyboxCubeDesc{
					.type = SG_BUFFERTYPE_VERTEXBUFFER,
					.data = SG_RANGE(skyboxCube),
					.label = "skyboxCube-buffer",
				};
				g_frameScene.skyboxBinds.vertex_buffers[0] = sg_make_buffer(skyboxCubeDesc);
			}

			{
				// in triangle-strip form
				auto rectangle2DWithUV = MakeArray< Vec1 >(
					0.0f, 0.0f,		0, 0,
					1.0f, 0.0f,		1, 0,
					0.0f, 1.0f,		0, 1,
					1.0f, 1.0f,		1, 1
				);

				sg_buffer_desc rectangle2DWithUVBufferDesc{
					.type = SG_BUFFERTYPE_VERTEXBUFFER,
					.data = SG_RANGE(rectangle2DWithUV),
					.label = "rectangle2DWithUV-buffer",
				};
				g_frameScene.spriteBinds.vertex_buffers[0] = sg_make_buffer(rectangle2DWithUVBufferDesc);

				sg_buffer_desc spriteBufferDesc{
					.size = sizeof(SpriteBufferData) * g_maxSpritesPerFrame,
					.type = SG_BUFFERTYPE_VERTEXBUFFER,
					.usage = SG_USAGE_STREAM,
					.label = "sprite-buffer",
				};
				g_frameScene.spriteBuffer = sg_make_buffer(spriteBufferDesc);
				g_frameScene.spriteBinds.vertex_buffers[1] = g_frameScene.spriteBuffer;
			}
		}

		//--------------------------------------------------------------------------------
		static void InitShaders
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
					.format = SG_VERTEXFORMAT_FLOAT3,
				};
				mainLayoutDesc.attrs[ATTR_main_vs_aNormal] = {
					.format = SG_VERTEXFORMAT_FLOAT3,
				};
				mainLayoutDesc.attrs[ATTR_main_vs_aTexCoord] = {
					.format = SG_VERTEXFORMAT_FLOAT2,
				};
				mainLayoutDesc.attrs[ATTR_main_vs_aTangent] = {
					.format = SG_VERTEXFORMAT_FLOAT3,
				};

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
				targetToScreenLayoutDesc.attrs[ATTR_render_target_to_screen_vs_aPos] = {
					.format = SG_VERTEXFORMAT_FLOAT2,
				};
				targetToScreenLayoutDesc.attrs[ATTR_render_target_to_screen_vs_aTexCoord] = {
					.format = SG_VERTEXFORMAT_FLOAT2,
				};

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
					.format = SG_VERTEXFORMAT_FLOAT3,
				};

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

			// sprite renderer
			{
				sg_layout_desc spritesLayoutDesc{};
				spritesLayoutDesc.attrs[ATTR_sprites_vs_aPos] = {
					.format = SG_VERTEXFORMAT_FLOAT2,
				};
				spritesLayoutDesc.attrs[ATTR_sprites_vs_aUV] = {
					.format = SG_VERTEXFORMAT_FLOAT2,
				};

				spritesLayoutDesc.attrs[ATTR_sprites_vs_aSpritePos] = {
					.buffer_index = 1,
					.format = SG_VERTEXFORMAT_FLOAT3,
				};
				spritesLayoutDesc.attrs[ATTR_sprites_vs_aSpriteScale] = {
					.buffer_index = 1,
					.format = SG_VERTEXFORMAT_FLOAT2,
				};
				spritesLayoutDesc.attrs[ATTR_sprites_vs_aSpriteRot] = {
					.buffer_index = 1,
					.format = SG_VERTEXFORMAT_FLOAT,
				};
				spritesLayoutDesc.attrs[ATTR_sprites_vs_aTopLeftUV] = {
					.buffer_index = 1,
					.format = SG_VERTEXFORMAT_FLOAT2,
				};
				spritesLayoutDesc.attrs[ATTR_sprites_vs_aUVDims] = {
					.buffer_index = 1,
					.format = SG_VERTEXFORMAT_FLOAT2,
				};
				spritesLayoutDesc.attrs[ATTR_sprites_vs_aSpriteDims] = {
					.buffer_index = 1,
					.format = SG_VERTEXFORMAT_FLOAT2,
				};
				spritesLayoutDesc.buffers[0] = {
					.step_func = SG_VERTEXSTEP_PER_VERTEX,
				};
				spritesLayoutDesc.buffers[1] = {
					.step_func = SG_VERTEXSTEP_PER_INSTANCE,
				};
				static_assert(sizeof(Vec3) + sizeof(Vec2) + sizeof(Vec1) + sizeof(Vec2) + sizeof(Vec2) + sizeof(Vec2) == sizeof(SpriteBufferData));

				sg_pipeline_desc spritesDesc{
					.shader = sg_make_shader(sprites_sg_shader_desc(sg_query_backend())),
					.layout = spritesLayoutDesc,
					.depth = {
						.compare = SG_COMPAREFUNC_LESS,
						.write_enabled = true,
					},
					.primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
					.cull_mode = SG_CULLMODE_BACK,
					.label = "sprites-pipeline",
				};
				spritesDesc.colors[0] = {
					.blend = {
						.enabled = true,
						.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
						.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
						.src_factor_alpha = SG_BLENDFACTOR_SRC_ALPHA,
						.dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
					},
				};

				io_state.Renderer(Renderer_Sprites) = Renderer{ sg_make_pipeline(spritesDesc) };
				io_state.Renderer(Renderer_Sprites)->AddValidPass(Pass_MainTarget);
				io_state.Renderer(Renderer_Sprites)->AllowGeneralBindings();
			}
		}

		//--------------------------------------------------------------------------------
		void SetupPipeline
		(
			int _mainRenderWidth,
			int _mainRenderHeight
		)
		{
			RenderStateFiller filler{ g_renderState };
			InitBuffers(_mainRenderWidth, _mainRenderHeight, filler);
			InitShaders(filler);
		}

		//--------------------------------------------------------------------------------
		void SetMainCameraParams
		(
			Core::Render::FrameData const& _rfd,
			Core::Render::MainCamera3D const& _cam,
			Core::Transform3D const& _t
		)
		{
			kaAssert(!g_renderState.IsMainCameraSet(), "only one MainCamera3D allowed!");
			g_frameScene.camera.proj = glm::perspective(glm::radians(_cam.m_povY), _rfd.renderArea.f.x / _rfd.renderArea.f.y, 0.01f, 1000.0f);

			Trans const cameraTrans = _t.CalculateWorldTransform();
			g_frameScene.camera.pos = cameraTrans.m_origin;
			g_frameScene.camera.view = glm::lookAt(cameraTrans.m_origin, cameraTrans.m_origin + cameraTrans.Forward(), Vec3(0.0f, 1.0f, 0.0f));

			g_renderState.MainCameraSet();
		}

		//--------------------------------------------------------------------------------
		template<typename T_ModelVisitor, typename T_MeshVisitor>
		void RenderMainScene(T_ModelVisitor const& _fnModelVisitor, T_MeshVisitor const& _fnMeshVisitor)
		{
			for (ModelScratchData const& mtd : g_frameScene.modelScratchData)
			{
				_fnModelVisitor(mtd.m_renderMatrix, mtd.m_model);

				for (Resource::MeshData const& mesh : mtd.m_model.m_meshes)
				{
					_fnMeshVisitor(mesh);

					g_renderState.Draw();
				}

			}
		}

		//--------------------------------------------------------------------------------
		static void RenderSpriteBuffer(Resource::TextureID _texture)
		{
			// fill and draw buffer
			g_frameScene.spriteBinds.vertex_buffer_offsets[1] = sg_append_buffer(g_frameScene.spriteBuffer, SG_RANGE_VEC(g_frameScene.spriteBufferData));
			g_frameScene.spriteBinds.fs_images[SLOT_sprites_textureAtlas] = _texture.GetValue();

			g_renderState.SetBinding(g_frameScene.spriteBinds, 4);
			kaAssert(g_frameScene.spriteBufferData.size() <= INT_MAX); // which it should be anyway if it's less than max sprites.
			g_renderState.Draw(static_cast<int>(g_frameScene.spriteBufferData.size()));

			g_frameScene.spriteBufferData.clear();
		}

		//--------------------------------------------------------------------------------
		static Mat4 GetDirectionalLightOrthoMat( Vec1 _bounds, Vec1 _nearPlane, Vec1 _farPlane)
		{
			// d3d needs off-centre, gl needs usual.
#if SOKOL_D3D11
			return glm::orthoRH_ZO(-_bounds, _bounds, -_bounds, _bounds, _nearPlane, _farPlane);
#else
			return glm::orthoRH_NO(-_bounds, _bounds, -_bounds, _bounds, _nearPlane, _farPlane);
#endif	
		}

		//--------------------------------------------------------------------------------
		static Mat4 GetSpriteOrthoMat(Core::Render::FrameData const& _rfd)
		{
			return glm::ortho(0.0f, _rfd.renderArea.f.x, _rfd.renderArea.f.y, 0.0f, -1.0f, 1.0f);
		}

		constexpr bool g_enableDirectionalShadow = true;
		constexpr bool g_enableMainRenderer = true;
		constexpr bool g_enableSkybox = true;
		constexpr bool g_enableSprites = true;


		//--------------------------------------------------------------------------------
		static void Render3D
		(
		)
		{
			auto modelsAccess = g_frameScene.models.Lock();
			auto lightsAccess = g_frameScene.lights.Lock();

			g_frameScene.modelScratchData.clear();
			g_frameScene.modelScratchData.reserve( modelsAccess->size() );
			for ( ModelToDraw const& mtd : *modelsAccess )
			{
				g_frameScene.modelScratchData.emplace_back( Resource::GetModel( mtd.m_model ), mtd.m_transform.GetRenderMatrix() );
			}


			// RENDER_PASSES
			Mat4 lightSpace{};
			if constexpr (g_enableDirectionalShadow)
			{
				g_renderState.NextPass(Pass_DirectionalLight);
				g_renderState.SetRenderer(Renderer_DepthOnly);

				Mat4 const lightProj = GetDirectionalLightOrthoMat( 10.0f, 1.0f, 50.0f );
				Vec3 const lightPos = g_frameScene.camera.pos - ( lightsAccess->directionalDir * 25.0f );
				Mat4 const lightView = glm::lookAt( lightPos, lightPos + lightsAccess->directionalDir, Vec3( 0.0f, 1.0f, 0.0f ) );
				lightSpace = lightProj * lightView;

				auto fnLightModelVisitor = [&lightSpace](Mat4 const& _modelMatrix, Resource::ModelData const& _model)
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
					g_renderState.SetBinding(bufOnlyBinds, _mesh.NumToDraw());
				};

				RenderMainScene(fnLightModelVisitor, fnLightMeshVisitor);

				g_renderState.NextPass(Pass_MainTarget);
			}

			if constexpr (g_enableMainRenderer)
			{
				g_renderState.SetRenderer(Renderer_Main);
				sg_apply_uniforms( SG_SHADERSTAGE_FS, SLOT_main_lights, SG_RANGE_REF( lightsAccess->shader_LightData() ) );

				main_vs_params_t vs_params = {
					.projection = g_frameScene.camera.proj,
				};
				sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_main_vs_params, SG_RANGE_REF(vs_params));

				auto fnMainModelVisitor = [&lightSpace](Mat4 const& _modelMatrix, Resource::ModelData const& _model)
				{
					main_model_params_t model_params = {
						.viewModel = g_frameScene.camera.view * _modelMatrix,
						.normal = glm::transpose(glm::inverse(model_params.viewModel)),
						.lightSpace = lightSpace * _modelMatrix,
					};

					sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_main_model_params, SG_RANGE_REF(model_params));
				};

				auto fnMainMeshVisitor = [](Resource::MeshData const& _mesh)
				{
					sg_bindings addShadowBinds = _mesh.m_bindings;
					addShadowBinds.fs_images[SLOT_main_directionalShadowMap] = g_frameScene.directionalShadowMap.GetValue();
					g_renderState.SetBinding(addShadowBinds, _mesh.NumToDraw());
					sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_main_material, SG_RANGE_REF(_mesh.m_material));
				};

				RenderMainScene(fnMainModelVisitor, fnMainMeshVisitor);
			}

			// render skybox (if exists)
			if constexpr (g_enableSkybox)
			{
				if (g_frameScene.skybox.IsValid())
				{
					g_renderState.SetRenderer(Renderer_Skybox);
					skybox_vs_params_t vs_params{
						.untranslated_projView = g_frameScene.camera.proj * Mat4(Mat3(g_frameScene.camera.view)),
					};
					sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_skybox_vs_params, SG_RANGE_REF(vs_params));

					skybox_fs_params_t fs_params{
						.sunDir = lightsAccess->directionalDir,
					};
					sg_apply_uniforms(SG_SHADERSTAGE_FS, SLOT_skybox_fs_params, SG_RANGE_REF(fs_params));

					g_frameScene.skyboxBinds.fs_images[SLOT_skybox_skybox] = g_frameScene.skybox.GetValue();

					g_renderState.SetBinding(g_frameScene.skyboxBinds, 14);
					g_renderState.Draw();
				}
			}
		}

		//--------------------------------------------------------------------------------
		static void RenderSprites
		(
			Core::Render::FrameData const& _rfd
		)
		{
			auto spritesAccess = g_frameScene.sprites.Lock();

			g_renderState.NextPass(Pass_MainTarget);
			g_renderState.SetRenderer(Renderer_Sprites);
			sprites_vs_params_t vs_params{
				.projection = GetSpriteOrthoMat(_rfd),
			};
			sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_sprites_vs_params, SG_RANGE_REF(vs_params));

			g_frameScene.spriteScratchData.clear();
			g_frameScene.spriteScratchData.reserve( spritesAccess->size() );
			for ( SpriteToDraw const& std : *spritesAccess )
			{
				g_frameScene.spriteScratchData.emplace_back( Resource::GetSprite( std.m_sprite ), std.m_transform );
			}

			// Order by texture
			std::sort(g_frameScene.spriteScratchData.begin(), g_frameScene.spriteScratchData.end(), [](auto const& _a, auto const& _b) -> bool
			{
				return _a.m_sprite.get().m_texture < _b.m_sprite.get().m_texture;
			});

			// For each texture, set up buffer and binding and draw sprites
			Resource::TextureID currentTexture;
			for (usize spriteI{ 0 }; spriteI < g_frameScene.spriteScratchData.size(); ++spriteI)
			{
				SpriteScratchData const& spriteScratch = g_frameScene.spriteScratchData[spriteI];
				Resource::SpriteData const& sprite = spriteScratch.m_sprite.get();
				if (currentTexture != sprite.m_texture)
				{
					if (spriteI != 0)
					{
						RenderSpriteBuffer(currentTexture);
					}

					currentTexture = sprite.m_texture;
				}


				g_frameScene.spriteBufferData.emplace_back(
					Vec3(spriteScratch.m_transform.m_pos, spriteScratch.m_transform.m_z),
					spriteScratch.m_transform.m_scale,
					spriteScratch.m_transform.m_rot.m_rads,
					sprite.m_topLeftUV,
					sprite.m_dimensionsUV,
					sprite.m_dimensions
				);
			}

			// final draw
			if (!g_frameScene.spriteBufferData.empty())
			{
				RenderSpriteBuffer(currentTexture);
			}
		}

		//--------------------------------------------------------------------------------
		static void RenderMainToScreen
		(
			Core::Render::FrameData const& _rfd
		)
		{
			g_renderState.NextPass(Pass_RenderToScreen);
			g_renderState.SetRenderer(Renderer_TargetToScreen);
			g_renderState.SetPassGlue(PassGlue_MainTarget_To_Screen);

			render_target_to_screen_vs_params_t aspectData{
				.aspectMult = (4.0f / 3.0f) * (_rfd.contextWindow.f.y / _rfd.contextWindow.f.x),
			};
			sg_apply_uniforms(SG_SHADERSTAGE_VS, SLOT_render_target_to_screen_vs_params, SG_RANGE_REF(aspectData));
			g_renderState.Draw();
		}

		//--------------------------------------------------------------------------------
		void Render
		(
			Core::Render::FrameData const& _rfd
		)
		{
			// Only do 3D stuff if main camera set.
			if (g_renderState.IsMainCameraSet())
			{
				Render3D();
			}

			// render sprites
			if constexpr (g_enableSprites)
			{
				RenderSprites(_rfd);
			}

			// frameScene cleanup
			g_frameScene.skybox = Resource::TextureSampleID{};
			g_frameScene.lights.Lock()->Reset();
			g_frameScene.models.Lock()->clear();
			g_frameScene.sprites.Lock()->clear();

			// end of the main drawing pass
			// begin of the screen drawing pass
			RenderMainToScreen(_rfd);

			// DEFAULT_PASS_END
			Core::Render::TextAndGLDebug::Render();

			Core::Render::DImGui::Render(); // imgui last, always a debug.

			// RENDER_END
			g_renderState.Commit();
		}

		//--------------------------------------------------------------------------------
		void Cleanup()
		{
			g_renderState.Cleanup();
			sg_shutdown();
		}

		//--------------------------------------------------------------------------------
		CameraState const& GetCameraState()
		{
			return g_frameScene.camera;
		}

		//--------------------------------------------------------------------------------
		LightSetter AddLightToScene()
		{
			return g_frameScene.lights.Lock()->AddLight();
		}

		//--------------------------------------------------------------------------------
		void AddAmbientLightToScene(Vec3 const& _col)
		{
			g_frameScene.lights.Lock()->ambientLight += _col;
		}

		//--------------------------------------------------------------------------------
		void SetDirectionalLightDir(Vec3 const& _dir)
		{
			g_frameScene.lights.Lock()->directionalDir = _dir;
		}

		//--------------------------------------------------------------------------------
		void AddSpriteToScene
		(
			Core::Resource::SpriteID _sprite,
			Trans2D const& _screenTrans
		)
		{
			auto spritesAccess = g_frameScene.sprites.Lock();
			spritesAccess->emplace_back(_sprite, _screenTrans);
			kaAssert( spritesAccess->size() < g_maxSpritesPerFrame );
		}

		//--------------------------------------------------------------------------------
		void AddModelToScene
		(
			Core::Resource::ModelID _model,
			Trans const& _worldTrans
		)
		{
			g_frameScene.models.Lock()->emplace_back(_model, _worldTrans);
		}

		//--------------------------------------------------------------------------------
		void AddSkyboxToScene
		(
			Core::Resource::TextureSampleID _skybox
		)
		{
			kaAssert(!g_frameScene.skybox.IsValid(), "only one skybox allowed at a time!");
			g_frameScene.skybox = _skybox;
		}
	}
}