#include <stdarg.h>
#include "uart.h"

volatile unsigned short * const SCSCR = (unsigned short *)0xFFFEB808;
volatile unsigned char * const SCFTDR = (unsigned char *)0xFFFEB80C;
volatile unsigned short * const SCFSR = (unsigned short *)0xFFFEB810;
volatile unsigned char * const SCFRDR = (unsigned char *)0xFFFEB814;
volatile unsigned short * const SCFCR = (unsigned short *)0xFFFEB818;
volatile unsigned short * const SCBRR = (unsigned short *)0xFFFEB804;

#define TDFE (1 << 5)
#define RE (1 << 4)
#define TE (1 << 5)
#define DATA_BUF_LEN   12

void uart_putchar (U8 ch)
{
	// Spin until to the UART FIFO is empty
	while (!(*SCFSR & TDFE));
	*SCFTDR = ch;
	*SCFSR &= ~TDFE;
	*SCSCR |= TE;
	while (!(*SCFSR & TDFE));
}

int uart_getchar_timeout( int timeout )
{
	int data;
	if( timeout >= 0 )
	{
		timeout *= 1000;
		while( timeout > 0 && !(*SCFSR & 2) )
			--timeout;
		data = -1;
		if( *SCFSR & 2 )
		{
			data = *SCFRDR;
			*SCFSR &= ~2;
		}
	}
	else
	{
		while( !(*SCFSR & 2) );
		data = *SCFRDR;
		*SCFSR &= ~2;
	}
	return data;
}

void uart_init()
{
	*SCSCR = RE;
	*SCFCR = 0x30;
}

void uart_put_string (const U8 *data_string)
{
	while(*data_string) uart_putchar (*data_string++);
}

U8 uart_mini_printf(char *format, ...)
{
    va_list arg_ptr;
    U8      *p,*sval;
    U8      u8_temp, n_sign, data_idx, min_size;
    U8      data_buf[DATA_BUF_LEN];
    S8      long_flag, alt_p_c;
    S8      s8_val;
    S16     s16_val;
    S32     s32_val;
    U16     u16_val;
    U32     u32_val;

    long_flag = FALSE;
    alt_p_c = FALSE;
    min_size = DATA_BUF_LEN-1;

    va_start(arg_ptr, format);   // make arg_ptr point to the first unnamed arg
    for (p = (U8 *) format; *p; p++)
    {
        if ((*p == '%') || (alt_p_c == TRUE))
        {
            p++;
        }
        else
        {
            uart_putchar(*p);
            alt_p_c = FALSE;
            long_flag = FALSE;
            continue;   // "switch (*p)" section skipped
        }
        switch (*p)
        {
            case 'c':
                if (long_flag == TRUE)      // ERROR: 'l' before any 'c'
                {
                    uart_putchar('l');
                    uart_putchar('c');
                }
                else
                {
                    s8_val = (S8)(va_arg(arg_ptr, int));    // s8_val = (S8)(va_arg(arg_ptr, S16));
                    uart_putchar((U8)(s8_val));
                }
                // Clean up
                min_size = DATA_BUF_LEN-1;
                alt_p_c = FALSE;
                long_flag = FALSE;
                break; // case 'c'
                
            case 's':
                if (long_flag == TRUE)      // ERROR: 'l' before any 's'
                {
                    uart_putchar('l');
                    uart_putchar('s');
                }
                else
                {
                    for (sval = va_arg(arg_ptr, U8 *); *sval; sval++)
                    {
                        uart_putchar(*sval);
                    }
                }
                // Clean up
                min_size = DATA_BUF_LEN-1;
                alt_p_c = FALSE;
                long_flag = FALSE;
                break;  // case 's'
                
            case 'l':  // It is not the number "ONE" but the lower case of "L" character
                if (long_flag == TRUE)      // ERROR: two consecutive 'l'
                {
                    uart_putchar('l');
                    alt_p_c = FALSE;
                    long_flag = FALSE;
                }
                else
                {
                    alt_p_c = TRUE;
                    long_flag = TRUE;
                }
                p--;
                break;  // case 'l'
                
            case 'd':
                n_sign  = FALSE;               
                for(data_idx = 0; data_idx < (DATA_BUF_LEN-1); data_idx++)
                {
                    data_buf[data_idx] = '0';
                }
                data_buf[DATA_BUF_LEN-1] = 0;
                data_idx = DATA_BUF_LEN - 2;
                if (long_flag)  // 32-bit
                {
                    s32_val = va_arg(arg_ptr, S32);
                    if (s32_val < 0)
                    {
                        n_sign = TRUE;
                        s32_val  = -s32_val;
                    }
                    while (1)
                    {
                        data_buf[data_idx] = s32_val % 10 + '0';
                        s32_val /= 10;
                        data_idx--;
						if (s32_val==0) break;
                   }
                }
                else  // 16-bit
                {
                    s16_val = (S16)(va_arg(arg_ptr, int)); // s16_val = va_arg(arg_ptr, S16);
                    if (s16_val < 0)
                    {
                        n_sign = TRUE;
                        s16_val  = -s16_val;
                    }
                    while (1)
                    {
                        data_buf[data_idx] = s16_val % 10 + '0';
                        s16_val /= 10;
                        data_idx--;
						if (s16_val==0) break;
                    }
                }
                if (n_sign) { uart_putchar('-'); }
                data_idx++;
                if (min_size < data_idx)
                {
                    data_idx = min_size;
                }
                uart_put_string (data_buf + data_idx);
                // Clean up
                min_size = DATA_BUF_LEN-1;
                alt_p_c = FALSE;
                long_flag = FALSE;
                break;  // case 'd'
                
            case 'u':
                for(data_idx = 0; data_idx < (DATA_BUF_LEN-1); data_idx++)
                {
                    data_buf[data_idx] = '0';
                }
                data_buf[DATA_BUF_LEN-1] = 0;
                data_idx = DATA_BUF_LEN - 2;
                if (long_flag)  // 32-bit
                {
                    u32_val = va_arg(arg_ptr, U32);
                    while (1)
                    {
                        data_buf[data_idx] = u32_val % 10 + '0';
                        u32_val /= 10;
                        data_idx--;
						if (u32_val==0) break;
                    }
                }
                else  // 16-bit
                {
                    u16_val = (U16)(va_arg(arg_ptr, int)); // u16_val = va_arg(arg_ptr, U16);
                    while (1)
                    {
                        data_buf[data_idx] = u16_val % 10 + '0';
                        data_idx--;
                        u16_val /= 10;
						if (u16_val==0) break;
                    }
                }
                data_idx++;
                if (min_size < data_idx)
                {
                    data_idx = min_size;
                }
                uart_put_string (data_buf + data_idx);
                // Clean up
                min_size = DATA_BUF_LEN-1;
                alt_p_c = FALSE;
                long_flag = FALSE;
                break;  // case 'u':
                
            case 'x':
            case 'X':
                for(data_idx = 0; data_idx < (DATA_BUF_LEN-1); data_idx++)
                {
                    data_buf[data_idx] = '0';
                }
                data_buf[DATA_BUF_LEN-1] = 0;
                data_idx = DATA_BUF_LEN - 2;
                if (long_flag)  // 32-bit
                { 
                    u32_val = va_arg(arg_ptr, U32);
                    while (u32_val)
                    {
                        u8_temp = (U8)(u32_val & 0x0F);
                        data_buf[data_idx] = (u8_temp < 10)? u8_temp+'0':u8_temp-10+(*p=='x'?'a':'A');
                        u32_val >>= 4;
                        data_idx--;
                    }
                }
                else  // 16-bit
                {
                    u16_val = (U16)(va_arg(arg_ptr, int)); // u16_val = va_arg(arg_ptr, U16);
                    while (u16_val)
                    {
                        u8_temp = (U8)(u16_val & 0x0F);
                        data_buf[data_idx] = (u8_temp < 10)? u8_temp+'0':u8_temp-10+(*p=='x'?'a':'A');
                        u16_val >>= 4;
                        data_idx--;
                    }
                }
                data_idx++;
                if (min_size < data_idx)
                {
                    data_idx = min_size;
                }
                uart_put_string (data_buf + data_idx);
                // Clean up
                min_size = DATA_BUF_LEN-1;
                alt_p_c = FALSE;
                long_flag = FALSE;
                break;  // case 'x' & 'X'
                
            case '0':   // Max allowed "min_size" 2 decimal digit, truncated to DATA_BUF_LEN-1.
                min_size = DATA_BUF_LEN-1;
                if (long_flag == TRUE)      // ERROR: 'l' before '0'
                {
                    uart_putchar('l');
                    uart_putchar('0');
                    // Clean up
                    alt_p_c = FALSE;
                    long_flag = FALSE;
                    break;
                }
                u8_temp = *++p;
                if ((u8_temp >='0') && (u8_temp <='9'))
                {
                    min_size = u8_temp & 0x0F;
                    u8_temp = *++p;
                    if ((u8_temp >='0') && (u8_temp <='9'))
                    {
                        min_size <<= 4;
                        min_size |= (u8_temp & 0x0F);
                        p++;
                    }
                    min_size = ((min_size & 0x0F) + ((min_size >> 4) *10));  // Decimal to hexa
                    if (min_size > (DATA_BUF_LEN-1))
                    {
                        min_size = (DATA_BUF_LEN-1);
                    }  // Truncation
                    min_size = DATA_BUF_LEN-1 - min_size;  // "min_size" formatted as "data_ix"
                }
                else      // ERROR: any "char" after '0'
                {
                    uart_putchar('0');
                    uart_putchar(*p);
                    // Clean up
                    alt_p_c = FALSE;
                    long_flag = FALSE;
                    break;
                }
                p-=2;
                alt_p_c = TRUE;
                // Clean up
                long_flag = FALSE;
                break;  // case '0'
                
            default:
                if (long_flag == TRUE)
                {
                    uart_putchar('l');
                }
                uart_putchar(*p);
                // Clean up
                min_size = DATA_BUF_LEN-1;
                alt_p_c = FALSE;
                long_flag = FALSE;
                break;  // default
                
        }   // switch (*p ...
        
    }   // for (p = ...
    
    va_end(arg_ptr);
    return 0;
}
