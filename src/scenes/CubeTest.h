#pragma once

#include "common.h"
#include "Scene.h"

void CubeTestSystems();

namespace Game::Scene
{
	class CubeTestScene : public Core::Scene::BaseScene
	{
	public:
		~CubeTestScene() override {}

		void Setup() override;
	};
}