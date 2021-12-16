#include "gpio.h"
#include "gpioextra.h"
#include "keyboard.h"
#include "console.h"
#include "shell.h"
#include "printf.h"
#include "timer.h"
#include "chat.h"
#include "protocol.h"
#include "myID.h"
#include "rbPacket.h"
#include <stdbool.h>
#include "packet.h"
#include "strings.h"
#include "interrupts.h"
#include "strings.h"
#include "assert.h"
#include "uart.h"

#define NROWS 20
#define NCOLS 40

void main(void)
{
   gpio_init();
	protocol_init();
    keyboard_init();
    console_init(NROWS, NCOLS);
    shell_init(console_printf, console_putchar);
    interrupts_global_enable();
    shell_run();
}

