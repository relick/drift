#include "EntityManager.h"

#include "components.h"

#include "CubeTest.h"

namespace Core
{
	EntityID::CoreType nextID = 0;

	EntityID CreateEntity()
	{
		return EntityID(nextID++);
	}

	void DestroyEntity
	(
		EntityID _entity
	)
	{
		kaError("DestroyEntity nyi");
	}

	namespace Scene
	{
		void AddDemoScene()
		{
			Core::EntityID const camera = Core::CreateEntity();
			Core::EntityID const character = Core::CreateEntity();
			fTrans const characterTrans{ fQuatIdentity(), fVec3(2.0f, 0.0f, 0.0f) };
			Core::AddComponent(character, Core::Transform3D(characterTrans));
			{
				Core::Physics::CharacterControllerDesc ccDesc{};
				ccDesc.m_viewObject = camera;
				ccDesc.m_halfHeight = 0.9f;
				ccDesc.m_radius = 0.5f;
				ccDesc.m_mass = 80.0f;
				ccDesc.m_startTransform = characterTrans;
				ccDesc.m_physicsWorld = Core::Physics::GetPrimaryWorldEntity();

				Core::AddComponent(character, ccDesc);
			}

			Core::AddComponent(camera, Core::Transform3D(fQuatIdentity(), fVec3(0.0f, 0.8f, 0.0f), character));
			Core::AddComponent(camera, Core::Render::MainCamera3D());
			Core::AddComponent(camera, Core::Render::DebugCameraControl());
			Core::AddComponent(camera, Game::Player::MouseLook());

			CubeTestEntities();
		}
	}
}