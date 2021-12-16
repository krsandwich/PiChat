/* File: rbPacket.c
 * ------------------
 * Simple lock-free ring buffer that allows for concurrent
 * access by 1 reader and 1 writer.
 *
 * Author: Philip Levis <pal@cs.stanford.edu>
 * Author: Julie Zelenski <zelenski@cs.stanford.edu>
 */

#include "rbPacket.h"
#include "malloc.h"
#include "packet.h"
#include <stdint.h>
#include <stdbool.h>
#include "printf.h"
#include "strings.h"
#include "assert.h"

/*
 * A ring buffer is represented using a struct containing a fixed-size array 
 * and head and tail fields, which are indexes into the entries[] array.
 * head is the index of the frontmost element (head advances during dequeue)
 * tail is the index of the next position to use (tail advances during enqueue)
 * Both head and tail advance circularly, i.e. index = (index + 1) % LENGTH
 * The ring buffer is empty if tail == head
 * The ring buffer is full if tail + 1 == head
 * (Note: one slot remains permanently empty to distinguish full from empty)
 */

struct rbPacket {
    packet* entries;
    unsigned int length;
    unsigned int head, tail; 
};


rb_p *rb_packet_new(int size) 
{
    rb_p *rb_packet= malloc(sizeof(struct rbPacket));       
    rb_packet-> entries = malloc(sizeof(packet) * size);
    rb_packet-> length = size;
    rb_packet->head = rb_packet->tail = 0;
    return rb_packet;
}

bool rb_packet_empty(rb_p *rb_packet) 
{
    return rb_packet->head == rb_packet->tail;
}

bool rb_packet_full(rb_p *rb_packet) 
{
    return (rb_packet->tail + 1) % rb_packet->length == rb_packet->head;
}

/*
 * Note: enqueue is called by writer. enqueue advances rb->tail, 
 * no changes to rb->head.  This design allows safe concurrent access.
 * Enqueues the address of the packet (4 bits worth of space)
 */
bool rb_packet_enqueue(rb_p *rb_packet, packet *elem) 
{
    if (rb_packet_full(rb_packet)) return false;
    rb_packet->entries[rb_packet->tail].sender = elem->sender;
    rb_packet->entries[rb_packet->tail].destination = elem->destination;
    rb_packet->entries[rb_packet->tail].payloadSize = elem->payloadSize;
    memcpy(rb_packet->entries[rb_packet->tail].payload, elem->payload, elem->payloadSize); 
    rb_packet->tail = (rb_packet->tail + 1) % rb_packet->length;
    return true;
}

/*
 * Note: dequeue is called by reader. dequeue advances rb->head,
 * no changes to rb->tail. This design allows safe concurrent access.
 */
bool rb_packet_dequeue(rb_p *rb_packet, packet *p_elem)
{

    //printf("dequeuing packet\n");
    if (rb_packet_empty(rb_packet)) return false;

    *p_elem = rb_packet->entries[rb_packet->head];
    p_elem->sender = rb_packet->entries[rb_packet->head].sender;
    p_elem->destination = rb_packet->entries[rb_packet->head].destination;
    memcpy(p_elem->payload, rb_packet->entries[rb_packet->head].payload, strlen(rb_packet->entries[rb_packet->head].payload)+1);
    rb_packet->head = (rb_packet->head + 1) % rb_packet->length;
    //assert(0);
    return true;
}
 