#ifndef CONSOLE_H
#define CONSOLE_H

/*
 * Interface to a text console displayed on the screen.
 *
 * You implement this interface in assignment 6.
 *
 * Author: Pat Hanrahan <hanrahan@cs.stanford.edu>
 * Author: Philip Levis <pal@cs.stanford.edu>
 * Date: 3/24/16
 */


/*
 * Initialize the console.
 *
 * @param nrows the requested number of rows in characters
 * @param ncols the requested number of columns in characters
 */
void console_init(unsigned int nrows, unsigned int ncols);

/*
 * Set all the characters in the console to ' ' and move the
 * cursor to the home position.
 */
void console_clear(void);

/*
 * Outputs a character to the console at the current cursor position
 * and advance the cursor.
 *
 * When processing characters, interpret the following special characters:
 *
 * '\n' :  move down to the beginning of next line
 * '\b' :  back space (deletes last character)
 * '\f' :  form feed, clear the console and set the cursor to the home position
 *
 * @param ch   the character to write to the console.
 * @return     the character written
 */
int console_putchar(int ch);

/* 
 * Print the formatted string to the console. The arguments to this function
 * are the same as `printf`.
 *
 * @return the number of characters written to the console.
 */
int console_printf(const char *format, int senderid, ...);

#endif