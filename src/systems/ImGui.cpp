#include "ImGui.h"

#include "components.h"
#include "SystemOrdering.h"

#include <sokol_app.h>
#include <sokol_gfx.h>

#include <imgui.h>
#define SOKOL_IMGUI_IMPL
#include <util/sokol_imgui.h>
#define SOKOL_GFX_IMGUI_IMPL
#include <util/sokol_gfx_imgui.h>

#include <sokol_time.h>
#include <ecs/ecs.h>

sg_imgui_t gfxImGuiState{};

namespace Core
{
	namespace Render
	{
		void ImGuiSetup()
		{
#if IMGUI_DEBUG_ENABLED
			simgui_desc_t simguiDesc{};
			simguiDesc.dpi_scale = sapp_dpi_scale();
			simguiDesc.ini_filename = "ImGuiSettings.ini";
			simgui_setup(&simguiDesc);
			sg_imgui_init(&gfxImGuiState);

			ecs::make_system<ecs::opts::group<Sys::FRAME_START>>([](Core::FrameData const& _fd, Core::Render::FrameData const& _rfd, Core::Render::Frame_Tag)
			{
				simgui_new_frame(_rfd.w, _rfd.h, _fd.unscaled_ddt > 0.0 ? _fd.unscaled_ddt : DBL_EPSILON);
			});

			ecs::make_system<ecs::opts::group<Sys::IMGUI>>([](Core::FrameData const& _fd, Core::Render::FrameData const& _rfd, Core::Render::Frame_Tag)
			{
				// Draw gfx debug UI
				sg_imgui_draw(&gfxImGuiState);

				// Draw menu bar
				if (ImGui::BeginMainMenuBar())
				{
					// Draw gfx toggler
					if (ImGui::BeginMenu("sokol-gfx"))
					{
						ImGui::MenuItem("Buffers", 0, &gfxImGuiState.buffers.open);
						ImGui::MenuItem("Images", 0, &gfxImGuiState.images.open);
						ImGui::MenuItem("Shaders", 0, &gfxImGuiState.shaders.open);
						ImGui::MenuItem("Pipelines", 0, &gfxImGuiState.pipelines.open);
						ImGui::MenuItem("Passes", 0, &gfxImGuiState.passes.open);
						ImGui::MenuItem("Calls", 0, &gfxImGuiState.capture.open);
						ImGui::EndMenu();
					}

					ImGui::Separator();

					static uint64 ticks = stm_now();
					double const timeSince = stm_sec(stm_since(ticks));

					static int fps = 0;
					++fps;
					static int lastFPS = 0;

					if (timeSince > 1.0)
					{
						// Update fps each second.
						ticks = stm_now();
						lastFPS = fps;
						fps = 0;
					}

					ImGui::Text("%iw/%ih", _rfd.w, _rfd.h);
					ImGui::Separator();

					ImGui::Text("FPS: %i", lastFPS);
					ImGui::Separator();



					ImGui::EndMainMenuBar();
				}
			});
#endif
		}

		void ImGuiRender()
		{
#if IMGUI_DEBUG_ENABLED
			simgui_render();
#endif
		}

		void ImGuiCleanup()
		{
#if IMGUI_DEBUG_ENABLED
			sg_imgui_discard(&gfxImGuiState);
			simgui_shutdown();
#endif
		}

		void ImGuiEvent(sapp_event const* _event)
		{
#if IMGUI_DEBUG_ENABLED
			simgui_handle_event(_event);
#endif
		}
	}
}
