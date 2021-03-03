#pragma once

#include <ecs/entity_id.h>
#include <compare>

namespace Core
{
	// don't you just love forward declares!
	class EntityID;

	namespace detail
	{
		inline ecs::entity_id AccessECSID(EntityID const);
	}

	class EntityID
	{
	public:
		using CoreType = ecs::entity_id;

	protected:
		friend CoreType detail::AccessECSID(EntityID const);

		static constexpr CoreType s_nullID{ static_cast<CoreType>(-1) };
		CoreType m_entity{ s_nullID };
	public:
		bool IsValid() const { return m_entity != s_nullID; }
		bool IsNull() const { return m_entity == s_nullID; }
		auto operator<=>(EntityID const&) const = default;
		bool operator==(EntityID const&) const = default;

		EntityID() = default;
		EntityID(CoreType _entity) : m_entity{ _entity } {}
		EntityID& operator=(EntityID const&) = default;

#if DEBUG_TOOLS
		CoreType GetDebugValue() const { return m_entity; }
		// GetDebugString()
#endif
	};

	namespace detail
	{
		inline EntityID::CoreType AccessECSID(EntityID const _e)
		{
			return _e.m_entity;
		}
	}
}

namespace std
{
	template<>
	struct hash<Core::EntityID>
	{
		std::size_t operator()(Core::EntityID const& _k) const
		{
			return hash<ecs::detail::entity_type>()(Core::detail::AccessECSID(_k));
		}
	};
}