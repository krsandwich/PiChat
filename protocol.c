#include "gpio.h"
#include "gpioextra.h"
#include "printf.h"
#include "assert.h"
#include "uart.h"
#include "interrupts.h"
#include "armtimer.h"
#include "protocol.h"
#include "rbPacket.h"
#include "packet.h"
#include "strings.h"

#include <stdbool.h>
#include <stdint.h>
#include "timer.h"
#include "myID.h"
#include "rand.h"
#include "ringbuffer.h"

//packet queues management 
#define RECEIVE_QUEUE_SIZE 5
#define SEND_QUEUE_SIZE 3
#define DELAY 100

static volatile rb_p *receive_buf;
static volatile rb_p *send_buf;

static const unsigned DATA  = GPIO_PIN16;

static volatile unsigned int bitNum = 0;
static volatile unsigned int checkTotal= 0;

static volatile unsigned int flag = 0;
static volatile unsigned int sending = 0;
static volatile unsigned int receiving = 0;
static volatile unsigned int bit = 0;

static volatile unsigned int bitSend = 0;
static volatile unsigned int checkSend = 0;

static packet receiveCache;
static packet sendCache; 

void protocol_init(void) 
{
    gpio_init();
 
    gpio_set_input(DATA); 
    gpio_set_pullup(DATA); 

    armtimer_init(DELAY); // small 1/1000s
        armtimer_enable(); // enable timer
        armtimer_enable_interrupts(); 

        interrupts_enable_basic(INTERRUPTS_BASIC_ARM_TIMER_IRQ); 

    bool ok = interrupts_attach_handler(wire_handler);
    assert(ok);

    interrupts_enable_source(INTERRUPTS_GPIO3);

    receive_buf =rb_packet_new(RECEIVE_QUEUE_SIZE);
    send_buf = rb_packet_new(SEND_QUEUE_SIZE);
}

void sendPacket(packet p){
	// wait until the queue is open
	while(rb_packet_full(send_buf)){
		timer_delay_ms(10);
	}

    rb_packet_enqueue(send_buf, &p);
}

packet getPacket(void){
    // get packet from ring buffer
    packet p;
    while(!rb_packet_dequeue(receive_buf, &p)){
            timer_delay_ms(10);
    }

    return p;
}

bool hasPacket(void){
        return !rb_packet_empty(receive_buf);
}


void wire_handler(unsigned int pc)
{
	if(armtimer_check_and_clear_interrupt()){
		// check if we are currently receiving something
		if(receiving){
			bit = gpio_read(DATA);
			if(bitNum <4){
				receiveCache.sender |= bit << bitNum;
				checkTotal += bit;
				bitNum ++;
			}else if(bitNum <8){
				receiveCache.destination |= bit << (bitNum-4);
				checkTotal += bit;
				bitNum ++;
			}else if(bitNum < 16){
				receiveCache.payloadSize |= bit << (bitNum -8);
				checkTotal += bit;
				bitNum++;
			}else if(bitNum < (16 + 8 * receiveCache.payloadSize)){
				receiveCache.payload[(bitNum-16)/8] |= bit << ((bitNum -16)%8);
				checkTotal += bit;
				bitNum++;
			}else if(bitNum < (16 + 8 * receiveCache.payloadSize + 1)){
				if((bit + checkTotal)%2 == 1){
					//put in ring buffer
					rb_packet_enqueue(receive_buf, &receiveCache);
				}
				receiving = 0;
				checkTotal = 0;
				bitNum = 0;
				memset(&receiveCache,0,sizeof(packet));

			}
		}
		// check if we are currently sending something
		else if(sending){
			bit = 0;
			if(bitSend <4){
				bit = (sendCache.sender >> bitSend) & 1;
				checkSend += bit;
				bitSend ++;
			}else if(bitSend <8){
				bit = (sendCache.destination >> (bitSend-4) & 1);
				checkSend += bit;
				bitSend ++;
			}else if(bitSend < 16){
				bit = (sendCache.payloadSize >> (bitSend-8)) & 1;
				checkSend += bit;
				bitSend ++;
			}else if(bitSend < (16 + 8 * sendCache.payloadSize)){
				bit = (sendCache.payload[(bitSend-16)/8] >> ((bitSend -16)%8)) & 1;
				checkSend += bit;
				bitSend ++;
			}else if(bitSend < (16 + 8 * sendCache.payloadSize + 1)){
				if(checkSend % 2 == 1){
					bit = 0;
				}
				else{ 
					bit = 1;
				}
				bitSend++;
			}
			else{
				bitSend = 0;
				checkSend = 0;
				sending = 0; 
				gpio_set_input(DATA);

				memset(&sendCache,0,sizeof(packet));
			}
			gpio_write(DATA, bit);
		}
		// did the line just go low?
		else if(gpio_read(DATA) == 0){ 
			//Set status to receiving (aka can't send)
			receiving = 1; 
		}
		// do we have something we'd like to send?
		else if(!rb_packet_empty(send_buf)){
			sending = 1;
			// pull the line low
			gpio_set_output(DATA);
			gpio_write(DATA, 0);

			// put the packet we want to send in cache
			rb_packet_dequeue(send_buf, &sendCache);
		}
	}
	
}