#pragma once

namespace Sys
{
	enum SystemOrdering
	{
	FRAME_START,
		// Game section - can be parallel
		GAME_START,
			GAME,

			PHYSICS_TRANSFORMS_IN,
			PHYSICS_STEP,
			PHYSICS_TRANSFORMS_OUT,
		GAME_END,
		// Render section - can NOT be parallel. Use MT_Only global component as non-const ref to force serial
		RENDER_START,
			IMGUI,
			TEXT,
			DEFAULT_PASS_START,
				DEFAULT_PASS,
			DEFAULT_PASS_END,
		RENDER_END,
	FRAME_END,
	};
}