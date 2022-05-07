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
	Trans2D fullScreen; // default will cover the whole screen.
	fullScreen.m_z = -0.9f; // Put behind all cards
	Core::Render::AddSpriteToScene( _mat.m_matSprite, fullScreen );
}

static void DrawGame
(
	Game::GinRummy::GameData const& _gameData,
	Game::GinRummy::GameRender& _gameRender
)
{
	// Debug draw whole deck
	if constexpr ( false )
	{
		bool flip = true;
		for ( usize s = 0; s < 4; ++s )
		{
			Trans2D pos;
			pos.m_pos.y = ( Vec1 )s * 60.0f;
			for ( usize f = 0; f < 13; ++f )
			{
				pos.m_pos.x = ( Vec1 )f * 24.0f;
				pos.m_z = ( Vec1 )( s * 13 + f ) / 52.0f;
				if ( flip )
				{
					Core::Render::AddSpriteToScene( _gameRender.m_cardFront[ _gameData.m_deck.m_cards[ s * 13 + f ].DeckIndex() ], pos );
				}
				else
				{
					Core::Render::AddSpriteToScene( _gameRender.m_cardBack, pos );
				}
				flip = !flip;
			}
		}
	}

	constexpr Vec1 deckMaxHeight{ 10 };
	constexpr Vec1 cardHeight = deckMaxHeight / 52.0f;

	// Draw deck stack
	{
		Trans2D deckPos;
		deckPos.m_pos = { 40, 88 };
		for ( usize cardI = 0; cardI < _gameData.m_deck.Size(); ++cardI )
		{
			Core::Render::AddSpriteToScene( _gameRender.m_cardBack, deckPos );
			deckPos.m_pos.y -= cardHeight;
			deckPos.m_z += 1.0f / 52.0f;
		}
	}
	
	// Draw discard stack
	{
		Trans2D discardPos;
		discardPos.m_pos = { 234, 88 };
		for ( usize cardI = 0; cardI < _gameData.m_discard.Size(); ++cardI )
		{
			if ( cardI == _gameData.m_discard.Size() - 1 )
			{
				kaAssert( _gameData.m_discard.m_topDiscard.has_value() );
				Core::Render::AddSpriteToScene( _gameRender.m_cardFront[ _gameData.m_discard.CheckTop().DeckIndex() ], discardPos );
			}
			else
			{
				Core::Render::AddSpriteToScene( _gameRender.m_cardFront[ 0 ], discardPos );
				discardPos.m_pos.y -= cardHeight;
				discardPos.m_z += 1.0f / 52.0f;
			}
		}
	}
}

void Setup()
{
	Core::MakeSystem<Sys::GAME>( GameSystem );

	Core::MakeSystem<Sys::RENDER_QUEUE>( DrawMat );
	Core::MakeSystem<Sys::RENDER_QUEUE>( DrawGame );
}

}
}