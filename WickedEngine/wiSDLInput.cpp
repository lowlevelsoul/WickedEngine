#include "wiSDLInput.h"

#if defined(SDL2) || defined(SDL3)
#if defined( SDL3 )
#	include <SDL3/SDL.h>
#elif defined( SDL2 )
#	include <SDL2/SDL.h>
#endif

#include "wiUnorderedMap.h"
#include "wiBacklog.h"

// (JS) BFH for SDL3. SDL3 has attempted to sanitize the event and gamepad
// naming conventions. For now, rather than re-writing the entire inout stuff for SDL3, we'll
// hijack the SDL2 input and hide the different SD3 naming conventions with macros.
#ifdef SDL3
#define SDL_GameController SDL_Gamepad
#define SDL_GameControllerAddMappingsFromFile SDL_AddGamepadMappingsFromFile
#define SDL_GameControllerOpen SDL_OpenGamepad
#define SDL_GameControllerClose SDL_CloseGamepad
#define SDL_GameControllerGetJoystick SDL_GetGamepadJoystick
#define SDL_JoystickInstanceID SDL_GetJoystickID
#define SDL_GameControllerRumble SDL_RumbleGamepad
#define SDL_GameControllerRumble SDL_RumbleGamepad

#define SDL_KEYDOWN SDL_EVENT_KEY_DOWN
#define SDL_KEYUP SDL_EVENT_KEY_UP
#define SDL_TEXTEDITING SDL_EVENT_TEXT_EDITING
#define SDL_TEXTINPUT SDL_EVENT_TEXT_INPUT
#define SDL_KEYMAPCHANGED SDL_EVENT_KEYMAP_CHANGED

#define SDL_JOYAXISMOTION SDL_EVENT_JOYSTICK_AXIS_MOTION
#define SDL_JOYBALLMOTION SDL_EVENT_JOYSTICK_BALL_MOTION
#define SDL_JOYHATMOTION SDL_EVENT_JOYSTICK_HAT_MOTION
#define SDL_JOYBUTTONDOWN SDL_EVENT_JOYSTICK_BUTTON_DOWN
#define SDL_JOYBUTTONUP SDL_EVENT_JOYSTICK_BUTTON_UP
#define SDL_JOYDEVICEADDED SDL_EVENT_JOYSTICK_ADDED
#define SDL_JOYDEVICEREMOVED SDL_EVENT_JOYSTICK_REMOVED
#define SDL_CONTROLLERAXISMOTION SDL_EVENT_GAMEPAD_AXIS_MOTION
#define SDL_CONTROLLER_AXIS_LEFTX SDL_GAMEPAD_AXIS_LEFTX
#define SDL_CONTROLLER_AXIS_LEFTY SDL_GAMEPAD_AXIS_LEFTY
#define SDL_CONTROLLER_AXIS_RIGHTY SDL_GAMEPAD_AXIS_RIGHTY
#define SDL_CONTROLLER_AXIS_RIGHTX SDL_GAMEPAD_AXIS_RIGHTX
#define SDL_CONTROLLER_AXIS_TRIGGERLEFT SDL_GAMEPAD_AXIS_LEFT_TRIGGER
#define SDL_CONTROLLER_AXIS_TRIGGERRIGHT SDL_GAMEPAD_AXIS_RIGHT_TRIGGER
#define SDL_CONTROLLERBUTTONDOWN SDL_EVENT_GAMEPAD_BUTTON_DOWN
#define SDL_CONTROLLERBUTTONUP SDL_EVENT_GAMEPAD_BUTTON_UP
#define SDL_CONTROLLERDEVICEADDED SDL_EVENT_GAMEPAD_ADDED
#define SDL_CONTROLLERDEVICEREMOVED SDL_EVENT_GAMEPAD_REMOVED
#define SDL_CONTROLLERDEVICEREMAPPED SDL_EVENT_GAMEPAD_REMAPPED
#define SDL_FINGERDOWN SDL_EVENT_FINGER_DOWN
#define SDL_FINGERUP SDL_EVENT_FINGER_UP
#define SDL_FINGERMOTION SDL_EVENT_FINGER_MOTION
#define SDL_CONTROLLER_BUTTON_DPAD_UP SDL_GAMEPAD_BUTTON_DPAD_UP
#define SDL_CONTROLLER_BUTTON_DPAD_DOWN SDL_GAMEPAD_BUTTON_DPAD_DOWN
#define SDL_CONTROLLER_BUTTON_DPAD_LEFT SDL_GAMEPAD_BUTTON_DPAD_LEFT
#define SDL_CONTROLLER_BUTTON_DPAD_RIGHT SDL_GAMEPAD_BUTTON_DPAD_RIGHT
#define SDL_CONTROLLER_BUTTON_X SDL_GAMEPAD_BUTTON_WEST
#define SDL_CONTROLLER_BUTTON_A SDL_GAMEPAD_BUTTON_SOUTH
#define SDL_CONTROLLER_BUTTON_B SDL_GAMEPAD_BUTTON_EAST
#define SDL_CONTROLLER_BUTTON_Y SDL_GAMEPAD_BUTTON_NORTH
#define SDL_CONTROLLER_BUTTON_LEFTSHOULDER SDL_GAMEPAD_BUTTON_LEFT_SHOULDER
#define SDL_CONTROLLER_BUTTON_RIGHTSHOULDER SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER
#define SDL_CONTROLLER_BUTTON_LEFTSTICK SDL_GAMEPAD_BUTTON_LEFT_STICK
#define SDL_CONTROLLER_BUTTON_RIGHTSTICK SDL_GAMEPAD_BUTTON_RIGHT_STICK
#define SDL_CONTROLLER_BUTTON_BACK SDL_GAMEPAD_BUTTON_BACK
#define SDL_CONTROLLER_BUTTON_START SDL_GAMEPAD_BUTTON_START

#define SDL_MOUSEMOTION SDL_EVENT_MOUSE_MOTION
#define SDL_MOUSEBUTTONDOWN SDL_EVENT_MOUSE_BUTTON_DOWN
#define SDL_MOUSEBUTTONUP SDL_EVENT_MOUSE_BUTTON_UP
#define SDL_MOUSEWHEEL SDL_EVENT_MOUSE_WHEEL
#endif

namespace wi::input::sdlinput
{
    wi::input::KeyboardState keyboard;
    wi::input::MouseState mouse;

    struct Internal_ControllerState
    {
        Sint32 portID;
        SDL_JoystickID internalID;
        SDL_GameController* controller;
        Uint16 rumble_l, rumble_r = 0;
        wi::input::ControllerState state;
    };
    wi::vector<Internal_ControllerState> controllers;
    wi::unordered_map<SDL_JoystickID, size_t> controller_mapped;

    wi::vector<SDL_Event> events;

    int to_wicked(const SDL_Scancode &key, const SDL_Keycode &keyk);
    void controller_to_wicked(uint32_t *current, Uint8 button, bool pressed);
    void controller_map_rebuild();

    void AddController(Sint32 id);
    void RemoveController(Sint32 id);
    void RemoveController();

    void Initialize() {
        if(!SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt")){
            wi::backlog::post("[SDL Input] No controller config loaded, add gamecontrollerdb.txt file next to the executable or download it from https://github.com/gabomdq/SDL_GameControllerDB");
        }
    }

    void ProcessEvent(const SDL_Event &event){
        events.push_back(event);
    }

    void Update()
    {
        mouse.delta_wheel = 0; // Do not accumulate mouse wheel motion delta
        mouse.delta_position = XMFLOAT2(0, 0); // Do not accumulate mouse position delta

        for(auto& event : events){
            switch(event.type){
                // Keyboard events
                case SDL_KEYDOWN:             // Key pressed
                {
#ifdef SDL3
					int converted = to_wicked(event.key.scancode, event.key.key );
#else
                    int converted = to_wicked(event.key.keysym.scancode, event.key.keysym.sym);
#endif
                    if (converted >= 0) {
                        keyboard.buttons[converted] = true;
                    }
                    break;
                }
                case SDL_KEYUP:               // Key released
                {
#ifdef SDL3
					int converted = to_wicked(event.key.scancode, event.key.key );
#else
					int converted = to_wicked(event.key.keysym.scancode, event.key.keysym.sym);
#endif
                    if (converted >= 0) {
                        keyboard.buttons[converted] = false;
                    }
                }
                case SDL_TEXTEDITING:         // Keyboard text editing (composition)
                case SDL_TEXTINPUT:           // Keyboard text input
                case SDL_KEYMAPCHANGED:       // Keymap changed due to a system event such as an
                    //     input language or keyboard layout change.
                    break;


                    // mouse events
                case SDL_MOUSEMOTION:          // Mouse moved
                    mouse.position.x = event.motion.x;
                    mouse.position.y = event.motion.y;
                    mouse.delta_position.x += event.motion.xrel;
                    mouse.delta_position.y += event.motion.yrel;
                    break;
                case SDL_MOUSEBUTTONDOWN:      // Mouse button pressed
                    switch(event.button.button){
                        case SDL_BUTTON_LEFT:
                            mouse.left_button_press = true;
                            break;
                        case SDL_BUTTON_RIGHT:
                            mouse.right_button_press = true;
                            break;
                        case SDL_BUTTON_MIDDLE:
                            mouse.middle_button_press = true;
                            break;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:        // Mouse button released
                    switch(event.button.button){
                        case SDL_BUTTON_LEFT:
                            mouse.left_button_press = false;
                            break;
                        case SDL_BUTTON_RIGHT:
                            mouse.right_button_press = false;
                            break;
                        case SDL_BUTTON_MIDDLE:
                            mouse.middle_button_press = false;
                            break;
                    }
                    break;
                case SDL_MOUSEWHEEL:           // Mouse wheel motion
                {
                    float delta = static_cast<float>(event.wheel.y);
                    if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                        delta *= -1;
                    }
                    mouse.delta_wheel += delta;
                    break;
                }


                    // Joystick events
                case SDL_JOYAXISMOTION:          // Joystick axis motion
                case SDL_JOYBALLMOTION:          // Joystick trackball motion
                case SDL_JOYHATMOTION:           // Joystick hat position change
                case SDL_JOYBUTTONDOWN:          // Joystick button pressed
                case SDL_JOYBUTTONUP:            // Joystick button released
                case SDL_JOYDEVICEADDED:         // A new joystick has been inserted into the system
                case SDL_JOYDEVICEREMOVED:       // An opened joystick has been removed
                    break;


                    // Game controller events
                case SDL_CONTROLLERAXISMOTION:          // Game controller axis motion
                {
#ifdef SDL3
					auto controller_get = controller_mapped.find(event.gaxis.which );
#else
                    auto controller_get = controller_mapped.find(event.caxis.which);
#endif

                    if(controller_get != controller_mapped.end()){
#ifdef SDL3
						float raw = event.gaxis.value / 32767.0f;
#else
                        float raw = event.caxis.value / 32767.0f;
#endif
                        const float deadzone = 0.2;
                        float deadzoned = (raw < -deadzone || raw > deadzone) ? raw : 0;
#ifdef SDL3
						switch(event.gaxis.axis){
#else
                        switch(event.caxis.axis){
#endif
 
                            case SDL_CONTROLLER_AXIS_LEFTX:
                                controllers[controller_get->second].state.thumbstick_L.x = deadzoned;
                                break;
                            case SDL_CONTROLLER_AXIS_LEFTY:
                                controllers[controller_get->second].state.thumbstick_L.y = -deadzoned;
                                break;
                            case SDL_CONTROLLER_AXIS_RIGHTX:
                                controllers[controller_get->second].state.thumbstick_R.x = deadzoned;
                                break;
                            case SDL_CONTROLLER_AXIS_RIGHTY:
                                controllers[controller_get->second].state.thumbstick_R.y = deadzoned;
                                break;
                            case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
                                controllers[controller_get->second].state.trigger_L = (deadzoned > 0.f) ? deadzoned : 0;
                                break;
                            case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
                                controllers[controller_get->second].state.trigger_R = (deadzoned > 0.f) ? deadzoned : 0;
                                break;
                        }
                    }
                    break;
                }
                case SDL_CONTROLLERBUTTONDOWN:          // Game controller button pressed
                {
#ifdef SDL3
                    auto find = controller_mapped.find(event.gbutton.which);
#else
					auto find = controller_mapped.find(event.cbutton.which);
#endif
                    if(find != controller_mapped.end()){

#ifdef SDL3
						controller_to_wicked(&controllers[find->second].state.buttons, event.gbutton.button, true);
#else
						controller_to_wicked(&controllers[find->second].state.buttons, event.cbutton.button, true);
#endif
                    }
                    break;
                }
                case SDL_CONTROLLERBUTTONUP:            // Game controller button released
                {
#ifdef SDL3
					auto find = controller_mapped.find(event.gbutton.which);
#else
					auto find = controller_mapped.find(event.cbutton.which);
#endif
                    if(find != controller_mapped.end()){
#ifdef SDL3
                        controller_to_wicked(&controllers[find->second].state.buttons, event.gbutton.button, false);
#else
						controller_to_wicked(&controllers[find->second].state.buttons, event.cbutton.button, false);
#endif
                    }
                    break;
                }
                case SDL_CONTROLLERDEVICEADDED:         // A new Game controller has been inserted into the system
                {
                    auto& controller = controllers.emplace_back();
                    controller.controller = SDL_GameControllerOpen(event.cdevice.which);
                    if(controller.controller){
                        controller.portID = event.cdevice.which;
                        controller.internalID = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller.controller));
                    }
                    controller_map_rebuild();
                    break;
                }
                case SDL_CONTROLLERDEVICEREMOVED:       // An opened Game controller has been removed
                {
                    auto find = controller_mapped.find(event.cdevice.which);
                    if(find != controller_mapped.end()){
                        SDL_GameControllerClose(controllers[find->second].controller);
                        controllers[find->second] = std::move(controllers.back());
                        controllers.pop_back();
                    }
                    controller_map_rebuild();
                    break;
                }
                case SDL_CONTROLLERDEVICEREMAPPED:      // The controller mapping was updated
                    break;


                    // Touch events
                case SDL_FINGERDOWN:
                case SDL_FINGERUP:
                case SDL_FINGERMOTION:
                    wi::backlog::post("finger!");
                    break;


                    // Gesture events
#ifndef SDL3
                case SDL_DOLLARGESTURE:
                case SDL_DOLLARRECORD:
                case SDL_MULTIGESTURE:
                    wi::backlog::post("gesture!");
                    break;
#endif
                default:
                    break;
            }
            // Clone all events for use outside the internal code, e.g. main_SDL2.cpp can benefit from this
            //external_events.push_back(event);
        }

        //Update rumble every call
        for(auto& controller : controllers){
            SDL_GameControllerRumble(
                controller.controller,
                controller.rumble_l,
                controller.rumble_r,
                60); //Buffer at 60ms
        }

        //Flush away stored events
        events.clear();
    }

    int to_wicked(const SDL_Scancode &key, const SDL_Keycode &keyk) {

        // Scancode Conversion Segment

        if(key >= 4 && key <= 29){ // A to Z
            return (key - 4) + CHARACTER_RANGE_START;
        }
        if(key >= 30 && key <= 39){ // 0 to 9
            return (key - 30) + DIGIT_RANGE_START;
        }
        if(key >= 58 && key <= 69){ // F1 to F12
            return (key - 58) + KEYBOARD_BUTTON_F1;
        }
        if(key >= 79 && key <= 82){ // Keyboard directional buttons
            return (82 - key) + KEYBOARD_BUTTON_UP;
        }
        switch(key){ // Individual scancode key conversion
            case SDL_SCANCODE_SPACE:
                return wi::input::KEYBOARD_BUTTON_SPACE;
            case SDL_SCANCODE_LSHIFT:
                return wi::input::KEYBOARD_BUTTON_LSHIFT;
            case SDL_SCANCODE_RSHIFT:
                return wi::input::KEYBOARD_BUTTON_RSHIFT;
            case SDL_SCANCODE_RETURN:
                return wi::input::KEYBOARD_BUTTON_ENTER;
            case SDL_SCANCODE_ESCAPE:
                return wi::input::KEYBOARD_BUTTON_ESCAPE;
            case SDL_SCANCODE_HOME:
                return wi::input::KEYBOARD_BUTTON_HOME;
            case SDL_SCANCODE_RCTRL:
                return wi::input::KEYBOARD_BUTTON_RCONTROL;
            case SDL_SCANCODE_LCTRL:
                return wi::input::KEYBOARD_BUTTON_LCONTROL;
            case SDL_SCANCODE_DELETE:
                return wi::input::KEYBOARD_BUTTON_DELETE;
            case SDL_SCANCODE_BACKSPACE:
                return wi::input::KEYBOARD_BUTTON_BACKSPACE;
            case SDL_SCANCODE_PAGEDOWN:
                return wi::input::KEYBOARD_BUTTON_PAGEDOWN;
            case SDL_SCANCODE_PAGEUP:
                return wi::input::KEYBOARD_BUTTON_PAGEUP;
			case SDL_SCANCODE_KP_0:
				return wi::input::KEYBOARD_BUTTON_NUMPAD0;
			case SDL_SCANCODE_KP_1:
				return wi::input::KEYBOARD_BUTTON_NUMPAD1;
			case SDL_SCANCODE_KP_2:
				return wi::input::KEYBOARD_BUTTON_NUMPAD2;
			case SDL_SCANCODE_KP_3:
				return wi::input::KEYBOARD_BUTTON_NUMPAD3;
			case SDL_SCANCODE_KP_4:
				return wi::input::KEYBOARD_BUTTON_NUMPAD4;
			case SDL_SCANCODE_KP_5:
				return wi::input::KEYBOARD_BUTTON_NUMPAD5;
			case SDL_SCANCODE_KP_6:
				return wi::input::KEYBOARD_BUTTON_NUMPAD6;
			case SDL_SCANCODE_KP_7:
				return wi::input::KEYBOARD_BUTTON_NUMPAD7;
			case SDL_SCANCODE_KP_8:
				return wi::input::KEYBOARD_BUTTON_NUMPAD8;
			case SDL_SCANCODE_KP_9:
				return wi::input::KEYBOARD_BUTTON_NUMPAD9;
			case SDL_SCANCODE_KP_MULTIPLY:
				return wi::input::KEYBOARD_BUTTON_MULTIPLY;
			case SDL_SCANCODE_KP_PLUS:
				return wi::input::KEYBOARD_BUTTON_ADD;
			case SDL_SCANCODE_SEPARATOR:
				return wi::input::KEYBOARD_BUTTON_SEPARATOR;
			case SDL_SCANCODE_KP_MINUS:
				return wi::input::KEYBOARD_BUTTON_SUBTRACT;
			case SDL_SCANCODE_KP_DECIMAL:
				return wi::input::KEYBOARD_BUTTON_DECIMAL;
			case SDL_SCANCODE_KP_DIVIDE:
				return wi::input::KEYBOARD_BUTTON_DIVIDE;
			case SDL_SCANCODE_INSERT:
				return wi::input::KEYBOARD_BUTTON_INSERT;
			case SDL_SCANCODE_TAB:
				return wi::input::KEYBOARD_BUTTON_TAB;
			case SDL_SCANCODE_GRAVE: 
    			return wi::input::KEYBOARD_BUTTON_TILDE;
			case SDL_SCANCODE_LALT:
				return wi::input::KEYBOARD_BUTTON_ALT;
			case SDL_SCANCODE_RALT:
				return wi::input::KEYBOARD_BUTTON_ALTGR;
        }


        // Keycode Conversion Segment

        if(keyk >= 91 && keyk <= 126){
            return keyk;
        }

        return -1;

    }

    void controller_to_wicked(uint32_t *current, Uint8 button, bool pressed){
        uint32_t btnenum;
        switch(button){
            case SDL_CONTROLLER_BUTTON_DPAD_UP: btnenum = wi::input::GAMEPAD_BUTTON_UP; break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: btnenum = wi::input::GAMEPAD_BUTTON_LEFT; break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: btnenum = wi::input::GAMEPAD_BUTTON_DOWN; break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: btnenum = wi::input::GAMEPAD_BUTTON_RIGHT; break;
            case SDL_CONTROLLER_BUTTON_X: btnenum = wi::input::GAMEPAD_BUTTON_1; break;
            case SDL_CONTROLLER_BUTTON_A: btnenum = wi::input::GAMEPAD_BUTTON_2; break;
            case SDL_CONTROLLER_BUTTON_B: btnenum = wi::input::GAMEPAD_BUTTON_3; break;
            case SDL_CONTROLLER_BUTTON_Y: btnenum = wi::input::GAMEPAD_BUTTON_4; break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: btnenum = wi::input::GAMEPAD_BUTTON_5; break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: btnenum = wi::input::GAMEPAD_BUTTON_6; break;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK: btnenum = wi::input::GAMEPAD_BUTTON_7; break;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK: btnenum = wi::input::GAMEPAD_BUTTON_8; break;
            case SDL_CONTROLLER_BUTTON_BACK: btnenum = wi::input::GAMEPAD_BUTTON_9; break;
            case SDL_CONTROLLER_BUTTON_START: btnenum = wi::input::GAMEPAD_BUTTON_10; break;
			default: assert(0); return;
        }
        btnenum = 1 << (btnenum - wi::input::GAMEPAD_RANGE_START - 1);
        if(pressed){
            *current |= btnenum;
        }else{
            *current &= ~btnenum;
        }
    }

    // Rebuild controller mappings for fast array access
    void controller_map_rebuild(){
        controller_mapped.clear();
        for(int index = 0; index < controllers.size(); ++index){
            controller_mapped.insert({controllers[index].internalID, index});
        }
    }

    void GetKeyboardState(wi::input::KeyboardState* state) {
        *state = keyboard;
    }
    void GetMouseState(wi::input::MouseState* state) {
        *state = mouse;
    }

    int GetMaxControllerCount() { return controllers.size(); }
    bool GetControllerState(wi::input::ControllerState* state, int index) {
        if(index < controllers.size()){
            if (state != nullptr)
			{
				*state = controllers[index].state;
			}
			return true;
        }
        return false;
    }
    void SetControllerFeedback(const wi::input::ControllerFeedback& data, int index) {
        if(index < controllers.size()){
#ifdef SDL2_FEATURE_CONTROLLER_LED
            SDL_GameControllerSetLED(
                controllers[index].controller,
                data.led_color.getR(),
                data.led_color.getG(),
                data.led_color.getB());
#endif
            controllers[index].rumble_l = (Uint16)floor(data.vibration_left * 0xFFFF);
            controllers[index].rumble_r = (Uint16)floor(data.vibration_right * 0xFFFF);
        }
    }
}
#else
namespace wi::input::sdlinput
{
    void Initialize() {}
    void Update() {}
    void GetKeyboardState(wi::input::KeyboardState* state) {}
    void GetMouseState(wi::input::MouseState* state) {}
    int GetMaxControllerCount() { return 0; }
    bool GetControllerState(wi::input::ControllerState* state, int index) { return false; }
    void SetControllerFeedback(const wi::input::ControllerFeedback& data, int index) {}
}
#endif // _WIN32
