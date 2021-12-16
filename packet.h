#ifndef PACKET_H
#define PACKET_H

//Each packets holds 4 bits representing the sender’s ID, 4 bits representing the destination’s ID,
//an 8 bit representation of the payload size, and a char array that can hold 128 characters
typedef struct {
	uint32_t sender:4;
	uint32_t destination:4;
	uint32_t payloadSize:8;
	char payload[128];
} packet;

#endif