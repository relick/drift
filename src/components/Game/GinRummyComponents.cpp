#include "GinRummyComponents.h"

#include "managers/ResourceManager.h"

#include <algorithm>
#include <random>

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
	kaAssert( m_topDiscard.has_value() );
	return m_topDiscard.value();
}

void Discard::Add
(
	Card _card
)
{
	m_discardPileSize++;
	m_secondDiscard = m_topDiscard;
	m_topDiscard = _card;
}

Card Discard::PickUpTop()
{
	kaAssert( m_topDiscard.has_value() );
	Card const top = m_topDiscard.value();
	m_topDiscard = m_secondDiscard;
	m_discardPileSize--;
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
					run.clear();
				}
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
		return GetJunk( matches, runs );
	}

	// Split runs into overlapping groups of 3.
	// Calculate values of all combinations of including or excluding matches
	// Take best - this should be a maximum of 4 calculations given a run must be at least 3 cards, leaving only 8 that can be matches, of which only 2 matches can fit
	std::vector<std::vector<Card>> splitRuns;
	for ( std::vector<Card> const& run : runs )
	{
		for ( usize i = 2; i < run.size(); ++i )
		{
			splitRuns.push_back( { run[ i - 2 ], run[ i - 1 ], run[ i ], } );
		}
	}

	return GetBestJunkCards_TestCombos( matches, splitRuns, 0 ).second;
}

std::vector<Card> Hand::GetJunk
(
	std::vector<std::vector<Card>> const& _matches,
	std::vector<std::vector<Card>> const& _splitRuns
)	const
{
	std::vector<Card> junk{ m_cards.begin(), m_cards.end() };
	std::vector<Card> allMatches;

	if ( m_drawnCard.has_value() )
	{
		junk.push_back( m_drawnCard.value() );
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
	usize _matchesN
)	const
{
	if ( _matchesN >= _matches.size() )
	{
		auto junk = GetJunk( _matches, _splitRuns );
		return { CalculateValue( junk ), junk };
	}
	else
	{
		auto include = GetBestJunkCards_TestCombos( _matches, _splitRuns, _matchesN + 1 );

		_matches.erase( _matches.begin() + _matchesN );
		auto exclude = GetBestJunkCards_TestCombos( _matches, _splitRuns, _matchesN );

		if ( include.first > exclude.first )
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
	Game::GinRummy::Mat const& _component
)
{
	Game::GinRummy::Mat newComponent = _component;
	if ( newComponent.m_matSprite.IsNull() )
	{
		bool const success = Core::Resource::LoadSprite( "assets/encrypted/sprites/ginrummy/mat.spr", newComponent.m_matSprite );
		kaAssert( success );
	}

	ECS::AddComponent( _entity, newComponent );
}

template<>
void AddComponent
(
	EntityID const _entity,
	Game::GinRummy::GameRender const& _component
)
{
	Game::GinRummy::GameRender newComponent = _component;
	if ( newComponent.m_cardBack.IsNull() )
	{
		bool const success = Core::Resource::LoadSprite( "assets/encrypted/sprites/ginrummy/cardback.spr", newComponent.m_cardBack );
		kaAssert( success );
	}
	if ( newComponent.m_cardFront[ 0 ].IsNull() )
	{
		bool const success = Core::Resource::LoadSprite( "assets/encrypted/sprites/ginrummy/cardfront.spr", newComponent.m_cardFront[ 0 ] );
		kaAssert( success );
	}

	ECS::AddComponent( _entity, newComponent );
}
}