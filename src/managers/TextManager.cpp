#include "TextManager.h"

#include "components.h"
#include "systems/Core/ImGuiSystems.h"
#include "managers/EntityManager.h"

#include <imgui.h>

#include <sokol_app.h>
#include <sokol_gfx.h>

#include <fontstash.h>
#include <util/sokol_fontstash.h>

namespace Core::Render::Text
{
	static FONScontext* g_fonsContext{ nullptr };
	static int32 g_fonsFontCount{ 0 };
	static Resource::FontID g_defaultFont{ 0 };

#if TEXT_TEST
	struct FontTest
	{
		Resource::FontID fontNormal;
		Vec2 pos{ 10, 100 };
		Vec2 sizes{ 124.0f, 24.0f };
		uint32 brown = sfons_rgba(192, 128, 0, 128);
		bool showImguiWin = false;
		bool showText = false;
		bool showDebug = false;
	};
	static FontTest g_fsTest;
#endif

	struct
	{
		Core::Render::FrameData rfd;
	} fontState;

	void Init()
	{
		g_fonsContext = sfons_create(512, 512, FONS_ZERO_TOPLEFT);

#if TEXT_TEST
		// Add font to stash.
		g_fsTest.fontNormal = fonsAddFont(g_fonsContext, "roboto", "assets/encrypted/fonts/MS Gothic.ttf");
		g_fonsFontCount++;
		g_defaultFont = g_fsTest.fontNormal;
#endif
	}

	void Setup()
	{
		Core::MakeSystem<Sys::TEXT_START>([](Core::MT_Only&, Core::Render::FrameData const& _rfd)
		{
			fontState.rfd = _rfd;
		});

#if TEXT_TEST
		Core::Render::DImGui::AddMenuItem("GL", "Text Debug", &g_fsTest.showImguiWin);

		Core::MakeSystem<Sys::IMGUI>([](Core::MT_Only&)
		{
			if (g_fsTest.showImguiWin)
			{
				if (ImGui::Begin("Text Debug", &g_fsTest.showImguiWin, 0))
				{
					ImGui::Checkbox("Show Text", &g_fsTest.showText);
					ImGui::DragFloat2("Text sizes", &g_fsTest.sizes[0], 10.0f, 0.0f, 124.0f);
					ImGui::Checkbox("Show Debug", &g_fsTest.showDebug);
					ImGui::DragFloat2("Positions", &g_fsTest.pos[0], 0.1f, 0.0f, 640.0f);
				}
				ImGui::End();
			}
		});

		Core::MakeSystem<Sys::TEXT>([](Core::MT_Only&)
		{
			Core::Render::Text::ShowDebugText();
		});
#endif
	}

	void Render()
	{
		sfons_flush(g_fonsContext);
	}

	void Cleanup()
	{
		sfons_destroy(g_fonsContext);
		g_fonsContext = nullptr;
	}

	void Event(sapp_event const* _event)
	{
		if (_event->type == SAPP_EVENTTYPE_RESIZED)
		{
			fonsResetAtlas(g_fonsContext, 512, 512);
		}
	}

#if TEXT_TEST
	void ShowDebugText()
	{
		fonsClearState(g_fonsContext);

		if (g_fsTest.showText)
		{
			Text::Write(g_fsTest.fontNormal, g_fsTest.pos, "The big ", g_fsTest.sizes[0]);
			Text::Write(g_fsTest.fontNormal, g_fsTest.pos, "brown fox", g_fsTest.sizes[1], g_fsTest.brown);
		}
		if (g_fsTest.showDebug)
		{
			fonsDrawDebug(g_fonsContext, g_fsTest.pos.x, g_fsTest.pos.y);
		}
	}
#endif

	bool Write
	(
		Resource::FontID _font,
		Vec2 _tlPos,
		char const* _text,
		Vec1 _size,
		uint32 _col
	)
	{
		if (!g_fonsContext)
		{
			return false;
		}
		if (_font.GetValue() >= g_fonsFontCount)
		{
			return false;
		}

		Vec2 const renderAreaToContextWindow = fontState.rfd.contextWindow.f / fontState.rfd.renderArea.f;

		_tlPos *= renderAreaToContextWindow;

		fonsSetFont(g_fonsContext, _font.GetValue());
		fonsSetSize(g_fonsContext, _size * renderAreaToContextWindow.y);
		fonsSetColor(g_fonsContext, _col);
		fonsDrawText(g_fonsContext, _tlPos.x, _tlPos.y, _text, nullptr);

		return true;
	}

	bool Write
	(
		Vec2 _tlPos,
		char const* _text,
		Vec1 _size,
		uint32 _col
	)
	{
		return Write(g_defaultFont, _tlPos, _text, _size, _col);
	}
}