﻿#include "ImGuiSystems.h"

#include "components.h"

#include <sokol_app.h>
#include <sokol_gfx.h>

#include <imgui.h>
#include <util/sokol_imgui.h>
#include <util/sokol_gfx_imgui.h>

#include <sokol_time.h>
#include <ecs/ecs.h>

#include <string>
#include <unordered_map>

static sg_imgui_t g_gfxImGuiState{};

struct MenuItem
{
	char const* m_name;
	bool* m_open;
};
static std::unordered_multimap<std::string, MenuItem> g_imguiMenuItems;

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
				simguiDesc.ini_filename = "ImGuiSettings.ini";
				simguiDesc.no_default_font = true;
				simgui_setup(&simguiDesc);
				sg_imgui_init(&g_gfxImGuiState);

				// use ms gothic
				{
					ImGuiIO& io = ImGui::GetIO();
					io.Fonts->AddFontFromFileTTF("assets/encrypted/fonts/msgothic.ttc", 13.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());

					sg_image_desc imgDesc{
						.pixel_format = SG_PIXELFORMAT_RGBA8,
						.min_filter = SG_FILTER_LINEAR,
						.mag_filter = SG_FILTER_LINEAR,
						.wrap_u = SG_WRAP_CLAMP_TO_EDGE,
						.wrap_v = SG_WRAP_CLAMP_TO_EDGE,
						.label = "sokol-imgui-font",
					};
					int bytesPerPixel = sizeof(uint32);
					uint8* pixelData = nullptr;
					io.Fonts->GetTexDataAsRGBA32(&pixelData, &imgDesc.width, &imgDesc.height, &bytesPerPixel);
					imgDesc.data.subimage[0][0].ptr = pixelData;
					imgDesc.data.subimage[0][0].size = static_cast<usize>(imgDesc.width) * static_cast<usize>(imgDesc.height) * static_cast<usize>(bytesPerPixel);

					simgui_setfont(sg_make_image(imgDesc));
				}
#endif
			}

			void Setup()
			{
#if IMGUI_DEBUG_ENABLED

				Core::MakeSystem<Sys::FRAME_START>([](Core::FrameData const& _fd, Core::Render::FrameData const& _rfd)
				{
					simgui_new_frame(
						{
							.width = _rfd.contextWindow.i.x,
							.height = _rfd.contextWindow.i.y,
							.delta_time = _fd.unscaled_ddt > 0.0 ? _fd.unscaled_ddt : DBL_EPSILON,
							.dpi_scale = sapp_dpi_scale() // TODO: need to fix?
						}
					);
				});

				Core::MakeSystem<Sys::IMGUI>([](Core::MT_Only&, Core::FrameData const& _fd, Core::Render::FrameData const& _rfd)
				{
					// Draw gfx debug UI
					sg_imgui_draw(&g_gfxImGuiState);

					// Draw menu bar
					if (ImGui::BeginMainMenuBar())
					{
						// Draw gfx toggler
						if (ImGui::BeginMenu("sokol-gfx"))
						{
							ImGui::MenuItem("Buffers", nullptr, &g_gfxImGuiState.buffers.open);
							ImGui::MenuItem("Images", nullptr, &g_gfxImGuiState.images.open);
							ImGui::MenuItem("Shaders", nullptr, &g_gfxImGuiState.shaders.open);
							ImGui::MenuItem("Pipelines", nullptr, &g_gfxImGuiState.pipelines.open);
							ImGui::MenuItem("Passes", nullptr, &g_gfxImGuiState.passes.open);
							ImGui::MenuItem("Calls", nullptr, &g_gfxImGuiState.capture.open);
							ImGui::EndMenu();
						}

						// Custom menu items from elsewhere
						std::string currentHeader;
						bool currentHeaderIsOpen = false;
						for (auto const& menuPair : g_imguiMenuItems)
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
								ImGui::MenuItem(menuPair.second.m_name, nullptr, menuPair.second.m_open);
							}
						}
						if (currentHeaderIsOpen)
						{
							ImGui::EndMenu();
						}
						ImGui::Separator();

						static uint64 ticks = stm_now();
						dVec1 const timeSince = stm_sec(stm_since(ticks));

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

						ImGui::Text("%iw/%ih", _rfd.contextWindow.i.x, _rfd.contextWindow.i.y);
						ImGui::Separator();

						ImGui::Text("%i FPS", lastFPS);
						ImGui::Separator();
#if SOKOL_D3D11
						ImGui::TextUnformatted("Direct3D 11");
#elif SOKOL_GLCORE33
						ImGui::TextUnformatted("OpenGL Core 3.30");
#endif
						ImGui::Separator();

						ImGui::Text("Speed: %.2fx", _fd.m_scale);
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
				sg_imgui_discard(&g_gfxImGuiState);
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
				g_imguiMenuItems.insert({ std::string(_header), { _name, _open } });
			}
		}
	}
}
