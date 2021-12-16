#include "keyboard.h"
#include "console.h"
#include "fb.h"
#include "shell.h"
#include "interrupts.h"
#include "gpio.h"
#include "gpioextra.h"
#include "uart.h"
#include "messengerui.h"
#include "strings.h"
#include "gl.h"
#include "printf.h"

#define charWidth gl_get_char_width()
#define charHeight gl_get_char_height()
#define screenWidth gl_get_width()
#define screenHeight gl_get_height()
#define GL_GRAY	0xFFBDC3C7
#define GL_LIGHTBLUE 0xFF62D5F4

volatile int bottomBoxYCoordinate;
volatile int newLocation = 0;

unsigned int spaceSize;
int totalHeight;
int maxBoxWidth;

void messengerui_init(){
	spaceSize = 20;
	maxBoxWidth = 15;
	totalHeight = 0;
	bottomBoxYCoordinate = screenHeight  - charHeight*2;
}

void draw_text_bubble(int sendOrReceive, char* str, int id){
	(void)*str++;
	int stringWidth = strlen(str);
	int colNum = 0;
	color c;
	int rowNum = stringWidth/maxBoxWidth + 1;
	int x;
	//sets the correct size for the text box based on the string with (the max width is 15 characters)
	if(stringWidth >= maxBoxWidth){
		colNum = maxBoxWidth;
	}else{
		colNum = stringWidth;
	}

	//sets values for the height and width of the text box 
	double height = (double) rowNum * ((double) charHeight) + ((double) charHeight + spaceSize);
	double width = (double) colNum * ((double) charWidth) + ((double) charWidth);

	//sets the correct color and starting x location
	if(sendOrReceive){
		//adjust the x value accordingly so the id value does not go off the screen for short messages
		if(colNum == 0){
			x = screenWidth - spaceSize*3 - charWidth*3;
		}
		else if(colNum <= 2){
			x = screenWidth - spaceSize*2 - width - charWidth*2;
		}else{
			x = screenWidth - spaceSize*2 - width;
		}
		c = GL_LIGHTBLUE;
	}else{
		x = spaceSize;
		c = GL_GRAY;
	} 
	scrollBoxes(height + spaceSize);
	totalHeight += (height + spaceSize);

	int y = bottomBoxYCoordinate - height;
	int letterY = y + charHeight/2;

	gl_draw_string(x, letterY - charHeight, "User: ", GL_BLACK);
	
	char identifier[2];
	snprintf(identifier, 2, "%d", id);
   
   	//draws in id above text box
    gl_draw_char(x + 5 * charWidth, letterY - charHeight, identifier[0], GL_BLACK);
	gl_draw_rect(x, y + charHeight, width + spaceSize, height - spaceSize, c);

	letterY += (charHeight + spaceSize);

	//draws in the message
    while(*str != '\0'){
    	int counter = 0;
    	int value = 0;
    	while(counter < colNum){
	        gl_draw_char(x + spaceSize + value, letterY - spaceSize, (int)*str, GL_BLACK);
	        value += charWidth;
	        (void) *str++;
	        counter++;
    	}   	
    	letterY += charHeight;   	
    }
}


void scrollBoxes(int scrollSize){
	int x1 = (charWidth * (maxBoxWidth + 1)+ spaceSize*2);
	for(int i = 0; i < x1; i++){
		for(int j = 0; j < bottomBoxYCoordinate; j++){
			newLocation = j + scrollSize;
			if(newLocation < bottomBoxYCoordinate){
				gl_draw_pixel(i, j, gl_read_pixel(i, newLocation));
				gl_draw_pixel(i, newLocation, GL_WHITE);
			}	
		}
	}

	for(int i = screenWidth - x1; i < screenWidth; i++){
		for(int j = 0; j < bottomBoxYCoordinate; j++){
			newLocation = j + scrollSize;
			if(newLocation < bottomBoxYCoordinate){
				gl_draw_pixel(i, j, gl_read_pixel(i, newLocation));
				gl_draw_pixel(i, newLocation, GL_WHITE);
			}	
		}
	}
}