#pragma once

#include <ecs/ecs.h>
#include "Entity.h"
#include "common.h"

namespace Core
{
	struct Transform
	{
		// origin = position, basis = transform
		fTrans m_transform;
		Core::EntityID m_parent;

		Transform(Core::EntityID _parent = Core::EntityID{})
			: m_transform{ fQuat::getIdentity() } // identity
			, m_parent{ _parent }
		{}
		explicit Transform(fTrans const& _t, Core::EntityID _parent = Core::EntityID{})
			: m_transform{ _t }
			, m_parent{ _parent }
		{}
		explicit Transform(fQuat const& _q, fVec3 const& _p, Core::EntityID _parent = Core::EntityID{})
			: m_transform{ _q, _p }
			, m_parent{ _parent }
		{}

		// shorthand accessor
		fTrans& T() { return m_transform; }
		fTrans const& T() const { return m_transform; }

		fTrans const CalculateWorldTransform() const
		{
			Core::EntityID nextParent = m_parent;
			fTrans finalTransform = m_transform;
			while (nextParent.IsValid())
			{
				Transform const* const parentTrans = ecs::get_component<Transform>(nextParent.GetValue());
				ASSERT(parentTrans != nullptr);

				finalTransform = parentTrans->m_transform * finalTransform;
				nextParent = parentTrans->m_parent;
			}

			return finalTransform;
		}

		fTrans const CalculateWorldTransform(fTrans const& _localTransform) const
		{
			Core::EntityID nextParent = m_parent;
			fTrans finalTransform = m_transform * _localTransform;
			while (nextParent.IsValid())
			{
				Transform const* const parentTrans = ecs::get_component<Transform>(nextParent.GetValue());
				ASSERT(parentTrans != nullptr);

				finalTransform = parentTrans->m_transform * finalTransform;
				nextParent = parentTrans->m_parent;
			}

			return finalTransform;
		}

		fTrans const CalculateLocalTransform(fTrans const& _worldTransform) const
		{
			Core::EntityID nextParent = m_parent;
			fTrans finalTransform = _worldTransform;
			while (nextParent.IsValid())
			{
				Transform const* const parentTrans = ecs::get_component<Transform>(nextParent.GetValue());
				ASSERT(parentTrans != nullptr);

				finalTransform *= parentTrans->m_transform;
				nextParent = parentTrans->m_parent;
			}

			return finalTransform;
		}
	};
}