#pragma once

#include "common.h"
#include "components.h"
#include "RenderIDs.h"

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

		[[nodiscard]] SpriteSceneID AddSpriteToScene( Core::Resource::SpriteID _sprite, Trans2D const& _screenTrans, uint32 _initFlags );
		void UpdateSpriteInScene( SpriteSceneID _sprite, Trans2D const& _screenTrans, uint32 _flags );
		void RemoveSpriteFromScene( SpriteSceneID _sprite );

		// Functions for adding graphics just this frame. The more this is done, the slower things are :)
		void DrawModelThisFrame(Core::Resource::ModelID _model, Trans const& _worldTrans);
		void DrawSkyboxThisFrame(Core::Resource::TextureSampleID _skybox);
		LightSetter AddLightThisFrame();
		void AddAmbientLightThisFrame(Vec3 const& _col);
		void SetDirectionalLightDir(Vec3 const& _dir);
	}
}