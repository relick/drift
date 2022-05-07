#pragma once

#include "common.h"

namespace Core::Scene
{
	// Interface class for setting up scenes and switching between them
	class BaseScene
	{
	public:
		virtual ~BaseScene() {}

		virtual void Setup() = 0;
		virtual std::string GetPreloadFile() const { return "assets/preload.res"; }
	};
}