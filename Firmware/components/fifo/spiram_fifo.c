/******************************************************************************
 * Copyright 2013-2015 Espressif Systems
 *
 * FileName: user_main.c
 *
 * Description: Routines to use a SPI RAM chip as a big FIFO buffer. Multi-
 * thread-aware: the reading and writing can happen in different threads and
 * will block if the fifo is empty and full, respectively.
 *
 * Modification history:
 *     2015/06/02, v1.0 File created.
*******************************************************************************/

#include "spiram_fifo.h"

#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"

extern void* kmalloc(size_t memorySize);
extern void* kcalloc(size_t elementCount, size_t elementSize);

static size_t buff_size = 16;
static uint8_t * stream_buff;
static StreamBufferHandle_t stream;
static StaticStreamBuffer_t static_data;

void setSPIRAMSIZE(unsigned size)
{
	buff_size = size;
}
unsigned getSPIRAMSIZE()
{
	return buff_size;
}

//Initialize the FIFO
void* spiRamFifoInit()
{
	if (stream_buff)
		free(stream_buff);

	stream_buff = kmalloc(buff_size);
	assert(stream_buff);

	stream = xStreamBufferCreateStatic(buff_size, 16, stream_buff, &static_data);
	assert(stream != NULL);
	return stream_buff;
}

void spiRamFifoReset()
{
	xStreamBufferReset(stream);
}

//Read bytes from the FIFO
size_t spiRamFifoRead(char *buff, unsigned len)
{
	return xStreamBufferReceive(stream, buff, len, portMAX_DELAY);
}

//Write bytes to the FIFO
size_t spiRamFifoWrite(const char *buff, unsigned buffLen)
{
	return xStreamBufferSend(stream, buff, buffLen, 0);
}

//Get amount of bytes in use
unsigned spiRamFifoFill()
{
	return buff_size - xStreamBufferSpacesAvailable(stream);
}

unsigned spiRamFifoFree()
{
	return xStreamBufferSpacesAvailable(stream);
}

void spiRamFifoDestroy()
{
	if (stream_buff)
		free (stream_buff);
	stream_buff = NULL;
}

unsigned spiRamFifoLen()
{
	return buff_size;
}

long spiRamGetOverrunCt()
{
	return 0;
}

long spiRamGetUnderrunCt()
{
	return 0;
}