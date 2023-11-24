#include "compiler.h"

void uart_putchar(U8 ch);
#define uart_getchar() uart_getchar_timeout( -1 )
int uart_getchar_timeout( int timeout );
void uart_init();
void uart_put_string (const U8 *data_string);
U8 uart_mini_printf(char *format, ...);
