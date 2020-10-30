#pragma once

namespace Sys
{
	enum SystemOrdering
	{
	FRAME_START,
		GAME_START,
			GAME,
		GAME_END,
		RENDER_START,
			IMGUI,
			DEFAULT_PASS_START,
				DEFAULT_PASS,
			DEFAULT_PASS_END,
		RENDER_END,
	FRAME_END,
	};
}