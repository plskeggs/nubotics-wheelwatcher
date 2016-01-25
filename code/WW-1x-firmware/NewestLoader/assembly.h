#ifndef _ASSEMBLY_H_
#define _ASSEMBLY_H_

void BOOTLOADER_SECTION write_page (unsigned int adr, unsigned char function);
void BOOTLOADER_SECTION fill_temp_buffer (unsigned int data,unsigned int adr);
unsigned int BOOTLOADER_SECTION read_program_memory (unsigned int adr,unsigned char cmd);
void BOOTLOADER_SECTION write_lock_bits (unsigned char val);
void BOOTLOADER_SECTION enableRWW(void);

#endif

