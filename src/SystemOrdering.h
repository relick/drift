#pragma once

namespace Sys
{
inline constexpr bool MainThread = true;
inline constexpr bool AnyThread = false;

#define SYSTEM_ORDERING_LIST \
X( FRAME_START, AnyThread ) \
	X( FILE_LOADING, MainThread ) /* main thread only */ \
	X( UI_UPDATE, AnyThread ) /* usually doesn't want to act the same time as GAME, but does want to be before sending off text and render requests and after a game update. */ \
	\
	X( GL_START, MainThread ) /* GL drawing can happen anywhere between here and GL_END */ \
	/* Render section - can NOT be parallel. Use MT_Only global component as non-const ref to force serial */ \
	/* This section is for lining up the scene ready to be drawn when the frame decides to */ \
	X( RENDER_PASS_START, MainThread ) \
		X( IMGUI, MainThread ) \
		X( GL_END, MainThread ) /* Don't do any more GL drawing after this until GL_START */ \
		X( TEXT_START, MainThread ) /* sets up ortho matrices */ \
		X( TEXT, MainThread ) \
		X( RENDER_QUEUE, AnyThread ) /* now's a good time to submit stuff to draw that captures a certain state. */ \
		\
		/* Game section - can be parallel */ \
	X( GAME_START, AnyThread ) \
		X( GAME, AnyThread ) \
		X( RENDER, MainThread ) /* = GAME, */ /* todo can't do this, must run on main thread... */ /* run bulk of the render code during the GAME section. */ \
		X( RENDER_PASS_END, MainThread ) \
		\
		X( PHYSICS_TRANSFORMS_IN, AnyThread ) \
		X( PHYSICS_STEP, AnyThread ) \
		X( PHYSICS_TRANSFORMS_OUT, AnyThread ) \
	X( GAME_END, AnyThread ) \
	\
X( FRAME_END, AnyThread )

#define X( NAME, THREADING ) NAME ,
enum SystemOrdering : int32
{
	SYSTEM_ORDERING_LIST

	COUNT,
};
#undef X

#define X( NAME, THREADING ) THREADING ,
inline constexpr bool c_mtOnlySysOrdering[ SystemOrdering::COUNT ] = {
	SYSTEM_ORDERING_LIST
};
#undef X

#undef SYSTEM_ORDERING_LIST

}