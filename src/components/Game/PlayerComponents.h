#pragma once

#include "common.h"

#include <ecs/flags.h>

namespace Game
{
	namespace Player
	{
		struct MouseLook
		{
			float m_yaw{ 0.0f };
			float m_pitch{ 0.0f };
		};
	}
}