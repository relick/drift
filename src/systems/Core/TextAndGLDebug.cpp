#include "TextAndGLDebug.h"

#include "components.h"
#include "SystemOrdering.h"
#include "ImGui.h"

#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>

// sokol gl+text
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define SOKOL_GL_IMPL
#include <util/sokol_gl.h>
#define FONTSTASH_IMPLEMENTATION
#include <fontstash.h>
#define SOKOL_FONTSTASH_IMPL
#include <util/sokol_fontstash.h>

#include <imgui.h>
#include <ecs/ecs.h>

#include <vector>
#include <mutex>
#include <unordered_map>

FONScontext* fonsContext{ nullptr };
uint32 fonsFontCount = 0;

#define TEXT_TEST DEBUG_TOOLS

#if TEXT_TEST
struct FontTest
{
	int fontNormal;
	fVec2 pos{ 10, 100 };
	fVec2 sizes{ 124.0f, 24.0f };
	unsigned int brown = sfons_rgba(192, 128, 0, 128);
	bool showImguiWin = false;
	bool showText = false;
	bool showDebug = false;
} fsTest;
#endif

namespace Core
{
	namespace Render
	{
		struct LineToDraw
		{
			fVec3 m_start{};
			fVec3 m_end{};
		};
		std::unordered_map<uint32, std::vector<LineToDraw>> linesToDraw{};
		std::mutex linesToDrawLock{};

		void FlushGL()
		{
			sgl_begin_lines();
			for (auto const& [col, lines] : linesToDraw)
			{
				sgl_c1i(col);
				for (auto const& line : lines)
				{
					sgl_v3f(line.m_start.x, line.m_start.y, line.m_start.z);
					sgl_v3f(line.m_end.x, line.m_end.y, line.m_end.z);
				}
			}
			sgl_end();
			linesToDraw.clear();
		}

		namespace TextAndGLDebug
		{
			void Init()
			{
				// gl for debug + text
				sgl_desc_t glDesc{};
				glDesc.color_format = static_cast<sg_pixel_format>(sapp_color_format());
				glDesc.depth_format = static_cast<sg_pixel_format>(sapp_depth_format());
				glDesc.sample_count = sapp_sample_count();
				sgl_setup(&glDesc);

				fonsContext = sfons_create(512, 512, FONS_ZERO_TOPLEFT);
			}

			void FlushGL();
			void Setup()
			{
				// Prepare GL matrices.
				ecs::make_system<ecs::opts::group<Sys::GL_START>>([](Core::MT_Only&, Core::Render::FrameData const& _rfd, Core::Render::Camera const& _cam, Core::Transform const& _camT, Core::Render::DefaultPass_Tag)
				{
					sgl_defaults();
					sgl_matrix_mode_projection();
					sgl_load_identity();
					sgl_perspective(glm::radians(_cam.m_povY), _rfd.fW / _rfd.fH, 0.01f, 1000.0f);

					sgl_matrix_mode_modelview();
					sgl_load_identity();

					fTrans const cameraTrans = _camT.CalculateWorldTransform();
					fMat4 const cameraMat = glm::lookAt(cameraTrans.m_origin, cameraTrans.m_origin + cameraTrans.m_basis[2], fVec3(0.0f, 1.0f, 0.0f));
					sgl_load_matrix(&cameraMat[0][0]);
				});

				// Prepare text matrices.
				ecs::make_system<ecs::opts::group<Sys::TEXT_START>>([](Core::MT_Only&, Core::Render::FrameData const& _rfd, Core::Render::Camera const& _cam, Core::Transform const& _camT, Core::Render::DefaultPass_Tag)
				{
					FlushGL();

					sgl_defaults();

					sgl_matrix_mode_projection();
					sgl_load_identity();
					sgl_ortho(0, _rfd.fW, _rfd.fH, 0, -1, 1);

					sgl_matrix_mode_modelview();
					sgl_load_identity();
				});

#if TEXT_TEST
				// Add font to stash.
				fsTest.fontNormal = fonsAddFont(fonsContext, "roboto", "assets/fonts/Roboto-Light.ttf");
				fonsFontCount++;

				Core::Render::DImGui::AddMenuItem("GL", "Text Debug", &fsTest.showImguiWin);

				ecs::make_system<ecs::opts::group<Sys::IMGUI>>([](Core::MT_Only&, Core::GlobalWorkaround_Tag)
				{
					if (fsTest.showImguiWin)
					{
						if (ImGui::Begin("Text Debug", &fsTest.showImguiWin, 0))
						{
							ImGui::Checkbox("Show Text", &fsTest.showText);
							ImGui::DragFloat2("Text sizes", &fsTest.sizes[0], 10.0f, 0.0f, 124.0f);
							ImGui::Checkbox("Show Debug", &fsTest.showDebug);
							ImGui::DragFloat2("Positions", &fsTest.pos[0], 0.1f, 0.0f, 640.0f);
						}
						ImGui::End();
					}
				});

				ecs::make_system<ecs::opts::group<Sys::TEXT>>([](Core::MT_Only&, Core::GlobalWorkaround_Tag)
				{
					fonsClearState(fonsContext);

					if (fsTest.showText)
					{
						RenderText(fsTest.fontNormal, fsTest.pos, "The big ", fsTest.sizes[0]);
						RenderText(fsTest.fontNormal, fsTest.pos, "brown fox", fsTest.sizes[1], fsTest.brown);
					}
					if (fsTest.showDebug)
					{
						fonsDrawDebug(fonsContext, fsTest.pos.x, fsTest.pos.y);
					}
				});
#endif
			}

			void Render()
			{
				// Flush the text before drawing
				sfons_flush(fonsContext);
				sgl_draw();
			}

			void Cleanup()
			{
				sfons_destroy(fonsContext);
				fonsContext = nullptr;
				sgl_shutdown();
			}
		}

		namespace Debug
		{
			// could buffer this at some point I guess.
			void DrawLine
			(
				fVec3 const& _start,
				fVec3 const& _end,
				uint32 _col
			)
			{
				std::scoped_lock lock(linesToDrawLock);
				linesToDraw[_col].push_back(LineToDraw{ _start, _end });
			}
		}

		namespace Text
		{
			bool Write
			(
				uint32 _fontI,
				fVec2 _tlPos,
				char const* _text,
				float _size,
				uint32 _col
			)
			{
				if (!fonsContext)
				{
					return false;
				}
				if (_fontI >= fonsFontCount)
				{
					return false;
				}

				fonsSetFont(fonsContext, _fontI);
				fonsSetSize(fonsContext, _size);
				fonsSetColor(fonsContext, _col);
				fonsDrawText(fonsContext, _tlPos.x, _tlPos.y, _text, NULL);

				return true;
			}
		}
	}
}
