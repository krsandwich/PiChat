#ifndef CHAT_H
#define CHAT_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

/*
 * Creates the packet and sends the message to the correct destination 
 * (if this destination is valid).
 *
 * @param   `buffer` char* with the message
 * @param   `destination` ID of the desired destination pi
 */
void createPacket(char* buffer, int destination);

#endif