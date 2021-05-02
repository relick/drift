#pragma once

#include <ecs/entity_id.h>

namespace Core
{
	// don't you just love forward declares!
	class EntityID;

	namespace detail
	{
		inline ecs::entity_id AccessECSID(EntityID);
	}

	class EntityID
	{
	public:
		using CoreType = ecs::entity_id;

	private:
		friend CoreType detail::AccessECSID(EntityID);

		static constexpr CoreType s_nullID{ static_cast<std::size_t>(-1) };
		CoreType m_entity{ s_nullID };

	public:
		bool IsValid() const { return m_entity != s_nullID; }
		bool IsNull() const { return m_entity == s_nullID; }
		bool operator<(EntityID const& _o) const
		{
			return m_entity < _o.m_entity;
		}
		bool operator==(EntityID const& _o) const
		{
			return m_entity == _o.m_entity;
		}

		EntityID() = default;
		EntityID(CoreType _entity) : m_entity{ _entity } {}

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

#include <functional>
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