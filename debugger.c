#include "uart.h"

void menu(U32 *regs);
int xmodemReceive(unsigned char *dest, int destsz);
int xmodemTransmit(unsigned char *src, int srcsz);
void ResetGPIO();
void SetupMemory();
void intr();

volatile unsigned int * const BRK_INST = (unsigned int *)0x87fffe0;
volatile unsigned short * const OLD_INST = (unsigned short *)0x87fffe4;
volatile unsigned int * const CCR1 = (unsigned int *)0xFFFC1000;

#define DISABLE_INTERRUPTS() \
        asm volatile( "stc      sr, r0\n\t"             \
                      "movi20   #0xFFFFFF0F, r4\n\t"    \
                      "and      r4, r0\n\t"             \
                      "or       #0xF0, r0\n\t"          \
                      "ldc      r0, sr"                 \
                     )

void watchdogDisable()
{
	*((unsigned short *)0xFFFE0000) = 0xa518;
}

void entry()
{
	DISABLE_INTERRUPTS();
	watchdogDisable();
	uart_init();
	ResetGPIO();

	*BRK_INST = 0x55555555; // Junk address
	uart_put_string((U8 *)"\r\nUW SuperH Debugger\r\n");

	menu(0);
}

void intrEntry(U32 j1, U32 j2, U32 j3, U32 j4, U32 pr, U32 r0, U32 r1, U32 r2, U32 r3, 
			   U32 r4, U32 r5, U32 r6, U32 r7, U32 r8, U32 r9, U32 r10, U32 r11,
			   U32 r12, U32 r13, U32 r14, U32 r15, U32 pc, U32 sr)
{
	DISABLE_INTERRUPTS();
	watchdogDisable();
	uart_init();

	if ((pc - 2) == *BRK_INST) {
		pc -= 2;

		uart_mini_printf("\r\nRestoring instruction at %08lx...\r\n", pc);
		// Bypass the cache when writing to memory
		*((unsigned short *)(pc | 0x20000000)) = *OLD_INST;
		// Flush the icache
		*CCR1 |= 0x800;

		// Update the return PC on the stack
		*(unsigned int *)r15 = pc;
	}

	uart_mini_printf("\r\n");
	uart_mini_printf("   r0 = %08lx,  r1 = %08lx,  r2 = %08lx,  r3 = %08lx,\r\n", r0, r1, r2, r3);
	uart_mini_printf("   r4 = %08lx,  r5 = %08lx,  r6 = %08lx,  r7 = %08lx,\r\n", r4, r5, r6, r7);
	uart_mini_printf("   r8 = %08lx,  r9 = %08lx, r10 = %08lx, r11 = %08lx,\r\n", r8, r9, r10, r11);
	uart_mini_printf("  r12 = %08lx, r13 = %08lx, r14 = %08lx, r15 = %08lx,\r\n", r12, r13, r14, r15 + 8);
	uart_mini_printf("   pc = %08lx,  pr = %08lx,  sr = %08lx\r\n", pc, pr, sr);

	menu((U32 *)(r15 - (16 * 4)));
}

unsigned int atoiHex(char *str)
{
	unsigned int sum = 0;

	for (;;) {
		if (*str >= '0' && *str <= '9') {
			sum <<= 4;
			sum += *str - '0';
		} else if (*str >= 'a' && *str <= 'f') {
			sum <<= 4;
			sum += *str - 'a' + 10;
		} else if (*str >= 'A' && *str <= 'F') {
			sum <<= 4;
			sum += *str - 'A' + 10;
		} else {
			return sum;
		}
		str++;
	}
}

U8 handleCmd(char *str, U32 *regs)
{
	unsigned int addr;
	unsigned int value;
	unsigned int i;
	unsigned char type;

	unsigned int bar;
	unsigned int mask;
	unsigned short bbr;

	unsigned int *p;

	if (*str == 'w') {
		type = *++str;

		for (; *str != ' '; str++);
		str++;

		addr = atoiHex(str);

		for (; *str != ' '; str++);
		str++;

		value = atoiHex(str);

		if (type == 'b') {
			*((unsigned char *)addr) = value;
		} else if (type == 'w') {
			*((unsigned short *)addr) = value;
		} else if (type == 'l') {
			*((unsigned int *)addr) = value;
		} else {
			uart_mini_printf("Unknown write type '%c'\r\n", type);
		}
	} else if (*str == 'r' && str[1] == ' ') {
		for (; *str != ' '; str++);
		str++;

		addr = atoiHex(str);

		for (; *str != ' '; str++);
		str++;

		value = atoiHex(str);

		for (i = 0; i < value; i++) {
			uart_mini_printf("%02x ", *((unsigned char *)addr));
			addr++;
			if ((i & 0xf) == 0xf) {
				uart_putchar('\r');
				uart_putchar('\n');
			}

		}

		if ((i & 0xf) != 0xf) {
			uart_putchar('\r');
			uart_putchar('\n');
		}
	} else if (*str == 'r' && str[1] == 'w') {
		for (; *str != ' '; str++);
		str++;

		addr = atoiHex(str);

		for (; *str != ' '; str++);
		str++;

		value = atoiHex(str);

		for (i = 0; i < value; i++) {
			uart_mini_printf("%04x ", *((unsigned short *)addr));
			addr += 2;
			if ((i & 0x7) == 0x7) {
				uart_putchar('\r');
				uart_putchar('\n');
			}

		}

		if ((i & 0x7) != 0x7) {
			uart_putchar('\r');
			uart_putchar('\n');
		}
	} else if (*str == 'r' && str[1] == 'l') {
		for (; *str != ' '; str++);
		str++;

		addr = atoiHex(str);

		for (; *str != ' '; str++);
		str++;

		value = atoiHex(str);

		for (i = 0; i < value; i++) {
			uart_mini_printf("%08x ", *((unsigned int *)addr));
			addr += 4;
			if ((i & 0x3) == 0x3) {
				uart_putchar('\r');
				uart_putchar('\n');
			}

		}

		if ((i & 0x3) != 0x3) {
			uart_putchar('\r');
			uart_putchar('\n');
		}
	} else if (*str == 'c') {
		for (; *str != ' '; str++);
		str++;

		addr = atoiHex(str);

		uart_mini_printf("Calling %08lx...\r\n", addr);

		((void (*)())addr)();
	} else if (*str == 'j') {
		for (; *str != ' '; str++);
		str++;

		addr = atoiHex(str);

		for (; *str != ' '; str++);
		str++;

		value = atoiHex(str);

		uart_mini_printf("Jumping to %08lx with r15 = %08lx...\r\n", addr, value);

		asm volatile("mov %1, r15\n\t" \
					 "jmp @%0\n\t" \
					 "nop" :: "r"(addr), "r"(value) );

	} else if (*str == '!') {
		uart_mini_printf("Rebooting...\r\n");

		asm("mov #0, r1\n" \
			"mov.l @r1+, r0\n" \
			"mov.l @r1, r15\n" \
			"jmp @r0\n" \
			"nop");
	} else if (*str == 'n') {
		return 1;
	} else if (*str == 'i') {
		uart_mini_printf("Initializing memory and setting exception handlers...\r\n");

		SetupMemory();

		*((unsigned short *)0x81e76be) = 0x2f96;
		*((unsigned int *)0x81e76c0) = 0x2fa62fb6;
		*((unsigned short *)0x81e76c4) = 0x2fe6;

		for (p = (unsigned int *)0xfff80010; p <= (unsigned int *)0xfff80080; p++)
			*p = (unsigned int)intr;
	} else if (*str == 'v') {
		for (; *str != ' '; str++);
		str++;

		addr = atoiHex(str);

		asm volatile("ldc %0, vbr" :: "r"(addr) );
	} else if (*str == 'b') {
		for (; *str != ' '; str++);
		str++;

		addr = atoiHex(str);
		
		*BRK_INST = addr;
		*OLD_INST = *(unsigned short *)addr;
		*(unsigned short *)(addr | 0x20000000) = 0xc320; // TRAP #20
		// Flush the icache
		*CCR1 |= 0x800;
		
		uart_mini_printf("Breakpoint set at %08lx\r\n", addr);
	} else if (*str == 't') {
		asm volatile("stc vbr, %0" : "=r"((value)) :: "r0", "r4");
		uart_mini_printf("VBR = %08lx\r\n", value);

		asm volatile("stc sr, %0" : "=r"((value)) :: "r0", "r4");
		uart_mini_printf("SR = %08lx\r\n", value);

		uart_mini_printf("BAR = %08lx, BAMR = %08lx, BBR = %08lx\r\n", 
			*((volatile unsigned int *)0xFFFC0400),
			*((volatile unsigned int *)0xFFFC0404),
			*((volatile unsigned short *)0xFFFC04a0));
	} else if (*str == 'x') {
		for (; *str != ' '; str++);
		str++;

		addr = atoiHex(str);
		
		for (; *str != ' '; str++);
		str++;

		value = atoiHex(str);

		xmodemReceive((unsigned char *)addr, value);
	} else if (*str == 'q') {
		addr = 0xdced8; // Entry point
		value = 0xfff87f00;// Stack pointer
		asm volatile("mov %1, r15\n\t" \
					 "jmp @%0\n\t" \
					 "nop" :: "r"(addr), "r"(value) );
	} else if (*str == 's') {
		for (; *str != ' '; str++);
		str++;

		addr = atoiHex(str);
		
		for (; *str != ' '; str++);
		str++;

		value = atoiHex(str);
		regs[addr] = value;
	} else if (*str == 'd') {
		for (; *str != ' '; str++);
		str++;

		addr = atoiHex(str);
		
		for (; *str != ' '; str++);
		str++;

		value = atoiHex(str);
		xmodemTransmit((unsigned char *)addr, value);
	} else {
		uart_mini_printf("Unknown command.\r\n");
	}

	return 0;
}

void menu(U32 *regs)
{
	char inbuf[64]; // lulz
	char inChar;
	int bufPtr;

	uart_put_string((U8 *)"> ");

	bufPtr = 0;
	inbuf[0] = 0;

	for (;;) {	
		inChar = uart_getchar();
		if (inChar == 8) { // Backspace
			if (bufPtr > 0) {
				uart_putchar(inChar);
				bufPtr--;
			}
		} else if (inChar == 13) {
			uart_putchar('\r');
			uart_putchar('\n');
			inbuf[bufPtr] = 0;

			if (handleCmd(inbuf, regs))
				return;

			uart_put_string((U8 *)"> ");
			bufPtr = 0;
		} else {
			if (bufPtr < 63) {
				inbuf[bufPtr] = inChar;
				bufPtr++;
				uart_putchar(inChar);
			}
		}
	}
}