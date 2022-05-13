#pragma once

#include "common.h"
#include <compare>
#include <concepts>
#include <sokol_gfx.h> // wish we didn't need to include mega-headers just for struct defs

namespace Core
{
	template<typename T_IDType, typename T_Value = uint32>
	class ID
	{
	public:
		using ValueType = T_Value;

	private:
		static constexpr ValueType null_id{ static_cast< ValueType >( -1 ) };
		ValueType m_id{ null_id };

	public:
		static ID<T_IDType, T_Value> FromIndex( usize i )
		{
			return ID<T_IDType, T_Value>{ ValueType( i ) };
		}

		ValueType GetValue() const { return m_id; }
		bool IsValid() const { return m_id != null_id; }
		bool IsNull() const { return m_id == null_id; }
		bool operator==(ID<T_IDType, ValueType> const& _b) const { return m_id == _b.m_id; }
		bool operator<(ID<T_IDType, ValueType> const& _b) const { return m_id < _b.m_id; }

		ID() = default;
		explicit ID(ValueType _idvalue) : m_id{ _idvalue } {}
		ID& operator=(ID<T_IDType, ValueType> const& _b) { m_id = _b.m_id; return *this; }
	};

	template<typename T_SokolIDValue, bool t_trackedByGame>
	class SokolIDWrapper;

	namespace detail
	{
		template<typename T, typename ValueType, bool t_trackedByGame>
		concept AssignableSokolID = std::same_as<T, SokolIDWrapper<ValueType, T::s_trackedByGame>> && requires
		{
			requires T::s_trackedByGame || T::s_trackedByGame == t_trackedByGame;
		};
	}

	template<typename T_SokolIDValue, bool t_trackedByGame>
	class SokolIDWrapper
	{
	public:
		using SokolCoreType = decltype(T_SokolIDValue::id);
		using SokolIDType = T_SokolIDValue;
		static constexpr bool s_trackedByGame = t_trackedByGame;

	private:
		static constexpr SokolIDType null_id{ static_cast<SokolCoreType>(SG_INVALID_ID) };
		SokolIDType m_id{ null_id };

	public:
		static SokolIDWrapper<T_SokolIDValue, t_trackedByGame> FromIndex( usize i )
		{
			return SokolIDWrapper<T_SokolIDValue, t_trackedByGame>{ SokolIDType{ SokolCoreType( i ) } };
		}

		SokolCoreType GetValue() const { return m_id.id; }
		SokolIDType GetSokolID() const { return m_id; }
		bool IsValid() const { return m_id.id != null_id.id; }
		bool IsNull() const { return m_id.id == null_id.id; }
		template<bool t_otherTrackedByGame>
		bool operator==(SokolIDWrapper<SokolIDType, t_otherTrackedByGame> const& _b) const { return m_id.id == _b.m_id.id; }
		template<bool t_otherTrackedByGame>
		bool operator<(SokolIDWrapper<SokolIDType, t_otherTrackedByGame> const& _b) const { return m_id.id < _b.m_id.id; }

		SokolIDWrapper() = default;
		SokolIDWrapper(SokolIDType _idvalue) : m_id{ _idvalue } {}
		template<detail::AssignableSokolID<SokolIDType, t_trackedByGame> T_ID>
		SokolIDWrapper(T_ID const& _b) : m_id{ _b.GetValue() } {}
		template<detail::AssignableSokolID<SokolIDType, t_trackedByGame> T_ID>
		SokolIDWrapper& operator=(T_ID const& _b) { m_id.id = _b.GetValue(); return *this; }
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

	template <typename T_SokolIDValue>
	struct hash<Core::SokolIDWrapper<T_SokolIDValue, true>>
	{
		std::size_t operator()(Core::SokolIDWrapper<T_SokolIDValue, true> const& _k) const
		{
			return hash<typename Core::SokolIDWrapper<T_SokolIDValue, true>::SokolCoreType>()(_k.GetValue());
		}
	};
}