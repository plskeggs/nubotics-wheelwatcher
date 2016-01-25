* HBTNativ.asm

* Copyright 2003 by RidgeSoft, LLC., PO Box 482, Pleasanton, CA  94566, U.S.A.
* www.ridgesoft.com
*
* RidgeSoft grants you the right to use, modify, make derivative works and
* redistribute this source file provided you do not remove this copyright notice.

* The following symbol defines the area in high memory where the native
* methods will be located.  When this file is assembled it must fit between
* this location and the end of memory.

NATIVE_AREA     equ     $fc00               ; the start address for native methods

* The native pointer enables the virtual machine to find the native methods.
* The location of the pointer is address $bfbe.

                org     $bfbe               ; location of the native pointer
                fdb     #NATIVE_AREA        ; point to the start of the native area

* Native methods header used by the virtual machine to register native methods.

                org     NATIVE_AREA
natvSignature   fdb     #$4e4d              ; native method signature
natvEnd         fdb     #nativeEnd          ; pointer to byte after native area
natvInitialize  fdb     #initialize         ; startup initialization routine
natvCount       fdb     #natvVectorsEnd-natvVectors/2 ; vector table size
natvVectors     fdb     #irPulseCount       ; read the number of IR pulses detected
natvVectorsEnd  equ     *

* Defines

REGISTERS       equ     $1000               ; base of HC11 registers
TIC1            equ     $10                 ; Timer Input Capture 1
TCTL2           equ     $21                 ; Timer Control Register 2
TMSK1           equ     $22                 ; Timer Interrupt Mask 1 Register
TFLG1           equ     $23                 ; Timer Interrupt Flag 1 Register

VECTORS         equ     $bfc0               ; base of interrupt vectors
IV_TIC1         equ     $2e                 ; timer input capture 1

TMSK1_IC1I      equ     $04                 ; T1C1 interrupt enable bit

TCTL2_EDG1B     equ     $20                 ; Input Capture 1 edge control bit B
TCTL2_EDG1A     equ     $10                 ; Input Capture 1 edge control bit A

TFLG1_IC1F      equ     $04                 ; TIC1 interrupt flag bit

* Native variables

pulseCount      rmb     4                   ; value of count on previous read

* Initialization routine

initialize      ldd     #0                  ; init. pulse count to zero
                std     pulseCount
                std     pulseCount+2

                ldx     #VECTORS            ; set IR (TIC1) interrupt vector
                ldd     #irInterrupt
                std     IV_TIC1,X

                ldx     #REGISTERS
                bset    TCTL2,X TCTL2_EDG1B ; set falling edge capture
                bclr    TCTL2,X TCTL2_EDG1A
                bset    TMSK1,X TMSK1_IC1I  ; enable interrupts
                rts

* Interrupt service routine

irInterrupt     ldd     pulseCount+2        ; increment the 32 bit pulse count
                addd    #1
                std     pulseCount+2
                ldd     pulseCount
                adcb    #0
                adca    #0
                std     pulseCount
                ldx     #REGISTERS
                ldaa    #TFLG1_IC1F         ; clear the interrupt
                staa    TFLG1,X
                rti                         ; return from interrupt

* Native Method

* static int irPulseCount()
* reads the count of IR pulses detected
irPulseCount    tpa                         ; disable interrupts while
                sei                         ; reading pulse count
                ldy     pulseCount          ; return count in Y:D
                ldx     pulseCount+2
                tap                         ; restore interrupts
                xgdx                        ; transfer low 16 bits to D
                rts

nativeEnd       equ     *               ; first byte after the end of native method memory
