#include "Input.h"

#include <sokol_app.h>

#include <array>

namespace Core
{
	namespace Input
	{
		struct InputState
		{
			// Menu seems to be the last one.
			bool keysPressed[SAPP_KEYCODE_MENU + 1]{ false };
			// Middle seems to be the last one.
			bool mouseButtonsPressed[SAPP_MOUSEBUTTON_MIDDLE + 1]{ false };

			struct MouseFrame
			{
				fVec2 dMouseMovement{ 0.0f, 0.0f };
				float dMouseScroll{ 0.0f };
			};

			MouseFrame thisFrame{};
			MouseFrame nextFrame{};
		};

		InputState inputState{};

		struct ActionData
		{
			KeyType keyType{ KeyType::Keyboard };
			int key{ SAPP_KEYCODE_INVALID };
		};
		std::array<std::array<ActionData, 2>, static_cast<usize>(Action::Count)> actions;

		void Setup()
		{
			// TODO: serialise
			// Defaults
			actions[static_cast<usize>(Action::Quit)][0].keyType = KeyType::Keyboard;
			actions[static_cast<usize>(Action::Quit)][0].key = SAPP_KEYCODE_ESCAPE;

			{
				actions[static_cast<usize>(Action::Select)][0].keyType = KeyType::Mouse;
				actions[static_cast<usize>(Action::Select)][0].key = SAPP_MOUSEBUTTON_LEFT;

				actions[static_cast<usize>(Action::Cancel)][0].keyType = KeyType::Mouse;
				actions[static_cast<usize>(Action::Cancel)][0].key = SAPP_MOUSEBUTTON_RIGHT;

				actions[static_cast<usize>(Action::Select)][1].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Select)][1].key = SAPP_KEYCODE_Z;

				actions[static_cast<usize>(Action::Cancel)][1].keyType = KeyType::Mouse;
				actions[static_cast<usize>(Action::Cancel)][1].key = SAPP_KEYCODE_X;
			}

			actions[static_cast<usize>(Action::Jump)][0].keyType = KeyType::Keyboard;
			actions[static_cast<usize>(Action::Jump)][0].key = SAPP_KEYCODE_SPACE;

			{
				actions[static_cast<usize>(Action::Forward)][0].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Forward)][0].key = SAPP_KEYCODE_W;

				actions[static_cast<usize>(Action::Back)][0].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Back)][0].key = SAPP_KEYCODE_S;

				actions[static_cast<usize>(Action::Left)][0].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Left)][0].key = SAPP_KEYCODE_A;

				actions[static_cast<usize>(Action::Right)][0].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Right)][0].key = SAPP_KEYCODE_D;

				actions[static_cast<usize>(Action::Forward)][1].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Forward)][1].key = SAPP_KEYCODE_UP;

				actions[static_cast<usize>(Action::Back)][1].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Back)][1].key = SAPP_KEYCODE_DOWN;

				actions[static_cast<usize>(Action::Left)][1].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Left)][1].key = SAPP_KEYCODE_LEFT;

				actions[static_cast<usize>(Action::Right)][1].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Right)][1].key = SAPP_KEYCODE_RIGHT;
			}

			actions[static_cast<usize>(Action::Debug_AimCamera)][0].keyType = KeyType::Mouse;
			actions[static_cast<usize>(Action::Debug_AimCamera)][0].key = SAPP_MOUSEBUTTON_MIDDLE;

		}

		// Run every frame
		void Update()
		{
			inputState.thisFrame = inputState.nextFrame;
			inputState.nextFrame = InputState::MouseFrame{};
		}

		// Receive events from sapp
		void Event(sapp_event const* _event)
		{
			switch (_event->type)
			{
			// key presses
			case SAPP_EVENTTYPE_KEY_DOWN:
			{
				inputState.keysPressed[_event->key_code] = true;
				break;
			}
			case SAPP_EVENTTYPE_KEY_UP:
			{
				inputState.keysPressed[_event->key_code] = false;
				break;
			}
			case SAPP_EVENTTYPE_MOUSE_DOWN:
			{
				inputState.mouseButtonsPressed[_event->mouse_button] = true;
				break;
			}
			case SAPP_EVENTTYPE_MOUSE_UP:
			{
				inputState.mouseButtonsPressed[_event->mouse_button] = false;
				break;
			}

			// analogue movement
			case SAPP_EVENTTYPE_MOUSE_MOVE:
			{
				inputState.nextFrame.dMouseMovement.x += _event->mouse_dx;
				inputState.nextFrame.dMouseMovement.y += _event->mouse_dy;
				break;
			}
			case SAPP_EVENTTYPE_MOUSE_SCROLL:
			{
				inputState.nextFrame.dMouseScroll += _event->scroll_y;
				break;
			}
			}
		}

		void SetActionKey(Action _action, int _index, KeyType _keyType, int _key)
		{
			ActionData& data = actions[static_cast<usize>(_action)][_index];
			data.keyType = _keyType;
			data.key = _key;
		}

		// Check against state after remapping actions to buttons.
		bool Pressed(Action _action)
		{
			bool isPressed = false;
			for (ActionData const& data : actions[static_cast<usize>(_action)])
			{
				switch (data.keyType)
				{
				case KeyType::Keyboard:
				{
					isPressed |= inputState.keysPressed[data.key];
					break;
				}
				case KeyType::Mouse:
				{
					isPressed |= inputState.mouseButtonsPressed[data.key];
					break;
				}
				}
			}
			return isPressed;
		}

		fVec2 GetMouseDelta()
		{
			return inputState.thisFrame.dMouseMovement;
		}

		float GetScrollDelta()
		{
			return inputState.thisFrame.dMouseScroll;
		}
	}
}