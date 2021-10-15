#pragma once

#include "common.h"
#include "components.h"

namespace Core
{
	namespace Render
	{
		struct LightSetter
		{
			Vec4& Col;
			Vec4& Pos;
			Vec4& Att;
			Vec4& Dir;
			Vec4& Cut;
		};

		struct CameraState
		{
			Mat4 proj{};
			Mat4 view{};
			Vec3 pos{};
		};

		void Init();
		void SetupPipeline(int _mainRenderWidth, int _mainRenderHeight);
		void SetMainCameraParams(Core::Render::FrameData const& _rfd, Core::Render::MainCamera3D const& _cam, Core::Transform3D const& _t);
		void Render(Core::Render::FrameData const& _rfd);
		void Cleanup();

		CameraState const& GetCameraState();

		void AddSpriteToScene(Core::Resource::SpriteID _sprite, Trans2D const& _screenTrans);
		void AddModelToScene(Core::Resource::ModelID _model, Trans const& _worldTrans);
		void AddSkyboxToScene(Core::Resource::TextureSampleID _skybox);
		LightSetter AddLightToScene();
		void AddAmbientLightToScene(Vec3 const& _col);
		void SetDirectionalLightDir(Vec3 const& _dir);
	}
}