#include "shellChat.h"
#include "shell_commands.h"
#include "keyboard.h"
#include "printf.h"
#include "strings.h"
#include "malloc.h"
#include "pi.h"
#include "ps2.h"
#include "chat.h"
#include "protocol.h"
#include "myID.h"
#include "consoleChat.h"

#define LINE_LEN 80

static int (*shell_printf)(const char * format, int senderid, ...);
static int (*shell_putchar)(int ch);

void shell_chat_init(formatted_fn_t print_fn, char_fn_t putchar_fn)
{
    shell_printf = print_fn;
    shell_putchar = putchar_fn;
}

void shell_bell(void)
{
    shell_putchar('\a');
}

void backSpace(char buf[], int location, int size){
    for(int i = location; i < size; i++){
        buf[i-1] = buf[i];
    }
    buf[size - 1] = ' ';
}

//shifts all of the characters forwarded when one is added in the middle of the line
//otherwise adds to the end
void addLetters(char buf[], int location, int size, char nextKey){

    for(int i = size; i > location; i--){
        buf[i + 1] = buf[i];
    }
    buf[location] = nextKey;
}

//updates the buffer and prints to the shell given the changes made to buf
void printBuf(char buf[], int location, int size){
    for(int i = location; i < size; i++){
        shell_putchar(buf[i]);
    }

    //backspaces to the correct location to lign up the cursor
    for(int i = 0; i < size - location; i++){
        shell_putchar('\b');
    }
}

//readline checks for an enter, backspace, and forward and backward arrow keys
void shell_chat_readline(char buf[], int bufsize)
{
    int index = 0;
    int size = 0;
    char ch;
    int location  = 0;
    while(bufsize > 1){
        int backspaceCalled = 0;
        //if the keyboard does not have a character, check for a packet
        //if there is a packet, retrieve it
        //process the message and console printf if the destination ID == the id of the user running the program on this computer or the desination ID == 8 (sends to everyone)
        while(!keyboard_has_character()){
            if(hasPacket()){
                packet message = getPacket();
                if(message.destination != getID() && message.destination != 8){
                    return; 
                }
                console_printf(message.payload, message.sender);             
            }
        }
        unsigned char nextKey = keyboard_read_next(); 
        if(nextKey == '\n'){
            buf[size] = '\n';
            shell_putchar('\n');
            return;         
        }
        //if there is a backspace key, delete the value before where the key is highlighted and move everything back to fill the space
        else if(nextKey == '\b'){
            shell_putchar('\b');
            shell_putchar(' ');
            shell_putchar('\b');
            buf[size - 1] = buf[size];
            size--;
            location--;
            bufsize++;
        } 
        //for just a letter key being pressed
        else{
            addLetters(buf, location, size, nextKey);
            shell_putchar(buf[location]);
            bufsize--;
            size ++;   
            location++;        
        }     
    }
    //null terminate
    buf[bufsize] = '\0';
    return;
}

void shell_chat_run(void)
{
    while (1) 
    {
        char line[LINE_LEN];
        shell_chat_readline(line, sizeof(line));       
    }
}