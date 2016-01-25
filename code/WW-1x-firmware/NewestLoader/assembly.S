.section .bootloader
;.text

;*********************************************************
; Place a 1 for the processor you want use
;*********************************************************

MEGA8    = 0
MEGA16   = 0
MEGA64   = 0
MEGA128  = 0
MEGA32   = 0
MEGA162  = 0
MEGA169  = 0
MEGA8515 = 0
;MEGA8535 = 0
MEGA163  = 0
MEGA323  = 0
MEGA48   = 0
MEGA88   = 0
MEGA168  = 0
MEGA165  = 0
MEGA3250 = 0
MEGA6450 = 0
MEGA3290 = 0
MEGA6490 = 0
MEGA406  = 0
MEGA640  = 0
MEGA1280 = 0
MEGA2560 = 0
MCAN128  = 0
MEGA164 = 0
MEGA328 = 0
MEGA324 = 0
MEGA325 = 0
MEGA644 = 0
MEGA645 = 0
MEGA1281 = 0
MEGA2561 = 0
MEGA404 = 0
MUSB1286 = 0
MUSB1287 = 0
MUSB162 = 0
MUSB646 = 0
MUSB647 = 0
MUSB82 = 0
MMCAN32 = 0
MMCAN64 = 0
MEGA329  = 0
MEGA649  = 0
MEGA256  = 0
TINY85 = 1

;*********************************************************
; For a bootloader that fit in 256k without EEprom & Lockbits
;*********************************************************

SMALL256 = 0

;*********************************************************
;*********************************************************
;*********************************************************
;*********************************************************
; DO NOT CHANGE ANYTHING BELOW THIS LINE !!!!!!!
;*********************************************************
;*********************************************************
;*********************************************************
;*********************************************************

.if MEGA64 | MEGA128 | MEGA1280
    SPMCR = 0x68
.elif MEGA324 | MEGA644
    SPMCR = 0x37
.else
    SPMCR = 0x57
.endif

;-----------------------------------------

; void write_page (unsigned int adr, unsigned char function);
; bits 8:15 adr addresses the page...(must setup RAMPZ beforehand!!!)
.global write_page
write_page:                 ;was _write_page::
    RCALL __WAIT_SPMEN__    ;was XCALL for ICC version
    movw    r30, r16        ;move address to z pointer (R31 = ZH, R30 = ZL)
    STS     SPMCR, R18      ;argument 2 decides function
    SPM                     ;perform pagewrite
    RET

;-----------------------------------------

; void fill_temp_buffer (unsigned int data, unsigned int adr);
; bits 7:1 in adr addresses the word in the page... (2=first word, 4=second word etc..)
.global fill_temp_buffer
fill_temp_buffer:           ;was _fill_temp_buffer::
    RCALL __WAIT_SPMEN__    ; was XCALL for ICC version
    movw    r30, r18        ;move adress to z pointer (R31=ZH R30=ZL)
    movw    r0, r16         ;move data to reg 0 and 1
    LDI     R19, 0x01
    STS     SPMCR, R19
    SPM                     ;Store program memory
    RET

;-----------------------------------------

.if SMALL256
.else
;unsigned int read_program_memory (unsigned int adr ,unsigned char cmd);
.global read_program_memory ;was _read_program_memory::
    movw    r30, r16        ;move adress to z pointer
    SBRC    R18, 0          ;read lockbits? (second argument = 0x09)
    STS     SPMCR, R18      ;if so, place second argument in SPMEN register
.if MEGA128 | MEGA2560 | MEGA2561 | MEGA1280 | MCAN128 | MEGA256 | MEGA1281
    ELPM    r16, Z+         ;read LSB
    ELPM    r17, Z          ;read MSB
.else
    LPM     r16, Z+
    LPM     r17, Z
.endif
    RET
.endif

;-----------------------------------------

.if SMALL256
.else
;void write_lock_bits (unsigned char val);
.global write_lock_bits
write_lock_bits: ; was _write_lock_bits::
     MOV     R0, R16
     LDI     R17, 0x09
     STS     SPMCR, R17
     SPM                ;write lockbits
     RET
.endif

;-----------------------------------------

.global enableRWW
enableRWW: ; was _enableRWW::
RCALL __WAIT_SPMEN__   ; was XCALL for ICC version
    LDI R27,0x11
    STS SPMCR,R27
    SPM
    RET

;-----------------------------------------
.global __WAIT_SPMEN__
__WAIT_SPMEN__:
    LDS     R27,SPMCR       ; load SPMCR to R27
    SBRC    R27,0           ; check SPMEN flag
    RJMP    __WAIT_SPMEN__  ; wait for SPMEN flag cleared
    RET

;-----------------------------------------


