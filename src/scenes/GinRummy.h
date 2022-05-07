#pragma once

#include "common.h"
#include "Scene.h"

namespace Game::Scene
{
class GinRummy : public Core::Scene::BaseScene
{
public:
	~GinRummy() override {}

	void Setup() override;
	std::string GetPreloadFile() const override { return "assets/encrypted/ginrummy.res"; }
};
}