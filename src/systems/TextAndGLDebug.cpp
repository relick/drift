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

FONScontext* fonsContext{ nullptr };
uint32 fonsFontCount = 0;

#define TEXT_TEST _DEBUG

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
							ImGui::DragFloat2("Text sizes", fsTest.sizes.m_floats, 10.0f, 0.0f, 124.0f);
							ImGui::Checkbox("Show Debug", &fsTest.showDebug);
							ImGui::DragFloat2("Positions", fsTest.pos.m_floats, 0.1f, 0.0f, 640.0f);
						}
						ImGui::End();
					}
				});

				ecs::make_system<ecs::opts::group<Sys::TEXT>>([](Core::MT_Only&, Core::GlobalWorkaround_Tag)
				{
					fonsClearState(fonsContext);

					if (fsTest.showText)
					{
						RenderText(fsTest.fontNormal, fsTest.pos, "The big ", fsTest.sizes.m_floats[0]);
						RenderText(fsTest.fontNormal, fsTest.pos, "brown fox", fsTest.sizes.m_floats[1], fsTest.brown);
					}
					if (fsTest.showDebug)
					{
						fonsDrawDebug(fonsContext, fsTest.pos.x, fsTest.pos.y);
					}
				});
#endif
			}

			void Render
			(
				float _w,
				float _h
			)
			{
				// Draw the text
				sgl_defaults();

				sgl_matrix_mode_projection();
				sgl_load_identity();
				sgl_ortho(0, _w, _h, 0, -1, 1);

				sgl_matrix_mode_modelview();
				sgl_load_identity();

				sfons_flush(fonsContext);
				sgl_draw();
			}

			void Cleanup()
			{
				sfons_destroy(fonsContext);
				fonsContext = nullptr;
				sgl_shutdown();
			}

			bool RenderText
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
