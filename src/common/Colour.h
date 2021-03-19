#pragma once

namespace Colour
{
	constexpr uint8 componentMax = 0xFF;

	namespace detail
	{
		constexpr int rShift = 0;
		constexpr int gShift = 8;
		constexpr int bShift = 16;
		constexpr int aShift = 24;
	}
	constexpr uint32 RGBA(uint8 _r, uint8 _g, uint8 _b, uint8 _a = componentMax)
	{
		return (_a << detail::aShift) + (_b << detail::bShift) + (_g << detail::gShift) + (_r << detail::rShift);
	}

	enum class Component
	{
		R, G, B, A,
	};
	template<Component _component>
	constexpr uint8 GetComponent(uint32 _rgba)
	{
		if constexpr (_component == Component::R) { return static_cast<uint8>((_rgba >> detail::rShift) & componentMax); }
		else if constexpr (_component == Component::G) { return static_cast<uint8>((_rgba >> detail::gShift) & componentMax); }
		else if constexpr (_component == Component::B) { return static_cast<uint8>((_rgba >> detail::bShift) & componentMax); }
		else if constexpr (_component == Component::A) { return static_cast<uint8>((_rgba >> detail::aShift) & componentMax); }
	}

	constexpr uint32 white = RGBA(componentMax, componentMax, componentMax);
	constexpr uint32 red = RGBA(componentMax, 0, 0);
	constexpr uint32 green = RGBA(0, componentMax, 0);
	constexpr uint32 blue = RGBA(0, 0, componentMax);

	constexpr uint32 ConvertRGB(fVec3 const& _col)
	{
		auto const r = static_cast<uint32>(gcem::round(_col.r * componentMax));
		auto const g = static_cast<uint32>(gcem::round(_col.g * componentMax));
		auto const b = static_cast<uint32>(gcem::round(_col.b * componentMax));
		return RGBA(r, g, b);
	}
}