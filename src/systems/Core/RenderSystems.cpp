#include "RenderSystems.h"

#include "components.h"

#include "managers/RenderManager.h"
#include "managers/InputManager.h"

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
					using enum Light::Type;

				case Ambient:
				{
					AddAmbientLightThisFrame(_light.m_colour * _light.m_intensity);
					break;
				}
				case Directional:
				{
					LightSetter lightSetter = AddLightThisFrame();
					lightSetter.Col = Vec4(_light.m_colour, _light.m_intensity);

					Trans const worldT = _t.CalculateWorldTransform();
					// forward() is vec/light dir.
					Vec3 const lightDir = Normalise(Vec3((GetCameraState().view * Vec4(worldT.Forward(), 0.0f)).xyz));
					lightSetter.Pos = Vec4(-lightDir, 0.0f);

					SetDirectionalLightDir(worldT.Forward());
					break;
				}
				case Point:
				{
					LightSetter lightSetter = AddLightThisFrame();
					lightSetter.Col = Vec4(_light.m_colour, _light.m_intensity);

					Trans const worldT = _t.CalculateWorldTransform();
					Vec3 const lightPos = (GetCameraState().view * Vec4(worldT.m_origin, 1.0f)).xyz;
					lightSetter.Pos = Vec4(lightPos, 1.0f);

					// set dir and cutoff values that allow for omnidirectional lighting.
					lightSetter.Dir = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
					lightSetter.Cut = Vec4(0.0f, -1.1f, 0.0f, 0.0f); // all cosine values are greater than this.

					lightSetter.Att = Vec4(_light.m_attenuation, 0.0f);
					break;
				}
				case Spotlight:
				{
					LightSetter lightSetter = AddLightThisFrame();
					lightSetter.Col = Vec4(_light.m_colour, _light.m_intensity);

					Trans const worldT = _t.CalculateWorldTransform();
					Vec3 const lightPos = (GetCameraState().view * Vec4(worldT.m_origin, 1.0f)).xyz;
					lightSetter.Pos = Vec4(lightPos, 1.0f);

					Mat3 const& camBasis = worldT.m_basis;
					Vec3 const& front = camBasis[2]; // get z, is front vec/light dir.
					Vec3 const lightDir = Normalise(Vec3((GetCameraState().view * Vec4(front, 0.0f)).xyz));
					lightSetter.Dir = Vec4(-lightDir, 0.0f);

					lightSetter.Cut = Vec4(_light.m_cutoffAngle, _light.m_outerCutoffAngle, 0.0f, 0.0f);

					lightSetter.Att = Vec4(_light.m_attenuation, 0.0f);
					break;
				}
				}
			});

			Core::MakeSystem<Sys::RENDER_QUEUE>([](Core::Render::Model const& _model, Core::Transform3D const& _t)
			{
				if (_model.m_drawDefaultPass)
				{
					DrawModelThisFrame(_model.m_modelID, _t.CalculateWorldTransform());
				}
			});

			Core::MakeSystem<Sys::RENDER_QUEUE>([](Core::Render::Skybox const& _skybox)
			{
				DrawSkyboxThisFrame(_skybox.m_cubemapID);
			});

			Core::MakeSystem<Sys::RENDER_QUEUE>([](Core::Render::Sprite const& _sprite, Core::Transform2D const& _t)
			{
				UpdateSpriteInScene( _sprite.m_spriteSceneID, _t.CalculateWorldTransform(), _sprite.m_spriteFlags );
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
						Core::Input::LockMouse( true );
					}
					else
					{
						_debugCamera.m_storedParent = _t.m_parent;
						_debugCamera.m_storedTransform = _t.T();
						_t.DetachFromParent();
						Vec3 const forward = _t.T().Forward();
						_debugCamera.m_angle.x = asin(forward.y); // pitch
						Vec1 const cosPitch = cos(_debugCamera.m_angle.x);
						if (cosPitch == 0.0f)
						{
							_debugCamera.m_angle.y = 0.0f; // if looking directly up or down then yaw can't be calc'd.. but also doesn't matter
						}
						else
						{
							// yaw1 and yaw2 should just be pi offset.
							Vec1 const yaw1 = acos(forward.x / cosPitch);
							//Vec1 const yaw2 = asin(forward.z / cosPitch);
							_debugCamera.m_angle.y = yaw1;
						}
						_debugCamera.m_debugCameraEnabled = true;
					}
				}

				if (_debugCamera.m_debugCameraEnabled)
				{
					if (Core::Input::Pressed(Core::Input::Action::Debug_AimCamera))
					{
						Core::Input::LockMouse( true );
						Vec2 const mouseDelta = Core::Input::GetMouseDelta();
						Vec1 const scale = 0.0005f;
						_debugCamera.m_angle.x -= mouseDelta.y * scale;
						_debugCamera.m_angle.y += mouseDelta.x * scale;
					}
					else
					{
						Core::Input::LockMouse( false );
					}

					constexpr bool d_checkAxes = false;
					if constexpr (d_checkAxes)
					{
						_t.T().m_origin = Vec3(0, 0, 0);
						if (Core::Input::Pressed(Core::Input::Action::Forward))
						{
							_t.T().m_origin += Vec3(0, 0, 1);
						}
						if (Core::Input::Pressed(Core::Input::Action::Backward))
						{
							_t.T().m_origin += Vec3(0, 0, -1);
						}
						if (Core::Input::Pressed(Core::Input::Action::Left))
						{
							_t.T().m_origin += Vec3(-1, 0, 0);
						}
						if (Core::Input::Pressed(Core::Input::Action::Right))
						{
							_t.T().m_origin += Vec3(1, 0, 0);
						}
						if (Core::Input::Pressed(Core::Input::Action::Debug_RaiseCamera))
						{
							_t.T().m_origin += Vec3(0, 1, 0);
						}
						if (Core::Input::Pressed(Core::Input::Action::Debug_LowerCamera))
						{
							_t.T().m_origin += Vec3(0, -1, 0);
						}

						if (Core::Input::PressedOnce(Core::Input::Action::Select))
						{
							Vec3 const col0 = _t.T().m_basis[0];
							_t.T().m_basis[0] = _t.T().m_basis[1];
							_t.T().m_basis[1] = _t.T().m_basis[2];
							_t.T().m_basis[2] = col0;
						}
					}
					else
					{
						// when at identity, forward == z
						Vec1 const yaw = _debugCamera.m_angle.y;
						Vec1 const pitch = _debugCamera.m_angle.x;


						Vec3 const forward = Normalise(Vec3{ cosf(yaw) * cosf(pitch), sinf(pitch), sinf(yaw) * cosf(pitch) });

						_t.T().m_basis = RotationFromForward(forward);

						// also re-calculate the Right and Up vector
						Vec3 const right = Normalise(glm::cross(forward, Vec3(0.0f, 1.0f, 0.0f)));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
						Vec3 const up = Normalise(glm::cross(right, forward));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.

						Vec1 const velocity = 0.8f * _fd.unscaled_dt;
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