#include "PlayerSystems.h"

#include "components.h"
#include "systems/Core/SystemOrdering.h"
#include <ecs/ecs.h>

#include "managers/InputManager.h"

#include <sokol_app.h>

namespace Game
{
	namespace Player
	{
		void MouseLookSystem(MouseLook& _p, Core::Transform& _t)
		{
#if DEBUG_TOOLS
			if (Core::Input::PressedOnce(Core::Input::Action::Debug_EnableCamera))
			{
				_p.m_debugCameraEnabled = !_p.m_debugCameraEnabled;
				sapp_lock_mouse(!_p.m_debugCameraEnabled);
			}

			if (_p.m_debugCameraEnabled)
			{
				return;
			}
#endif

			fVec2 const mouseDelta = Core::Input::GetMouseDelta();
			_p.m_pitch -= mouseDelta.y * 0.05f;
			if (_p.m_pitch < -85.0f)
			{
				_p.m_pitch = -85.0f;
			}
			else if (_p.m_pitch > 85.0f)
			{
				_p.m_pitch = 85.0f;
			}
			_p.m_yaw += mouseDelta.x * 0.05f;

			fVec3 forward;
			forward.x = gcem::cos(glm::radians(_p.m_yaw)) * gcem::cos(glm::radians(_p.m_pitch));
			forward.y = gcem::sin(glm::radians(_p.m_pitch));
			forward.z = gcem::sin(glm::radians(_p.m_yaw)) * gcem::cos(glm::radians(_p.m_pitch));
			forward = glm::normalize(forward);

			_t.T().m_basis = RotationFromForward(forward);
		}

		void Setup()
		{
			Core::MakeSystem<Sys::GAME>(MouseLookSystem);
		}
	}
}