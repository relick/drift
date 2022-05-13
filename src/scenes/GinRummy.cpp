#include "GinRummy.h"

#include "managers/EntityManager.h"
#include "managers/ResourceManager.h"
#include "components.h"
#include "systems.h"

#include <random>

namespace Game::Scene
{
void GinRummy::Setup()
{
	{
		Core::EntityID const gameData = Core::CreateEntity();
		Core::AddComponent( gameData, Game::GinRummy::GameData() );
		Core::AddComponent( gameData, Game::GinRummy::GameRender() );
		Core::AddComponent( gameData, Game::GinRummy::PlayerInteraction() );
	}

	{
		Core::EntityID const mat = Core::CreateEntity();
		Trans2D fullScreen; // default will cover the whole screen.
		fullScreen.m_z = -0.9f; // Put behind all cards
		Core::AddComponent( mat, Core::Render::SpriteDesc{ "assets/encrypted/sprites/ginrummy/mat.spr", fullScreen } );
	}

	/*std::array<Core::Resource::SpriteID, 53> cardieSprites;

	static constexpr std::array<char const*, 4> suitInitials{ "D", "C", "H", "S", };

	for ( usize suitI = 0; suitI < 4; ++suitI )
	{
		for ( usize faceI = 0; faceI < 13; ++faceI )
		{
			bool const success = Core::Resource::LoadSprite(
				std::format( "assets/encrypted/sprites/ginrummy/cardfront_sprites/{:s}{:d}.spr", suitInitials[ suitI ], faceI ),
				cardieSprites[ suitI * 13 + faceI ]
			);
			kaAssert( success );
		}
	}

	{
		bool const success = Core::Resource::LoadSprite( "assets/encrypted/sprites/ginrummy/cardback.spr", cardieSprites[ 52 ] );
		kaAssert( success );
	}

	usize const N = 262'140;

	std::random_device rd;
	std::mt19937 rng{ rd() };
	std::uniform_real_distribution dist( -1.0f, 1.0f );

	for ( usize i = 0; i < N; ++i )
	{
		Game::GinRummy::Cardie cardie;
		cardie.m_dir = Normalise( Vec2{ dist( rng ), dist( rng ) } );

		Core::Transform2D trans;
		trans.m_transform.m_z = ( Vec1 )i / N;

		Core::Render::SpriteDesc sprite;
		sprite.m_spriteInit = cardieSprites[ i % 52 ];
		sprite.m_initTrans = trans.m_transform;

		Core::EntityID const cardieEntity = Core::CreateEntity();
		Core::AddComponent( cardieEntity, cardie );
		Core::AddComponent( cardieEntity, trans );
		Core::AddComponent( cardieEntity, sprite );
	}*/
}
}