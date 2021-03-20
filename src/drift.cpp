#include "common.h"

#include "managers/EntityManager.h"
#include "managers/InputManager.h"
#include "managers/ResourceManager.h"
#include "managers/RenderManager.h"
#include "managers/SoundManager.h"

#include "components.h"
#include "systems.h"

#include <ecs/ecs.h>

// sokol
#include <sokol_app.h>
#include <sokol_time.h>

#include "CubeTest.h"

constexpr int const g_renderAreaWidth = 320;
constexpr int const g_renderAreaHeight = (g_renderAreaWidth / 4) * 3;
constexpr int const g_windowStartWidth = g_renderAreaWidth * 3;
constexpr int const g_windowStartHeight = (g_windowStartWidth / 4) * 3;

void initialise_cb()
{
	// initialisation
	{
		Core::Resource::Init();
		Core::Render::Init();
		Core::Render::TextAndGLDebug::Init();
		Core::Render::DImGui::Init();
		Core::Sound::Init();
		Core::Physics::Init();
		stm_setup();
	}

	// system and data setup
	{
		Core::Resource::SetupData();
		Core::Render::SetupPipeline(g_renderAreaWidth, g_renderAreaHeight);
		Core::Render::Setup();
		Core::Render::TextAndGLDebug::Setup();
		Core::Render::DImGui::Setup();
		Core::Resource::Setup();
		Core::Sound::Setup();
		Core::Physics::Setup();
		Core::Input::Setup();
	}

	// game setup
	{
		CubeTestSystems();

		Game::Player::Setup();
		Game::UI::Setup();
	}

	// pre-scene required entity setup
	{
		Core::EntityID const primaryPhysicsWorld = Core::CreateEntity();
		Core::AddComponent(primaryPhysicsWorld, Core::Physics::World{});

		Core::ECS::CommitChanges();
	}

	// Basic system controls
	Core::MakeSystem<Sys::GAME>([](Core::FrameData const& _fd)
	{
		if (Core::Input::PressedOnce(Core::Input::Action::Pause))
		{
			Core::FrameData& fd = ecs::get_global_component<Core::FrameData>();
			fd.m_scale = (fd.m_scale == 0.0 ? fd.m_unpausedSpeed : 0.0);
		}
		if (Core::Input::PressedOnce(Core::Input::Action::Debug_SpeedUp))
		{
			Core::FrameData& fd = ecs::get_global_component<Core::FrameData>();
			fd.m_unpausedSpeed = (fd.m_unpausedSpeed == 1.0 ? 2.0 : (fd.m_unpausedSpeed == 2.0 ? 5.0 : 1.0));
			fd.m_scale = (fd.m_scale == 0.0 ? 0.0 : fd.m_unpausedSpeed);
		}
		if (Core::Input::Pressed(Core::Input::Action::Quit))
		{
			sapp_request_quit();
		}
	});

	// Setup entity manager
	// todo

	// Preload assets
	Core::EntityID const preloadEntity = Core::CreateEntity();
	Core::AddComponent(preloadEntity, Core::Resource::Preload());
	Core::AddComponent(preloadEntity, Game::UI::LoadingScreen());

	// Finalise initial entities
	Core::ECS::CommitChanges();
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

#if DEBUG_TOOLS
		fd.m_debug_elapsedTime += fd.unscaled_ddt;
		fd.m_debug_frameCount++;
#endif
	}

	{
		Core::Render::FrameData& rfd = ecs::get_global_component<Core::Render::FrameData>();
		rfd.contextWindow.i.x = sapp_width();
		rfd.contextWindow.i.y = sapp_height();
		rfd.contextWindow.f = rfd.contextWindow.i;
		rfd.renderArea.i.x = g_renderAreaWidth;
		rfd.renderArea.i.y = g_renderAreaHeight;
		rfd.renderArea.f = rfd.renderArea.i;
	}

	Core::Input::Update();

	Core::ECS::Update();
}

void cleanup_cb()
{
	Core::Physics::Cleanup();
	Core::Sound::Cleanup();
	Core::Render::DImGui::Cleanup();
	Core::Render::TextAndGLDebug::Cleanup();
	Core::Render::Cleanup();
	Core::Resource::Cleanup();
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
	InitialiseLogging();

	sapp_desc desc{
		.init_cb = &initialise_cb,
		.frame_cb = &frame_cb,
		.cleanup_cb = &cleanup_cb,
		.event_cb = &event_cb,
		.fail_cb = &fail_cb,

		.width = g_windowStartWidth,
		.height = g_windowStartHeight,
		.sample_count = 1,
		.swap_interval = 0,
		.window_title = "drift",
	};

	return desc;
}