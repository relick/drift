#include "RenderSystems.h"

#include "SystemOrdering.h"
#include "components.h"

#include "managers/RenderManager.h"

namespace Core
{
	namespace Render
	{
		void Setup()
		{
			Core::MakeSystem<Sys::RENDER_QUEUE>([](Core::Render::Light const& _light, Core::Transform const& _t)
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

			Core::MakeSystem<Sys::RENDER_QUEUE>([](Core::Render::Model const& _model, Core::Transform const& _t)
			{
				if (_model.m_drawDefaultPass)
				{
					fTrans const worldTransform = _t.CalculateWorldTransform();
					AddModelToScene(_model.m_modelID, worldTransform);
				}
			});


			Core::MakeSystem<Sys::RENDER_PASS_START>([](Core::Render::FrameData const& _rfd, Core::Render::Camera const& _cam, Core::Transform const& _t, MT_Only&)
			{
				Core::Render::StartPass(_rfd, _cam, _t);
			});

			Core::MakeSystem<Sys::RENDER>([](Core::Render::FrameData const& _rfd, Core::Render::Camera const& _cam, MT_Only&)
			{
				Core::Render::Render(_rfd);
			});
		}
	}
}