#ifndef SHELLCHAT_H
#define SHELLCHAT_H

/*
 * Interface to the CS107E shell. You implement the beginnings
 * of your shell in assignment 5 and complete it in assignment 7.
 *
 * Author: Julie Zelenski <zelenski@cs.stanford.edu>
 * Date: Februrary 2016
 */

typedef int (*formatted_fn_t)(const char *format, int senderid, ...);
typedef int (*char_fn_t)(int ch);

/*
 * shell_init
 * ==========
 *
 * Initialize the shell
 *
 * Takes two *function pointers*, `printf_fn` and `putchar_fn`, that are used
 * to configure where the shell directs its output.
 *
 * Arguments:
 *   * `printf_fn` - used for the formatted output
 *   * `putchar_fn` - used to output a single `char`
 *
 * Example usage:
 *   * `shell_init(printf, uart_putchar)`
 *   * `shell_init(console_printf, console_putchar)`
 */
void shell_chat_init(formatted_fn_t print_fn, char_fn_t putchar_fn);

/*
 * shell_readline
 * ==============
 *
 * Reads a single line of input from the user. Store the characters typed on
 * the keyboard and stores them into a buffer `buf` of size `bufsize`. Reading
 * stops when the user types enter (\n).
 *
 * When the user types backspace (\b):
 *   If there are any characters currently in the buffer, deletes the last one.
 *   Otherwise, calls `shell_bell`.
 */
void shell_chat_readline(char buf[], int bufsize);

/* 
 * shell_run
 * =========
 *
 * Main function of the shell module, must be called after `shell_init`.
 * Enters a read, eval, print loop that uses the  `shell_readline`, and
 * `shell_evaluate`, `printf_fn` (argument to `shell_init`).
 */
void shell_chat_run(void);


#endif