#pragma once

#include "common.h"

namespace Game
{
	namespace Player
	{
		struct MouseLook
		{
			Vec1 m_yaw{ 0.0f };
			Vec1 m_pitch{ 0.0f };
		};
	}
}