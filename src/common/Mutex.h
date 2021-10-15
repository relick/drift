#pragma once

#include <mutex>

template< typename T_Mutex, typename T_Value >
class MutexGuardBase
{
	std::scoped_lock< T_Mutex > m_lock;
	T_Value& m_value;
public:

	MutexGuardBase( T_Mutex& _mutex, T_Value& _val )
		: m_lock{ _mutex }
		, m_value{ _val }
	{}

	T_Value& operator*() { return m_value; }
	T_Value* operator->() { return &m_value; }
};

template< typename T_Mutex, typename T_Value >
class MutexBase
{
	T_Mutex m_mutex;
	T_Value m_value;

public:
	using Guard = MutexGuardBase< T_Mutex, T_Value >;

public:
	MutexBase() = default;
	MutexBase( T_Value const& _val ) : m_value{ _val } {}
	MutexBase( T_Value&& _val ) : m_value{ std::move(_val) } {}

	Guard Lock()
	{
		return { m_mutex, m_value };
	}
};

template< typename T >
using Mutex = MutexBase< std::mutex, T >;