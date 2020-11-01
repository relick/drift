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

#include <string>
#include <unordered_map>

sg_imgui_t gfxImGuiState{};

struct MenuItem
{
	char const* m_name;
	bool* m_open;
};
std::unordered_multimap<std::string, MenuItem> imguiMenuItems;

namespace Core
{
	namespace Render
	{
		namespace DImGui
		{
			void Init()
			{
#if IMGUI_DEBUG_ENABLED
				simgui_desc_t simguiDesc{};
				simguiDesc.dpi_scale = sapp_dpi_scale();
				simguiDesc.ini_filename = "ImGuiSettings.ini";
				simgui_setup(&simguiDesc);
				sg_imgui_init(&gfxImGuiState);
#endif
			}

			void Setup
			(
			)
			{
#if IMGUI_DEBUG_ENABLED

				ecs::make_system<ecs::opts::group<Sys::FRAME_START>>([](Core::FrameData const& _fd, Core::Render::FrameData const& _rfd, Core::Render::Frame_Tag)
				{
					simgui_new_frame(_rfd.w, _rfd.h, _fd.unscaled_ddt > 0.0 ? _fd.unscaled_ddt : DBL_EPSILON);
				});

				ecs::make_system<ecs::opts::group<Sys::IMGUI>>([](Core::MT_Only&, Core::Render::FrameData const& _rfd, Core::GlobalWorkaround_Tag)
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

						// Custom menu items from elsewhere
						std::string currentHeader;
						bool currentHeaderIsOpen = false;
						for (auto const& menuPair : imguiMenuItems)
						{
							if (currentHeader != menuPair.first)
							{
								if (currentHeaderIsOpen)
								{
									ImGui::EndMenu();
								}

								currentHeaderIsOpen = ImGui::BeginMenu(menuPair.first.data());
								currentHeader = menuPair.first;
							}
							if (currentHeaderIsOpen)
							{
								ImGui::MenuItem(menuPair.second.m_name, 0, menuPair.second.m_open);
							}
						}
						if (currentHeaderIsOpen)
						{
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

			void Render()
			{
#if IMGUI_DEBUG_ENABLED
				simgui_render();
#endif
			}

			void Cleanup()
			{
#if IMGUI_DEBUG_ENABLED
				sg_imgui_discard(&gfxImGuiState);
				simgui_shutdown();
#endif
			}

			void Event(sapp_event const* _event)
			{
#if IMGUI_DEBUG_ENABLED
				simgui_handle_event(_event);
#endif
			}
			void AddMenuItem
			(
				char const* _header,
				char const* _name,
				bool* _open
			)
			{
				imguiMenuItems.insert({ std::string(_header), { _name, _open } });
			}
		}
	}
}
