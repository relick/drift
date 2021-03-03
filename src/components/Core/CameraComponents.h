#pragma once

#include "common.h"

#include <ecs/flags.h>

namespace Core
{
	namespace Render
	{
		// Attach to the entity with the pass tag you want
		struct Camera
		{
			float m_povY{ 60.0f }; // degrees

			std::optional<fVec3> m_lastPos; // used to calculate sound velocity
		};

		struct DebugCameraControl
		{
			fVec2 m_angle{ 0.0f, 0.0f };
			Core::EntityID m_storedParent;
			fTrans m_storedTransform;
		};
	}
}