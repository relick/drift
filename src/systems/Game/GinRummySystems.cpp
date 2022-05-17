#include "GinRummySystems.h"

#include "components.h"
#include <ecs/ecs.h>

#include "managers/RenderManager.h"
#include "managers/InputManager.h"
#include "managers/TextManager.h"
#include "shaders/sprites_constants.glslh"

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
static const Vec2 c_handValueTextPos = Vec2{ 5.0f, c_playerStartLoc.y + 15.0f };

static const Vec2 c_aiStartLoc{ 234.0f, 6.0f };
static const Vec2 c_aiDrawnLoc = c_aiStartLoc - Vec2{ 10.5f * c_handCardSeparation, 0.0f };

static const Rect2D c_passBox{ Vec2{c_discardStart.x - 50, 156}, Vec2{c_discardStart.x, 166} };
static const Rect2D c_takeBox{ Vec2{c_discardStart.x, 156}, Vec2{c_discardStart.x + 50, 166} };
static const Rect2D c_drawBox{ Vec2{c_deckStart.x, 156}, Vec2{c_deckStart.x + 50, 166} };
static const Rect2D c_knockBox{ Vec2{135, 156}, Vec2{185, 166} };
static const Rect2D c_discardDropBox{ Vec2{221, 84}, Vec2{293, 156} };

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

static Vec2 GetDeckTop
(
	Game::GinRummy::GameData const& _gameData
)
{
	return c_deckStart - Vec2{ 0, ( Vec1 )_gameData.m_deck.Size() * c_pileCardHeight };
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
			anim.m_cardValue = *_gameData.m_players[ 1 ].m_hand.m_drawnCard;
			
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
		anim.m_cardValue = _gameData.m_discard.CheckTop();

		_gameData.QueueAnim( std::move( anim ) );

		break;
	}
	case RoundState::AITurn1:
	{
		bool const madeChoice = MakeAIChoice( _gameData );
		if ( !madeChoice )
		{
			_gameData.m_players[ 1 ].m_hand.m_drawnCard = _gameData.m_deck.Draw();
		}

		_gameData.m_roundState = RoundState::AITurn2;

		AnimMoveCard anim;
		anim.m_start = madeChoice ? GetDiscardTop( _gameData ) : GetDeckTop( _gameData );
		anim.m_end = c_aiDrawnLoc;
		anim.m_hideDrawn[ 1 ] = true;
		anim.m_cardValue = *_gameData.m_players[ 1 ].m_hand.m_drawnCard;

		_gameData.QueueAnim( std::move( anim ) );
		_gameData.QueueAnim( AnimAIDelay{} );
		break;
	}
	case RoundState::AITurn2:
	{
		// todo proper game ending but just play for now
		MakeAIDiscard( _gameData );
		_gameData.m_roundState = RoundState::PlayerTurn;

		AnimMoveCard anim;
		anim.m_start = c_aiDrawnLoc;
		anim.m_end = GetDiscardTop( _gameData );
		anim.m_hideTopDiscard = true;
		anim.m_cardValue = _gameData.m_discard.CheckTop();

		_gameData.QueueAnim( std::move( anim ) );

		/*bool const bigGin = _gameData.m_players[1].m_hand.CalculateValue(true) == 0;
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
		}*/
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

static void DrawDeck
(
	Core::FrameData const& _fd,
	Game::GinRummy::GameData const& _gameData,
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
			if ( cardI == _gameData.m_deck.Size() - 1 )
			{
				if ( _hideTopCard )
				{
					break;
				}
				deckPos.m_pos = glm::floor( deckPos.m_pos );
			}

			GameRender::CardRender& card = _gameRender.m_cards[ _gameData.m_deck.m_cards[ cardI ].DeckIndex() ];
			card.m_showCard = true;
			card.m_showFront = false;
			card.m_trans = deckPos;

			deckPos.m_pos.y -= c_pileCardHeight;
			deckPos.m_z += 1.0f / 108.0f;
		}
	}
}

static void DrawDiscard
(
	Core::FrameData const& _fd,
	Game::GinRummy::GameData const& _gameData,
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
				discardPos.m_pos = glm::floor( discardPos.m_pos );

				GameRender::CardRender& card = _gameRender.m_cards[ _gameData.m_discard.CheckTop().DeckIndex() ];
				card.m_showCard = true;
				card.m_showFront = true;
				card.m_trans = discardPos;
			}
			else if ( cardI == _gameData.m_discard.Size() - 2 )
			{
				discardPos.m_pos = glm::floor( discardPos.m_pos );

				GameRender::CardRender& card = _gameRender.m_cards[ _gameData.m_discard.m_discards[ cardI ].DeckIndex() ];
				card.m_showCard = true;
				card.m_showFront = true;
				card.m_trans = discardPos;
			}
			else
			{
				GameRender::CardRender& card = _gameRender.m_cards[ _gameData.m_discard.m_discards[ cardI ].DeckIndex() ];
				card.m_showCard = true;
				card.m_showFront = true;
				card.m_trans = discardPos;
			}
			discardPos.m_pos.y -= c_pileCardHeight;
			discardPos.m_z += 1.0f / 108.0f;
		}
	}
}

static void DrawPlayerHand
(
	Core::FrameData const& _fd,
	Game::GinRummy::GameData const& _gameData,
	Game::GinRummy::GameRender& _gameRender,
	bool _hideDrawnCard = false
)
{
	_gameRender.m_playerHandValue = _gameData.m_players[ 0 ].m_hand.CalculateValue( false );
	_gameRender.m_playerFullHandValue = _gameData.m_players[ 0 ].m_hand.CalculateValue( true );

	Trans2D cardPos;
	cardPos.m_pos = c_playerStartLoc;
	cardPos.m_z = 0.5f;
	for ( usize cardI = 0; cardI < 10; ++cardI )
	{
		if ( _gameRender.m_holdingCard.has_value() )
		{
			if ( cardI == _gameRender.m_holdingCard->m_cardI )
			{
				Trans2D heldCardTrans;
				heldCardTrans.m_pos = glm::floor( Core::Input::GetMousePos() - _gameRender.m_holdingCard->m_grabPoint );
				heldCardTrans.m_z = cardPos.m_z;
				cardPos.m_z += 1.0f / 108.0f;
				cardPos.m_pos.x += c_handCardSeparation;

				GameRender::CardRender& card = _gameRender.m_cards[ _gameData.m_players[ 0 ].m_hand.m_cards[ cardI ].DeckIndex() ];
				card.m_showCard = true;
				card.m_showFront = true;
				card.m_trans = heldCardTrans;

				continue;
			}
		}

		if ( cardI == _gameRender.m_highlightedPlayerCard )
		{
			cardPos.m_pos.y = c_playerStartLoc.y - 10;
		}
		GameRender::CardRender& card = _gameRender.m_cards[ _gameData.m_players[ 0 ].m_hand.m_cards[ cardI ].DeckIndex() ];
		card.m_showCard = true;
		card.m_showFront = true;
		card.m_trans = cardPos;

		cardPos.m_pos.x += c_handCardSeparation;
		cardPos.m_pos.y = c_playerStartLoc.y;
		cardPos.m_z += 1.0f / 108.0f;
	}

	if ( !_hideDrawnCard && _gameData.m_players[ 0 ].m_hand.m_drawnCard.has_value() )
	{
		if ( _gameRender.m_holdingCard.has_value() )
		{
			if ( 10 == _gameRender.m_holdingCard->m_cardI )
			{
				Trans2D heldCardTrans;
				heldCardTrans.m_pos = glm::floor( Core::Input::GetMousePos() - _gameRender.m_holdingCard->m_grabPoint );
				heldCardTrans.m_z = cardPos.m_z;

				GameRender::CardRender& card = _gameRender.m_cards[ _gameData.m_players[ 0 ].m_hand.m_drawnCard->DeckIndex() ];
				card.m_showCard = true;
				card.m_showFront = true;
				card.m_trans = heldCardTrans;
				return;
			}
		}

		cardPos.m_pos = c_playerDrawnLoc;

		GameRender::CardRender& card = _gameRender.m_cards[ _gameData.m_players[ 0 ].m_hand.m_drawnCard->DeckIndex() ];
		card.m_showCard = true;
		card.m_showFront = true;
		card.m_trans = cardPos;
	}
}

static void DrawAIHand
(
	Core::FrameData const& _fd,
	Game::GinRummy::GameData const& _gameData,
	Game::GinRummy::GameRender& _gameRender,
	bool _hideDrawnCard = false
)
{
	Trans2D cardPos;
	cardPos.m_pos = c_aiStartLoc;
	for ( usize cardI = 0; cardI < 10; ++cardI )
	{
		GameRender::CardRender& card = _gameRender.m_cards[ _gameData.m_players[ 1 ].m_hand.m_cards[ cardI ].DeckIndex() ];
		card.m_showCard = true;
		card.m_showFront = false;
		card.m_trans = cardPos;

		cardPos.m_pos.x -= c_handCardSeparation;
		cardPos.m_z += 1.0f / 108.0f;
	}

	if ( !_hideDrawnCard && _gameData.m_players[ 1 ].m_hand.m_drawnCard.has_value() )
	{
		cardPos.m_pos = c_aiDrawnLoc;

		GameRender::CardRender& card = _gameRender.m_cards[ _gameData.m_players[ 1 ].m_hand.m_drawnCard->DeckIndex() ];
		card.m_showCard = true;
		card.m_showFront = false;
		card.m_trans = cardPos;
	}
}

static void DrawGame
(
	Core::FrameData const& _fd,
	Game::GinRummy::GameData& _gameData,
	Game::GinRummy::GameRender& _gameRender
)
{
	_gameRender.m_playerHandValue = std::nullopt;
	_gameRender.m_playerFullHandValue = std::nullopt;

	// Reset all cards
	std::for_each(
		std::execution::par_unseq,
		_gameRender.m_cards.begin(),
		_gameRender.m_cards.end(),
		[]( GameRender::CardRender& _card )
		{
			_card.m_showCard = false;
		}
	);

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

				GameRender::CardRender& card = _gameRender.m_cards[ s * 13 + f ];
				card.m_trans = pos;
				card.m_showCard = true;
				card.m_showFront = flip;

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

						GameRender::CardRender& card = _gameRender.m_cards[ nextCardAI ? _gameData.m_players[ 1 ].m_hand.m_cards[ aiI ].DeckIndex() : _gameData.m_players[ 0 ].m_hand.m_cards[ playerI ].DeckIndex() ];
						card.m_showCard = true;
						card.m_showFront = false;

						if ( diff < 0.0f )
						{
							deckSize = 52 - cardI;
							Vec2 const topDeckPos = c_deckStart - Vec2{ 0, ( Vec1 )deckSize * c_pileCardHeight };

							Trans2D cardTrans;
							cardTrans.m_pos = topDeckPos;
							cardTrans.m_z = 1.5f - targetTransAI.m_z;

							card.m_trans = cardTrans;
						}
						else if ( diff <= 1.0f )
						{
							deckSize = 52 - cardI;
							Vec2 const topDeckPos = c_deckStart - Vec2{ 0, ( Vec1 )deckSize * c_pileCardHeight };

							Vec2 const cardPos = glm::floor( Lerp( topDeckPos, nextCardAI ? targetTransAI.m_pos : targetTransP.m_pos, diff ) );
							Trans2D cardTrans;
							cardTrans.m_pos = cardPos;
							cardTrans.m_z = 1.0f;

							card.m_trans = cardTrans;
						}
						else
						{
							card.m_trans = nextCardAI ? targetTransAI : targetTransP;
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
						deckSize = 31;

						Vec2 const topDeckPos = c_deckStart - Vec2{ 0, ( Vec1 )deckSize * c_pileCardHeight };
						Trans2D targetTrans;
						targetTrans.m_pos = c_discardStart;
						targetTrans.m_z = 1.0f;

						Vec1 const diff = anim.m_animatingTime - 20.0f;
						targetTrans.m_pos = glm::floor( Lerp( topDeckPos, targetTrans.m_pos, diff ) );

						GameRender::CardRender& card = _gameRender.m_cards[ _gameData.m_discard.CheckTop().DeckIndex() ];
						card.m_showCard = true;
						card.m_showFront = false;
						card.m_trans = targetTrans;
					}

					{
						Trans2D deckPos;
						deckPos.m_pos = c_deckStart;
						for ( usize cardI = 0; cardI < _gameData.m_deck.m_cards.size(); ++cardI )
						{
							GameRender::CardRender& card = _gameRender.m_cards[ _gameData.m_deck.m_cards[ cardI ].DeckIndex() ];
							card.m_showCard = true;
							card.m_showFront = false;
							card.m_trans = deckPos;

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
					DrawDeck( _fd, _gameData, _gameRender, anim.m_hideTopDeck );
					DrawDiscard( _fd, _gameData, _gameRender, anim.m_hideTopDiscard );
					DrawPlayerHand( _fd, _gameData, _gameRender, anim.m_hideDrawn[ 0 ] );
					DrawAIHand( _fd, _gameData, _gameRender, anim.m_hideDrawn[ 1 ] );

					Trans2D cardPos;
					cardPos.m_pos = glm::floor( Lerp( anim.m_start, anim.m_end, anim.m_animatingTime ) );
					cardPos.m_z = 1.0f;

					GameRender::CardRender& card = _gameRender.m_cards[ anim.m_cardValue.DeckIndex() ];
					card.m_showCard = true;
					card.m_showFront = anim.m_showFront;
					card.m_trans = cardPos;
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

	// Push to render manager
	std::for_each(
		std::execution::par_unseq,
		_gameRender.m_cards.begin(),
		_gameRender.m_cards.end(),
		[]( GameRender::CardRender const& _card )
		{
			Core::Render::UpdateSpriteInScene( _card.m_cardFront, _card.m_trans, _card.m_showCard && _card.m_showFront ? 0u : SpriteFlag_Hidden );
			Core::Render::UpdateSpriteInScene( _card.m_cardBack, _card.m_trans, _card.m_showCard && !_card.m_showFront ? 0u : SpriteFlag_Hidden );
		}
	);
}

static void DrawText
(
	Core::MT_Only& _mt,
	Core::FrameData const& _fd,
	Game::GinRummy::GameData const& _gameData,
	Game::GinRummy::GameRender const& _gameRender
)
{
	if ( _gameRender.m_playerFullHandValue.has_value() )
	{
		Core::Render::Text::Write( c_handValueTextPos + Vec2{ 0, 10.0f }, "VALUE", 10.0f, Colour::black );
		Core::Render::Text::Write( c_handValueTextPos + Vec2{ 0, 20.0f }, std::format( "{:d}", *_gameRender.m_playerFullHandValue ).c_str(), 10.0f, Colour::black );
	}

	if ( _gameData.m_roundState == RoundState::PlayerChoice )
	{
		if ( !_gameData.m_players[ 0 ].m_hand.m_drawnCard.has_value() )
		{
			Core::Render::Text::Write( c_passBox.m_min + Vec2{ 0, 10.0f }, "PASS", 10.0f, Colour::black );
			Core::Render::Text::Write( c_takeBox.m_min + Vec2{ 0, 10.0f }, "TAKE", 10.0f, Colour::black );
		}
	}

	if ( _gameData.m_roundState == RoundState::PlayerTurn )
	{
		if ( _gameData.m_players[ 0 ].m_hand.m_drawnCard.has_value() )
		{
			if ( _gameRender.m_playerHandValue.value_or( ~0u ) <= 10 || _gameRender.m_playerFullHandValue.value_or( ~0u ) == 0 )
			{
				Core::Render::Text::Write( c_knockBox.m_min + Vec2{ 0, 10.0f }, "KNOCK", 10.0f, Colour::black );
			}
		}
		else
		{
			Core::Render::Text::Write( c_drawBox.m_min + Vec2{ 0, 10.0f }, "DRAW", 10.0f, Colour::black );
			Core::Render::Text::Write( c_takeBox.m_min + Vec2{ 0, 10.0f }, "TAKE", 10.0f, Colour::black );
		}
	}
}

static void HandleInteraction
(
	Game::GinRummy::GameData& _gameData,
	Game::GinRummy::GameRender& _gameRender,
	Game::GinRummy::PlayerInteraction& _interaction
)
{
	Vec2 const mousePos = Core::Input::GetMousePos();
	if ( _gameData.m_gameState == GameState::Round )
	{
		// Allow rearranging if in turn phases
		if ( _gameData.m_roundState == RoundState::PlayerChoice || _gameData.m_roundState == RoundState::PlayerTurn )
		{
			bool const selectHeld = Core::Input::Pressed( Core::Input::Action::GinRummy_Select );
			bool highlighted = false;

			Rect2D cardBox;
			if ( _gameData.m_players[ 0 ].m_hand.m_drawnCard.has_value() )
			{
				cardBox.m_min = c_playerDrawnLoc;
				cardBox.m_max = cardBox.m_min + c_cardSize;

				if ( 10 == _gameRender.m_highlightedPlayerCard )
				{
					cardBox.m_min.y = c_playerDrawnLoc.y - 10;
				}
				if ( cardBox.Contains( mousePos ) )
				{
					if ( !highlighted )
					{
						_gameRender.m_highlightedPlayerCard = 10;
						highlighted = true;
					}
					if ( selectHeld )
					{
						if ( !_gameRender.m_holdingCard.has_value() )
						{
							_interaction.m_initialHoldingCardPos = mousePos;
							_gameRender.m_holdingCard = GameRender::HeldCard{
								mousePos - cardBox.m_min,
								10,
							};
						}
					}
				}
			}

			cardBox.m_min = c_playerStartLoc + Vec2{ 9 * c_handCardSeparation, 0 };
			cardBox.m_max = cardBox.m_min + c_cardSize;

			for ( usize cardIN = 10; cardIN > 0; --cardIN )
			{
				usize cardI = cardIN - 1;
				if ( selectHeld && _gameRender.m_holdingCard.has_value() && cardI == _gameRender.m_holdingCard->m_cardI )
				{
					Vec2 const distDragged = mousePos - _interaction.m_initialHoldingCardPos;
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
				if ( cardBox.Contains( mousePos ) )
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
							_interaction.m_initialHoldingCardPos = mousePos;
							_gameRender.m_holdingCard = GameRender::HeldCard{
								mousePos - cardBox.m_min,
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
				if ( _gameRender.m_holdingCard.has_value() && _gameData.m_players[0].m_hand.m_drawnCard.has_value() && c_discardDropBox.Contains(mousePos) )
				{
					if ( _gameRender.m_holdingCard->m_cardI == 10 )
					{
						_gameData.m_discard.Add( *_gameData.m_players[ 0 ].m_hand.m_drawnCard );
					}
					else
					{
						_gameData.m_discard.Add( _gameData.m_players[ 0 ].m_hand.m_cards[ _gameRender.m_holdingCard->m_cardI ] );
						_gameData.m_players[ 0 ].m_hand.m_cards[ _gameRender.m_holdingCard->m_cardI ] = *_gameData.m_players[ 0 ].m_hand.m_drawnCard;
					}
					_gameData.m_players[ 0 ].m_hand.m_drawnCard = std::nullopt;

					_gameData.m_roundState = RoundState::AITurn1;
					_gameData.QueueAnim( AnimAIDelay{} );
				}

				_gameRender.m_holdingCard = std::nullopt;
			}

			if ( selectHeld || !highlighted )
			{
				_gameRender.m_highlightedPlayerCard = ~0u;
			}
		}

		if ( _gameData.m_roundState == RoundState::PlayerChoice )
		{
			if ( !_gameData.m_players[ 0 ].m_hand.m_drawnCard.has_value() )
			{
				bool const selected = Core::Input::PressedOnce( Core::Input::Action::GinRummy_Select );

				if ( c_passBox.Contains( mousePos ) && selected )
				{
					_gameData.m_roundState = _gameData.m_aiIsDealer ? RoundState::AIChoice : RoundState::AITurn1;
					_gameData.QueueAnim( AnimAIDelay{} );
				}

				if ( c_takeBox.Contains( mousePos ) && selected )
				{
					_gameData.m_players[ 0 ].m_hand.m_drawnCard = _gameData.m_discard.PickUpTop();
				}
			}
		}

		if ( _gameData.m_roundState == RoundState::PlayerTurn )
		{
			bool const selected = Core::Input::PressedOnce( Core::Input::Action::GinRummy_Select );
			if ( _gameData.m_players[ 0 ].m_hand.m_drawnCard.has_value() )
			{
				if ( _gameRender.m_playerHandValue.value_or( ~0u ) <= 10 || _gameRender.m_playerFullHandValue.value_or( ~0u ) == 0 )
				{
					if ( c_knockBox.Contains( mousePos ) && selected )
					{
						_gameData.m_aiIsKnocker = false;
						_gameData.m_roundState = RoundState::Knock;
					}
				}
			}
			else
			{
				if ( c_drawBox.Contains( mousePos ) && selected )
				{
					_gameData.m_players[ 0 ].m_hand.m_drawnCard = _gameData.m_deck.Draw();

					AnimMoveCard anim;
					anim.m_start = GetDeckTop( _gameData );
					anim.m_end = c_playerDrawnLoc;
					anim.m_hideDrawn[ 0 ] = true;
					anim.m_cardValue = *_gameData.m_players[ 0 ].m_hand.m_drawnCard;

					_gameData.QueueAnim( std::move( anim ) );
				}

				if ( c_takeBox.Contains( mousePos ) && selected )
				{
					_gameData.m_players[ 0 ].m_hand.m_drawnCard = _gameData.m_discard.PickUpTop();

					AnimMoveCard anim;
					anim.m_start = GetDiscardTop( _gameData );
					anim.m_end = c_playerDrawnLoc;
					anim.m_hideDrawn[ 0 ] = true;
					anim.m_cardValue = *_gameData.m_players[ 0 ].m_hand.m_drawnCard;

					_gameData.QueueAnim( std::move( anim ) );
				}
			}
		}
	}
}

void Setup()
{
	Core::MakeSystem<Sys::GAME>( GameSystem );
	Core::MakeSystem<Sys::GAME2>( HandleInteraction );

	Core::MakeSystem<Sys::RENDER_QUEUE>( DrawGame );
	Core::MakeSystem<Sys::TEXT>( DrawText );

	Core::MakeSystem<Sys::GAME>(
		[]
		(
			Core::FrameData const& _fd,
			Core::Render::FrameData const& _rfd,
			Cardie& _cardie,
			Core::Transform2D& _trans
		)
		{
			_trans.T().m_pos += 200.0f * _fd.dt * _cardie.m_dir;
			if ( _trans.T().m_pos.x <= 0 )
			{
				_trans.T().m_pos.x = 0;
				_cardie.m_dir.x = -_cardie.m_dir.x;
			}
			if ( _trans.T().m_pos.x >= _rfd.renderArea.f.x )
			{
				_trans.T().m_pos.x = _rfd.renderArea.f.x;
				_cardie.m_dir.x = -_cardie.m_dir.x;
			}
			if ( _trans.T().m_pos.y <= 0 )
			{
				_trans.T().m_pos.y = 0;
				_cardie.m_dir.y = -_cardie.m_dir.y;
			}
			if ( _trans.T().m_pos.y >= _rfd.renderArea.f.y )
			{
				_trans.T().m_pos.y = _rfd.renderArea.f.y;
				_cardie.m_dir.y = -_cardie.m_dir.y;
			}
		}
	);
}

}
}