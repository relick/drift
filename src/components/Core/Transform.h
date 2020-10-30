#pragma once

namespace Core
{
	struct Transform
	{
		// origin = position, basis = transform
		fTrans m_transform;

		Transform() : m_transform{ fQuat::getIdentity() } {} // identity
		explicit Transform(fTrans const& _t) : m_transform{ _t } {}
		explicit Transform(fQuat const& _q, fVec3 const& _p) : m_transform{ _q, _p } {}

		// shorthand accessor
		fTrans& T() { return m_transform; }
	};
}