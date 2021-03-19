#pragma once

// fTrans
// fRot2D
// fTrans2D

#include <LinearMath/btTransform.h>
struct fTrans
{
	fMat3 m_basis{ 1.0f };
	fVec3 m_origin{};

	fTrans operator*(fTrans const& _t) const
	{
		return fTrans{ m_basis * _t.m_basis,
			(*this)(_t.m_origin) };
	}

	fVec3 operator()(fVec3 const& _v) const
	{
		return (_v * m_basis) + m_origin;
	}

	fTrans& operator*=(fTrans const& _t)
	{
		m_origin += m_basis * _t.m_origin;
		m_basis *= _t.m_basis;
		return *this;
	}

	fVec3 right() const
	{
		return glm::normalize(m_basis[0]);
	}
	fVec3 up() const
	{
		return glm::normalize(m_basis[1]);
	}
	fVec3 forward() const
	{
		return glm::normalize(m_basis[2]);
	}

	fMat4 GetRenderMatrix() const
	{
		// initialise directly
		fVec3 const& col1 = m_basis[0];
		fVec3 const& col2 = m_basis[1];
		fVec3 const& col3 = m_basis[2];

		fVec3 const& posi = m_origin;

		return {
			col1[0], col1[1], col1[2], 0.0f,
			col2[0], col2[1], col2[2], 0.0f,
			col3[0], col3[1], col3[2], 0.0f,
			posi[0], posi[1], posi[2], 1.0f,
		};
	}

	btTransform GetBulletTransform() const
	{
		// initialise directly
		return btTransform(
			btMatrix3x3(
				m_basis[0][0], m_basis[1][0], m_basis[2][0],
				m_basis[0][1], m_basis[1][1], m_basis[2][1],
				m_basis[0][2], m_basis[1][2], m_basis[2][2]),
			btVector3(m_origin[0], m_origin[1], m_origin[2])
		);
	}

	fTrans ToLocal(fTrans const& _a) const
	{
		fMat3 const invB = glm::inverse(m_basis);
		return { _a.m_basis * invB, (_a.m_origin - m_origin) * invB };
	}

	fTrans() = default;
	fTrans(fMat3 const& _b, fVec3 const& _o = fVec3(0.0f))
		: m_basis(_b)
		, m_origin(_o)
	{}
	fTrans(fQuat const& _q, fVec3 const& _o = fVec3(0.0f))
		: m_basis(_q)
		, m_origin(_o)
	{}

	explicit fTrans(btTransform const& _t)
		: m_basis{
		_t.getBasis().getRow(0).x(), _t.getBasis().getRow(1).x(), _t.getBasis().getRow(2).x(),
		_t.getBasis().getRow(0).y(), _t.getBasis().getRow(1).y(), _t.getBasis().getRow(2).y(),
		_t.getBasis().getRow(0).z(), _t.getBasis().getRow(1).z(), _t.getBasis().getRow(2).z(),
	}
	, m_origin{
	_t.getOrigin().x(), _t.getOrigin().y(), _t.getOrigin().z()
	}
	{}
};

struct fRot2D
{
	float m_rads; // radians

	fMat2 Matrix() const
	{
		float cosRot = gcem::cos(m_rads);
		float sinRot = gcem::sin(m_rads);
		return glm::mat2(cosRot, -sinRot, sinRot, cosRot);
	}

	fRot2D Inverse() const
	{
		return { -m_rads };
	}

	fRot2D(float _rads)
		: m_rads(_rads)
	{}

	fRot2D(fMat2 const& _mat)
		: m_rads(gcem::acos(_mat[0][0]))
	{}

	fVec2 operator*(fVec2 const& _o) const
	{
		return Matrix() * _o;
	}

	fRot2D operator*(fRot2D const& _o) const
	{
		return { m_rads + _o.m_rads };
	}

	fRot2D& operator*=(fRot2D const& _o)
	{
		m_rads += _o.m_rads;
		return *this;
	}
};

inline fVec2 operator*(fVec2 const& _o, fRot2D const& _r)
{
	return _o * _r.Matrix();
}

struct fTrans2D
{
	fRot2D m_rot;
	fVec2 m_pos;
	fVec2 m_scale;
	float m_z; // -1 to 1


	fTrans2D operator*(fTrans2D const& _t) const
	{
		return fTrans2D{ m_rot * _t.m_rot,
			(*this)(_t.m_pos), m_scale * _t.m_scale, gcem::min(m_z, _t.m_z) };
	}

	fVec2 operator()(fVec2 const& _v) const
	{
		return (_v * m_rot.Matrix()) + m_pos;
	}

	fTrans2D& operator*=(fTrans2D const& _t)
	{
		m_pos += m_rot * _t.m_pos;
		m_rot *= _t.m_rot;
		m_scale *= _t.m_scale;
		return *this;
	}

	fVec2 right() const
	{
		return glm::normalize(m_rot.Matrix()[0]);
	}
	fVec2 up() const
	{
		return glm::normalize(m_rot.Matrix()[1]);
	}

	fTrans2D ToLocal(fTrans2D const& _a) const
	{
		fRot2D const invRot = m_rot.Inverse();
		fVec2 const invScale = 1.0f / m_scale;
		// todo, not sure this is right
		kaError("fTrans2D::ToLocal fix me");
		return { _a.m_rot * invRot, (_a.m_pos - m_pos) * invRot, _a.m_scale, m_z };
	}

	fTrans2D(fRot2D const& _rot = 0.0f, fVec2 const& _pos = fVec2(0.0f), fVec2 const& _scale = fVec2(1.0f), float _z = 0.0f)
		: m_rot(_rot)
		, m_pos(_pos)
		, m_scale(_scale)
		, m_z(_z)
	{}
};

inline fMat3 RotationFromForward(fVec3 const& _f)
{
	fVec3 const normF = glm::normalize(_f);
	fVec3 const right = glm::normalize(glm::cross(normF, fVec3(0.0f, 1.0f, 0.0f)));
	fVec3 const up = glm::normalize(glm::cross(right, normF));
	return fMat3{
		right.x,	right.y,	right.z,	// x-right
		up.x,		up.y,		up.z,		// y-up
		normF.x,	normF.y,	normF.z,	// z-forward
	};
}

inline fVec3 ConvertFrombtVector3(btVector3 const& _btVec3)
{
	return { _btVec3.x(), _btVec3.y(), _btVec3.z() };
}
inline btVector3 ConvertTobtVector3(fVec3 const& _fVec3)
{
	return { _fVec3.x, _fVec3.y, _fVec3.z };
}