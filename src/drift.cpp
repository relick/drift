#include "common.h"

InputStuff inputStuff{};

#include "components.h"
#include "systems.h"

#include <ecs/ecs.h>

#include "managers/EntityManager.h"

#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_CPP_MODE
#include "HandmadeMath.h"

// sokol app+gfx
#define SOKOL_IMPL
#define SOKOL_TRACE_HOOKS
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_time.h>

#include <util/sokol_gl.h>

#include "CubeTest2.h"

void initialise_cb()
{
	// sokol setup start
	{
		sg_desc gfxDesc{};
		gfxDesc.context = sapp_sgcontext();
		sg_setup(&gfxDesc);
		Core::Render::TextAndGLDebug::Init();
		Core::Render::DImGui::Init();
		stm_setup();
	}
	// sokol setup done

	// lock mouse
	sapp_lock_mouse(true);

	// Setup entity manager
	ecs::entity_id global = Core::CreateEntity();
	ecs::add_component(global, Core::GlobalWorkaround_Tag());

	ecs::entity_id renderEntity = Core::CreateEntity();
	ecs::add_component(renderEntity, Core::Render::Frame_Tag());
	ecs::add_component(renderEntity, Core::Render::DefaultPass_Tag());
	ecs::add_component(renderEntity, Core::Transform(fQuat::getIdentity(), LoadVec3(1.4f, 1.5f, 4.0f))); // camera transform
	ecs::add_component(renderEntity, Core::Render::Camera());
	ecs::add_component(renderEntity, Core::Render::DebugCameraControl_Tag());
	Core::Render::DImGui::Setup();

	// Setup entity initialisers
	setup_cube2();

	// Setup systems
	// debug camera control
	ecs::make_system<ecs::opts::group<Sys::GAME>>([](Core::FrameData const& _fd, Core::Render::Camera& _cam, Core::Transform& _t, Core::Render::DebugCameraControl_Tag)
	{
		_cam.angle.x -= inputStuff.mouse_dy * 0.01f;
		_cam.angle.y -= inputStuff.mouse_dx * 0.01f;

		_t.T().getBasis().setEulerZYX(_cam.angle.x, _cam.angle.y, 0.0f);

		fVec3 front;
		front.z = -cos(_cam.angle.y) * cos(_cam.angle.x);
		front.y = sin(_cam.angle.x);
		front.x = -sin(_cam.angle.y) * cos(_cam.angle.x);
		front.normalize();
		// also re-calculate the Right and Up vector
		fVec3 right = front.cross(LoadVec3(0.0f, 1.0f, 0.0f)).normalize();  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.

		float velocity = 0.8f * _fd.dt;
		if (inputStuff.up)
			_t.T().getOrigin() += front * velocity;
		if (inputStuff.down)
			_t.T().getOrigin() -= front * velocity;
		if (inputStuff.left)
			_t.T().getOrigin() -= right * velocity;
		if (inputStuff.right)
			_t.T().getOrigin() += right * velocity;
	});

	ecs::make_system<ecs::opts::group<Sys::FRAME_START>>([](Core::FrameData& _fd, Core::GlobalWorkaround_Tag)
	{
		uint64 const lappedTicks = stm_laptime(&_fd.m_lastFrameTicks);
		// stm_round_to_common_refresh_rate?
		_fd.unscaled_ddt = stm_sec(lappedTicks);
		_fd.unscaled_dt = static_cast<float>(_fd.unscaled_ddt);
		_fd.ddt = _fd.m_scale * _fd.unscaled_ddt;
		_fd.dt = static_cast<float>(_fd.ddt);
	});

	ecs::make_system<ecs::opts::group<Sys::RENDER_START>>([](Core::MT_Only&, Core::Render::FrameData& _rfd, Core::GlobalWorkaround_Tag)
	{
		_rfd.w = sapp_width();
		_rfd.fW = static_cast<float>(_rfd.w);
		_rfd.h = sapp_height();
		_rfd.fH = static_cast<float>(_rfd.h);
	});

	ecs::make_system<ecs::opts::group<Sys::RENDER_START>>([](Core::MT_Only&, Core::Render::FrameData const& _rfd, Core::Render::Frame_Tag)
	{
		// update sgl viewport
		sgl_viewport(0, 0, _rfd.w, _rfd.h, true);
		sgl_scissor_rect(0, 0, _rfd.w, _rfd.h, true);

	});

	ecs::make_system<ecs::opts::group<Sys::DEFAULT_PASS_START>>([](Core::MT_Only&, Core::Render::FrameData const& _rfd, Core::Render::DefaultPass_Tag)
	{
		sg_pass_action pass_action{};
		sg_color_attachment_action color_attach_action{};
		color_attach_action.action = SG_ACTION_CLEAR;
		color_attach_action.val[0] = 0.1f;
		color_attach_action.val[1] = 0.1f;
		color_attach_action.val[2] = 0.1f;
		color_attach_action.val[3] = 1.0f;
		pass_action.colors[0] = color_attach_action;

		sg_begin_default_pass(&pass_action, _rfd.w, _rfd.h);
	});

	ecs::make_system<ecs::opts::group<Sys::DEFAULT_PASS_END>>([](Core::MT_Only&, Core::Render::FrameData const& _rfd, Core::Render::DefaultPass_Tag)
	{
		// End of the default pass -
		// 3D scene drawn already, text layer next, imgui last.
		Core::Render::TextAndGLDebug::Render(_rfd.w, _rfd.h);
		Core::Render::DImGui::Render();
		sg_end_pass();
	});

	ecs::make_system<ecs::opts::group<Sys::RENDER_END>>([](Core::MT_Only&, Core::Render::Frame_Tag)
	{
		sg_commit();
	});

	// Finalise initial components
	ecs::commit_changes();

}

bool receivedMouseEvent = true;

void frame_cb()
{
	if (!receivedMouseEvent)
	{
		inputStuff.mouse_dx = 0.0f;
		inputStuff.mouse_dy = 0.0f;
	}
	receivedMouseEvent = false;
	ecs::update();
}

void cleanup_cb()
{
	Core::Render::DImGui::Cleanup();
	Core::Render::TextAndGLDebug::Cleanup();
	sg_shutdown();
}

void event_cb(sapp_event const* _event)
{
	Core::Render::DImGui::Event(_event);

	switch (_event->type)
	{
	case SAPP_EVENTTYPE_KEY_DOWN:
	{
		switch (_event->key_code)
		{
		case SAPP_KEYCODE_ESCAPE:
		{
			sapp_request_quit();
			break;
		}
		case SAPP_KEYCODE_W:
		{
			inputStuff.up = true;
			break;
		}
		case SAPP_KEYCODE_A:
		{
			inputStuff.left = true;
			break;
		}
		case SAPP_KEYCODE_S:
		{
			inputStuff.down = true;
			break;
		}
		case SAPP_KEYCODE_D:
		{
			inputStuff.right = true;
			break;
		}
		}
		break;
	}
	case SAPP_EVENTTYPE_KEY_UP:
	{
		switch (_event->key_code)
		{
		case SAPP_KEYCODE_W:
		{
			inputStuff.up = false;
			break;
		}
		case SAPP_KEYCODE_A:
		{
			inputStuff.left = false;
			break;
		}
		case SAPP_KEYCODE_S:
		{
			inputStuff.down = false;
			break;
		}
		case SAPP_KEYCODE_D:
		{
			inputStuff.right = false;
			break;
		}
		}
		break;
	}
	case SAPP_EVENTTYPE_MOUSE_MOVE:
		receivedMouseEvent = true;
		inputStuff.mouse_dx = _event->mouse_dx;
		inputStuff.mouse_dy = _event->mouse_dy;
		break;
	default:
		break;
	}
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

	desc.width = WINDOW_START_WIDTH;
	desc.height = WINDOW_START_HEIGHT;
	desc.sample_count = 4;
	desc.window_title = "drift";

	return desc;
}