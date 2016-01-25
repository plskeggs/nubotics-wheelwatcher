PRG            = ww11
OBJ            = ww11.o USI_UART/USI_UART.o USI_I2C/USI_TWI_Slave.o NewestLoader/main.o NewestLoader/assembly.o ww_settings.o
ASRC 	       = assembly.S
MCU_TARGET     = attiny85
OPTIMIZE       = -O2
AVRDUDE        = avrdude
# Output format. (can be srec, ihex, binary)
FORMAT         = ihex
DEFS           =
LIBS           =

# You should not have to change anything below here.

CC             = avr-gcc

# Override is only needed by avr-lib build system.

override CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
override LDFLAGS       = -Wl,-Map,$(PRG).map -Wl,--section-start=.bootloader*=0x1c00  -Wl,--section-start=.bootloader=0x1c00

OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump

ASFLAGS = -Wa,-adhlns=$(<:.S=.lst),-gstabs 
ALL_ASFLAGS = -mmcu=$(MCU_TARGET) -I. -x assembler-with-cpp $(ASFLAGS)


all: $(PRG).elf lst text eeprom

$(PRG).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf *.o $(PRG).elf *.eps *.png *.pdf *.bak *.hex *.eep
	rm -rf *.lst *.map $(EXTRA_CLEAN_FILES)

lst:  $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images

text: hex bin srec

hex:  $(PRG).hex
bin:  $(PRG).bin
srec: $(PRG).srec

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O $(FORMAT) -R .eeprom $< $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

# Assemble: create object files from assembler source files.
%.o : %.S
	$(CC) -c $(ALL_ASFLAGS) $< -o $@

# Rules for building the .eeprom rom images

eeprom: $(PRG).hex $(PRG).eep

%.eep: %.elf
	@echo
	@echo $(MSG_EEPROM) $@
	-$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O $(FORMAT) $< $@


# Program the device.  
program: $(PRG).hex $(PRG).eep
	$(AVRDUDE) $(AVRDUDE_FLAGS) $(AVRDUDE_WRITE_FLASH) $(AVRDUDE_WRITE_EEPROM)


# Every thing below here is used by avr-libc's build system and can be ignored
# by the casual user.

FIG2DEV                 = fig2dev
EXTRA_CLEAN_FILES       = *.hex *.bin *.srec


dox: eps png pdf

eps: $(PRG).eps
png: $(PRG).png
pdf: $(PRG).pdf

%.eps: %.fig
	$(FIG2DEV) -L eps $< $@

%.pdf: %.fig
	$(FIG2DEV) -L pdf $< $@

%.png: %.fig
	$(FIG2DEV) -L png $< $@

