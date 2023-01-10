#ifndef SDLCENGINE_INPUT_H
#define SDLCENGINE_INPUT_H

#include <stdbool.h>

typedef enum input_key {
    INPUT_KEY_LEFT,
    INPUT_KEY_RIGHT,
    INPUT_KEY_UP,
    INPUT_KEY_DOWN,
    INPUT_KEY_ESCAPE
} Input_Key;

typedef enum key_state {
    KS_INVALID,
    KS_UNPRESSED,
    KS_PRESSED,
    KS_HELD
} Key_State;

typedef struct input_state {
    Key_State left;
    Key_State right;
    Key_State up;
    Key_State down;
    Key_State escape;
} Input_State;

/**
 * Initializes input variables.
 */
void init_input(void);
void input_update(void);
/**
 * Get the state of a current key by name
 * @param key name of the key as a string
 * @return state of the key, use bitwise AND with the KS_State enums to check its state! KS_Invalid if no match.
 */
Key_State get_key_state_str(const char* key);
/**
 * Get the state of a current key by key id
 * @param key Key id of the key
 * @return state of the key, use bitwise AND with the KS_State enums to check its state! KS_Invalid if no match.
 */
Key_State get_key_state_id(SDL_KeyCode key);
/**
 * Get the state of a current key by scancode (technically the fastest method)
 * @param scancode scancode of the key
 * @return state of the key, use bitwise AND with the KS_State enums to check its state! KS_Invalid if no match.
 */
Key_State get_key_state_scancode(SDL_Scancode scancode);

#endif //SDLCENGINE_INPUT_H
