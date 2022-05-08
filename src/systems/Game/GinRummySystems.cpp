#include "GinRummySystems.h"

#include "components.h"
#include <ecs/ecs.h>

#include "managers/RenderManager.h"
#include "managers/InputManager.h"

namespace Game
{
namespace GinRummy
{

static const Vec2 c_cardSize{ 46.0f, 64.0f };
static constexpr Vec1 c_pileMaxHeight{ 16.0f };
static constexpr Vec1 c_pileCardHeight{ c_pileMaxHeight / 52.0f };
static constexpr Vec1 c_handCardSeparation{ 20.0f };
static const Vec2 c_deckStart{ 40.0f, 88.0f };
static const Vec2 c_discardStart{ 234.0f, 88.0f };
static const Vec2 c_playerStartLoc{ 40.0f, 170.0f };
static const Vec2 c_playerDrawnLoc = c_playerStartLoc + Vec2{ 10.5f * c_handCardSeparation, 0.0f };
static const Vec2 c_aiStartLoc{ 234.0f, 6.0f };
static const Vec2 c_aiDrawnLoc = c_aiStartLoc - Vec2{ 10.5f * c_handCardSeparation, 0.0f };
static constexpr Vec1 c_animSpeed{ 6.0f };

static void MakeAIDiscard
(
	Game::GinRummy::GameData& _gameData
)
{
	// Very lazy AI

	auto& aiHand = _gameData.m_players[ 1 ].m_hand;
	usize worstI = 10;
	uint32 worstPoints = aiHand.CalculateValue( false );

	for ( usize i = 0; i < 10; ++i )
	{
		Card drawnCard = *aiHand.m_drawnCard;
		aiHand.m_drawnCard = aiHand.m_cards[ i ];
		aiHand.m_cards[ i ] = drawnCard;

		uint32 points = aiHand.CalculateValue( false );
		if ( points > worstPoints )
		{
			worstPoints = points;
			worstI = i;
		}
	}

	if ( worstI < 9 )
	{
		// worst card was put at worstI + 1
		Card drawnCard = *aiHand.m_drawnCard;
		aiHand.m_drawnCard = aiHand.m_cards[ worstI + 1 ];
		aiHand.m_cards[ worstI + 1 ] = drawnCard;
	}
	else if ( worstI == 9 )
	{
		// worst card is in drawn position
	}
	else if ( worstI == 10 )
	{
		// worst card was initial draw, in 0 position
		Card drawnCard = *aiHand.m_drawnCard;
		aiHand.m_drawnCard = aiHand.m_cards[ 0 ];
		aiHand.m_cards[ 0 ] = drawnCard;
	}

	_gameData.m_discard.Add( *aiHand.m_drawnCard );
	aiHand.m_drawnCard = std::nullopt;
}

static bool MakeAIChoice
(
	Game::GinRummy::GameData& _gameData
)
{
	// Lazy AI

	auto& aiHand = _gameData.m_players[ 1 ].m_hand;
	aiHand.m_drawnCard = _gameData.m_discard.CheckTop();
	uint32 withDrawn = aiHand.CalculateValue( true );
	uint32 withoutDrawn = aiHand.CalculateValue( false );

	if ( withDrawn > withoutDrawn )
	{
		_gameData.m_discard.PickUpTop();
		return true;
	}
	else
	{
		aiHand.m_drawnCard = std::nullopt;
	}
	return false;
}

static Vec2 GetDiscardTop
(
	Game::GinRummy::GameData const& _gameData
)
{
	return c_discardStart - Vec2{ 0, ( Vec1 )_gameData.m_discard.Size() * c_pileCardHeight };
}

static bool ProcessRound
(
	Game::GinRummy::GameData& _gameData
)
{
	if ( !_gameData.Ready() )
	{
		return false;
	}

	switch ( _gameData.m_roundState )
	{
	case RoundState::Deal:
	{
		_gameData.m_deck = {};
		_gameData.m_discard = {};

		usize dealer = 0;
		usize nonDealer = 1;
		if ( _gameData.m_aiIsDealer )
		{
			std::swap( dealer, nonDealer );
		}
		for ( usize i = 0; i < 10; ++i )
		{
			_gameData.m_players[ nonDealer ].m_hand.m_cards[ i ] = _gameData.m_deck.Draw();
			_gameData.m_players[ dealer ].m_hand.m_cards[ i ] = _gameData.m_deck.Draw();
		}

		_gameData.m_discard.Add( _gameData.m_deck.Draw() );

		_gameData.m_roundState = _gameData.m_aiIsDealer ? RoundState::PlayerChoice : RoundState::AIChoice;

		_gameData.QueueAnim( AnimDeal{} );
		if ( !_gameData.m_aiIsKnocker )
		{
			_gameData.QueueAnim( AnimAIDelay{} );
		}

		break;
	}

	case RoundState::AIChoice:
	{
		bool const madeChoice = MakeAIChoice( _gameData );
		if ( madeChoice )
		{
			_gameData.m_roundState = RoundState::AIDiscard;

			AnimMoveCard anim;
			anim.m_start = GetDiscardTop( _gameData );
			anim.m_end = c_aiDrawnLoc;
			anim.m_hideDrawn[ 1 ] = true;
			_gameData.QueueAnim( std::move( anim ) );
			_gameData.QueueAnim( AnimAIDelay{} );
		}
		else
		{
			_gameData.m_roundState = _gameData.m_aiIsDealer ? RoundState::PlayerTurn : RoundState::PlayerChoice;
		}

		break;
	}
	case RoundState::AIDiscard:
	{
		MakeAIDiscard( _gameData );
		_gameData.m_roundState = RoundState::PlayerTurn;

		AnimMoveCard anim;
		anim.m_start = c_aiDrawnLoc;
		anim.m_end = GetDiscardTop( _gameData );
		anim.m_hideTopDiscard = true;
		_gameData.QueueAnim( std::move( anim ) );

		break;
	}
	case RoundState::AITurn:
	{
		bool const madeChoice = MakeAIChoice( _gameData );
		if ( !madeChoice )
		{
			_gameData.m_players[ 1 ].m_hand.m_drawnCard = _gameData.m_deck.Draw();
		}
		bool const bigGin = _gameData.m_players[ 1 ].m_hand.CalculateValue( true ) == 0;
		if ( bigGin )
		{
			_gameData.m_aiIsKnocker = true;
			_gameData.m_roundState = RoundState::Knock;
		}
		else
		{
			MakeAIDiscard( _gameData );
			bool const gin = _gameData.m_players[ 1 ].m_hand.CalculateValue( true ) == 0;
			if ( gin )
			{
				_gameData.m_aiIsKnocker = true;
				_gameData.m_roundState = RoundState::Knock;
			}
			else
			{
				_gameData.m_roundState = RoundState::PlayerTurn;
			}
		}
		break;
	}

	case RoundState::PlayerChoice:
	{
		break;
	}
	case RoundState::PlayerTurn:
	{
		break;
	}

	case RoundState::Knock:
	{
		_gameData.m_roundState = RoundState::MakeCombinations;

		if ( _gameData.m_aiIsKnocker )
		{
			if ( _gameData.m_players[ 1 ].m_hand.CalculateValue( true ) == 0 )
			{
				// No combinations
				_gameData.m_roundState = RoundState::End;
			}
		}
		else
		{
			if ( _gameData.m_players[ 0 ].m_hand.CalculateValue( true ) == 0 )
			{
				// No combinations
				_gameData.m_roundState = RoundState::End;
			}
		}
		break;
	}
	case RoundState::MakeCombinations:
	{
		if ( !_gameData.m_aiIsKnocker )
		{
			// Lazy AI
			_gameData.m_roundState = RoundState::End;
		}
		break;
	}
	
	case RoundState::End:
	{
		return true;
	}
	}

	return false;
}

static void GameSystem
(
	Game::GinRummy::GameData& _gameData
)
{
	switch ( _gameData.m_gameState )
	{
		case GameState::PreGame:
		{
			_gameData.m_gameState = GameState::Round;
			_gameData.m_roundState = RoundState::Deal;
			break;
		}
		case GameState::Round:
		{
			bool const done = ProcessRound( _gameData );
			if ( done )
			{
				_gameData.m_gameState = GameState::BetweenRounds;
			}
			break;
		}
		case GameState::BetweenRounds:
		{
			break;
		}
		case GameState::EndGame:
		{
			break;
		}
	}
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

static void DrawDeck
(
	Core::FrameData const& _fd,
	Game::GinRummy::GameData& _gameData,
	Game::GinRummy::GameRender& _gameRender,
	bool _hideTopCard = false
)
{
	// Draw deck stack
	{
		Trans2D deckPos;
		deckPos.m_pos = c_deckStart;
		for ( usize cardI = 0; cardI < _gameData.m_deck.Size(); ++cardI )
		{
			if ( _hideTopCard && cardI == _gameData.m_deck.Size() - 1 )
			{
				break;
			}
			Core::Render::AddSpriteToScene( _gameRender.m_cardBack, deckPos );
			deckPos.m_pos.y -= c_pileCardHeight;
			deckPos.m_z += 1.0f / 52.0f;
		}
	}
}

static void DrawDiscard
(
	Core::FrameData const& _fd,
	Game::GinRummy::GameData& _gameData,
	Game::GinRummy::GameRender& _gameRender,
	bool _hideTopCard = false
)
{
	// Draw discard stack
	{
		Trans2D discardPos;
		discardPos.m_pos = c_discardStart;
		for ( usize cardI = 0; cardI < _gameData.m_discard.Size(); ++cardI )
		{
			if ( cardI == _gameData.m_discard.Size() - 1 )
			{
				if ( _hideTopCard )
				{
					break;
				}
				kaAssert( _gameData.m_discard.m_topDiscard.has_value() );
				Core::Render::AddSpriteToScene( _gameRender.m_cardFront[ _gameData.m_discard.CheckTop().DeckIndex() ], discardPos );
			}
			else
			{
				Core::Render::AddSpriteToScene( _gameRender.m_cardFront[ 0 ], discardPos );
				discardPos.m_pos.y -= c_pileCardHeight;
				discardPos.m_z += 1.0f / 52.0f;
			}
		}
	}
}

static void DrawPlayerHand
(
	Core::FrameData const& _fd,
	Game::GinRummy::GameData& _gameData,
	Game::GinRummy::GameRender& _gameRender,
	bool _hideDrawnCard = false
)
{
	Trans2D cardPos;
	cardPos.m_pos = c_playerStartLoc;
	for ( usize cardI = 0; cardI < 10; ++cardI )
	{
		if ( _gameRender.m_holdingCard.has_value() )
		{
			if ( cardI == _gameRender.m_holdingCard->m_cardI )
			{
				Trans2D heldCardTrans;
				heldCardTrans.m_pos = glm::floor( Core::Input::GetMousePos() - _gameRender.m_holdingCard->m_grabPoint );
				heldCardTrans.m_z = cardPos.m_z;
				cardPos.m_z += 1.0f / 52.0f;
				Core::Render::AddSpriteToScene( _gameRender.m_cardFront[ _gameData.m_players[ 0 ].m_hand.m_cards[ cardI ].DeckIndex() ], heldCardTrans );
				cardPos.m_pos.x += c_handCardSeparation;
				continue;
			}
		}

		if ( cardI == _gameRender.m_highlightedPlayerCard )
		{
			cardPos.m_pos.y = c_playerStartLoc.y - 10;
		}
		Core::Render::AddSpriteToScene( _gameRender.m_cardFront[ _gameData.m_players[ 0 ].m_hand.m_cards[ cardI ].DeckIndex() ], cardPos );
		cardPos.m_pos.x += c_handCardSeparation;
		cardPos.m_pos.y = c_playerStartLoc.y;
		cardPos.m_z += 1.0f / 52.0f;
	}

	if ( !_hideDrawnCard && _gameData.m_players[ 0 ].m_hand.m_drawnCard.has_value() )
	{
		cardPos.m_pos = c_playerDrawnLoc;
		Core::Render::AddSpriteToScene( _gameRender.m_cardFront[ _gameData.m_players[ 0 ].m_hand.m_drawnCard->DeckIndex() ], cardPos );
	}
}

static void DrawAIHand
(
	Core::FrameData const& _fd,
	Game::GinRummy::GameData& _gameData,
	Game::GinRummy::GameRender& _gameRender,
	bool _hideDrawnCard = false
)
{
	Trans2D cardPos;
	cardPos.m_pos = c_aiStartLoc;
	for ( usize cardI = 0; cardI < 10; ++cardI )
	{
		Core::Render::AddSpriteToScene( _gameRender.m_cardBack, cardPos );
		cardPos.m_pos.x -= c_handCardSeparation;
		cardPos.m_z += 1.0f / 52.0f;
	}

	if ( !_hideDrawnCard && _gameData.m_players[ 1 ].m_hand.m_drawnCard.has_value() )
	{
		cardPos.m_pos = c_aiDrawnLoc;
		Core::Render::AddSpriteToScene( _gameRender.m_cardBack, cardPos );
	}
}

static void DrawGame
(
	Core::FrameData const& _fd,
	Game::GinRummy::GameData& _gameData,
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

	if ( _gameData.m_gameState == GameState::Round )
	{
		bool needToDraw = _gameData.m_animQueue.empty();
		if ( !_gameData.m_animQueue.empty() )
		{
			Animation& curAnim = _gameData.m_animQueue.front();
			bool done = false;
			switch ( curAnim.index() )
			{
			case AnimOptions::eAnimDeal:
			{
				AnimDeal& anim = std::get<AnimDeal>( curAnim );
				anim.m_animatingTime += _fd.dt * c_animSpeed;

				// 1 second per card
				// 20 cards + discard
				if ( anim.m_animatingTime >= 21.0f )
				{
					done = true;
				}
				else
				{
					bool nextCardAI = !_gameData.m_aiIsDealer;
					usize playerI = 0;
					usize aiI = 0;
					Trans2D targetTransP;
					targetTransP.m_pos = c_playerStartLoc + Vec2{ 9 * c_handCardSeparation, 0 };
					targetTransP.m_z = 1.0f / 2.0f;
					Trans2D targetTransAI;
					targetTransAI.m_pos = c_aiStartLoc;
					targetTransAI.m_z = 1.0f / 2.0f;

					usize deckSize = 52;
					for ( usize cardI = 0; cardI < 20; ++cardI )
					{
						Vec1 const upToHereTime = ( Vec1 )cardI;
						Vec1 const diff = anim.m_animatingTime - upToHereTime;
						if ( diff <= 1.0f )
						{
							deckSize = 52 - cardI;
							Vec2 const topDeckPos = c_deckStart - Vec2{ 0, ( Vec1 )deckSize * c_pileCardHeight };

							Vec2 const cardPos = glm::floor( Lerp( topDeckPos, nextCardAI ? targetTransAI.m_pos : targetTransP.m_pos, diff ) );
							Trans2D cardTrans;
							cardTrans.m_pos = cardPos;
							cardTrans.m_z = 1.0f;
							Core::Render::AddSpriteToScene( _gameRender.m_cardBack, cardTrans );
							break;
						}
						else
						{
							Core::Render::AddSpriteToScene( _gameRender.m_cardBack, nextCardAI ? targetTransAI : targetTransP );
						}

						if ( nextCardAI )
						{
							targetTransAI.m_pos.x -= c_handCardSeparation;
							aiI++;
						}
						else
						{
							targetTransP.m_pos.x -= c_handCardSeparation;
							playerI++;
						}
						targetTransP.m_z += 1.0f / 108.0f;
						targetTransAI.m_z += 1.0f / 108.0f;
						nextCardAI = !nextCardAI;
					}

					if ( anim.m_animatingTime >= 20.0f )
					{
						deckSize = 32;

						Vec2 const topDeckPos = c_deckStart - Vec2{ 0, ( Vec1 )deckSize * c_pileCardHeight };
						Trans2D targetTrans;
						targetTrans.m_pos = c_discardStart;
						targetTrans.m_z = 1.0f;

						Vec1 const diff = anim.m_animatingTime - 20.0f;
						targetTrans.m_pos = glm::floor( Lerp( topDeckPos, targetTrans.m_pos, diff ) );
						Core::Render::AddSpriteToScene( _gameRender.m_cardBack, targetTrans );
					}

					{
						deckSize--;
						Trans2D deckPos;
						deckPos.m_pos = c_deckStart;
						for ( usize cardI = 0; cardI < deckSize; ++cardI )
						{
							Core::Render::AddSpriteToScene( _gameRender.m_cardBack, deckPos );
							deckPos.m_pos.y -= c_pileCardHeight;
							deckPos.m_z += 1.0f / 108.0f;
						}
					}
				}

				break;
			}
			case AnimOptions::eAnimMoveCard:
			{
				AnimMoveCard& anim = std::get<AnimMoveCard>( curAnim );
				anim.m_animatingTime += _fd.dt * c_animSpeed;
				if ( anim.m_animatingTime >= 1.0f )
				{
					done = true;
				}
				else
				{
					Trans2D cardPos;
					cardPos.m_pos = glm::floor( Lerp( anim.m_start, anim.m_end, anim.m_animatingTime ) );
					cardPos.m_z = 1.0f;
					if ( anim.m_cardValue.has_value() )
					{
						Core::Render::AddSpriteToScene( _gameRender.m_cardFront[ anim.m_cardValue->DeckIndex() ], cardPos );
					}
					else
					{
						Core::Render::AddSpriteToScene( _gameRender.m_cardBack, cardPos );
					}

					DrawDeck( _fd, _gameData, _gameRender, anim.m_hideTopDeck );
					DrawDiscard( _fd, _gameData, _gameRender, anim.m_hideTopDiscard );
					DrawPlayerHand( _fd, _gameData, _gameRender, anim.m_hideDrawn[ 0 ] );
					DrawAIHand( _fd, _gameData, _gameRender, anim.m_hideDrawn[ 1 ] );
				}
				break;
			}
			case AnimOptions::eAnimAIDelay:
			{
				AnimAIDelay& anim = std::get<AnimAIDelay>( curAnim );
				anim.m_animatingTime += _fd.dt * c_animSpeed;
				if ( anim.m_animatingTime >= 5.0f )
				{
					done = true;
				}
				needToDraw = true;
				break;
			}
			}

			if ( done )
			{
				_gameData.m_animQueue.erase( _gameData.m_animQueue.begin() );
				needToDraw = true;
			}
		}
		
		if ( needToDraw )
		{
			DrawDeck( _fd, _gameData, _gameRender );
			DrawDiscard( _fd, _gameData, _gameRender );
			DrawPlayerHand( _fd, _gameData, _gameRender );
			DrawAIHand( _fd, _gameData, _gameRender );
		}
	}
	else
	{
		DrawDeck( _fd, _gameData, _gameRender );
		DrawDiscard( _fd, _gameData, _gameRender );
	}
}

static void HandleInteraction
(
	Game::GinRummy::GameData& _gameData,
	Game::GinRummy::GameRender& _gameRender,
	Game::GinRummy::PlayerInteraction& _interaction
)
{
	if ( _gameData.m_gameState == GameState::Round )
	{
		// Allow rearranging if in turn phases
		if ( _gameData.m_roundState == RoundState::PlayerChoice || _gameData.m_roundState == RoundState::PlayerTurn )
		{
			Rect2D cardBox;
			cardBox.m_min = c_playerStartLoc + Vec2{ 9 * c_handCardSeparation, 0 };
			cardBox.m_max = cardBox.m_min + c_cardSize;

			bool const selectHeld = Core::Input::Pressed( Core::Input::Action::GinRummy_Select );

			bool highlighted = false;
			for ( usize cardIN = 10; cardIN > 0; --cardIN )
			{
				usize cardI = cardIN - 1;
				if ( selectHeld && _gameRender.m_holdingCard.has_value() && cardI == _gameRender.m_holdingCard->m_cardI )
				{
					Vec2 const distDragged = Core::Input::GetMousePos() - _interaction.m_initialHoldingCardPos;
					if ( distDragged.x > c_handCardSeparation && cardI < 9 )
					{
						std::swap( _gameData.m_players[ 0 ].m_hand.m_cards[ cardI ], _gameData.m_players[ 0 ].m_hand.m_cards[ cardI + 1 ] );
						_gameRender.m_holdingCard->m_cardI++;
						_interaction.m_initialHoldingCardPos += c_handCardSeparation;
					}
					else if ( distDragged.x < -c_handCardSeparation && cardI > 0 )
					{
						std::swap( _gameData.m_players[ 0 ].m_hand.m_cards[ cardI - 1 ], _gameData.m_players[ 0 ].m_hand.m_cards[ cardI ] );
						_gameRender.m_holdingCard->m_cardI--;
						_interaction.m_initialHoldingCardPos -= c_handCardSeparation;
					}
				}

				if ( cardI == _gameRender.m_highlightedPlayerCard )
				{
					cardBox.m_min.y = c_playerStartLoc.y - 10;
				}
				if ( cardBox.Contains( Core::Input::GetMousePos() ) )
				{
					if ( !highlighted )
					{
						_gameRender.m_highlightedPlayerCard = cardI;
						highlighted = true;
					}
					if ( selectHeld )
					{
						if ( !_gameRender.m_holdingCard.has_value() )
						{
							_interaction.m_initialHoldingCardPos = Core::Input::GetMousePos();
							_gameRender.m_holdingCard = GameRender::HeldCard{
								Core::Input::GetMousePos() - cardBox.m_min,
								cardI,
							};
						}
					}
				}
				cardBox.m_min.x -= c_handCardSeparation;
				cardBox.m_max.x -= c_handCardSeparation;
				cardBox.m_min.y = c_playerStartLoc.y;
			}

			if ( !selectHeld )
			{
				_gameRender.m_holdingCard = std::nullopt;
			}

			if ( selectHeld || !highlighted )
			{
				_gameRender.m_highlightedPlayerCard = ~0u;
			}
		}
	}
}

void Setup()
{
	Core::MakeSystem<Sys::GAME>( GameSystem );
	Core::MakeSystem<Sys::GAME>( HandleInteraction );

	Core::MakeSystem<Sys::RENDER_QUEUE>( DrawMat );
	Core::MakeSystem<Sys::RENDER_QUEUE>( DrawGame );
}

}
}