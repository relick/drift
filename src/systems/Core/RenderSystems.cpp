#include "RenderSystems.h"

#include "SystemOrdering.h"
#include "components.h"

#include "managers/RenderManager.h"
#include "managers/InputManager.h"

#include <sokol_app.h>

namespace Core
{
	namespace Render
	{
		void Setup()
		{
			Core::MakeSystem<Sys::RENDER_QUEUE>([](Core::Render::Light const& _light, Core::Transform3D const& _t)
			{
				switch (_light.m_type)
				{
				case Light::Type::Ambient:
				{
					AddAmbientLightToScene(_light.m_colour * _light.m_intensity);
					break;
				}
				case Light::Type::Directional:
				{
					LightSetter lightSetter = AddLightToScene();
					lightSetter.Col = fVec4(_light.m_colour, _light.m_intensity);

					fTrans const worldT = _t.CalculateWorldTransform();
					// forward() is vec/light dir.
					fVec3 const lightDir = glm::normalize(fVec3((GetCameraState().view * fVec4(worldT.forward(), 0.0f)).xyz));
					lightSetter.Pos = fVec4(-lightDir, 0.0f);

					SetDirectionalLightDir(worldT.forward());
					break;
				}
				case Light::Type::Point:
				{
					LightSetter lightSetter = AddLightToScene();
					lightSetter.Col = fVec4(_light.m_colour, _light.m_intensity);

					fTrans const worldT = _t.CalculateWorldTransform();
					fVec3 const lightPos = (GetCameraState().view * fVec4(worldT.m_origin, 1.0f)).xyz;
					lightSetter.Pos = fVec4(lightPos, 1.0f);

					// set dir and cutoff values that allow for omnidirectional lighting.
					lightSetter.Dir = fVec4(0.0f, 0.0f, 0.0f, 0.0f);
					lightSetter.Cut = fVec4(0.0f, -1.1f, 0.0f, 0.0f); // all cosine values are greater than this.

					lightSetter.Att = fVec4(_light.m_attenuation, 0.0f);
					break;
				}
				case Light::Type::Spotlight:
				{
					LightSetter lightSetter = AddLightToScene();
					lightSetter.Col = fVec4(_light.m_colour, _light.m_intensity);

					fTrans const worldT = _t.CalculateWorldTransform();
					fVec3 const lightPos = (GetCameraState().view * fVec4(worldT.m_origin, 1.0f)).xyz;
					lightSetter.Pos = fVec4(lightPos, 1.0f);

					fMat3 const& camBasis = worldT.m_basis;
					fVec3 const& front = camBasis[2]; // get z, is front vec/light dir.
					fVec3 const lightDir = glm::normalize(fVec3((GetCameraState().view * fVec4(front, 0.0f)).xyz));
					lightSetter.Dir = fVec4(-lightDir, 0.0f);

					lightSetter.Cut = fVec4(_light.m_cutoffAngle, _light.m_outerCutoffAngle, 0.0f, 0.0f);

					lightSetter.Att = fVec4(_light.m_attenuation, 0.0f);
					break;
				}
				}
			});

			Core::MakeSystem<Sys::RENDER_QUEUE>([](Core::Render::Model const& _model, Core::Transform3D const& _t)
			{
				if (_model.m_drawDefaultPass)
				{
					AddModelToScene(_model.m_modelID, _t.CalculateWorldTransform());
				}
			});

			Core::MakeSystem<Sys::RENDER_QUEUE>([](Core::Render::Skybox const& _skybox)
			{
				AddSkyboxToScene(_skybox.m_cubemapID);
			});

			Core::MakeSystem<Sys::RENDER_QUEUE>([](Core::Render::Sprite const& _sprite, Core::Transform2D const& _t)
			{
				AddSpriteToScene(_sprite.m_spriteID, _t.CalculateWorldTransform());
			});


			Core::MakeSystem<Sys::RENDER_PASS_START>([](Core::Render::FrameData const& _rfd, Core::Render::MainCamera3D const& _cam, Core::Transform3D const& _t, MT_Only&)
			{
				Core::Render::SetMainCameraParams(_rfd, _cam, _t);
			});

			Core::MakeSystem<Sys::RENDER>([](Core::Render::FrameData const& _rfd, MT_Only&)
			{
				Core::Render::Render(_rfd);
			});

			//////
			// debug camera control
			Core::MakeSystem<Sys::GAME>([](Core::FrameData const& _fd, Core::Render::MainCamera3D& _cam, Core::Transform3D& _t, Core::Render::DebugCameraControl& _debugCamera)
			{
				if (Core::Input::PressedOnce(Core::Input::Action::Debug_EnableCamera))
				{
					if (_debugCamera.m_debugCameraEnabled)
					{
						_t.m_parent = _debugCamera.m_storedParent;
						_t.T() = _debugCamera.m_storedTransform;
						_debugCamera.m_debugCameraEnabled = false;
						sapp_lock_mouse(true);
					}
					else
					{
						_debugCamera.m_storedParent = _t.m_parent;
						_debugCamera.m_storedTransform = _t.T();
						_t.DetachFromParent();
						fVec3 const forward = _t.T().forward();
						_debugCamera.m_angle.x = asin(forward.y); // pitch
						float const cosPitch = cos(_debugCamera.m_angle.x);
						if (cosPitch == 0.0f)
						{
							_debugCamera.m_angle.y = 0.0f; // if looking directly up or down then yaw can't be calc'd.. but also doesn't matter
						}
						else
						{
							// yaw1 and yaw2 should just be pi offset.
							float const yaw1 = acos(forward.x / cosPitch);
							//float const yaw2 = asin(forward.z / cosPitch);
							_debugCamera.m_angle.y = yaw1;
						}
						_debugCamera.m_debugCameraEnabled = true;
					}
				}

				if (_debugCamera.m_debugCameraEnabled)
				{
					if (Core::Input::Pressed(Core::Input::Action::Debug_AimCamera))
					{
						sapp_lock_mouse(true);
						fVec2 const mouseDelta = Core::Input::GetMouseDelta();
						float const scale = 0.0005f;
						_debugCamera.m_angle.x -= mouseDelta.y * scale;
						_debugCamera.m_angle.y += mouseDelta.x * scale;
					}
					else
					{
						sapp_lock_mouse(false);
					}

					constexpr bool d_checkAxes = false;
					if constexpr (d_checkAxes)
					{
						_t.T().m_origin = fVec3(0, 0, 0);
						if (Core::Input::Pressed(Core::Input::Action::Forward))
						{
							_t.T().m_origin += fVec3(0, 0, 1);
						}
						if (Core::Input::Pressed(Core::Input::Action::Backward))
						{
							_t.T().m_origin += fVec3(0, 0, -1);
						}
						if (Core::Input::Pressed(Core::Input::Action::Left))
						{
							_t.T().m_origin += fVec3(-1, 0, 0);
						}
						if (Core::Input::Pressed(Core::Input::Action::Right))
						{
							_t.T().m_origin += fVec3(1, 0, 0);
						}
						if (Core::Input::Pressed(Core::Input::Action::Debug_RaiseCamera))
						{
							_t.T().m_origin += fVec3(0, 1, 0);
						}
						if (Core::Input::Pressed(Core::Input::Action::Debug_LowerCamera))
						{
							_t.T().m_origin += fVec3(0, -1, 0);
						}

						if (Core::Input::PressedOnce(Core::Input::Action::Select))
						{
							fVec3 const col0 = _t.T().m_basis[0];
							_t.T().m_basis[0] = _t.T().m_basis[1];
							_t.T().m_basis[1] = _t.T().m_basis[2];
							_t.T().m_basis[2] = col0;
						}
					}
					else
					{
						// when at identity, forward == z
						float const yaw = _debugCamera.m_angle.y;
						float const pitch = _debugCamera.m_angle.x;


						fVec3 const forward = glm::normalize(fVec3{ cosf(yaw) * cosf(pitch), sinf(pitch), sinf(yaw) * cosf(pitch) });

						_t.T().m_basis = RotationFromForward(forward);

						// also re-calculate the Right and Up vector
						fVec3 const right = glm::normalize(glm::cross(forward, fVec3(0.0f, 1.0f, 0.0f)));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
						fVec3 const up = glm::normalize(glm::cross(right, forward));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.

						float const velocity = 0.8f * _fd.unscaled_dt;
						if (Core::Input::Pressed(Core::Input::Action::Forward))
						{
							_t.T().m_origin += forward * velocity;
						}
						if (Core::Input::Pressed(Core::Input::Action::Backward))
						{
							_t.T().m_origin -= forward * velocity;
						}
						if (Core::Input::Pressed(Core::Input::Action::Left))
						{
							_t.T().m_origin -= right * velocity;
						}
						if (Core::Input::Pressed(Core::Input::Action::Right))
						{
							_t.T().m_origin += right * velocity;
						}
						// should this up really be used? or world up?
						if (Core::Input::Pressed(Core::Input::Action::Debug_RaiseCamera))
						{
							_t.T().m_origin += up * velocity;
						}
						if (Core::Input::Pressed(Core::Input::Action::Debug_LowerCamera))
						{
							_t.T().m_origin -= up * velocity;
						}
					}
				}
			});
		}
	}
}