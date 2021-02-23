#include "common.h"

#include "managers/EntityManager.h"
#include "managers/Input.h"
#include "managers/Resources.h"
#include "managers/Render.h"

#include "components.h"
#include "systems.h"

#include <ecs/ecs.h>

// sokol
#include <sokol_app.h>
#include <sokol_time.h>

#include "CubeTest.h"

void initialise_cb()
{
	// initialisation
	{
		Core::Resource::Init();
		Core::Render::Init();
		Core::Render::TextAndGLDebug::Init();
		Core::Render::DImGui::Init();
		Core::Render::InitPipeline(); //needs to come after imgui's hooks
		Core::Physics::Init();
		stm_setup();
		sapp_lock_mouse(true);
	}


	// system and data setup
	{
		Core::Resource::Setup();
		Core::Render::Setup();
		Core::Render::TextAndGLDebug::Setup();
		Core::Render::DImGui::Setup();
		Core::Physics::Setup();
		Core::Input::Setup();
	}

	{
		Game::Player::Setup();
	}

	// initial entity setup
	{
		Core::EntityID const primaryPhysicsWorld = Core::CreateEntity();
		Core::AddComponent(primaryPhysicsWorld, Core::Physics::World{});

		ecs::commit_changes();
	}


	ecs::make_system<ecs::opts::group<Sys::GAME>>([](Core::FrameData const& _fd)
	{
		static double speed = 1.0;
		if (Core::Input::PressedOnce(Core::Input::Action::Pause))
		{
			Core::FrameData& fd = ecs::get_global_component<Core::FrameData>();
			fd.m_scale = (fd.m_scale == 0.0 ? speed : 0.0);
		}
		if (Core::Input::PressedOnce(Core::Input::Action::Debug_SpeedUp))
		{
			speed = (speed == 1.0 ? 2.0 : (speed == 2.0 ? 5.0 : 1.0));
			Core::FrameData& fd = ecs::get_global_component<Core::FrameData>();
			fd.m_scale = (fd.m_scale == 0.0 ? 0.0 : speed);
		}
		if (Core::Input::Pressed(Core::Input::Action::Quit))
		{
			sapp_request_quit();
		}
	});

	// Setup entity manager
	Core::EntityID const renderEntity = Core::CreateEntity();
	Core::EntityID const character = Core::CreateEntity();
	fTrans const characterTrans{ fQuatIdentity(), fVec3(2.0f, 0.0f, 0.0f) };
	Core::AddComponent(character, Core::Transform(characterTrans));
	{
		Core::Physics::CharacterControllerDesc ccDesc{};
		ccDesc.m_viewObject = renderEntity;
		ccDesc.m_halfHeight = 0.9f;
		ccDesc.m_radius = 0.5f;
		ccDesc.m_mass = 80.0f;
		ccDesc.m_startTransform = characterTrans;
		ccDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

		Core::AddComponent(character, ccDesc);
	}

	Core::AddComponent(renderEntity, Core::Render::Frame_Tag());
	Core::Transform const renderTrans(fQuatIdentity(), fVec3(0.0f, 0.8f, 0.0f), character);
	Core::AddComponent(renderEntity, renderTrans); // camera transform
	Core::AddComponent(renderEntity, Core::Render::Camera());
	Core::AddComponent(renderEntity, Core::Render::DebugCameraControl());
	Core::AddComponent(renderEntity, Game::Player::MouseLook());

	// Setup entity initialisers
	setup_cube();

	// Setup systems
	// debug camera control
	ecs::make_system<ecs::opts::group<Sys::GAME>>([](Core::FrameData const& _fd, Core::Render::Camera& _cam, Core::Transform& _t, Core::Render::DebugCameraControl& _debugCamera)
	{
		static bool debugCameraEnabled = false;

		if (Core::Input::PressedOnce(Core::Input::Action::Debug_EnableCamera))
		{
			if (debugCameraEnabled)
			{
				_t.m_parent = _debugCamera.m_storedParent;
				_t.T() = _debugCamera.m_storedTransform;
				debugCameraEnabled = false;
				sapp_lock_mouse(true);
			}
			else
			{
				_debugCamera.m_storedParent = _t.m_parent;
				_debugCamera.m_storedTransform = _t.T();
				_t.DetachFromParent();
				fVec3 const forward = _t.T().forward();
				_debugCamera.m_angle.x = asin(forward.y); // pitch
				float const cosPitch = cos(_debugCamera.m_angle.x);
				if (cosPitch == 0.0f)
				{
					_debugCamera.m_angle.y = 0.0f; // if looking directly up or down then yaw can't be calc'd.. but also doesn't matter
				}
				else
				{
					// yaw1 and yaw2 should just be pi offset.
					float const yaw1 = acos(forward.x / cosPitch);
					//float const yaw2 = asin(forward.z / cosPitch);
					_debugCamera.m_angle.y = yaw1;
				}
				debugCameraEnabled = true;
			}
		}

		if (debugCameraEnabled)
		{
			if (Core::Input::Pressed(Core::Input::Action::Debug_AimCamera))
			{
				sapp_lock_mouse(true);
				fVec2 const mouseDelta = Core::Input::GetMouseDelta();
				_debugCamera.m_angle.x -= mouseDelta.y * 0.0005f;
				_debugCamera.m_angle.y += mouseDelta.x * 0.0005f;
			}
			else
			{
				sapp_lock_mouse(false);
			}

#define CHECK_AXES 0
#if CHECK_AXES
			_t.T().m_origin = fVec3(0, 0, 0);
			if (Core::Input::Pressed(Core::Input::Action::Forward))
			{
				_t.T().m_origin += fVec3(0, 0, 1);
			}
			if (Core::Input::Pressed(Core::Input::Action::Backward))
			{
				_t.T().m_origin += fVec3(0, 0, -1);
			}
			if (Core::Input::Pressed(Core::Input::Action::Left))
			{
				_t.T().m_origin += fVec3(-1, 0, 0);
			}
			if (Core::Input::Pressed(Core::Input::Action::Right))
			{
				_t.T().m_origin += fVec3(1, 0, 0);
			}
			if (Core::Input::Pressed(Core::Input::Action::Debug_RaiseCamera))
			{
				_t.T().m_origin += fVec3(0, 1, 0);
			}
			if (Core::Input::Pressed(Core::Input::Action::Debug_LowerCamera))
			{
				_t.T().m_origin += fVec3(0, -1, 0);
			}

			if (Core::Input::PressedOnce(Core::Input::Action::Select))
			{
				fVec3 const col0 = _t.T().m_basis[0];
				_t.T().m_basis[0] = _t.T().m_basis[1];
				_t.T().m_basis[1] = _t.T().m_basis[2];
				_t.T().m_basis[2] = col0;
			}
#else
			// when at identity, forward == z
			float const yaw = _debugCamera.m_angle.y;
			float const pitch = _debugCamera.m_angle.x;


			fVec3 forward;
			forward.x = cosf(yaw) * cosf(pitch);
			forward.y = sinf(pitch);
			forward.z = sinf(yaw) * cosf(pitch);
			forward = glm::normalize(forward);

			_t.T().m_basis = RotationFromForward(forward);

			// also re-calculate the Right and Up vector
			fVec3 right = glm::normalize(glm::cross(forward, fVec3(0.0f, 1.0f, 0.0f)));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
			fVec3 up = glm::normalize(glm::cross(right, forward));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.

			float velocity = 0.8f * _fd.unscaled_dt;
			if (Core::Input::Pressed(Core::Input::Action::Forward))
			{
				_t.T().m_origin += forward * velocity;
			}
			if (Core::Input::Pressed(Core::Input::Action::Backward))
			{
				_t.T().m_origin -= forward * velocity;
			}
			if (Core::Input::Pressed(Core::Input::Action::Left))
			{
				_t.T().m_origin -= right * velocity;
			}
			if (Core::Input::Pressed(Core::Input::Action::Right))
			{
				_t.T().m_origin += right * velocity;
			}
			// should this up really be used? or world up?
			if (Core::Input::Pressed(Core::Input::Action::Debug_RaiseCamera))
			{
				_t.T().m_origin += up * velocity;
			}
			if (Core::Input::Pressed(Core::Input::Action::Debug_LowerCamera))
			{
				_t.T().m_origin -= up * velocity;
			}
#endif
		}
	});

	// Finalise initial components
	ecs::commit_changes();

}

void frame_cb()
{
	{
		Core::FrameData& fd = ecs::get_global_component<Core::FrameData>();
		uint64 const lappedTicks = stm_laptime(&fd.m_lastFrameTicks);
		// stm_round_to_common_refresh_rate?
		fd.unscaled_ddt = stm_sec(lappedTicks);
		fd.unscaled_dt = static_cast<float>(fd.unscaled_ddt);
		fd.ddt = fd.m_scale * fd.unscaled_ddt;
		fd.dt = static_cast<float>(fd.ddt);
	}

	{
		Core::Render::FrameData& rfd = ecs::get_global_component<Core::Render::FrameData>();
		rfd.contextWindow.w = sapp_width();
		rfd.contextWindow.fW = static_cast<float>(rfd.contextWindow.w);
		rfd.contextWindow.h = sapp_height();
		rfd.contextWindow.fH = static_cast<float>(rfd.contextWindow.h);
		rfd.renderArea.w = RENDER_AREA_WIDTH;
		rfd.renderArea.fW = static_cast<float>(rfd.renderArea.w);
		rfd.renderArea.h = RENDER_AREA_HEIGHT;
		rfd.renderArea.fH = static_cast<float>(rfd.renderArea.h);
	}

	Core::Input::Update();
	ecs::update();
}

void cleanup_cb()
{
	Core::Physics::Cleanup();
	Core::Render::DImGui::Cleanup();
	Core::Render::TextAndGLDebug::Cleanup();
	Core::Render::Cleanup();
}

void event_cb(sapp_event const* _event)
{
	Core::Render::DImGui::Event(_event);
	Core::Input::Event(_event);
}

void fail_cb(char const* _error)
{

}

sapp_desc sokol_main(int argc, char* argv[])
{
	sapp_desc desc{
		.init_cb = &initialise_cb,
		.frame_cb = &frame_cb,
		.cleanup_cb = &cleanup_cb,
		.event_cb = &event_cb,
		.fail_cb = &fail_cb,

		.width = WINDOW_START_WIDTH,
		.height = WINDOW_START_HEIGHT,
		.sample_count = 1,
		.swap_interval = 0,
		.window_title = "drift",
	};

	return desc;
}