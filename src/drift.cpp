#include "common.h"

#include "components.h"
#include "systems.h"

#include <ecs/ecs.h>

#include "managers/EntityManager.h"

#define HANDMADE_MATH_IMPLEMENTATION
#include "HandmadeMath.h"

// sokol app+gfx
#define SOKOL_IMPL
#define SOKOL_TRACE_HOOKS
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>

#include "CubeTest.h"

void initialise_cb()
{
	// gfx setup start
	{
		sg_desc gfxDesc{};
		gfxDesc.context = sapp_sgcontext();
		sg_setup(&gfxDesc);
	}
#if IMGUI_DEBUG_ENABLED
	Core::Render::ImGuiSetup();
#endif
	// gfx setup done



	// Setup entity manager
	ecs::entity_id global = Core::CreateEntity();
	ecs::add_component(global, Core::GlobalWorkaround_Tag());

	ecs::entity_id renderEntity = Core::CreateEntity();
	ecs::add_component(renderEntity, Core::Render::Frame_Tag());
	ecs::add_component(renderEntity, Core::Render::DefaultPass_Tag());

	// Setup entity initialisers
	setup_cube();

	// Setup systems
	ecs::make_system<ecs::opts::group<Sys::RENDER_START>>([](Core::Render::FrameData& _fd, Core::GlobalWorkaround_Tag)
	{
		_fd.w = sapp_width();
		_fd.fW = static_cast<float>(_fd.w);
		_fd.h = sapp_height();
		_fd.fH = static_cast<float>(_fd.h);
	});

	ecs::make_system<ecs::opts::group<Sys::RENDER_START>>([](Core::Render::FrameData& _fd, Core::Render::Frame_Tag)
	{

	});

	ecs::make_system<ecs::opts::group<Sys::DEFAULT_PASS_START>>([](Core::Render::FrameData& _fd, Core::Render::DefaultPass_Tag)
	{
		sg_pass_action pass_action{};
		sg_color_attachment_action color_attach_action{};
		color_attach_action.action = SG_ACTION_CLEAR;
		color_attach_action.val[0] = 0.0f;
		color_attach_action.val[1] = 0.0f;
		color_attach_action.val[2] = 0.0f;
		color_attach_action.val[3] = 1.0f;
		pass_action.colors[0] = color_attach_action;

		sg_begin_default_pass(&pass_action, static_cast<int>(_fd.w), static_cast<int>(_fd.h));
	});

	ecs::make_system<ecs::opts::group<Sys::DEFAULT_PASS_END>>([](Core::Render::DefaultPass_Tag)
	{
#if IMGUI_DEBUG_ENABLED
		Core::Render::ImGuiRender();
#endif
		sg_end_pass();
	});

	ecs::make_system<ecs::opts::group<Sys::RENDER_END>>([](Core::Render::Frame_Tag)
	{
		sg_commit();
	});

	// Finalise initial components
	ecs::commit_changes();

}

void frame_cb()
{
	ecs::update();
}

void cleanup_cb()
{
#if IMGUI_DEBUG_ENABLED
	Core::Render::ImGuiCleanup();
#endif
	sg_shutdown();
}

void event_cb(sapp_event const* _event)
{
#if IMGUI_DEBUG_ENABLED
	Core::Render::ImGuiEvent(_event);
#endif
}

void fail_cb(char const* _error)
{

}

sapp_desc sokol_main(int argc, char* argv[])
{
	sapp_desc desc{};
	desc.init_cb = &initialise_cb;
	desc.frame_cb = &frame_cb;
	desc.cleanup_cb = &cleanup_cb;
	desc.event_cb = &event_cb;
	desc.fail_cb = &fail_cb;

	desc.width = 640;
	desc.height = 480;
	desc.sample_count = 4;
	desc.window_title = "drift";

	return desc;
}