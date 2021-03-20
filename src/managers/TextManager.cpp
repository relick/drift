#include "TextManager.h"

#include "components.h"
#include "systems/Core/ImGuiSystems.h"
#include "SystemOrdering.h"
#include "managers/EntityManager.h"

#include <imgui.h>

#include <sokol_gfx.h>

#include <fontstash.h>
#include <util/sokol_fontstash.h>

namespace Core::Render::Text
{
	FONScontext* fonsContext{ nullptr };
	uint32 fonsFontCount = 0;

#if TEXT_TEST
	struct FontTest
	{
		Resource::FontID fontNormal;
		fVec2 pos{ 10, 100 };
		fVec2 sizes{ 124.0f, 24.0f };
		unsigned int brown = sfons_rgba(192, 128, 0, 128);
		bool showImguiWin = false;
		bool showText = false;
		bool showDebug = false;
		Core::Render::FrameData rfd;
	} fsTest;
#endif

	void Init()
	{
		fonsContext = sfons_create(512, 512, FONS_ZERO_TOPLEFT);

#if TEXT_TEST
		// Add font to stash.
		fsTest.fontNormal = fonsAddFont(fonsContext, "roboto", "assets/encrypted/fonts/MS Gothic.ttf");
		fonsFontCount++;
#endif
	}

	void Setup()
	{
		Core::MakeSystem<Sys::TEXT_START>([](Core::MT_Only&, Core::Render::FrameData const& _rfd)
		{
			fsTest.rfd = _rfd;
		});

#if TEXT_TEST
		Core::Render::DImGui::AddMenuItem("GL", "Text Debug", &fsTest.showImguiWin);

		Core::MakeSystem<Sys::IMGUI>([](Core::MT_Only&)
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

		Core::MakeSystem<Sys::TEXT>([](Core::MT_Only&)
		{
			Core::Render::Text::ShowDebugText();
		});
#endif
	}

	void Render()
	{
		sfons_flush(fonsContext);
	}

	void Cleanup()
	{
		sfons_destroy(fonsContext);
		fonsContext = nullptr;
	}

#if TEXT_TEST
	void ShowDebugText()
	{
		fonsClearState(fonsContext);

		if (fsTest.showText)
		{
			Text::Write(fsTest.fontNormal, fsTest.pos, "The big ", fsTest.sizes[0]);
			Text::Write(fsTest.fontNormal, fsTest.pos, "brown fox", fsTest.sizes[1], fsTest.brown);
		}
		if (fsTest.showDebug)
		{
			fonsDrawDebug(fonsContext, fsTest.pos.x, fsTest.pos.y);
		}
	}
#endif

	bool Write
	(
		Resource::FontID _font,
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
		if (_font.GetValue() >= fonsFontCount)
		{
			return false;
		}

		fVec2 const renderAreaToContextWindow = fsTest.rfd.contextWindow.f / fsTest.rfd.renderArea.f;

		_tlPos *= renderAreaToContextWindow;

		fonsSetFont(fonsContext, _font.GetValue());
		fonsSetSize(fonsContext, _size * renderAreaToContextWindow.y);
		fonsSetColor(fonsContext, _col);
		fonsDrawText(fonsContext, _tlPos.x, _tlPos.y, _text, NULL);

		return true;
	}

	bool Write
	(
		fVec2 _tlPos,
		char const* _text,
		float _size,
		uint32 _col
	)
	{
		return Write(fsTest.fontNormal, _tlPos, _text, _size, _col);
	}
}