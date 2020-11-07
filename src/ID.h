#pragma once

#include "common.h"
#include <compare>

namespace Core
{
	template<typename T_IDType, typename T_Value = uint32>
	class ID
	{
		static constexpr T_Value null_id{ static_cast<T_Value>(-1) };
		T_Value m_id{ null_id };
	public:
		using ValueType = T_Value;
		T_Value GetValue() const { return m_id; }
		bool IsValid() const { return m_id != null_id; }
		bool IsNull() const { return m_id == null_id; }
		template<typename T_OtherIDType>
		bool operator==(ID<T_OtherIDType> const& _b) const { return _b.m_id == m_id; }

		ID() = default;
		ID(T_Value _idvalue) : m_id{ _idvalue } {}
		template<typename T_OtherIDType>
		ID& operator=(ID<T_OtherIDType> const& _b) { m_id = _b.m_id; return *this; }
	};
}

namespace std
{
	template <typename T_IDType, typename T_Value>
	struct hash<Core::ID<T_IDType, T_Value>>
	{
		std::size_t operator()(Core::ID<T_IDType, T_Value> const& _k) const
		{
			return hash<T_Value>()(_k.GetValue());
		}
	};
}