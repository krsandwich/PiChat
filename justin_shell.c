#include "shell.h"
#include "shell_commands.h"
#include <stddef.h>
#include "keyboard.h"
#include "strings.h"
#include "malloc.h"
#include "pi.h"
#include "ps2.h"
#include "gprof.h"

#define LINE_LEN 80
#define CTRL_A_KEY 200
#define CTRL_E_KEY 201
#define CTRL_U_KEY 202
#define MAX_HISTORY 10

static int (*shell_printf)(const char * format, ...);
static int (*shell_putchar)(int ch);
static char* history[MAX_HISTORY];
static int historyNums[MAX_HISTORY];
static int histSize = 0;
static int commandNum = 1;

//function declaration for history
int cmd_history(int argc, const char *argv[]);
int cmd_profile(int argc, const char *argv[]);

#define COMMAND_LEN 7
static const command_t commands[] = {
    {"help",   "<cmd> prints a list of commands or description of cmd", cmd_help},
    {"echo",   "<...> echos the user input to the screen", cmd_echo},
    {"reboot", "reboot the Raspberry Pi back to the bootloader", cmd_reboot},
    {"peek", "<hex_addr> prints the 4-byte value stored at hex_addr", cmd_peek},
    {"poke", "<hex_addr> <hex_value> stores the 4-byte value hex_value at hex_addr", cmd_poke},
    {"history", "prints out the last 10 commands (or fewer of not available)", cmd_history},
    {"profile", "profile [on | off | status | results]", cmd_profile},
};

int cmd_echo(int argc, const char *argv[]) 
{
    for (int i = 1; i < argc; ++i) 
        shell_printf("%s ", argv[i]);
    shell_printf("\n");
    return 0;
}

int cmd_help(int argc, const char *argv[]) 
{
    if(argc == 1){
        for (int i=0; i<COMMAND_LEN; i++){
            shell_printf("%s: %s\n", commands[i].name, commands[i].description);
        }
    }
    // only does the first command listed (behavior undefined for more than 1)
    else{
        int found = 0;

        for (int i=0; i<COMMAND_LEN; i++){
            if(strcmp(argv[1], commands[i].name) == 0){
                found = 1;
                shell_printf("%s: %s\n", commands[i].name, commands[i].description);
                break;
            }
        }
        if(!found){
            shell_printf("error: no such command \'%s\'\n", argv[1]);
            return 1;
        }
    }
    return 0;
}

int cmd_reboot(int argc, const char *argv[]){
    pi_reboot();
    return 0;
}

int cmd_peek(int argc, const char *argv[]){
    if(argc != 2){
        shell_printf("error: peek expects 1 argument [hex address]\n");
        return 1;
    }

    char *end;

    int address = strtou(argv[1], &end, 16);

    if(*end != '\0'){
        shell_printf("error: peek cannot convert \'%s\'\n", argv[1]);
        return 1;
    }

    if(address % 4 != 0){
        shell_printf("error: peek address must be 4-byte aligned\n");
        return 1;
    }

    int *ptr = (int *)address;

    shell_printf("%p:  %08x\n", ptr, *ptr);

    return 0;
}

int cmd_poke(int argc, const char *argv[]){
    if(argc != 3){
        shell_printf("error: poke expects 2 argument [hex address]\n");
        return 1;
    }

    char *end;

    int address = strtou(argv[1], &end, 16);

    if(*end != '\0'){
        shell_printf("error: poke cannot convert \'%s\'\n", argv[1]);
        return 1;
    }

    int value = strtou(argv[2], &end, 16);

    if(*end != '\0'){
        shell_printf("error: poke cannot convert \'%s\'\n", argv[2]);
        return 1;
    }

    if(address % 4 != 0){
        shell_printf("error: poke address must be 4-byte aligned\n");
        return 1;
    }

    int *ptr = (int *)address;

    *ptr = value;

    return 0;
}

int cmd_history(int argc, const char *argv[]){
    for(int i=0; i<histSize; i++){
        shell_printf("  %d %s\n", historyNums[i], history[i]);
    }

    return 0;
}

int cmd_profile(int argc, const char *argv[]){
    if(argc != 2){
        shell_printf("error: profile expects 1 argument [on | off | status | results]\n");
        return 1;
    }

    if(strcmp(argv[1], "on") == 0){
        gprof_on();
    }
    else if(strcmp(argv[1], "off") == 0){
        gprof_off();
    }
    else if(strcmp(argv[1], "status") == 0){
        if(gprof_is_active()){
            shell_printf("Status: on\n");
        }
        else{
            shell_printf("Status: off\n");
        }
    }
    else if(strcmp(argv[1], "results") == 0){
        gprof_dump();
    }
    else{
        shell_printf("error: profile could not handle %s\n", argv[1]);
        return 1;
    }

    return 0;
}

void shell_init(formatted_fn_t print_fn, char_fn_t putchar_fn)
{
    shell_printf = print_fn;
    shell_putchar = putchar_fn;
}

void shell_bell(void)
{
    shell_putchar('\a');
}

void print_command_string_end(char buf[], int index, int size){
    for(int i=index; i <size; i++){
        shell_putchar(buf[i]);
    }

    //space at end because of potentail delete in middle of word
    shell_putchar(' ');

    for(int i=0; i< size - index + 1; i++){
        shell_putchar('\b');
    }
}

void print_command_string_full(char buf[], int oldIndex, int newIndex, int oldSize, int newSize){
    //go to the beginning
    for(int i=0; i<oldIndex; i++){
        shell_putchar('\b');
    }

    //print out string
    for(int i=0; i<newSize; i++){
        shell_putchar(buf[i]);
    }

    //make sure end is cleared with spaces
    for(int i=0; i<oldSize-newSize; i++){
        shell_putchar(' ');
    }

    //go back to end of new string
    for(int i=0; i<oldSize-newSize; i++){
        shell_putchar('\b');
    }

    //go to newIndex
    for(int i=0; i<newSize-newIndex; i++){
        shell_putchar('\b');
    }
}

void insert_character(char buf[], char ch, int index, int size){
    //move everything to the right
    for(int i=size; i>index; i--){
        buf[i] = buf[i-1];
    }

    buf[index] = ch;
}

void delete_character(char buf[], int index, int size){
    //move everything to the left
    for(int i=index; i<size; i++){
        buf[i-1] = buf[i];
    }
}

void shell_readline(char buf[], int bufsize)
{
    int index = 0;
    int size = 0;
    char ch;
    int historyIndex = histSize;

    // caches bottom line and size for history scrolling
    char* bottomLine = malloc(bufsize);
    int bottomSize = 0;

    while(size < bufsize-1 && (ch = keyboard_read_next()) != '\n'){
        if(ch == '\b'){ //backspace
            if(index == 0){
                shell_bell();
            }
            else{
                delete_character(buf, index, size);
                index--;
                size--;
                shell_putchar('\b');
                print_command_string_end(buf, index, size);
            }
        }
        else if(ch == PS2_KEY_NUMPAD_4){ //left
            if(index > 0){
                shell_putchar('\b');
                index--;
            }
            else{
                shell_bell();
            }
        }
        else if(ch == PS2_KEY_NUMPAD_6){ //right 
            if(index == size){
                shell_bell();
            }
            else{
                shell_putchar(buf[index]);
                index++;
            }
        }
        // NOTE: although bash usually allows the history to be rewritten when scrolling
        // our implementation does not. So, if someone scrolls to a certain past statement
        // changes it, scrolls away, and then scrolls back, it will not persist their earlier
        // changes. Although this is different from bash, this implmenetation is quite easier
        // and philosophically upholds the tenet that history shouldn't be altered :)
        else if(ch == PS2_KEY_NUMPAD_8){ //up
            if(historyIndex == 0){
                shell_bell();
            }
            else{
                // if currently on bottom line, save it
                if(historyIndex == histSize){
                    memcpy(bottomLine, buf, size);
                    bottomSize = size;
                }
                historyIndex--;
                memcpy(buf, history[historyIndex], strlen(history[historyIndex]));

                int oldIndex = index;
                int oldSize = size;

                size = strlen(history[historyIndex]);
                index = size;

                print_command_string_full(buf, oldIndex, index, oldSize, size);
            }
        }
        else if(ch == PS2_KEY_NUMPAD_2){ //down
            if(historyIndex == histSize){
                shell_bell();
            }
            else{
                historyIndex++;
                int oldIndex = index;
                int oldSize = size;

                if(historyIndex == histSize){
                    memcpy(buf, bottomLine, bottomSize);
                    size = bottomSize;
                    index = size;
                }
                else{
                    memcpy(buf, history[historyIndex], strlen(history[historyIndex]));
                    size = strlen(history[historyIndex]);
                    index = size;
                }

                print_command_string_full(buf, oldIndex, index, oldSize, size);
            }
        }
        else if(ch == CTRL_E_KEY){
            for(int i= index; i<size; i++){
                shell_putchar(buf[index]);
                index++;
            }
        }
        else if(ch == CTRL_A_KEY){
            for(int i= index; i>0; i--){
                shell_putchar('\b');
                index--;
            }
        }
        else if(ch == CTRL_U_KEY){
            int oldIndex = index;
            int oldSize = size;
            size = 0;
            index = 0;
            print_command_string_full(buf, oldIndex, index, oldSize, size);
        }
        else if(ch < PS2_KEY_SHIFT && ch != PS2_KEY_NONE){
            insert_character(buf, ch, index, size);
            shell_putchar(ch);
            index++;
            size++;
            print_command_string_end(buf, index, size);
        }
    }

    shell_putchar('\n');
    buf[size] = '\0';
    free(bottomLine);
}

static int isWhitespace(char c){
    return (c == ' ') || (c == '\t'); // || (c == '\r') || (c == '\f') || (c == '\v'); don't think i need the rest
}

static int countTokens(const char* line){
    int count = 0;
    int index = 0;
    int inWord = 0;

    while(index < strlen(line)){
        if(!isWhitespace(line[index]) && !inWord){
            count++;
            inWord = 1;
        }
        else if(isWhitespace(line[index]) && inWord){
            inWord = 0;
        }
        index++;
    }

    return count;
}
static int lenWord(const char *word){
    int i=0;

    while(!isWhitespace(word[i]) && word[i] != '\0'){
        i++;
    }

    return i;
}
static const char** tokenizeLine(const char* line){
    int numTokens = countTokens(line);
    char** argv = malloc(sizeof(char *) * numTokens);

    int tokenNum=0;
    int index = 0;

    while(index < strlen(line)){
        if(isWhitespace(line[index])){
            index++;
        }

        else{
            int length = lenWord(line + index);
            argv[tokenNum] = malloc(length + 1);
            memcpy(argv[tokenNum], line + index, length);
            argv[tokenNum][length] = '\0';
            tokenNum++;
            index+= length;
        }
    }

    return (const char**) argv;
}

static void addToHistory(const char *line){
    char* copy = malloc(strlen(line) + 1);
    memcpy(copy, line, strlen(line) + 1);

    if(histSize < MAX_HISTORY){
        history[histSize] = copy;
        historyNums[histSize] = commandNum;
        histSize++;
    }
    else{
        free(history[0]);

        for(int i=0; i<MAX_HISTORY-1; i++){
            history[i] = history[i+1];
            historyNums[i] = historyNums[i+1];
        }

        history[MAX_HISTORY-1] = copy;
        historyNums[MAX_HISTORY-1] = commandNum;
    }
}

int shell_evaluate(const char *line)
{
    int argc = countTokens(line);

    if(argc == 0){
        return 0;
    }

    int returnVal = 1;
    int foundCommand = 0;

    const char** argv = tokenizeLine(line);
    addToHistory(line);

    for(int i=0; i<COMMAND_LEN; i++){
        if(strcmp(argv[0], commands[i].name) == 0){
            returnVal =  commands[i].fn(argc, argv);
            foundCommand=1;
        }
    }
    
    if(!foundCommand){
        shell_printf("error: no such command \'%s\'\n", argv[0]);
    }

    //free memory
    for(int i=0;i<argc; i++){
        free((void *) argv[i]);
    }

    free((void *) argv);

    return returnVal;
}

void shell_run(void)
{
    shell_printf("Welcome to the CS107E shell. Remember to type on your PS/2 keyboard!\n");
    while (1) 
    {
        char line[LINE_LEN];

        shell_printf("[%d] Pi> ", commandNum);
        shell_readline(line, sizeof(line));
        shell_evaluate(line);
        commandNum++;
    }
}