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
		struct ActionKey
		{
			KeyType keyType{ KeyType::Keyboard };
			int key{ SAPP_KEYCODE_INVALID };
		};

		void Setup();
		void Update();
		void Event(sapp_event const* _event);

		// key should be SAPP_KEYCODE if keyboard type, SAPP_MOUSEBUTTON if mouse type
		void SetActionKey(Action _action, int _index, ActionKey _actionKey);
		ActionKey GetActionKey(Action _action, int _index);
		bool Pressed(Action _action);
		fVec2 GetMouseDelta();
		float GetScrollDelta();
	}
}