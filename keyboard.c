#include "gpio.h"
#include "gpioextra.h"
#include "keyboard.h"
#include "ps2.h"
#include "printf.h"
#include "interrupts.h"
#include "ringbuffer.h"
#include "assert.h"
#include "timer.h"
#include <stdbool.h>

#define CTRL_A_KEY 200
#define CTRL_E_KEY 201
#define CTRL_U_KEY 202

const unsigned int CLK  = GPIO_PIN23;
const unsigned int DATA = GPIO_PIN24; 

//volatile int count;
//unsigned char gChar;

static volatile int started = 0; //could have put started as negative index, but better readability
static volatile unsigned char value;
static volatile int index = 0;
static volatile int sum = 0;
static rb_t *rb; 

static void keyboard_handler(unsigned int pc) {
    if(gpio_check_event(CLK) == GPIO_DETECT_FALLING_EDGE){
        //count++;
        int data = gpio_read(DATA);
        
        if(!started && data == 0){ 
            started = 1;
        }
        else if(index == 8){ //parity bit
            sum += data;
            index++;
        }
        else if(index == 9){ //last bit
            if((data) && sum % 2 == 1){
                rb_enqueue(rb, value);
                //gChar = value;

            }
            started = 0;
            value = 0;
            index = 0;
            sum = 0;
        }
        else{
            value |= data << index;
            sum += data;
            index++;
        }
        gpio_clear_event(CLK);
    }
}

void keyboard_init(void) 
{
    rb = rb_new();
    gpio_set_input(CLK); 
    gpio_set_pullup(CLK); 
 
    gpio_set_input(DATA); 
    gpio_set_pullup(DATA);

    gpio_enable_event_detection(CLK, GPIO_DETECT_FALLING_EDGE);
    bool ok = interrupts_attach_handler(keyboard_handler);
    assert(ok);
    interrupts_enable_source(INTERRUPTS_GPIO3);
}

static int isModifier(char ch){
    if(ch == PS2_KEY_SCROLL_LOCK || ch == PS2_KEY_NUM_LOCK || ch == PS2_KEY_CAPS_LOCK ||
       ch == PS2_KEY_SHIFT || ch == PS2_KEY_ALT || ch == PS2_KEY_CTRL  ){
        return 1;
    }

    return 0;
}

static char characterFromEvent(key_event_t event){
    int capsLockOn = (event.modifiers & KEYBOARD_MOD_CAPS_LOCK) == KEYBOARD_MOD_CAPS_LOCK;
    int shiftPressed = (event.modifiers & KEYBOARD_MOD_SHIFT) == KEYBOARD_MOD_SHIFT;
    int controlPressed = (event.modifiers & KEYBOARD_MOD_CTRL) == KEYBOARD_MOD_CTRL;

    
    if(controlPressed){
        if(event.key.ch == 'a'){
            return CTRL_A_KEY;
        }
        else if(event.key.ch == 'e'){
            return CTRL_E_KEY;
        }
        else if(event.key.ch == 'u'){
            return CTRL_U_KEY;
        }
    }

    if(event.key.ch >= 'a' && event.key.ch <= 'z'){
        if(capsLockOn || shiftPressed){
            return event.key.other_ch;
        }
        else{
            return event.key.ch;
        }
    }

    else if(event.key.other_ch != 0 && shiftPressed){
        return event.key.other_ch;
    }

    return event.key.ch;
}

bool keyboard_has_character(char *ch){
    if(rb_empty(rb)){
        return false;
    }
    key_event_t event = keyboard_read_event();
    if(event.action == KEYBOARD_ACTION_UP || isModifier(event.key.ch)){
        return false;
    }
    *ch = characterFromEvent(event);
    return true;
}

unsigned char keyboard_read_scancode(void) 
{
    int val;

    while(!rb_dequeue(rb, &val)){
        timer_delay_ms(10);
    }

    return val;
}

int keyboard_read_sequence(unsigned char seq[])
{
    seq[0] = keyboard_read_scancode();

    if(seq[0] == PS2_CODE_RELEASE || seq[0] == PS2_CODE_EXTEND){
        seq[1] = keyboard_read_scancode();
        if(seq[1] == PS2_CODE_RELEASE){ // assuming won't get two releases in a row
            seq[2] = keyboard_read_scancode();
            return 3;
        }
        return 2;
    }
    return 1;
}

static unsigned int currentFlags = 0;
static unsigned int pressed = 0;

static void updateNonLockModifier(int action, int modifier){
    if(action == KEYBOARD_ACTION_DOWN){
        currentFlags |= modifier;
    }
    else{
        currentFlags -= currentFlags & modifier;
    }
    
}

static void updateLockModifier(int action, int modifier){
    int currentlyOn = (currentFlags & modifier) == modifier;
    int currentlyPressed = (pressed & modifier) == modifier;

    if(action == KEYBOARD_ACTION_DOWN){
        if(!currentlyPressed){
            pressed |= modifier;

            if(!currentlyOn){
                currentFlags |= modifier;
            }
            else{
                currentFlags = ~(~currentFlags | modifier);
            }
        }
    }
    else{
        pressed = ~(~pressed | modifier);
    }
}

key_event_t keyboard_read_event(void) 
{
    key_event_t event;
    
    //get the sequence
    int size = keyboard_read_sequence(event.seq);

    // get the scancode and its ps2_key
    int scancode;
    scancode = event.seq[size-1];
    ps2_key_t key = ps2_keys[scancode];

    // get the action
    unsigned int action;
    if((size == 2 && event.seq[0] == PS2_CODE_RELEASE) || size == 3){
        action = KEYBOARD_ACTION_UP;
    }
    else{
        action = KEYBOARD_ACTION_DOWN;
    }

    // get the modifiers
    if(key.ch == PS2_KEY_SCROLL_LOCK){
        updateLockModifier(action, KEYBOARD_MOD_SCROLL_LOCK);
    }
    else if(key.ch == PS2_KEY_NUM_LOCK){
        updateLockModifier(action, KEYBOARD_MOD_NUM_LOCK);
    }
    else if(key.ch == PS2_KEY_CAPS_LOCK){
        updateLockModifier(action, KEYBOARD_MOD_CAPS_LOCK);
    }
    else if(key.ch == PS2_KEY_SHIFT){
        updateNonLockModifier(action, KEYBOARD_MOD_SHIFT);
    }
    else if(key.ch == PS2_KEY_ALT){
        updateNonLockModifier(action, KEYBOARD_MOD_ALT);
    }
    else if(key.ch == PS2_KEY_CTRL){
        updateNonLockModifier(action, KEYBOARD_MOD_CTRL);\
    }

    // package the event
    event.seq_len = size;
    event.key = key;
    event.action = action;
    event.modifiers = currentFlags;

    return event;
}

// doesn't work well with numlock, but not required
// strange behavior reported at https://piazza.com/class/ja62e5etp2k709?cid=298
unsigned char keyboard_read_next(void) 
{
    key_event_t event = keyboard_read_event();

    // skips past events if they are keyboard up or modifier key
    while(event.action == KEYBOARD_ACTION_UP || isModifier(event.key.ch)){
        event = keyboard_read_event();
    }

    return characterFromEvent(event);
}