#pragma once

namespace Sys
{
	enum SystemOrdering
	{
	FRAME_START,

		FILE_LOADING, // main thread only
		UI_UPDATE, // usually doesn't want to act the same time as GAME, but does want to be before sending off text and render requests and after a game update.

		GL_START, // GL drawing can happen anywhere between here and GL_END
		// Render section - can NOT be parallel. Use MT_Only global component as non-const ref to force serial
		// This section is for lining up the scene ready to be drawn when the frame decides to
		RENDER_PASS_START,
			IMGUI,
			GL_END, // Don't do any more GL drawing after this until GL_START
			TEXT_START, // sets up ortho matrices
			TEXT,
			RENDER_QUEUE, // now's a good time to submit stuff to draw that captures a certain state.

		// Game section - can be parallel
		GAME_START,
			GAME,
			RENDER,// = GAME, //todo can't do this, must run on main thread... // run bulk of the render code during the GAME section.
			RENDER_PASS_END,

			PHYSICS_TRANSFORMS_IN,
			PHYSICS_STEP,
			PHYSICS_TRANSFORMS_OUT,
		GAME_END,

	FRAME_END,
	};
}