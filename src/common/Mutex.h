#pragma once

#include <absl/synchronization/mutex.h>
#include <mutex>
#include <shared_mutex>

template< typename T_Mutex, typename T_MutexReadLocker, typename T_Value >
class MutexReadGuardBase
{
	T_MutexReadLocker m_lock;
	T_Value const& m_value;
public:

	MutexReadGuardBase( T_Mutex& _mutex, T_Value const& _val )
		: m_lock{ _mutex }
		, m_value{ _val }
	{}

	T_Value const& operator*() const { return m_value; }
	T_Value const* operator->() const { return &m_value; }
};

template< typename T_Mutex, typename T_MutexWriteLocker, typename T_Value >
class MutexWriteGuardBase
{
	T_MutexWriteLocker m_lock;
	T_Value& m_value;
public:

	MutexWriteGuardBase( T_Mutex& _mutex, T_Value& _val )
		: m_lock{ _mutex }
		, m_value{ _val }
	{}

	T_Value& operator*() { return m_value; }
	T_Value* operator->() { return &m_value; }
};

namespace detail
{
// For some nonsense reason, absl locks take Mutex* instead of Mutex&. this helper just passes them through.
template< typename T_AbslLock >
class AbslLockHelper : public T_AbslLock
{
public:
	explicit AbslLockHelper( absl::Mutex& mu ) : T_AbslLock{ &mu }
	{}
};
}

template< typename T_Mutex, typename T_MutexReadLocker, typename T_MutexWriteLocker, typename T_Value >
class MutexBase
{
	T_Mutex m_mutex;
	T_Value m_value;

public:
	using ReadGuard = MutexReadGuardBase< T_Mutex, T_MutexReadLocker,  T_Value >;
	using WriteGuard = MutexWriteGuardBase< T_Mutex, T_MutexWriteLocker,  T_Value >;

public:
	MutexBase() = default;
	MutexBase( T_Value const& _val ) : m_value{ _val } {}
	MutexBase( T_Value&& _val ) : m_value{ std::move(_val) } {}

	ReadGuard Read()
	{
		return { m_mutex, m_value };
	}

	WriteGuard Write()
	{
		return { m_mutex, m_value };
	}
};

template< typename T >
using Mutex = MutexBase< absl::Mutex, detail::AbslLockHelper< absl::ReaderMutexLock >, detail::AbslLockHelper< absl::WriterMutexLock >, T >;

template< typename T >
using RecursiveMutex = MutexBase< std::recursive_mutex, std::scoped_lock< std::recursive_mutex >, std::scoped_lock< std::recursive_mutex >, T >;
