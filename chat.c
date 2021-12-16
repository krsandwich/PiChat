#include "chat.h"
#include "protocol.h"
#include "myID.h"
#include "strings.h"
#include "printf.h"
#include <stdbool.h>
#include <stdint.h>
#include "packet.h"
#include "malloc.h"

static volatile int senderID; 


void createPacket(char* buffer, int destination){
	//only sends if the destination is in valid range
	if(destination < 0 || destination > 8){
		return;
	}

	packet p;
	
	(void) *buffer++;
	p.sender = getID();

	p.destination = destination; 

	p.payloadSize = strlen(buffer) + 1;

	memcpy(p.payload, buffer, strlen(buffer)+1);

	sendPacket(p);
}