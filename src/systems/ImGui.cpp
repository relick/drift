#include "ImGui.h"

#if IMGUI_DEBUG_ENABLED
#include "components.h"
#include "SystemOrdering.h"

#include <sokol_app.h>
#include <sokol_gfx.h>

#include <imgui.h>
#define SOKOL_IMGUI_IMPL
#include <util/sokol_imgui.h>
#define SOKOL_GFX_IMGUI_IMPL
#include <util/sokol_gfx_imgui.h>

#include <ecs/ecs.h>

sg_imgui_t gfxImGuiState;

namespace Core
{
	namespace Render
	{
		void ImGuiSetup()
		{
			simgui_desc_t simguiDesc{};
			simguiDesc.dpi_scale = sapp_dpi_scale();
			simguiDesc.ini_filename = "ImGuiSettings.ini";
			simgui_setup(&simguiDesc);
			sg_imgui_init(&gfxImGuiState);

			ecs::make_system<ecs::opts::group<Sys::FRAME_START>>([](Core::Render::FrameData& _fd, Core::Render::Frame_Tag)
			{
				simgui_new_frame(_fd.w, _fd.h, 0.0166666);
			});

			ecs::make_system<ecs::opts::group<Sys::IMGUI>>([](Core::Render::Frame_Tag)
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
					ImGui::EndMainMenuBar();
				}
			});
		}

		void ImGuiRender()
		{
			simgui_render();
		}

		void ImGuiCleanup()
		{
			sg_imgui_discard(&gfxImGuiState);
			simgui_shutdown();
		}

		void ImGuiEvent(sapp_event const* _event)
		{
			simgui_handle_event(_event);
		}
	}
}

#endif