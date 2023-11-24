SHCC = sh-linux-gnu-gcc
SHCFLAGS = -mb -Os -ffreestanding -fno-builtin
SHLDFLAGS = -mb -nostdlib -nodefaultlibs -Wl,-T,linker.script
CFLAGS = -O3 
BINS = debugger.bin
OBJS = intr.o uart.o debugger.o crc16.o xmodem.o

.PHONY: all clean
.SECONDARY: $(OBJS)

all: $(BINS)

clean:
	$(RM) $(BINS) $(OBJS)

%.o: %.c
	$(SHCC) $(SHCFLAGS) -c -o $@ $<

%.o: %.s
	$(SHCC) $(SHCFLAGS) -c -o $@ $<

debugger.bin: $(OBJS) linker.script
	$(SHCC) $(SHLDFLAGS) -o $@ $(OBJS)
