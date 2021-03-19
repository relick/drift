#pragma once

#include "common.h"

namespace Core
{
	namespace Render
	{
		namespace detail
		{
			struct Camera
			{
				float m_povY{ 60.0f }; // degrees
			};
		}

		struct MainCamera3D : detail::Camera
		{
			std::optional<fVec3> m_lastPos; // used to calculate sound velocity
		};

		struct DebugCameraControl
		{
			bool m_debugCameraEnabled{ false };

			fVec2 m_angle{ 0.0f, 0.0f };
			Core::EntityID m_storedParent;
			fTrans m_storedTransform;
		};
	}
}