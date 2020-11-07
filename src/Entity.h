#pragma once

#include <ecs/entity_id.h>
#include <compare>

namespace Core
{
	class EntityID
	{
		static constexpr ecs::entity_id null_id{ static_cast<ecs::entity_id>(-1) };
		ecs::entity_id m_entity{ null_id };
	public:
		ecs::entity_id GetValue() const { return m_entity; }
		bool IsValid() const { return m_entity != null_id; }
		bool IsNull() const { return m_entity == null_id; }
		auto operator<=>(EntityID const&) const = default;
		bool operator==(EntityID const&) const = default;

		EntityID() = default;
		EntityID(ecs::entity_id _entity) : m_entity{ _entity } {}
		EntityID& operator=(EntityID const&) = default;
	};
}

namespace std
{
	template<>
	struct hash<Core::EntityID>
	{
		std::size_t operator()(Core::EntityID const& _k) const
		{
			return hash<ecs::detail::entity_type>()(_k.GetValue());
		}
	};
}