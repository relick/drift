#pragma once

#include "common/MathDefs.h"

// Defines:
// Rect2D

template< ScalarType T >
struct Rect2DBase
{
	using Vec2 = Vector2Base< T >;

	Vec2 m_min;
	Vec2 m_max;

	bool Contains( Vec2 _p, bool _inclusive = true ) const
	{
		return _inclusive
			? ( glm::all( glm::greaterThanEqual( _p, m_min ) ) && glm::all( glm::lessThanEqual( _p, m_max ) ) )
			: ( glm::all( glm::greaterThan( _p, m_min ) ) && glm::all( glm::lessThan( _p, m_max ) ) )
			;
	}

	Rect2DBase() = default;
	Rect2DBase( Vec2 const& _min, Vec2 const& _max )
		: m_min( _min )
		, m_max( _max )
	{}
};

// Typedefs
using Rect2D = Rect2DBase< float >;

using dRect2D = Rect2DBase< double >;

using iRect2D = Rect2DBase< int32 >;