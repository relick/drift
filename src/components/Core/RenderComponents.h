#pragma once

#include "common.h"
#include "managers/EntityManager.h"
#include "managers/ResourceIDs.h"

#include <ecs/flags.h>

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
				fVec2 f{ i };
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
			fVec3 m_colour;
			float m_intensity;
			fVec3 m_attenuation{ 1.0f, 0.7f, 1.8f }; // constant, linear, quadratic
			float m_cutoffAngle{ 0.9978f }; // spotlights only
			float m_outerCutoffAngle{ 0.953f }; // spotlights only

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
			std::string m_filePath;
		};

		struct Sprite
		{
			use_initialiser;

			Resource::SpriteID m_spriteID;
		};

	}

	template<>
	void AddComponent(EntityID const _entity, Render::ModelDesc const& _desc);

	template<>
	void AddComponent(EntityID const _entity, Render::SkyboxDesc const& _desc);

	template<>
	void AddComponent(EntityID const _entity, Render::SpriteDesc const& _desc);

}