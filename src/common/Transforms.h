#pragma once

// Defines:
// Transform
// Rotation2D
// Transform2D

#include <LinearMath/btTransform.h>

template< ScalarType T >
struct TransformBase
{
	using Vec3 = Vector3Base< T >;
	using Mat3 = Matrix3Base< T >;
	using Mat4 = Matrix4Base< T >;
	using Quat = QuaternionBase< T >;

	Mat3 m_basis{ 1 };
	Vec3 m_origin{};

	TransformBase< T > operator*( TransformBase< T > const& _t ) const
	{
		return TransformBase< T >{ m_basis * _t.m_basis,
			( *this )( _t.m_origin ) };
	}

	Vec3 operator()( Vec3 const& _v ) const
	{
		return ( _v * m_basis ) + m_origin;
	}

	TransformBase< T >& operator*=( TransformBase< T > const& _t )
	{
		m_origin += m_basis * _t.m_origin;
		m_basis *= _t.m_basis;
		return *this;
	}

	Vec3 Right() const
	{
		return Normalise( m_basis[ 0 ] );
	}
	Vec3 Up() const
	{
		return Normalise( m_basis[ 1 ] );
	}
	Vec3 Forward() const
	{
		return Normalise( m_basis[ 2 ] );
	}

	Mat4 GetRenderMatrix() const
	{
		// initialise directly
		Vec3 const& col1 = m_basis[ 0 ];
		Vec3 const& col2 = m_basis[ 1 ];
		Vec3 const& col3 = m_basis[ 2 ];

		Vec3 const& posi = m_origin;

		return {
			col1[ 0 ], col1[ 1 ], col1[ 2 ], T( 0 ),
			col2[ 0 ], col2[ 1 ], col2[ 2 ], T( 0 ),
			col3[ 0 ], col3[ 1 ], col3[ 2 ], T( 0 ),
			posi[ 0 ], posi[ 1 ], posi[ 2 ], T( 1 ),
		};
	}

	// Bullet transform only supported by float Transforms.
	btTransform GetBulletTransform() const requires std::same_as< T, float >
	{
		// initialise directly
		return btTransform(
			btMatrix3x3(
				m_basis[ 0 ][ 0 ], m_basis[ 1 ][ 0 ], m_basis[ 2 ][ 0 ],
				m_basis[ 0 ][ 1 ], m_basis[ 1 ][ 1 ], m_basis[ 2 ][ 1 ],
				m_basis[ 0 ][ 2 ], m_basis[ 1 ][ 2 ], m_basis[ 2 ][ 2 ]
			),
			btVector3( m_origin[ 0 ], m_origin[ 1 ], m_origin[ 2 ] )
		);
	}

	TransformBase< T > ToLocal( TransformBase< T > const& _a ) const
	{
		Mat3 const invB = glm::inverse( m_basis );
		return { _a.m_basis * invB, ( _a.m_origin - m_origin ) * invB };
	}

	TransformBase() = default;
	TransformBase( Mat3 const& _b, Vec3 const& _o = Vec3( 0 ) )
		: m_basis( _b )
		, m_origin( _o )
	{}
	TransformBase( Quat const& _q, Vec3 const& _o = Vec3( 0 ) )
		: m_basis( _q )
		, m_origin( _o )
	{}

	// Bullet transform only supported by float Transforms.
	explicit TransformBase( btTransform const& _t ) requires std::same_as< T, float >
		: m_basis{
			_t.getBasis().getRow( 0 ).x(), _t.getBasis().getRow( 1 ).x(), _t.getBasis().getRow( 2 ).x(),
			_t.getBasis().getRow( 0 ).y(), _t.getBasis().getRow( 1 ).y(), _t.getBasis().getRow( 2 ).y(),
			_t.getBasis().getRow( 0 ).z(), _t.getBasis().getRow( 1 ).z(), _t.getBasis().getRow( 2 ).z(),
		}
		, m_origin{ _t.getOrigin().x(), _t.getOrigin().y(), _t.getOrigin().z() }
	{}
};

template< ScalarType T >
struct Rotation2DBase
{
	using Vec2 = Vector2Base< T >;
	using Mat2 = Matrix2Base< T >;

	T m_rads; // radians

	Mat2 Matrix() const
	{
		T cosRot = gcem::cos( m_rads );
		T sinRot = gcem::sin( m_rads );
		return glm::mat2( cosRot, -sinRot, sinRot, cosRot );
	}

	Rotation2DBase< T > Inverse() const
	{
		return { -m_rads };
	}

	Rotation2DBase( T _rads )
		: m_rads( _rads )
	{}

	Rotation2DBase( Mat2 const& _mat )
		: m_rads( gcem::acos( _mat[ 0 ][ 0 ] ) )
	{}

	Vec2 operator*( Vec2 const& _o ) const
	{
		return Matrix() * _o;
	}

	Rotation2DBase< T > operator*( Rotation2DBase< T > const& _o ) const
	{
		return { m_rads + _o.m_rads };
	}

	Rotation2DBase< T >& operator*=( Rotation2DBase< T > const& _o )
	{
		m_rads += _o.m_rads;
		return *this;
	}
};

template< ScalarType T >
inline typename Rotation2DBase< T >::Vec2 operator*( typename Rotation2DBase< T >::Vec2 const& _o, Rotation2DBase< T > const& _r )
{
	return _o * _r.Matrix();
}

template< ScalarType T >
struct Transform2DBase
{
	using Vec2 = Vector2Base< T >;
	using Rot2D = Rotation2DBase< T >;

	Rot2D m_rot;
	Vec2 m_pos;
	Vec2 m_scale;
	T m_z; // -1 to 1


	Transform2DBase< T > operator*( Transform2DBase< T > const& _t ) const
	{
		return Transform2DBase< T >{ m_rot* _t.m_rot,
			( *this )( _t.m_pos ), m_scale* _t.m_scale, gcem::min( m_z, _t.m_z ) };
	}

	Vec2 operator()( Vec2 const& _v ) const
	{
		return ( _v * m_rot.Matrix() ) + m_pos;
	}

	Transform2DBase< T >& operator*=( Transform2DBase< T > const& _t )
	{
		m_pos += m_rot * _t.m_pos;
		m_rot *= _t.m_rot;
		m_scale *= _t.m_scale;
		return *this;
	}

	Vec2 Right() const
	{
		return Normalise( m_rot.Matrix()[ 0 ] );
	}
	Vec2 Up() const
	{
		return Normalise( m_rot.Matrix()[ 1 ] );
	}

	Transform2DBase< T > ToLocal( Transform2DBase< T > const& _a ) const
	{
		Rot2D const invRot = m_rot.Inverse();
		[[maybe_unused]] Vec2 const invScale = T( 1 ) / m_scale;
		// todo, not sure this is right
		kaError( "Transform2DBase::ToLocal fix me" );
		return { _a.m_rot * invRot, ( _a.m_pos - m_pos ) * invRot, _a.m_scale, m_z };
	}

	Transform2DBase(Rot2D const& _rot = T(0), Vec2 const& _pos = Vec2(0), Vec2 const& _scale = Vec2(1), T _z = 0)
		: m_rot(_rot)
		, m_pos(_pos)
		, m_scale(_scale)
		, m_z(_z)
	{}
};

template< ScalarType T >
inline Matrix3Base< T > RotationFromForward( Vector3Base< T > const& _f )
{
	Vector3Base< T > const normF = Normalise( _f );
	Vector3Base< T > const right = Normalise( glm::cross( normF, Vector3Base< T >( 0, 1, 0 ) ) );
	Vector3Base< T > const up = Normalise( glm::cross( right, normF ) );
	return Matrix3Base< T >{
		right.x,	right.y,	right.z,	// x-right
		up.x,		up.y,		up.z,		// y-up
		normF.x,	normF.y,	normF.z,	// z-forward
	};
}

inline Vec3 ConvertFrombtVector3(btVector3 const& _btVec3)
{
	return { _btVec3.x(), _btVec3.y(), _btVec3.z() };
}
inline btVector3 ConvertTobtVector3(Vec3 const& _vec3)
{
	return { _vec3.x, _vec3.y, _vec3.z };
}

// Typedefs
using Trans = TransformBase< float >;
using Rot2D = Rotation2DBase< float >;
using Trans2D = Transform2DBase< float >;

using dTrans = TransformBase< double >;
using dRot2D = Rotation2DBase< double >;
using dTrans2D = Transform2DBase< double >;

using iTrans = TransformBase< int32 >;
using iRot2D = Rotation2DBase< int32 >;
using iTrans2D = Transform2DBase< int32 >;