#pragma once

#include "common.h"
#include "managers/EntityManager.h"
#include "managers/Resources.h"

#include <ecs/component_specifier.h>

namespace Core
{
	namespace Render
	{
		struct FrameData
		{
			ecs_flags(ecs::flag::global);

			int w{ WINDOW_START_WIDTH };
			int h{ WINDOW_START_HEIGHT };
			float fW{ static_cast<float>(w) };
			float fH{ static_cast<float>(h) };
		};
		struct Frame_Tag
		{
			ecs_flags(ecs::flag::tag);
		};
		struct DefaultPass_Tag
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
			fVec4Data m_colour; // w == intensity
			float m_intensity;
			fVec4Data m_position; // if w == 0, xyz == direction
			fVec3Data m_direction; // for spotlights
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