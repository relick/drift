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
			Pause,
			Select,
			Cancel,
			Jump,

			Forward,
			Backward,
			Left,
			Right,

			Debug_SpeedUp,
			Debug_AimCamera,
			Debug_RaiseCamera,
			Debug_LowerCamera,
			Debug_EnableCamera,

			Count,
		};

		enum class KeyIndex : uint8
		{
			Primary,
			Secondary,

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
			usize key{ 0 /* keycode invalid */ };
		};

		void Setup();
		void Update();
		void Event(sapp_event const* _event);

		// key should be SAPP_KEYCODE if keyboard type, SAPP_MOUSEBUTTON if mouse type
		void SetActionKey(Action _action, KeyIndex _index, ActionKey _actionKey);
		ActionKey GetActionKey(Action _action, KeyIndex _index);
		bool Pressed(Action _action);
		bool PressedOnce(Action _action);
		Vec2 GetMouseDelta();
		Vec1 GetScrollDelta();
		void LockMouse( bool _lock );
	}
}