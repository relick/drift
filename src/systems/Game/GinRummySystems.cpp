#include "GinRummySystems.h"

#include "components.h"
#include <ecs/ecs.h>

#include "managers/RenderManager.h"
#include "managers/InputManager.h"

namespace Game
{
namespace GinRummy
{

static void GameSystem
(
	Game::GinRummy::GameData& _gameData
)
{

}

static void DrawMat
(
	Game::GinRummy::Mat const& _mat
)
{
	Trans2D fullScreenSpriteTrans; // default will cover the whole screen.
	fullScreenSpriteTrans.m_z = 0.5f; // set to the back
	Core::Render::AddSpriteToScene( _mat.m_matSprite, fullScreenSpriteTrans );
}

static void DrawGame
(
	Game::GinRummy::GameData const& _gameData,
	Game::GinRummy::GameRender& _gameRender
)
{
	Trans2D left; // default will place in middle, one card
	left.m_pos = { 25, 25 };
	left.m_z = 0.75f;
	Trans2D right; // default will place in middle, one card
	right.m_pos = { 75, 25 };
	right.m_z = 0.75f;
	Core::Render::AddSpriteToScene( _gameRender.m_cardFront[0], left);
	Core::Render::AddSpriteToScene( _gameRender.m_cardBack, right);
}

void Setup()
{
	Core::MakeSystem<Sys::GAME>( GameSystem );

	Core::MakeSystem<Sys::RENDER_QUEUE>( DrawMat );
	Core::MakeSystem<Sys::RENDER_QUEUE>( DrawGame );
}

}
}