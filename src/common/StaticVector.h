#pragma once

#include "common/Debug.h"
#include "common/MathDefs.h"

#include <vector>
#include <type_traits>
#include <memory>
#include <utility>

// Defines:
// StaticVector

template< typename T_ID, typename T_Value >
class StaticVector;

template< typename T_ID, typename T_Value >
class StaticVectorIterator
{
	StaticVector< T_ID, T_Value >& m_vec;
	usize m_pos{ ~0u };

public:
	explicit StaticVectorIterator( StaticVectorIterator< T_ID, T_Value > const& ) = default;
	explicit StaticVectorIterator( StaticVectorIterator< T_ID, T_Value >&& ) = default;
	StaticVectorIterator< T_ID, T_Value >& operator=( StaticVectorIterator< T_ID, T_Value > const& ) = default;
	StaticVectorIterator< T_ID, T_Value >& operator=( StaticVectorIterator< T_ID, T_Value >&& ) = default;

	StaticVectorIterator( StaticVector< T_ID, T_Value >& _vec, usize _pos = ~0u )
		: m_vec{ _vec }
		, m_pos{ _vec.FindNextActiveSlot( _pos ) }
	{}

	std::pair<T_ID, T_Value&> operator*()
	{
		T_ID const id{ typename T_ID::ValueType( m_pos ) };
		return { id, m_vec[ id ] };
	}

	std::pair<T_ID, T_Value const&> operator*() const
	{
		T_ID const id{ typename T_ID::ValueType( m_pos ) };
		return { id, m_vec[ id ] };
	}

	bool operator==( StaticVectorIterator< T_ID, T_Value > const& _o )
	{
		return &_o.m_vec == &m_vec && _o.m_pos == m_pos;
	}

	bool operator!=( StaticVectorIterator< T_ID, T_Value > const& _o )
	{
		return &_o.m_vec != &m_vec || _o.m_pos != m_pos;
	}

	StaticVectorIterator< T_ID, T_Value >& operator++()
	{
		m_pos = m_vec.FindNextActiveSlot( m_pos - 1u );
		return *this;
	}
	StaticVectorIterator< T_ID, T_Value > operator++( int )
	{
		StaticVectorIterator< T_ID, T_Value > temp = *this;
		m_pos = m_vec.FindNextActiveSlot( m_pos - 1u );
		return temp;
	}

	StaticVectorIterator< T_ID, T_Value >& operator--()
	{
		m_pos = m_vec.FindNextActiveSlot( m_pos - 1u );
		return *this;
	}
	StaticVectorIterator< T_ID, T_Value > operator--(int)
	{
		StaticVectorIterator< T_ID, T_Value > temp = *this;
		m_pos = m_vec.FindNextActiveSlot( m_pos - 1u );
		return temp;
	}
};

template< typename T_ID, typename T_Value >
class StaticVector
{
	using Iter = StaticVectorIterator< T_ID, T_Value >;
	friend Iter;

	enum ActiveState : uint8
	{
		Free = 0u,
		Active = 1u,
	};
	using RawData = std::aligned_storage_t< sizeof( T_Value ), alignof( T_Value ) >;

	std::vector<RawData> m_data;
	std::vector<uint8> m_active;

public:
	StaticVector()
	{}

	~StaticVector()
	{
		for ( usize i = 0; i < m_active.size(); ++i )
		{
			if ( m_active[ i ] == Active )
			{
				std::destroy_at( std::launder( reinterpret_cast< T_Value* >( &m_data[ i ] ) ) );
			}
		}
	}

	template<typename... Args>
	T_ID Emplace( Args&&... _args )
	{
		usize const nextSlot = FindFirstFreeSlot();
		kaAssert( nextSlot <= m_active.size() );
		if ( nextSlot == m_active.size() )
		{
			m_data.emplace_back();
			m_active.emplace_back();
		}
		m_active[ nextSlot ] = Active;
		::new ( &m_data[ nextSlot ] ) T_Value( std::forward<Args>( _args )... );

		return T_ID{ typename T_ID::ValueType( nextSlot ) };
	}

	T_Value& operator[]( T_ID _id )
	{
		kaAssert( _id.GetValue() < m_active.size() );
		kaAssert( m_active[ _id.GetValue() ] == Active );
		return *std::launder( reinterpret_cast< T_Value* >( &m_data[ _id.GetValue() ] ) );
	}

	T_Value const& operator[]( T_ID _id ) const
	{
		kaAssert( _id.GetValue() < m_active.size() );
		kaAssert( m_active[ _id.GetValue() ] == Active );
		return *std::launder( reinterpret_cast< T_Value const* >( &m_data[ _id.GetValue() ] ) );
	}

	void Erase( T_ID _id )
	{
		kaAssert( _id.GetValue() < m_active.size() );
		kaAssert( m_active[ _id.GetValue() ] == Active );
		std::destroy_at( std::launder( reinterpret_cast< T_Value* >( &m_data[ _id.GetValue() ] ) ) );
		m_active[ _id.GetValue() ] = Free;
	}

	Iter begin()
	{
		return Iter{ *this, 0u };
	}

	Iter begin() const
	{
		return Iter{ *this, 0u };
	}

	Iter cbegin() const
	{
		return begin();
	}

	Iter end()
	{
		return Iter{ *this };
	}

	Iter end() const
	{
		return Iter{ *this };
	}

	Iter cend() const
	{
		return end();
	}

private:
	usize FindFirstFreeSlot() const
	{
		for ( usize i = 0; i < m_active.size(); ++i )
		{
			if ( m_active[ i ] == Free )
			{
				return i;
			}
		}

		return m_active.size();
	}

	usize FindNextActiveSlot( usize _start = 0u ) const
	{
		for ( usize i = _start; i < m_active.size(); ++i )
		{
			if ( m_active[ i ] == Active )
			{
				return i;
			}
		}

		return m_active.size();
	}

	usize FindPrevActiveSlot( usize _start ) const
	{
		for ( usize i = _start; i > 0u; --i )
		{
			if ( m_active[ i ] == Active )
			{
				return i;
			}
		}

		return !m_active.empty() && m_active[ 0 ] == Active ? 0u : m_active.size();
	}
};
