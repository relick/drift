#pragma once

#include "common.h"

struct sapp_event;

namespace Core
{
	namespace Input
	{
		enum class Action : uint16
		{
			Quit,
			Select,
			Cancel,
			Jump,

			Forward,
			Back,
			Left,
			Right,

			Debug_AimCamera,

			Count,
		};

		enum class KeyType : uint8
		{
			Keyboard,
			Mouse,
		};

		void Setup();
		void Update();
		void Event(sapp_event const* _event);

		// key should be SAPP_KEYCODE if keyboard type, SAPP_MOUSEBUTTON if mouse type
		void SetActionKey(Action _action, int _index, KeyType _keyType, int _key);
		bool Pressed(Action _action);
		fVec2 GetMouseDelta();
		float GetScrollDelta();
	}
}