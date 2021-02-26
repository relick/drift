#pragma once

#include "common.h"
#include "managers/EntityManager.h"
#include "managers/ResourceManager.h"

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
				int w{ WINDOW_START_WIDTH };
				int h{ WINDOW_START_HEIGHT };
				float fW{ static_cast<float>(w) };
				float fH{ static_cast<float>(h) };
			};
			Target contextWindow{};
			Target renderArea{};
		};
		struct Frame_Tag
		{
			ecs_flags(ecs::flag::tag);
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

	}

	template<>
	void AddComponent(EntityID const _entity, Render::ModelDesc const& _desc);

	template<>
	void RemoveComponent<Render::Model>(EntityID const _entity);

}