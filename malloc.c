/*
 * File: malloc.c
 * --------------
 * Implicit free list implementation of malloc
 */

#include "malloc.h"
#include <stddef.h> // for NULL
#include <stdint.h> // for uint32_t
#include "strings.h"
#include "printf.h"
#include "backtrace.h"

extern int __bss_end__;

// Simple macro to round up x to multiple of n.
// The efficient but tricky bitwise approach it uses
// works only if n is a power of two
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))

#define TOTAL_HEAP_SIZE 0x1000000 // 16 MB
#define ROUND 8

// Redzone value
static const unsigned int REDZONE = 0x107e107e;

struct header
{
    uint32_t size: 31; // NOTE: we are storing the unrounded size; see code for details
    uint32_t available: 1;
    struct frame mallocFrames[3];
};

// Keep track of start (header) and end of heap
static struct header *heap_start = NULL;
static void *heap_max = NULL;

/* Implicit free list allocator
 * Traverses through list of headers until finds a payload region of sufficient size
 */
void *malloc(size_t nbytes) 
{
    // Initialize starting header
    if (!heap_start) {
        heap_start = (void *)&__bss_end__;

        // Note: TOTAL_HEAP_SIZE assumed to already by multiple of ROUND
        heap_start->size = TOTAL_HEAP_SIZE - roundup(sizeof(struct header),ROUND);
        heap_start->available = 1;
        heap_max = (char *)heap_start + TOTAL_HEAP_SIZE;
    }

    // need to make sure payload (with redzone) and header are aligned
    size_t roundedBytes = roundup(nbytes + 2*sizeof(unsigned int), ROUND) + roundup(sizeof(struct header), ROUND);
    struct header *current = heap_start;
    void *alloc = NULL;

    // traverse through headers until one is open or reached end of list
    while(alloc == NULL && (void *)current < heap_max){
        // if can fit in the current header
        if(current->available && current->size >= roundedBytes){
            // make unavailable
            current->available = 0;

            // move forward by the size of the header and left redzone
            alloc = ((char *)current) + roundup(sizeof(struct header), ROUND) + sizeof(unsigned int);

            // redzone placement
            char* start = (char *) alloc - sizeof(unsigned int);
            memcpy(start, &REDZONE, sizeof(unsigned int));
            char* end = (char *) alloc + nbytes;
            memcpy(end, &REDZONE, sizeof(unsigned int));
            backtrace(current->mallocFrames, 3);
            
            // NOTE: assuming size for available is always a multiple of ROUND
            // Will be ensured in free (and already the case initially)
            int oldSize = current->size;

            // NOTE: storing unrounded value for red zone; round up when traversing
            current->size = nbytes + 2*sizeof(unsigned int);

            // Write next header
            struct header *next = (struct header *) (((char *)current) + roundedBytes);
            next->available = 1;
            next->size = oldSize - roundedBytes;
        }

        // move to next block (past rounded header and payload)
        int bytesToNext = roundup(sizeof(struct header), ROUND) + roundup(current->size, ROUND);
        current = (struct header *) ((char *)current + bytesToNext);
    }

    return alloc;
}

// forward coalesces free blocks
// @param head a pointer to the header of a free block
static void forwardCoalesce(struct header *head){
    struct header *next = (struct header *) ((char *)head + roundup(sizeof(struct header), ROUND) + head->size);

    //go forward and coalesce adjacent free blocks
    while((void *)next < heap_max && next->available){
        // updates the head's size
        head->size += roundup(sizeof(struct header), ROUND) + next->size;

        //move to next block
        next = (struct header *) ((char *)next + roundup(sizeof(struct header), ROUND) + next->size);
    }
}

// updates block header and coalesces the forward blocks
void free(void *ptr) 
{
    // corner case if NULL
    if(ptr == NULL){
        return;
    }

    // get header before and make available
    struct header *head = (struct header *) ((char *)ptr - roundup(sizeof(struct header), ROUND) - sizeof(unsigned int));
    head->available = 1;

    // check red zones
    unsigned int before = 0;
    unsigned int after = 0;
    char* start = (char *) ptr - sizeof(unsigned int);
    memcpy(&before, start, sizeof(unsigned int));
    char* end = (char *) ptr + head->size - 2*sizeof(unsigned int);
    memcpy(&after, end, sizeof(unsigned int));

    if(before != REDZONE || after != REDZONE){
        printf("Address %p has damaged the red zone(s): [%x][%x] (Should both be [%x])\n", ptr, before, after, REDZONE);
        printf("Block of size %x bytes (in hex), allocated by\n", head->size - 2*sizeof(unsigned int));
        print_frames(head->mallocFrames, 3);
    }

    // makes sure available blocks have size multiple of ROUND
    head->size = roundup(head->size, ROUND);

    forwardCoalesce(head);
}


// reallocates in place if possible
void *realloc (void *old_ptr, size_t new_size)
{
    // cornercases
    if(old_ptr == NULL){
        void *alloc = malloc(new_size);
        return alloc;
    }
    else if(new_size == 0){
        free(old_ptr);
        return malloc(0);
    }

    // get header before
    int backup = roundup(sizeof(struct header), ROUND) + sizeof(unsigned int);
    struct header *head = (struct header *) ((char *)old_ptr - backup);

    int newPayload = new_size + 2*sizeof(unsigned int);

    // round down (but need to have space for new header)
    if(newPayload < head->size){
        // don't need to make new header
        if(roundup(newPayload, ROUND) == roundup(head->size, ROUND)){
            // update size
            head->size = newPayload;

            // update redzone
            char* end = (char *) old_ptr + new_size;
            memcpy(end, &REDZONE, sizeof(unsigned int));
            backtrace(head->mallocFrames, 3);
            return old_ptr;
        }
        // if can fit a new header
        else if(roundup(head->size, ROUND) - roundup(newPayload, ROUND) >= roundup(sizeof(struct header), ROUND)){
            //update size
            int oldSize = head->size;
            head->size = newPayload;

            // update redzone
            char* end = (char *) old_ptr + new_size;
            memcpy(end, &REDZONE, sizeof(unsigned int));
            backtrace(head->mallocFrames, 3);

            // put a new header
            struct header *next = (struct header *) ((char *)old_ptr - sizeof(unsigned int) + roundup(head->size, ROUND));
            next->available = 1;
            next->size = roundup(oldSize, ROUND) - roundup(newPayload, ROUND) - roundup(sizeof(struct header), ROUND);
            return old_ptr;
        }
    }
    // new size is greater than
    else{

        // see if can just be rounded up with no moving header
        if(newPayload <= roundup(head->size, ROUND)){
            //update size
            head->size = newPayload;

            // updates redzone
            char* end = (char *) old_ptr + new_size;
            memcpy(end, &REDZONE, sizeof(unsigned int));
            backtrace(head->mallocFrames, 3);

            return old_ptr;
        }

        // see if next block is free and can fit
        struct header *next = (struct header *) ((char *)old_ptr - sizeof(unsigned int) + roundup(head->size, ROUND));

        if(next->available){
            // could be cases where free didn't coalesce
            // (for ex freeing memory in increasing addresses)
            forwardCoalesce(next);

            // see if can fit with next free block
            // Note: We are "forcing" the translation of the header of the next free block.
            // Although there could be a case where we just use and remove the header of the
            // next free block, there are many corner casese that make this unwieldy
            // especially if sizeof(struct header) > ROUND.
            size_t total_space = roundup(head->size, ROUND) + next->size;
            if(newPayload <= total_space){
                //update size
                head->size = newPayload;
                
                // update redzone
                char* end = (char *) old_ptr + new_size;
                memcpy(end, &REDZONE, sizeof(unsigned int));
                backtrace(head->mallocFrames, 3);

                // get address of translated next header
                int leftover = total_space - roundup(newPayload, ROUND);
                int nextOffset = - sizeof(unsigned int) + roundup(head->size, ROUND);
                struct header *newNext = (struct header *) ((char *)old_ptr + nextOffset);
                
                //update translated next header
                newNext->available = 1;
                newNext->size = leftover;
                return old_ptr;
            }
        }
    }

    // couldn't reallocate in place
    void *new_ptr = malloc(new_size);
    if (!new_ptr) return NULL;
    
    memcpy(new_ptr, old_ptr, new_size);
    free(old_ptr);
    return new_ptr;
}
