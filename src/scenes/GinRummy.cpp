#include "GinRummy.h"

#include "managers/EntityManager.h"
#include "components.h"
#include "systems.h"


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
}
}