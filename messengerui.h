#ifndef MESSENGERUI_H
#define MESSENGERUI_H

#include "gl.h"

/* 
 * Initializes the messenger user interface module
 */
void messengerui_init();

/* 
 * Draws the correct text dox on the console.
 *
 * @param   `sendOrReceive` if sendOrReceive == 0, the user is receiving the message. If sendOrReceive == 1 if the user is sending the message
 * @param   `str` the message
 * @param   `id` the ID of who sent the message
 */
void draw_text_bubble(int sendOrReceive, char* str, int id);

/* 
 * Loops through the screen's pixels and shifts everything up by scrollSize.
 *
 * @param   `scrollSize` the size needed for the new message
 */
void scrollBoxes(int scrollSize);

#endif