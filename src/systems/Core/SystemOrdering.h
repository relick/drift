#pragma once

namespace Sys
{
	enum SystemOrdering
	{
	FRAME_START,
		GL_START, // GL drawing can happen anywhere between here and GL_END
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
			GL_END, // Don't do any more GL drawing after this until GL_START
			TEXT_START, // sets up matrices
			TEXT,
			DEFAULT_PASS_START,
				RENDER_ADD, // fill in drawing data
				RENDER_PASSES, // make draw calls for each pass
			DEFAULT_PASS_END,
		RENDER_END,
	FRAME_END,
	};
}