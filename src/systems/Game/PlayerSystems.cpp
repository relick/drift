#include "PlayerSystems.h"

#include "components.h"
#include <ecs/ecs.h>

#include "managers/InputManager.h"

namespace Game
{
	namespace Player
	{
		static void MouseLookSystem(
#if DEBUG_TOOLS
			Core::EntityID::CoreType _entity,
#endif
			MouseLook& _p, Core::Transform3D& _t)
		{
#if DEBUG_TOOLS
			auto const debugCameraControl = Core::GetComponent<Core::Render::DebugCameraControl>(_entity);
			if (debugCameraControl != nullptr && debugCameraControl->m_debugCameraEnabled)
			{
				return;
			}
#endif
			Core::Input::LockMouse( true );

			Vec2 const mouseDelta = Core::Input::GetMouseDelta();
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

			Vec3 forward;
			forward.x = gcem::cos(glm::radians(_p.m_yaw)) * gcem::cos(glm::radians(_p.m_pitch));
			forward.y = gcem::sin(glm::radians(_p.m_pitch));
			forward.z = gcem::sin(glm::radians(_p.m_yaw)) * gcem::cos(glm::radians(_p.m_pitch));
			forward = Normalise(forward);

			_t.T().m_basis = RotationFromForward(forward);
		}

		void Setup()
		{
			Core::MakeSystem<Sys::GAME>(MouseLookSystem);
		}
	}
}