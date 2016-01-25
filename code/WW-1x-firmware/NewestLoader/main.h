void BootLoad(void);
void SendDeviceID(void);
void GetPageNumber(void);
void ExecCode(void);
char GetPage(void);
void WriteFlash(void);
char CheckFlash(void);
unsigned char IsChar(void);
unsigned char RxChar(void);
void TxChar(unsigned char ch);
void Wait(void);
#ifdef __IMAGECRAFT__
void 
#else
int
#endif
#ifdef INTEGRATED
bootloader(void);
#else
main(void);
#endif

