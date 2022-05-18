#pragma once

#include <memory>

// Defines:
// PolymorphicValue

namespace detail
{

template< typename T >
struct IPolValTypeHelper
{
	virtual T* Copy( T* ) const = 0;
};

template< typename T, typename U >
struct PolValTypeHelper : public IPolValTypeHelper< T >
{
	T* Copy( T* _o ) const override
	{
		if ( _o == nullptr ) return nullptr;
		new U( *static_cast< U* >( _o ) );
	}
};

}

template< typename T >
class PolymorphicValue
{
	T* m_val{ nullptr };
	std::shared_ptr<detail::IPolValTypeHelper> m_copy;

public:
	PolymorphicValue() = default;

	template< typename U >
	PolymorphicValue( U* _o )
		: m_val{ _o }
		, m_copy{ new detail::PolValTypeHelper<T, U>() }
	{}

	PolymorphicValue( PolymorphicValue<T> const& _o )
		: m_val{ _o.m_copy ? _o.m_copy->Copy( _o.m_val ) : nullptr }
		, m_copy{ _o.m_copy }
	{}

	PolymorphicValue( PolymorphicValue<T>&& _o )
		: m_val{ _o.m_val }
		, m_copy{ _o.m_copy }
	{
		_o.m_val = nullptr;
		_o.m_copy = nullptr;
	}

	~PolymorphicValue()
	{
		delete m_val;
	}

	PolymorphicValue<T>& operator=( PolymorphicValue<T> const& _o )
	{
		delete m_val;
		m_val = _o.m_copy ? m_copy->Copy( _o.m_val ) : nullptr;
		m_copy = _o.m_copy;

		return *this;
	}

	PolymorphicValue<T>& operator=( PolymorphicValue<T>&& _o )
	{
		delete m_val;
		m_val = _o.m_val;
		m_copy = _o.m_copy;

		_o.m_val = nullptr;
		_o.m_copy = nullptr;

		return *this;
	}

	T* operator->()
	{
		return m_val;
	}

	T& operator*()
	{
		return *m_val;
	}

	T const* operator->() const
	{
		return m_val;
	}

	T const& operator*() const
	{
		return *m_val;
	}

	explicit operator bool() const
	{
		return m_val != nullptr;
	}
};

// Typedefs