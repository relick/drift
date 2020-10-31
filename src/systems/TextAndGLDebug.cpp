#include "TextAndGLDebug.h"

#include "components.h"
#include "SystemOrdering.h"

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

#include <ecs/ecs.h>

struct FontTest
{
	FONScontext* fonsContext;
	int fontNormal;
	float dx = 10, dy = 100;
	unsigned int white = sfons_rgba(255, 255, 255, 255);
	unsigned int brown = sfons_rgba(192, 128, 0, 128);
} fsTest;

namespace Core
{
	namespace Render
	{
		void TextAndGLDebugSetup()
		{
			// gl for debug + text
			sgl_desc_t glDesc{};
			glDesc.color_format = static_cast<sg_pixel_format>(sapp_color_format());
			glDesc.depth_format = static_cast<sg_pixel_format>(sapp_depth_format());
			glDesc.sample_count = sapp_sample_count();
			sgl_setup(&glDesc);

			fsTest.fonsContext = sfons_create(512, 512, FONS_ZERO_TOPLEFT);

			// text test
			// Add font to stash.
			fsTest.fontNormal = fonsAddFont(fsTest.fonsContext, "roboto", "assets/fonts/Roboto-Light.ttf");
		}

		void TextAndGLDebugRender
		(
			int _w,
			int _h
		)
		{
			// text test
			{
				fonsClearState(fsTest.fonsContext);

				fonsSetFont(fsTest.fonsContext, fsTest.fontNormal);
				fonsSetSize(fsTest.fonsContext, 124.0f);
				fonsSetColor(fsTest.fonsContext, fsTest.white);
				fonsDrawText(fsTest.fonsContext, fsTest.dx, fsTest.dy, "The big ", NULL);

				fonsSetSize(fsTest.fonsContext, 24.0f);
				fonsSetColor(fsTest.fonsContext, fsTest.brown);
				fonsDrawText(fsTest.fonsContext, fsTest.dx, fsTest.dy, "brown fox", NULL);

				fonsDrawDebug(fsTest.fonsContext, -2.0, -2.0);
			}

			{
				sgl_defaults();

				sgl_matrix_mode_projection();
				sgl_load_identity();
				sgl_ortho(0, _w, _h, 0, -1, 1);

				sgl_matrix_mode_modelview();
				sgl_load_identity();

				sfons_flush(fsTest.fonsContext);
			}
			sgl_draw();
		}

		void TextAndGLDebugCleanup()
		{
			sfons_destroy(fsTest.fonsContext);
			sgl_shutdown();
		}
	}
}
