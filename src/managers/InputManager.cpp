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
				Vec2 dMouseMovement{ 0.0f, 0.0f };
				Vec1 dMouseScroll{ 0.0f };
			};

			MouseFrame thisFrame{};
			MouseFrame nextFrame{};
		};

		static InputState g_inputState{};

		static std::array<std::array<ActionKey, static_cast<usize>(KeyIndex::Count)>, static_cast<usize>(Action::Count)> g_actions;

		void Setup()
		{
			// TODO: serialise
			// Defaults
			g_actions[static_cast<usize>(Action::Quit)][0].keyType = KeyType::Keyboard;
			g_actions[static_cast<usize>(Action::Quit)][0].key = SAPP_KEYCODE_ESCAPE;

			g_actions[static_cast<usize>(Action::Pause)][0].keyType = KeyType::Keyboard;
			g_actions[static_cast<usize>(Action::Pause)][0].key = SAPP_KEYCODE_P;

			{
				g_actions[static_cast<usize>(Action::Select)][0].keyType = KeyType::Mouse;
				g_actions[static_cast<usize>(Action::Select)][0].key = SAPP_MOUSEBUTTON_LEFT;

				g_actions[static_cast<usize>(Action::Cancel)][0].keyType = KeyType::Mouse;
				g_actions[static_cast<usize>(Action::Cancel)][0].key = SAPP_MOUSEBUTTON_RIGHT;

				g_actions[static_cast<usize>(Action::Select)][1].keyType = KeyType::Keyboard;
				g_actions[static_cast<usize>(Action::Select)][1].key = SAPP_KEYCODE_Z;

				g_actions[static_cast<usize>(Action::Cancel)][1].keyType = KeyType::Mouse;
				g_actions[static_cast<usize>(Action::Cancel)][1].key = SAPP_KEYCODE_X;
			}

			g_actions[static_cast<usize>(Action::Jump)][0].keyType = KeyType::Keyboard;
			g_actions[static_cast<usize>(Action::Jump)][0].key = SAPP_KEYCODE_SPACE;

			{
				g_actions[static_cast<usize>(Action::Forward)][0].keyType = KeyType::Keyboard;
				g_actions[static_cast<usize>(Action::Forward)][0].key = SAPP_KEYCODE_W;

				g_actions[static_cast<usize>(Action::Backward)][0].keyType = KeyType::Keyboard;
				g_actions[static_cast<usize>(Action::Backward)][0].key = SAPP_KEYCODE_S;
				
				g_actions[static_cast<usize>(Action::Left)][0].keyType = KeyType::Keyboard;
				g_actions[static_cast<usize>(Action::Left)][0].key = SAPP_KEYCODE_A;
				
				g_actions[static_cast<usize>(Action::Right)][0].keyType = KeyType::Keyboard;
				g_actions[static_cast<usize>(Action::Right)][0].key = SAPP_KEYCODE_D;
				
				g_actions[static_cast<usize>(Action::Forward)][1].keyType = KeyType::Keyboard;
				g_actions[static_cast<usize>(Action::Forward)][1].key = SAPP_KEYCODE_UP;
				
				g_actions[static_cast<usize>(Action::Backward)][1].keyType = KeyType::Keyboard;
				g_actions[static_cast<usize>(Action::Backward)][1].key = SAPP_KEYCODE_DOWN;
				
				g_actions[static_cast<usize>(Action::Left)][1].keyType = KeyType::Keyboard;
				g_actions[static_cast<usize>(Action::Left)][1].key = SAPP_KEYCODE_LEFT;
				
				g_actions[static_cast<usize>(Action::Right)][1].keyType = KeyType::Keyboard;
				g_actions[static_cast<usize>(Action::Right)][1].key = SAPP_KEYCODE_RIGHT;
			}

			g_actions[static_cast<usize>(Action::Debug_SpeedUp)][0].keyType = KeyType::Keyboard;
			g_actions[static_cast<usize>(Action::Debug_SpeedUp)][0].key = SAPP_KEYCODE_O;
			
			g_actions[static_cast<usize>(Action::Debug_AimCamera)][0].keyType = KeyType::Mouse;
			g_actions[static_cast<usize>(Action::Debug_AimCamera)][0].key = SAPP_MOUSEBUTTON_MIDDLE;
			
			g_actions[static_cast<usize>(Action::Debug_RaiseCamera)][0].keyType = KeyType::Keyboard;
			g_actions[static_cast<usize>(Action::Debug_RaiseCamera)][0].key = SAPP_KEYCODE_E;
			
			g_actions[static_cast<usize>(Action::Debug_LowerCamera)][0].keyType = KeyType::Keyboard;
			g_actions[static_cast<usize>(Action::Debug_LowerCamera)][0].key = SAPP_KEYCODE_Q;
			
			g_actions[static_cast<usize>(Action::Debug_EnableCamera)][0].keyType = KeyType::Keyboard;
			g_actions[static_cast<usize>(Action::Debug_EnableCamera)][0].key = SAPP_KEYCODE_KP_ENTER;

		}

		// Run every frame
		void Update()
		{
			for (usize keyI = 0; keyI < g_inputState.pressedOnce.keys.size(); ++keyI)
			{
				g_inputState.pressedOnce.keys[keyI] = !g_inputState.pressedThisFrame.keys[keyI] && g_inputState.pressedNextFrame.keys[keyI];
			}
			for (usize mouseI = 0; mouseI < g_inputState.pressedOnce.mouseButtons.size(); ++mouseI)
			{
				g_inputState.pressedOnce.mouseButtons[mouseI] = !g_inputState.pressedThisFrame.mouseButtons[mouseI] && g_inputState.pressedNextFrame.mouseButtons[mouseI];
			}
			g_inputState.pressedThisFrame = g_inputState.pressedNextFrame;

			g_inputState.thisFrame = g_inputState.nextFrame;
			g_inputState.nextFrame = InputState::MouseFrame{};
		}

		// Receive events from sapp
		void Event(sapp_event const* _event)
		{
			switch (_event->type)
			{
			// key presses
			case SAPP_EVENTTYPE_KEY_DOWN:
			{
				g_inputState.pressedNextFrame.keys[_event->key_code] = true;
				break;
			}
			case SAPP_EVENTTYPE_KEY_UP:
			{
				g_inputState.pressedNextFrame.keys[_event->key_code] = false;
				break;
			}
			case SAPP_EVENTTYPE_MOUSE_DOWN:
			{
				g_inputState.pressedNextFrame.mouseButtons[_event->mouse_button] = true;
				break;
			}
			case SAPP_EVENTTYPE_MOUSE_UP:
			{
				g_inputState.pressedNextFrame.mouseButtons[_event->mouse_button] = false;
				break;
			}

			// analogue movement
			case SAPP_EVENTTYPE_MOUSE_MOVE:
			{
				g_inputState.nextFrame.dMouseMovement.x += _event->mouse_dx;
				g_inputState.nextFrame.dMouseMovement.y += _event->mouse_dy;
				break;
			}
			case SAPP_EVENTTYPE_MOUSE_SCROLL:
			{
				g_inputState.nextFrame.dMouseScroll += _event->scroll_y;
				break;
			}
			default:
			{
				break; // only respond to interesting things
			}
			}
		}

		void SetActionKey(Action _action, KeyIndex _index, ActionKey _actionKey)
		{
			ActionKey& data = g_actions[static_cast<usize>(_action)][static_cast<usize>(_index)];
			data = _actionKey;
		}

		ActionKey GetActionKey(Action _action, KeyIndex _index)
		{
			return g_actions[static_cast<usize>(_action)][static_cast<usize>(_index)];
		}

		// Check against state after remapping actions to buttons.
		bool Pressed(Action _action)
		{
			bool isPressed = false;
			for (ActionKey const& data : g_actions[static_cast<usize>(_action)])
			{
				switch (data.keyType)
				{
				case KeyType::Keyboard:
				{
					isPressed |= g_inputState.pressedThisFrame.keys[data.key];
					break;
				}
				case KeyType::Mouse:
				{
					isPressed |= g_inputState.pressedThisFrame.mouseButtons[data.key];
					break;
				}
				}
			}
			return isPressed;
		}

		bool PressedOnce(Action _action)
		{
			bool isPressed = false;
			for (ActionKey const& data : g_actions[static_cast<usize>(_action)])
			{
				switch (data.keyType)
				{
				case KeyType::Keyboard:
				{
					isPressed |= g_inputState.pressedOnce.keys[data.key];
					break;
				}
				case KeyType::Mouse:
				{
					isPressed |= g_inputState.pressedOnce.mouseButtons[data.key];
					break;
				}
				}
			}
			return isPressed;
		}

		Vec2 GetMouseDelta()
		{
			return g_inputState.thisFrame.dMouseMovement;
		}

		Vec1 GetScrollDelta()
		{
			return g_inputState.thisFrame.dMouseScroll;
		}
	}
}