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
	Core::EntityID const gameData = Core::CreateEntity();
	Core::AddComponent( gameData, Game::GinRummy::GameData() );
	Core::AddComponent( gameData, Game::GinRummy::GameRender() );
	Core::AddComponent( gameData, Game::GinRummy::PlayerInteraction() );

	Core::EntityID const mat = Core::CreateEntity();
	Core::AddComponent( mat, Game::GinRummy::Mat() );

	std::array<Core::Resource::SpriteID, 53> cardieSprites;

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

	usize const N = 10'000;

	std::random_device rd;
	std::mt19937 rng{ rd() };
	std::uniform_real_distribution dist( -1.0f, 1.0f );

	for(usize i = 0; i < N; ++i )
	{
		Game::GinRummy::Cardie cardie;
		cardie.m_sprite = cardieSprites[ i % 53 ];
		cardie.m_dir = Normalise( Vec2{ dist( rng ), dist( rng ) } );
		cardie.m_trans.m_z = ( Vec1 )i / N;

		Core::EntityID const cardieEntity = Core::CreateEntity();
		Core::AddComponent( cardieEntity, cardie );
	}
}
}