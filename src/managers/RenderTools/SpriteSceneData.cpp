#include "SpriteSceneData.h"

#include "managers/ResourceManager.h"

namespace Core::Render
{

//--------------------------------------------------------------------------------
void SpriteSceneData::ProcessReorder()
{
	if ( m_orderDirty )
	{
		for ( auto const& [sceneSpriteID, spriteData] : m_sceneSpriteData )
		{
			if ( spriteData.m_needsReorder )
			{
				Reorder( sceneSpriteID );
			}
		}

		m_orderDirty = false;
	}
}

//--------------------------------------------------------------------------------
void SpriteSceneData::ProcessDrawCallList()
{
	if ( m_callListDirty )
	{
		m_drawCallList.clear();

		Resource::TextureID currentTexture;
		usize firstSpriteWithTextureI = 0;
		for ( usize spriteI{ 0 }; spriteI < m_spriteBuffer.size(); ++spriteI )
		{
			if ( currentTexture != m_sceneSpriteData[ m_ordering[ spriteI ] ].m_texture )
			{
				if ( spriteI != 0 )
				{
					m_drawCallList.emplace_back(
						( int )( sizeof( SpriteBufferData ) * firstSpriteWithTextureI ),
						spriteI - firstSpriteWithTextureI,
						currentTexture
					);
				}

				currentTexture = m_sceneSpriteData[ m_ordering[ spriteI ] ].m_texture;
				firstSpriteWithTextureI = spriteI;
			}
		}

		// final draw
		m_drawCallList.emplace_back(
			( int )( sizeof( SpriteBufferData ) * firstSpriteWithTextureI ),
			m_spriteBuffer.size() - firstSpriteWithTextureI,
			currentTexture
		);

		m_callListDirty = false;
	}
}

//--------------------------------------------------------------------------------
SpriteSceneID SpriteSceneData::Add( Resource::SpriteID _sprite, Trans2D const& _screenTrans, uint32 _flags )
{
	absl::MutexLock lock( &m_mutex );

	Resource::SpriteData const& spriteData = Core::Resource::GetSprite( _sprite );
	SpriteSceneID const sceneSprite = m_sceneSpriteData.Emplace( _sprite, m_ordering.size(), spriteData.m_texture, spriteData.m_useAlpha );
	m_ordering.emplace_back( sceneSprite );
	m_spriteBuffer.emplace_back(
		Vec3( _screenTrans.m_pos, _screenTrans.m_z ),
		_screenTrans.m_scale,
		_screenTrans.m_rot.m_rads,
		spriteData.m_topLeftUV,
		spriteData.m_dimensionsUV,
		spriteData.m_dimensions,
		_flags
	);

	MarkForReorder( sceneSprite );

	m_callListDirty = true;

	kaAssert( m_ordering.size() < c_maxSprites );

	return sceneSprite;
}

//--------------------------------------------------------------------------------
void SpriteSceneData::Erase( SpriteSceneID _sprite )
{
	absl::MutexLock lock( &m_mutex );

	usize pos = FindSprite( _sprite );
	m_ordering.erase( m_ordering.begin() + pos );
	m_spriteBuffer.erase( m_spriteBuffer.begin() + pos );
	m_sceneSpriteData.Erase( _sprite );
}

//--------------------------------------------------------------------------------
void SpriteSceneData::RunRender
(
	std::function< void( std::vector<SpriteBufferData> const& ) > const& _start,
	std::function< void( DrawCall const& )> const& _draw
)
{
	absl::MutexLock lock( &m_mutex );

	if ( !m_spriteBuffer.empty() )
	{
		ProcessReorder();
		ProcessDrawCallList();

		_start( m_spriteBuffer );

		for ( SpriteSceneData::DrawCall const& call : m_drawCallList )
		{
			_draw( call );
		}

	}
}

//--------------------------------------------------------------------------------
usize SpriteSceneData::FindSprite( SpriteSceneID _sprite ) const
{
	return m_sceneSpriteData[ _sprite ].m_pos;
}

//--------------------------------------------------------------------------------
void SpriteSceneData::MarkForReorder( SpriteSceneID _sprite )
{
	m_sceneSpriteData[ _sprite ].m_needsReorder = true;
	m_orderDirty = true;
}

//--------------------------------------------------------------------------------
void SpriteSceneData::Reorder( SpriteSceneID _sprite )
{
	usize curPos = FindSprite( _sprite );
	m_sceneSpriteData[ _sprite ].m_needsReorder = false;

	auto isLess = [this]( usize _a, usize _b )
	{
		// Explanation:
		// a and b both alpha - sort by Z, then sort by texture (lessZ || lessTex)
		// a alpha, b not - a must be above b (false)
		// b alpha, a not - b must be above a (true)
		// a and b not alpha - sort by texture (lessTex)
		/*if ( bSpriteData.m_useAlpha )
		{
			if ( aSpriteData.m_useAlpha )
			{
				return _a.m_transform.m_z < _b.m_transform.m_z || aSpriteData.m_texture < bSpriteData.m_texture;
			}
			else
			{
				return true;
			}
		}
		else if ( aSpriteData.m_useAlpha )
		{
			return false;
		}
		else
		{
			return aSpriteData.m_texture < bSpriteData.m_texture;
		}*/
		SpriteData const& a = m_sceneSpriteData[ m_ordering[ _a ] ];
		SpriteData const& b = m_sceneSpriteData[ m_ordering[ _b ] ];
		bool const lessTexture = a.m_texture < b.m_texture;
		bool const lessZ = m_spriteBuffer[ _a ].m_position.z < m_spriteBuffer[ _b ].m_position.z;
		bool const equalAlpha = a.m_useAlpha == b.m_useAlpha;
		return ( !equalAlpha && b.m_useAlpha ) || ( equalAlpha && ( ( b.m_useAlpha && lessZ ) || lessTexture ) );
	};

	while ( curPos > 0u )
	{
		usize prevPos = curPos - 1u;
		if ( isLess( prevPos, curPos ) )
		{
			break;
		}
		m_sceneSpriteData[ m_ordering[ prevPos ] ].m_pos = curPos;
		m_sceneSpriteData[ m_ordering[ curPos ] ].m_pos = prevPos;
		std::swap( m_ordering[ prevPos ], m_ordering[ curPos ] );
		std::swap( m_spriteBuffer[ prevPos ], m_spriteBuffer[ curPos ] );
		curPos--;
		m_callListDirty = true;
	}

	while ( curPos < m_ordering.size() - 1u )
	{
		usize nextPos = curPos + 1u;
		if ( isLess( curPos, nextPos ) )
		{
			break;
		}
		m_sceneSpriteData[ m_ordering[ curPos ] ].m_pos = nextPos;
		m_sceneSpriteData[ m_ordering[ nextPos ] ].m_pos = curPos;
		std::swap( m_ordering[ curPos ], m_ordering[ nextPos ] );
		std::swap( m_spriteBuffer[ curPos ], m_spriteBuffer[ nextPos ] );
		curPos++;
		m_callListDirty = true;
	}

}

//--------------------------------------------------------------------------------
void SpriteSceneData::Update
(
	SpriteSceneID _sprite,
	Trans2D const& _screenTrans,
	uint32 _flags
)
{
	SpriteBufferData& sbData = m_spriteBuffer[ FindSprite( _sprite ) ];
	Vec1 const prevZ = sbData.m_position.z;
	sbData.m_position = Vec3( _screenTrans.m_pos, _screenTrans.m_z );
	sbData.m_scale = _screenTrans.m_scale;
	sbData.m_rotation = _screenTrans.m_rot.m_rads;
	sbData.m_flags = _flags;

	if ( prevZ != _screenTrans.m_z )
	{
		MarkForReorder( _sprite );
	}
}

}