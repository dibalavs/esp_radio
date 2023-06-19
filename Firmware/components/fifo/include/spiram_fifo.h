#ifndef _SPIRAM_FIFO_H_
#define _SPIRAM_FIFO_H_

#include <stddef.h>

#define SPIRAMSIZE (30*1024) //for a 23LC1024 chip
void*  spiRamFifoInit();
size_t  spiRamFifoRead(char *buff, unsigned len);
size_t  spiRamFifoWrite(const char *buff, unsigned len);
unsigned  spiRamFifoFill();
unsigned  spiRamFifoFree();
long  spiRamGetOverrunCt();
long  spiRamGetUnderrunCt();
void setSPIRAMSIZE(unsigned size);
unsigned getSPIRAMSIZE();

void spiRamFifoReset();
void spiRamFifoDestroy();
unsigned spiRamFifoLen();

#endif
