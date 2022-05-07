#include "common.h"

#include "managers/EntityManager.h"
#include "managers/InputManager.h"
#include "managers/ResourceManager.h"
#include "managers/RenderManager.h"
#include "managers/SoundManager.h"

#include "components.h"
#include "systems.h"

#include "cpuid.h"
#include <boxer/boxer.h>

// sokol
#include <sokol_app.h>
#include <sokol_time.h>

#include "scenes/CubeTest.h"
#include "scenes/GinRummy.h"

#include <format>

constexpr int32 g_renderAreaWidth = 320;
constexpr int32 g_renderAreaHeight = (g_renderAreaWidth / 4) * 3;
constexpr int32 g_windowStartWidth = g_renderAreaWidth * 3;
constexpr int32 g_windowStartHeight = (g_windowStartWidth / 4) * 3;

void Initialise();
void Frame();
void Cleanup();
void Event(sapp_event const* _event);
void Fail(char const* _error);

bool EnsureRequiredCPUFeatures(char const*& o_missingFeature)
{
	Setup::CpuInfo cpuInfo;
#if __AVX__
	if ( !cpuInfo.AVX() )
	{
		o_missingFeature = "AVX";
		return false;
	}
#endif

	return true;
}

sapp_desc sokol_main(int argc, char* argv[])
{
	{
		char const* missingFeature{ nullptr };
		if ( !EnsureRequiredCPUFeatures( missingFeature ) )
		{
			std::string const errorMessage = std::format( "Your computer is missing the '{:s}' feature, which is required with this build of the game.", missingFeature );
			boxer::show( errorMessage.c_str(), "Critical Error", boxer::Style::Error, boxer::Buttons::Quit );
			std::abort();
		}
	}

	InitialiseLogging();

	sapp_desc desc{
		.init_cb = &Initialise,
		.frame_cb = &Frame,
		.cleanup_cb = &Cleanup,
		.event_cb = &Event,
		.fail_cb = &Fail,

		.width = g_windowStartWidth,
		.height = g_windowStartHeight,
		.sample_count = 1,
		.swap_interval = 0,
		.window_title = "drift",
	};

	return desc;
}

void Initialise()
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
		Core::Input::Setup(g_renderAreaWidth, g_renderAreaHeight);
	}

	// game setup
	{
		CubeTestSystems();

		Game::Player::Setup();
		Game::UI::Setup();
		Game::GinRummy::Setup();
	}

	// pre-scene required entity setup
	{
		Core::EntityID const primaryPhysicsWorld = Core::CreatePersistentEntity();
		Core::AddComponent(primaryPhysicsWorld, Core::Physics::World{});

		Core::ECS::CommitChanges();
	}

	// Basic system controls
	Core::MakeSystem<Sys::GAME>([](Core::FrameData const& _fd)
	{
		if (Core::Input::PressedOnce(Core::Input::Action::Pause))
		{
			Core::FrameData& fd = Core::GetGlobalComponent<Core::FrameData>();
			fd.m_scale = (fd.m_scale == 0.0 ? fd.m_unpausedSpeed : 0.0);
		}
		if (Core::Input::PressedOnce(Core::Input::Action::Debug_SpeedUp))
		{
			Core::FrameData& fd = Core::GetGlobalComponent<Core::FrameData>();
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

	// Setup scene
	{
		Core::EntityID const preloadEntity = Core::CreateEntity();
		Game::UI::SceneLoadDesc startScene;
		startScene.m_nextScene = std::make_shared<Game::Scene::GinRummy>();
		Core::AddComponent( preloadEntity, startScene );
	}

	// Finalise initial entities
	Core::ECS::CommitChanges();
}

void Frame()
{
	{
		Core::FrameData& fd = Core::GetGlobalComponent<Core::FrameData>();
		uint64 const lappedTicks = stm_laptime(&fd.m_lastFrameTicks);
		// stm_round_to_common_refresh_rate?
		fd.unscaled_ddt = stm_sec(lappedTicks);
		fd.unscaled_dt = static_cast< Vec1 >(fd.unscaled_ddt);
		fd.ddt = fd.m_scale * fd.unscaled_ddt;
		fd.dt = static_cast< Vec1 >(fd.ddt);

#if DEBUG_TOOLS
		fd.m_debug_elapsedTime += fd.unscaled_ddt;
		fd.m_debug_frameCount++;
#endif
	}

	{
		Core::Render::FrameData& rfd = Core::GetGlobalComponent<Core::Render::FrameData>();
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

void Cleanup()
{
	Core::DestroyAllEntities();

	Core::Physics::Cleanup();
	Core::Sound::Cleanup();
	Core::Render::DImGui::Cleanup();
	Core::Render::TextAndGLDebug::Cleanup();
	Core::Render::Cleanup();
	Core::Resource::Cleanup();
}

void Event(sapp_event const* _event)
{
	Core::Render::TextAndGLDebug::Event(_event);
	Core::Render::DImGui::Event(_event);
	Core::Input::Event(_event);
}

void Fail(char const* _error)
{
	kaError(_error);
}