#include "GinRummyComponents.h"

#include "managers/ResourceManager.h"
#include "managers/RenderManager.h"
#include "shaders/sprites_constants.glslh"

#include <algorithm>
#include <random>
#include <format>

namespace Game
{
namespace GinRummy
{

Deck::Deck()
{
	Reset();
	Shuffle();
}

void Deck::Reset()
{
	m_cards.clear();
	for ( usize suitI = 0; suitI < ( usize )Card::Suit::Count; ++suitI )
	{
		for ( usize faceI = 0; faceI < ( usize )Card::Face::Count; ++faceI )
		{
			m_cards.push_back( { ( Card::Suit )suitI, ( Card::Face )faceI } );
		}
	}
}

void Deck::Shuffle()
{
	std::random_device rd;
	std::mt19937 rng{ rd() };

	std::shuffle( m_cards.begin(), m_cards.end(), rng );

	// Hand override
	/*m_cards.assign(52, {Card::Suit::Diamonds, Card::Face::Ace});

	m_cards[ 50 ] = { Card::Suit::Spades, Card::Face::Two };
	m_cards[ 48 ] = { Card::Suit::Spades, Card::Face::Three };
	m_cards[ 46 ] = { Card::Suit::Spades, Card::Face::Four };
	m_cards[ 44 ] = { Card::Suit::Spades, Card::Face::Five };
	m_cards[ 42 ] = { Card::Suit::Clubs, Card::Face::Four };
	m_cards[ 40 ] = { Card::Suit::Hearts, Card::Face::Four };
	m_cards[ 38 ] = { Card::Suit::Diamonds, Card::Face::Four };
	m_cards[ 36 ] = { Card::Suit::Hearts, Card::Face::Seven };
	m_cards[ 34 ] = { Card::Suit::Hearts, Card::Face::Eight };
	m_cards[ 32 ] = { Card::Suit::Clubs, Card::Face::Ace };*/
}

Card Deck::Draw()
{
	kaAssert( m_cards.size() > 0 );
	Card const end = m_cards.back();
	m_cards.pop_back();
	return end;
}

Card Discard::CheckTop
(
)	const
{
	kaAssert( !m_discards.empty() );
	return m_discards.back();
}

void Discard::Add
(
	Card _card
)
{
	m_discards.push_back( _card );
}

Card Discard::PickUpTop()
{
	kaAssert( !m_discards.empty() );
	Card const top = m_discards.back();
	m_discards.pop_back();
	return top;
}

std::vector<Card> Hand::GetBestJunkCards
(
	bool _includeDrawn
)	const
{
	std::vector<std::vector<Card>> matches;
	std::vector<std::vector<Card>> runs;

	// Get Matches
	for ( usize faceI = 0; faceI < ( usize )Card::Face::Count; ++faceI )
	{
		uint8 matchCount = 0;
		std::vector<Card> match;
		for ( usize suitI = 0; suitI < ( usize )Card::Suit::Count; ++suitI )
		{
			Card const thisCard{ ( Card::Suit )suitI, ( Card::Face )faceI };
			bool const foundCard = std::find( m_cards.begin(), m_cards.end(), thisCard ) != m_cards.end() || ( _includeDrawn && MatchesDrawnCard( thisCard ) );

			if ( foundCard )
			{
				match.push_back( thisCard );
				++matchCount;
			}
		}

		if ( matchCount >= 3 )
		{
			matches.push_back( match );
		}
	}

	// Get Runs
	for ( usize suitI = 0; suitI < ( usize )Card::Suit::Count; ++suitI )
	{
		bool runStarted = false;
		std::vector<Card> run;
		for ( usize faceI = 0; faceI < ( usize )Card::Face::Count; ++faceI )
		{
			Card const thisCard{ ( Card::Suit )suitI, ( Card::Face )faceI };
			bool const foundCard = std::find( m_cards.begin(), m_cards.end(), thisCard ) != m_cards.end() || ( _includeDrawn && MatchesDrawnCard( thisCard ) );

			if ( foundCard )
			{
				run.push_back( thisCard );
				runStarted = true;
			}

			if ( runStarted && !foundCard )
			{
				if ( run.size() >= 3 )
				{
					runs.push_back( run );
				}
				run.clear();
				runStarted = false;
			}
		}

		// If run includes king
		if ( runStarted )
		{
			if ( run.size() >= 3 )
			{
				runs.push_back( run );
			}
		}
	}

	if ( matches.empty() || runs.empty() )
	{
		// All matches or all runs means no overlaps.
		return GetJunk( matches, runs, _includeDrawn );
	}

	// Split matches and runs into overlapping groups of 3.
	// Calculate values of all combinations of including or excluding matches
	// Take best - this calculation method sucks but I think it works and that'll do.
	std::vector<std::vector<Card>> splitMatches;
	for ( std::vector<Card> const& match : matches )
	{
		splitMatches.push_back( match );
		if ( match.size() > 3 )
		{
			splitMatches.push_back( { match[ 0 ], match[ 1 ], match[ 2 ], } );
			splitMatches.push_back( { match[ 0 ], match[ 1 ], match[ 3 ], } );
			splitMatches.push_back( { match[ 0 ], match[ 2 ], match[ 3 ], } );
			splitMatches.push_back( { match[ 1 ], match[ 2 ], match[ 3 ], } );
		}
	}

	std::vector<std::vector<Card>> splitRuns;
	for ( std::vector<Card> const& run : runs )
	{
		for ( usize i = 2; i < run.size(); ++i )
		{
			splitRuns.push_back( { run[ i - 2 ], run[ i - 1 ], run[ i ], } );
		}
	}

	return GetBestJunkCards_TestCombos( splitMatches, splitRuns, _includeDrawn, 0 ).second;
}

std::vector<Card> Hand::GetJunk
(
	std::vector<std::vector<Card>> const& _matches,
	std::vector<std::vector<Card>> const& _splitRuns,
	bool _includeDrawn
)	const
{
	std::vector<Card> junk{ m_cards.begin(), m_cards.end() };
	std::vector<Card> allMatches;

	if ( _includeDrawn && m_drawnCard.has_value() )
	{
		junk.push_back( *m_drawnCard );
	}

	for ( std::vector<Card> const& l : _matches )
	{
		for ( Card const& c : l )
		{
			allMatches.push_back( c );
			std::erase( junk, c );
		}
	}

	for ( std::vector<Card> const& l : _splitRuns )
	{
		bool skip = false;
		for ( Card const& c : l )
		{
			if ( std::find( allMatches.begin(), allMatches.end(), c ) != allMatches.end() )
			{
				skip = true;
				break;
			}
		}
		if ( skip )
		{
			continue;
		}
		for ( Card const& c : l )
		{
			std::erase( junk, c );
		}
	}

	return junk;
}

std::pair<uint32, std::vector<Card>> Hand::GetBestJunkCards_TestCombos
(
	std::vector<std::vector<Card>> _matches,
	std::vector<std::vector<Card>> const& _splitRuns,
	bool _includeDrawn,
	usize _matchesN
)	const
{
	if ( _matchesN >= _matches.size() )
	{
		auto junk = GetJunk( _matches, _splitRuns, _includeDrawn );
		return { CalculateValue( junk ), junk };
	}
	else
	{
		auto include = GetBestJunkCards_TestCombos( _matches, _splitRuns, _includeDrawn, _matchesN + 1 );

		_matches.erase( _matches.begin() + _matchesN );
		auto exclude = GetBestJunkCards_TestCombos( _matches, _splitRuns, _includeDrawn, _matchesN );

		if ( include.first < exclude.first )
		{
			return include;
		}
		else
		{
			return exclude;
		}
	}
}

uint32 Hand::CalculateValue
(
	std::vector<Card> const& _junkCards
)	const
{
	uint32 value{ 0 };
	for ( Card const& card : _junkCards )
	{
		value += card.GetValue();
	}
	return value;
}

uint32 Hand::CalculateValue
(
	bool _includesDrawn
)	const
{
	return CalculateValue( GetBestJunkCards( _includesDrawn ) );
}

}
}

namespace Core
{
template<>
void AddComponent
(
	EntityID const _entity,
	Game::GinRummy::GameRender const& _component
)
{
	Game::GinRummy::GameRender newComponent = _component;
	Core::Resource::SpriteID cardBack;
	{
		bool const success = Core::Resource::LoadSprite( "assets/encrypted/sprites/ginrummy/cardback.spr", cardBack );
		kaAssert( success );
	}

	static constexpr std::array<char const*, 4> suitInitials{ "D", "C", "H", "S", };
	absl::InlinedVector<Core::Resource::SpriteID, 52> cardFronts;
	cardFronts.resize( 52 );

	for ( usize suitI = 0; suitI < 4; ++suitI )
	{
		for ( usize faceI = 0; faceI < 13; ++faceI )
		{
			bool const success = Core::Resource::LoadSprite(
				std::format( "assets/encrypted/sprites/ginrummy/cardfront_sprites/{:s}{:d}.spr", suitInitials[ suitI ], faceI ),
				cardFronts[ suitI * 13 + faceI ]
			);
			kaAssert( success );
		}
	}

	for ( usize cardI = 0; cardI < 52; ++cardI )
	{
		newComponent.m_cards[ cardI ].m_cardFront = Core::Render::AddSpriteToScene( cardFronts[ cardI ], Trans2D(), SpriteFlag_Hidden );
		newComponent.m_cards[ cardI ].m_cardBack = Core::Render::AddSpriteToScene( cardBack, Trans2D(), SpriteFlag_Hidden );
	}

	ECS::AddComponent( _entity, newComponent );
}
}