CC = iccavr
LIB = ilibw
CFLAGS =  -IC:\progra~1\iccavr\include\ -e -D__ICC_VERSION="7.20" -D_EE_EXTIO -DATMega324P  -l -g -MLongJump -MHasMul -MEnhanced 
ASFLAGS = $(CFLAGS) 
LFLAGS =  -LC:\progra~1\iccavr\lib\ -g -e:0x8000 -ucrtboot.o -bvector:0x7c00.0x8000 -bfunc_lit:0x7c7c.0x8000 -dram_end:0x8ff -bdata:0x100.0x8ff -dhwstk_size:16 -beeprom:0.1024 -fihx_coff -S2
FILES = assembly.o main.o 

BOOTLOAD:	$(FILES)
	$(CC) -o BOOTLOAD $(LFLAGS) @BOOTLOAD.lk   -lcatmega
assembly.o:	assembly.s
	$(CC) -c $(ASFLAGS) assembly.s
main.o: C:\progra~1\iccavr\include\macros.h C:\progra~1\iccavr\include\AVRdef.h .\..\..\..\progra~1\iccavr\include\iom2561v.h C:\progra~1\iccavr\include\_iom640to2561v.h .\assembly.h
main.o:	main.c
	$(CC) -c $(CFLAGS) main.c
