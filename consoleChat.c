#include "consoleChat.h"
#include "malloc.h"
#include "gl.h"
#include "printf.h"
#include "myID.h"
#include "strings.h"
#include "messengerui.h"
#include "chat.h"
#include "protocol.h"
#include <stdbool.h>

static int x;
static int cols;
static int rows;
int charNumber = 0;
int messageNumber = 0;
char** allMessages;

volatile int receiver;
static volatile int foundReceiver = 0;

static void drawScreen();
void adjustBuffer(int x, int ch);
bool checkFoundReceiver(int ch);

#define charwidth gl_get_char_width()
#define charheight gl_get_char_height()
#define screenwidth gl_get_width()
#define screenheight gl_get_height()

#define BACKGROUND GL_WHITE
#define FOREGROUND GL_BLACK
#define MAX_OUTPUT_LEN 128
//create space for 129 characters == 128 allowable from the message + space for the ID char at the beginning
#define MESSAGESIZE 129

static char* bottomBuffer;
static char message[MESSAGESIZE];
volatile int ID;

static void setBottomBuffer(int xPos, char value){
	bottomBuffer[xPos] = value;
}

static char getBottomBuffer(int xPos){
	return bottomBuffer[xPos];
}

// for both update graphs and update character we are updating the front and back buffer
// to make sure both are consistent. This allows us to make some assumptions about the
// current state of the screen, that makes our code run much faster.
static void updateGraphics(){
	drawScreen();
	gl_swap_buffer();
	//need to update the back too
	gl_draw_rect(0, (screenheight - charheight*2), screenwidth, charheight, GL_BLACK);
	drawScreen();
}

static void drawScreen(){
	gl_clear(BACKGROUND);
	for(int i=0; i<cols; i++){
		char bottom = getBottomBuffer(i);
		if(bottom != ' '){
			gl_draw_char(i * charwidth, screenheight - charheight, bottom, FOREGROUND);
		}
	}
	for(int i = 0; i < messageNumber; i++){
		//gets the identifier from the first index of the message (each message is stored as the sender's ID and then the message)
		int identifier = allMessages[i][0] - '0';
		if(identifier == ID){
			draw_text_bubble(1, allMessages[i], ID);
		}else{
			draw_text_bubble(0, allMessages[i], identifier);
		}	
	}
	//draws the bottom black rectangle under which the user inputs their text
	gl_draw_rect(0, (screenheight - charheight*2), screenwidth, charheight, GL_BLACK);
}

static void drawCharacter(char c, int i, int j){
	gl_draw_rect(i * charwidth, j, charwidth, charheight, BACKGROUND);
	char bottom = getBottomBuffer(i);
	if(bottom != ' '){
		gl_draw_char(i * charwidth, j, bottom, FOREGROUND);
	}
	gl_swap_buffer();

	//need to update back too
	gl_draw_rect(i * charwidth, j, charwidth, charheight, BACKGROUND);
	if(bottom != ' '){
		gl_draw_char(i * charwidth, j, bottom, FOREGROUND);
	}
}

void console_chat_init(unsigned int nrows, unsigned int ncols)
{
	//malloc enough space in the bottom line buffer for the number of colums on the screen
    bottomBuffer = malloc(ncols);
    x = 0;
    allMessages = malloc(100);
    cols = ncols;
    rows = nrows;
    gl_init(cols * charwidth, rows * charheight, GL_DOUBLEBUFFER);

    updateGraphics();
    messengerui_init();

    //get the user's id from the myID file to be used to determine whether each message comes from the user or a different sender
    ID = getID();

}

//used when the bottom line has been filled - scrolls all of the letters to the left to make room for more
static void moveLeft(void){
	//move all of the characters left by one
	for(int i=0; i<cols;i++){
		adjustBuffer(i, getBottomBuffer(i+1));
	}
	//clear last letter
	adjustBuffer(cols, ' ');
}

//clears the bottom line
void clearLine(void){
	x=0;
	for(int i = 0; i < cols; i++){
		setBottomBuffer(i, ' ');
	}
}

void adjustBuffer(int x, int ch){
	setBottomBuffer(x, (char) ch);
	drawCharacter(ch, x, screenheight - charheight);
}

bool checkFoundReceiver(int ch){
	if(foundReceiver == 0){
		receiver = (char) ch - '0';
		if(receiver >= 0 && receiver <= 9){
			adjustBuffer(x++, ch);
			foundReceiver++;
			return true;
		}
	}else if(foundReceiver == 1){
		if((char) ch == ':'){
			adjustBuffer(x++, ch);
			foundReceiver++;
			return true;
		}
	}else if(foundReceiver == 2){
		if((char) ch == ' '){
			adjustBuffer(x++, ch);
			foundReceiver++;
			//adds the sender's id to the beggining of the message and increment the char array character counter
			snprintf(message, 128, "%d", ID);
			charNumber++; 
			return true;
		}
	}
	return false;
}

int console_bottom_putchar(int ch)
{
	//this section checks that the user is using the correct format of integer, colon, space, then message
	if(foundReceiver < 3){
		if(ch == '\b' && foundReceiver > 0){
	    	foundReceiver--;
	    	x--;
	    	adjustBuffer(x, ch);
			return 0;
		}
		checkFoundReceiver(ch);
		return 0;
	}

	if(charNumber >= MAX_OUTPUT_LEN + 3){
		return 0;
	}
    if(ch == '\a'){
    	return '\a';
    }
    if(ch == '\b'){
    	if(x != 0){
    		x--;
    	}
    }
    //if the user types enter: create space for a new message and add it into the larger message array
    //next, create the correct packet (this will send the packet to the correct destination) and clear the message
    //establish that the sender ID is the ID of the computer being typed into and then update the graphics
    //lastly, clear the bottom and reset the character index and foundReceiver flag
    else if(ch == '\n'){
    	x = 0; 
    	message[charNumber] ='\0';
    	allMessages[messageNumber] = malloc(128);
		memcpy(allMessages[messageNumber++], message, 128);

		createPacket(message, receiver);
		
		memset(&message, 0, 128);
		clearLine(); 
		updateGraphics();		
		charNumber = 0;	
    	foundReceiver = 0;
    }
    //otherwise add the character into the bottom buffer and the message char array - if the x value reaches the end of the line, the characters in the line to the left
	else{
		adjustBuffer(x, ch);
		message[charNumber++] = (char) ch;

		if(x < cols){
			x++;
		}
		else if (x >= cols){
			moveLeft();
		}
	}
	return ch;
}

//should this be the sender id instead of receiver id -- want to display who sent it on the left side messagess
int console_top_printf(const char *format, int senderid, ...)
{
    char buf[MAX_OUTPUT_LEN];

    va_list ap;
    va_start(ap, senderid);
    int curPos = vsnprintf(buf, MAX_OUTPUT_LEN, format, ap);
    va_end(ap);

    //malloc space for a new message
    allMessages[messageNumber] = malloc(128);
    //put the sender's id at the beggining of the char* and then concatenate the message to formatted
    char* formatted = malloc(MESSAGESIZE);
	snprintf(formatted, MESSAGESIZE, "%d", senderid);
    strlcat(formatted, format, strlen(format) + 2);
    //copy the formatted message into the array of all of the messages
	memcpy(allMessages[messageNumber], formatted, 128);
	messageNumber++;
	updateGraphics();

    return curPos;
}