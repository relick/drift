#pragma once

#include "Entity.h"
#include "managers/EntityManager.h"
#include "common.h"

namespace Core
{
	struct Transform
	{
		// origin = position, basis = rotation
		fTrans m_transform{};
		Core::EntityID m_parent;

		Transform(Core::EntityID _parent = Core::EntityID{})
			: m_parent{ _parent }
		{}
		explicit Transform(fTrans const& _t, Core::EntityID _parent = Core::EntityID{})
			: m_transform{ _t }
			, m_parent{ _parent }
		{}
		explicit Transform(fQuat const& _q, fVec3 const& _p, Core::EntityID _parent = Core::EntityID{})
			: m_transform{ static_cast<fMat3>(_q), _p }
			, m_parent{ _parent }
		{}
		explicit Transform(fMat3 const& _m, fVec3 const& _p, Core::EntityID _parent = Core::EntityID{})
			: m_transform{ _m, _p }
			, m_parent{ _parent }
		{}

		// shorthand accessor
		fTrans& T() { return m_transform; }
		fTrans const& T() const { return m_transform; }

		void DetachFromParent()
		{
			m_transform = CalculateWorldTransform();
			m_parent = Core::EntityID();
		}

		void SetLocalTransformFromWorldTransform(fTrans const& _worldTransform)
		{
			Core::EntityID nextParent = m_parent;
			fTrans finalTransform = fTrans();
			while (nextParent.IsValid())
			{
				Transform const* const parentTrans = Core::GetComponent<Transform>(nextParent);
				kaAssert(parentTrans != nullptr);

				finalTransform = parentTrans->m_transform * finalTransform;
				nextParent = parentTrans->m_parent;
			}

			m_transform = finalTransform.ToLocal(_worldTransform);
		}

		fTrans CalculateWorldTransform() const
		{
			Core::EntityID nextParent = m_parent;
			fTrans finalTransform = m_transform;
			while (nextParent.IsValid())
			{
				Transform const* const parentTrans = Core::GetComponent<Transform>(nextParent);
				kaAssert(parentTrans != nullptr);

				finalTransform = parentTrans->m_transform * finalTransform;
				nextParent = parentTrans->m_parent;
			}

			return finalTransform;
		}

		fTrans CalculateWorldTransform(fTrans const& _localTransform) const
		{
			return CalculateWorldTransform() * _localTransform;
		}

		fTrans CalculateLocalTransform(fTrans const& _worldTransform) const
		{
			return CalculateWorldTransform().ToLocal(_worldTransform);
		}
	};
}