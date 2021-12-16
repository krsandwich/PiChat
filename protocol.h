#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>
#include "packet.h"

/* 
 * Initializes the protocol file. This sets up the correct data pins, enables timer interrupts, sets up the wire interrupt handler,
 * and creates two new packets, one for receving a packet and one for sending a packet.
 */
void protocol_init(void);

/* 
 * Adds the packet to the ringbuffer, which will cause it to be consoled - printf'd in the shell file
 *
 * @param   `p` the packet to send
 */
void sendPacket(packet p);

/* 
 * Retrieves the frontmost element in the ringbuffer if the ring buffer is not empty. If the ringbuffer is empty, nothing happens to the ring buffer.
 *
 * @return  the next packet in the ringbuffer
 */
packet getPacket(void);

/* 
 * Check if the ring buffer contains a packet. 
 *
 * @return  true if the ringbuffer contains a packet, false otherwise
 */
bool hasPacket(void);

/* 
 * Interrupt system for building the packet. 
 * 
 * Checks if in recieving or sending mode
 * 		Send/recieve 1 bit of packet per interrupt 
 * 		- 4 bit sender address
 * 		- 4 bit destination address 
 * 		- 8 bit payload size 
 * 		- N bit payload (N is defined by size)
 * 		- 1 bit odd parity checksum
 *
 * If not sending/recieving, check if another pi has brought the line low (switched to output)
 * 		Set mode to receiving
 * Lastly, if nothing else is happening, check if anything needs to be sent
 *		Set mode to sending
 * 		Bring line low 
 *
 *
 * @param   `pc` 
 */
void wire_handler(unsigned int pc);


#endif