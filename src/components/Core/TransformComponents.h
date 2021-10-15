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
		Trans m_transform{};
		Core::EntityID m_parent;

		Transform3D(Core::EntityID _parent = Core::EntityID{})
			: m_parent{ _parent }
		{}
		explicit Transform3D(Trans const& _t, Core::EntityID _parent = Core::EntityID{})
			: m_transform{ _t }
			, m_parent{ _parent }
		{}
		explicit Transform3D(Quat const& _q, Vec3 const& _p, Core::EntityID _parent = Core::EntityID{})
			: m_transform{ static_cast<Mat3>(_q), _p }
			, m_parent{ _parent }
		{}
		explicit Transform3D(Mat3 const& _m, Vec3 const& _p, Core::EntityID _parent = Core::EntityID{})
			: m_transform{ _m, _p }
			, m_parent{ _parent }
		{}

		// shorthand accessor
		Trans& T() { return m_transform; }
		Trans const& T() const { return m_transform; }

		void DetachFromParent()
		{
			m_transform = CalculateWorldTransform();
			m_parent = Core::EntityID();
		}

		void SetLocalTransformFromWorldTransform(Trans const& _worldTransform)
		{
			Core::EntityID nextParent = m_parent;
			Trans finalTransform = Trans();
			while (nextParent.IsValid())
			{
				Transform3D const* const parentTrans = Core::GetComponent<Transform3D>(nextParent);
				kaAssert(parentTrans != nullptr);

				finalTransform = parentTrans->m_transform * finalTransform;
				nextParent = parentTrans->m_parent;
			}

			m_transform = finalTransform.ToLocal(_worldTransform);
		}

		Trans CalculateWorldTransform() const
		{
			Core::EntityID nextParent = m_parent;
			Trans finalTransform = m_transform;
			while (nextParent.IsValid())
			{
				Transform3D const* const parentTrans = Core::GetComponent<Transform3D>(nextParent);
				kaAssert(parentTrans != nullptr);

				finalTransform = parentTrans->m_transform * finalTransform;
				nextParent = parentTrans->m_parent;
			}

			return finalTransform;
		}

		Trans CalculateWorldTransform(Trans const& _localTransform) const
		{
			return CalculateWorldTransform() * _localTransform;
		}

		Trans CalculateLocalTransform(Trans const& _worldTransform) const
		{
			return CalculateWorldTransform().ToLocal(_worldTransform);
		}
	};

	// Used by sprites
	struct Transform2D
	{
		Trans2D m_transform;
		Core::EntityID m_parent;

		Transform2D(Core::EntityID _parent = Core::EntityID{})
			: m_parent{ _parent }
		{}
		explicit Transform2D(Trans2D const& _t, Core::EntityID _parent = Core::EntityID{})
			: m_transform{ _t }
			, m_parent{ _parent }
		{}

		// shorthand accessor
		Trans2D& T() { return m_transform; }
		Trans2D const& T() const { return m_transform; }

		void DetachFromParent()
		{
			m_transform = CalculateWorldTransform();
			m_parent = Core::EntityID();
		}

		void SetLocalTransformFromWorldTransform(Trans2D const& _worldTransform)
		{
			Core::EntityID nextParent = m_parent;
			Trans2D finalTransform = Trans2D();
			while (nextParent.IsValid())
			{
				Transform2D const* const parentTrans = Core::GetComponent<Transform2D>(nextParent);
				kaAssert(parentTrans != nullptr);

				finalTransform = parentTrans->m_transform * finalTransform;
				nextParent = parentTrans->m_parent;
			}

			m_transform = finalTransform.ToLocal(_worldTransform);
		}

		Trans2D CalculateWorldTransform() const
		{
			Core::EntityID nextParent = m_parent;
			Trans2D finalTransform = m_transform;
			while (nextParent.IsValid())
			{
				Transform2D const* const parentTrans = Core::GetComponent<Transform2D>(nextParent);
				kaAssert(parentTrans != nullptr);

				finalTransform = parentTrans->m_transform * finalTransform;
				nextParent = parentTrans->m_parent;
			}

			return finalTransform;
		}

		Trans2D CalculateWorldTransform(Trans2D const& _localTransform) const
		{
			return CalculateWorldTransform() * _localTransform;
		}

		Trans2D CalculateLocalTransform(Trans2D const& _worldTransform) const
		{
			return CalculateWorldTransform().ToLocal(_worldTransform);
		}
	};
}