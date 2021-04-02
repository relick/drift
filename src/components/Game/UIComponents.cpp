#include "UIComponents.h"

#include "managers/ResourceManager.h"

namespace Core
{
	template<>
	void AddComponent
	(
		EntityID const _entity,
		Game::UI::LoadingScreen const& _component
	)
	{
		Game::UI::LoadingScreen newComponent = _component;
		if (newComponent.m_fullScreenSprite.IsNull())
		{
			bool const success = Core::Resource::LoadSprite("assets/sprites/loading/loading.spr", newComponent.m_fullScreenSprite);
			kaAssert(success);
		}

		ECS::AddComponent(_entity, newComponent);
	}
}