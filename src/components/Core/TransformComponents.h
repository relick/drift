#pragma once

#include "Entity.h"
#include "managers/EntityManager.h"
#include "common.h"

namespace Core
{
	// Transform types split between 'normal' (3D) and '2D' (sprites only).
	// In a way this is kind of inconvenient but I don't think it's ever going to be an actual issue
	// In fact, if the same entity ever wants to exist in both 3D and 2D, it feels better that they control those two transforms separately.

	// Used by non-sprites
	struct Transform3D
	{
		// origin = position, basis = rotation
		fTrans m_transform{};
		Core::EntityID m_parent;

		Transform3D(Core::EntityID _parent = Core::EntityID{})
			: m_parent{ _parent }
		{}
		explicit Transform3D(fTrans const& _t, Core::EntityID _parent = Core::EntityID{})
			: m_transform{ _t }
			, m_parent{ _parent }
		{}
		explicit Transform3D(fQuat const& _q, fVec3 const& _p, Core::EntityID _parent = Core::EntityID{})
			: m_transform{ static_cast<fMat3>(_q), _p }
			, m_parent{ _parent }
		{}
		explicit Transform3D(fMat3 const& _m, fVec3 const& _p, Core::EntityID _parent = Core::EntityID{})
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
				Transform3D const* const parentTrans = Core::GetComponent<Transform3D>(nextParent);
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
				Transform3D const* const parentTrans = Core::GetComponent<Transform3D>(nextParent);
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

	// Used by sprites
	struct Transform2D
	{
		fTrans2D m_transform;
		Core::EntityID m_parent;

		Transform2D(Core::EntityID _parent = Core::EntityID{})
			: m_parent{ _parent }
		{}
		explicit Transform2D(fTrans2D const& _t, Core::EntityID _parent = Core::EntityID{})
			: m_transform{ _t }
			, m_parent{ _parent }
		{}

		// shorthand accessor
		fTrans2D& T() { return m_transform; }
		fTrans2D const& T() const { return m_transform; }

		void DetachFromParent()
		{
			m_transform = CalculateWorldTransform();
			m_parent = Core::EntityID();
		}

		void SetLocalTransformFromWorldTransform(fTrans2D const& _worldTransform)
		{
			Core::EntityID nextParent = m_parent;
			fTrans2D finalTransform = fTrans2D();
			while (nextParent.IsValid())
			{
				Transform2D const* const parentTrans = Core::GetComponent<Transform2D>(nextParent);
				kaAssert(parentTrans != nullptr);

				finalTransform = parentTrans->m_transform * finalTransform;
				nextParent = parentTrans->m_parent;
			}

			m_transform = finalTransform.ToLocal(_worldTransform);
		}

		fTrans2D CalculateWorldTransform() const
		{
			Core::EntityID nextParent = m_parent;
			fTrans2D finalTransform = m_transform;
			while (nextParent.IsValid())
			{
				Transform2D const* const parentTrans = Core::GetComponent<Transform2D>(nextParent);
				kaAssert(parentTrans != nullptr);

				finalTransform = parentTrans->m_transform * finalTransform;
				nextParent = parentTrans->m_parent;
			}

			return finalTransform;
		}

		fTrans2D CalculateWorldTransform(fTrans2D const& _localTransform) const
		{
			return CalculateWorldTransform() * _localTransform;
		}

		fTrans2D CalculateLocalTransform(fTrans2D const& _worldTransform) const
		{
			return CalculateWorldTransform().ToLocal(_worldTransform);
		}
	};
}