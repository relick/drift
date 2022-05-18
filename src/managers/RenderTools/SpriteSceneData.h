#pragma once

#include "common.h"
#include "common/StaticVector.h"
#include "common/Mutex.h"

#include "managers/ResourceIDs.h"
#include "managers/RenderIDs.h"

#include <vector>
#include <functional>

namespace Core::Render
{

inline constexpr usize c_maxSprites = 262'144;

//--------------------------------------------------------------------------------
struct SpriteBufferData
{
	Vec3 m_position;
	Vec2 m_scale;
	Vec1 m_rotation;
	Vec2 m_topLeftUV;
	Vec2 m_UVDims;
	Vec2 m_spriteDims;
	uint32 m_flags;

	SpriteBufferData( Vec3 _position, Vec2 _scale, Vec1 _rotation, Vec2 _topLeftUV, Vec2 _uvDims, Vec2 _spriteDims, uint32 _flags = 0u )
		: m_position( _position )
		, m_scale( _scale )
		, m_rotation( _rotation )
		, m_topLeftUV( _topLeftUV )
		, m_UVDims( _uvDims )
		, m_spriteDims( _spriteDims )
		, m_flags( _flags )
	{}
};

//--------------------------------------------------------------------------------
class SpriteSceneData
{
public:
	struct DrawCall
	{
		int vertexOffset;
		usize count;
		Resource::TextureID texture;
	};

private:
	struct SpriteData
	{
		Resource::SpriteID m_sprite;
		usize m_pos{ ~0u };
		Resource::TextureID m_texture;
		bool m_useAlpha{ false };
		bool m_needsReorder{ false };
	};

	StaticVector<SpriteSceneID, SpriteData> m_sceneSpriteData;
	std::vector<SpriteSceneID> m_ordering;
	std::vector<SpriteBufferData> m_spriteBuffer;
	std::vector<DrawCall> m_drawCallList;
	bool m_callListDirty{ false };
	bool m_orderDirty{ false };
	absl::Mutex m_mutex;

	void ProcessReorder();
	void ProcessDrawCallList();

public:
	// The following operations are thread-safe amongst themselves, but not with the other operations
	SpriteSceneID Add( Resource::SpriteID _sprite, Trans2D const& _screenTrans, uint32 _flags );
	void Erase( SpriteSceneID _sprite );
	void RunRender( std::function< void( std::vector<SpriteBufferData> const& ) > const& _start, std::function< void( DrawCall const& ) > const& _draw );

	// The following operations are thread-safe amongst themselves, but not with the other operations
	usize FindSprite( SpriteSceneID _sprite ) const;
	void MarkForReorder( SpriteSceneID _sprite );
	void Reorder( SpriteSceneID _sprite );
	void Update
	(
		SpriteSceneID _sprite,
		Trans2D const& _screenTrans,
		uint32 _flags
	);
};

}