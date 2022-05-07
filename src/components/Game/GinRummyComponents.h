#pragma once

#include "common.h"

#include "managers/ResourceIDs.h"
#include "managers/EntityManager.h"

#include <array>
#include <optional>

namespace Game
{
namespace GinRummy
{

struct Card
{
	enum class Suit : uint8
	{
		Diamonds,
		Clubs,
		Hearts,
		Spades,

		Count,
	};

	enum class Face : uint8
	{
		Ace,
		Two,
		Three,
		Four,
		Five,
		Six,
		Seven,
		Eight,
		Nine,
		Ten,
		Jack,
		Queen,
		King,

		Count,
	};

	static constexpr std::array<uint8, ( usize )Face::Count> c_ginRummyFaceValues{
		1,2,3,4,5,6,7,8,9,10,10,10,10,
	};

	Suit m_suit;
	Face m_face;

	bool operator==( Card const& _o ) const { return _o.m_face == m_face && _o.m_suit == m_suit; }
	uint8 GetValue() const { return c_ginRummyFaceValues[ ( usize )m_face ]; }
	usize DeckIndex() const { return ( usize )m_suit * ( usize )Face::Count + ( usize )m_face; } // Useful for reaching into another ordered list.
};

struct Deck
{
	std::vector<Card> m_cards;

	Deck();

	void Reset();
	void Shuffle();
	Card Draw();

	usize Size() const { return m_cards.size(); }
};

struct Discard
{
	// Only top 2 are relevant, the rest are in an inaccessible pit
	std::optional<Card> m_topDiscard;
	std::optional<Card> m_secondDiscard;
	usize m_discardPileSize{ 0 };

	usize Size() const { return m_discardPileSize; }
	Card CheckTop() const;

	void Add( Card _card );
	Card PickUpTop();
};

struct Hand
{
	std::array<Card, 10> m_cards;
	std::optional<Card> m_drawnCard;
	
	bool MatchesDrawnCard( Card const& _other ) const { return m_drawnCard.has_value() && *m_drawnCard == _other; }

	std::vector<Card> GetBestJunkCards( bool _includeDrawn ) const;

	std::vector<Card> GetJunk
	(
		std::vector<std::vector<Card>> const& _matches,
		std::vector<std::vector<Card>> const& _splitRuns
	)	const;

	std::pair<uint32, std::vector<Card>> GetBestJunkCards_TestCombos
	(
		std::vector<std::vector<Card>> _matches,
		std::vector<std::vector<Card>> const& _splitRuns,
		usize _matchesN
	)	const;

	uint32 CalculateValue( std::vector<Card> const& _junkCards ) const;
	uint32 CalculateValue( bool _includesDrawn ) const;
};

struct Player
{
	Hand m_hand;
	uint32 m_points{ 0 };
};

enum class GameState : uint8
{
	PreGame,
	Round,
	BetweenRounds,
	EndGame,
};

enum class RoundState : uint8
{
	Deal,

	NonDealerChoice,
	DealerChoice,

	NonDealerTurn,
	DealerTurn,

	Knock,
	MakeCombinations,

	End,
};

struct GameData
{
	GameState m_gameState{ GameState::PreGame };

	std::array< Player, 2 > m_players;
	bool m_aiIsDealer{ true };
	bool m_aiIsKnocker{ false };

	RoundState m_roundState;

	Deck m_deck;
	Discard m_discard;

	bool m_animating{ false };
	Vec1 m_animatingTime{ 0.0f };

	void Animate() { m_animating = true; m_animatingTime = 0.0f; }
	bool Ready() const { return !m_animating; }
};

struct Mat
{
	Core::Resource::SpriteID m_matSprite;
};

struct GameRender
{
	std::array<Core::Resource::SpriteID, 52> m_cardFront;
	Core::Resource::SpriteID m_cardBack;
	usize m_highlightedPlayerCard{ ~0u };

	struct HeldCard
	{
		Vec2 m_grabPoint;
		usize m_cardI;
	};

	std::optional< HeldCard > m_holdingCard;
};

struct PlayerInteraction
{
	Vec2 m_initialHoldingCardPos;
};

}
}

namespace Core
{
template<>
void AddComponent( EntityID const _entity, Game::GinRummy::Mat const& _component );
template<>
void AddComponent( EntityID const _entity, Game::GinRummy::GameRender const& _component );
}