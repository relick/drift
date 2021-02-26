#pragma once

#include "common.h"
#include "components.h"

namespace Core
{
	namespace Render
	{
		struct LightSetter
		{
			fVec4& Col;
			fVec4& Pos;
			fVec4& Att;
			fVec4& Dir;
			fVec4& Cut;
		};

		struct CameraState
		{
			fMat4 proj{};
			fMat4 view{};
			fVec3 pos{};
		};

		void Init();
		void SetupPipeline();
		void StartPass(Core::Render::FrameData const& _rfd, Core::Render::Camera const& _cam, Core::Transform const& _t);
		void Render(Core::Render::FrameData const& _rfd);
		void Cleanup();

		CameraState const& GetCameraState();

		void AddModelToScene(Core::Resource::ModelID const& _model, fTrans const& _worldTrans);
		LightSetter AddLightToScene();
		void AddAmbientLightToScene(fVec3 const& _col);
		void SetDirectionalLightDir(fVec3 const& _dir);
	}
}