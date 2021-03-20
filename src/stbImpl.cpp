#include "common.h"

// All stb-style implementations should be defined in this file

#define SOKOL_IMPL
#if DEBUG_TOOLS
	#define SOKOL_TRACE_HOOKS
#else
	#define DISABLE_GL_ERROR
#endif
#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>
#include <sokol_time.h>
#include <sokol_fetch.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <imgui.h>
#define SOKOL_IMGUI_IMPL
#include <util/sokol_imgui.h>
#define SOKOL_GFX_IMGUI_IMPL
#include <util/sokol_gfx_imgui.h>

#define SOKOL_GL_IMPL
#include <util/sokol_gl.h>
#define FONTSTASH_IMPLEMENTATION
#define FONS_USE_FREETYPE
#include <fontstash.h>
#define SOKOL_FONTSTASH_IMPL
#include <util/sokol_fontstash.h>