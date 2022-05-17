#pragma once

#include "common.h"
#include "managers/EntityManager.h"
#include "managers/ResourceIDs.h"
#include "managers/RenderIDs.h"

#include <ecs/flags.h>
#include <variant>

namespace Core
{
	namespace Render
	{
		struct FrameData
		{
			ecs_flags(ecs::flag::global, ecs::flag::immutable);

			struct Target
			{
				iVec2 i{ 0 };
				Vec2 f{ i };
			};
			Target contextWindow{};
			Target renderArea{};
		};

		// Render objects
		struct Light
		{
			enum class Type : uint8
			{
				Ambient,
				Directional,
				Point,
				Spotlight,
			};

			Type m_type;
			Vec3 m_colour;
			Vec1 m_intensity;
			Vec3 m_attenuation{ 1.0f, 0.7f, 1.8f }; // constant, linear, quadratic
			Vec1 m_cutoffAngle{ 0.9978f }; // spotlights only
			Vec1 m_outerCutoffAngle{ 0.953f }; // spotlights only

			// position and direction defined by transform component, which is required.
		};

		struct ModelDesc
		{
			std::string m_filePath;
		};

		struct Model
		{
			use_initialiser;

			Resource::ModelID m_modelID;
			bool m_drawDefaultPass{ true }; // expand to array for multiple passes?
		};

		struct SkyboxDesc
		{
			std::string m_cubemapPath;
		};

		struct Skybox
		{
			use_initialiser;

			Resource::TextureID m_cubemapID;
		};

		struct SpriteDesc
		{
			using SpriteInit = std::variant< std::string, Resource::SpriteID >;
			SpriteInit m_spriteInit;
			Trans2D m_initTrans;
			uint32 m_initFlags;
		};

		struct Sprite
		{
			use_initialiser;

			Resource::SpriteID m_spriteID;
			Render::SpriteSceneID m_spriteSceneID;
			uint32 m_spriteFlags;
		};

	}

	template<>
	void AddComponent(EntityID const _entity, Render::ModelDesc const& _desc);

	template<>
	void AddComponent(EntityID const _entity, Render::SkyboxDesc const& _desc);

	template<>
	void AddComponent(EntityID const _entity, Render::SpriteDesc const& _desc);

	template<>
	void CleanupComponent<Render::Sprite>( EntityID const _entity );
}