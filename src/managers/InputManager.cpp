#include "InputManager.h"

#include <sokol_app.h>

#include <array>

namespace Core
{
	namespace Input
	{
		struct InputState
		{
			struct Keys
			{
				// Menu seems to be the last one.
				std::array<bool, SAPP_KEYCODE_MENU + 1> keys{ false };
				// Middle seems to be the last one.
				std::array<bool, SAPP_MOUSEBUTTON_MIDDLE + 1> mouseButtons{ false };
			};

			Keys pressedNextFrame{};
			Keys pressedThisFrame{};
			Keys pressedOnce{};

			struct MouseFrame
			{
				fVec2 dMouseMovement{ 0.0f, 0.0f };
				float dMouseScroll{ 0.0f };
			};

			MouseFrame thisFrame{};
			MouseFrame nextFrame{};
		};

		InputState inputState{};

		std::array<std::array<ActionKey, 2>, static_cast<usize>(Action::Count)> actions;

		void Setup()
		{
			// TODO: serialise
			// Defaults
			actions[static_cast<usize>(Action::Quit)][0].keyType = KeyType::Keyboard;
			actions[static_cast<usize>(Action::Quit)][0].key = SAPP_KEYCODE_ESCAPE;

			actions[static_cast<usize>(Action::Pause)][0].keyType = KeyType::Keyboard;
			actions[static_cast<usize>(Action::Pause)][0].key = SAPP_KEYCODE_P;

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

				actions[static_cast<usize>(Action::Backward)][0].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Backward)][0].key = SAPP_KEYCODE_S;

				actions[static_cast<usize>(Action::Left)][0].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Left)][0].key = SAPP_KEYCODE_A;

				actions[static_cast<usize>(Action::Right)][0].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Right)][0].key = SAPP_KEYCODE_D;

				actions[static_cast<usize>(Action::Forward)][1].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Forward)][1].key = SAPP_KEYCODE_UP;

				actions[static_cast<usize>(Action::Backward)][1].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Backward)][1].key = SAPP_KEYCODE_DOWN;

				actions[static_cast<usize>(Action::Left)][1].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Left)][1].key = SAPP_KEYCODE_LEFT;

				actions[static_cast<usize>(Action::Right)][1].keyType = KeyType::Keyboard;
				actions[static_cast<usize>(Action::Right)][1].key = SAPP_KEYCODE_RIGHT;
			}

			actions[static_cast<usize>(Action::Debug_SpeedUp)][0].keyType = KeyType::Keyboard;
			actions[static_cast<usize>(Action::Debug_SpeedUp)][0].key = SAPP_KEYCODE_O;

			actions[static_cast<usize>(Action::Debug_AimCamera)][0].keyType = KeyType::Mouse;
			actions[static_cast<usize>(Action::Debug_AimCamera)][0].key = SAPP_MOUSEBUTTON_MIDDLE;

			actions[static_cast<usize>(Action::Debug_RaiseCamera)][0].keyType = KeyType::Keyboard;
			actions[static_cast<usize>(Action::Debug_RaiseCamera)][0].key = SAPP_KEYCODE_E;

			actions[static_cast<usize>(Action::Debug_LowerCamera)][0].keyType = KeyType::Keyboard;
			actions[static_cast<usize>(Action::Debug_LowerCamera)][0].key = SAPP_KEYCODE_Q;

			actions[static_cast<usize>(Action::Debug_EnableCamera)][0].keyType = KeyType::Keyboard;
			actions[static_cast<usize>(Action::Debug_EnableCamera)][0].key = SAPP_KEYCODE_KP_ENTER;

		}

		// Run every frame
		void Update()
		{
			for (usize keyI = 0; keyI < inputState.pressedOnce.keys.size(); ++keyI)
			{
				inputState.pressedOnce.keys[keyI] = !inputState.pressedThisFrame.keys[keyI] && inputState.pressedNextFrame.keys[keyI];
			}
			for (usize mouseI = 0; mouseI < inputState.pressedOnce.mouseButtons.size(); ++mouseI)
			{
				inputState.pressedOnce.mouseButtons[mouseI] = !inputState.pressedThisFrame.mouseButtons[mouseI] && inputState.pressedNextFrame.mouseButtons[mouseI];
			}
			inputState.pressedThisFrame = inputState.pressedNextFrame;

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
				inputState.pressedNextFrame.keys[_event->key_code] = true;
				break;
			}
			case SAPP_EVENTTYPE_KEY_UP:
			{
				inputState.pressedNextFrame.keys[_event->key_code] = false;
				break;
			}
			case SAPP_EVENTTYPE_MOUSE_DOWN:
			{
				inputState.pressedNextFrame.mouseButtons[_event->mouse_button] = true;
				break;
			}
			case SAPP_EVENTTYPE_MOUSE_UP:
			{
				inputState.pressedNextFrame.mouseButtons[_event->mouse_button] = false;
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
			default:
			{
				break; // only respond to interesting things
			}
			}
		}

		void SetActionKey(Action _action, int _index, ActionKey _actionKey)
		{
			ActionKey& data = actions[static_cast<usize>(_action)][_index];
			data = _actionKey;
		}

		ActionKey GetActionKey(Action _action, int _index)
		{
			return actions[static_cast<usize>(_action)][_index];
		}

		// Check against state after remapping actions to buttons.
		bool Pressed(Action _action)
		{
			bool isPressed = false;
			for (ActionKey const& data : actions[static_cast<usize>(_action)])
			{
				switch (data.keyType)
				{
				case KeyType::Keyboard:
				{
					isPressed |= inputState.pressedThisFrame.keys[data.key];
					break;
				}
				case KeyType::Mouse:
				{
					isPressed |= inputState.pressedThisFrame.mouseButtons[data.key];
					break;
				}
				}
			}
			return isPressed;
		}

		bool PressedOnce(Action _action)
		{
			bool isPressed = false;
			for (ActionKey const& data : actions[static_cast<usize>(_action)])
			{
				switch (data.keyType)
				{
				case KeyType::Keyboard:
				{
					isPressed |= inputState.pressedOnce.keys[data.key];
					break;
				}
				case KeyType::Mouse:
				{
					isPressed |= inputState.pressedOnce.mouseButtons[data.key];
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