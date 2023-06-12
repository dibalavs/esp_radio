/*
 * VS1053.h
 *
 *  Created on: 25-04-2011
 *      Author: Przemyslaw Stasiak
 *
 * Mofified for KaRadio32
 *		Author: KaraWin
 */
#pragma once
#ifndef VS1053_H_
#define VS1053_H_

#include "esp_system.h"
#include "interface.h"

// gpio are defined in gpio.h

//public functions
//extern int vsVersion;
int getVsVersion();
bool 	VS1053_HW_init();
bool    VS1053_CheckPresent(void);
void 	VS1053_SineTest();
void	VS1053_Start();
void	VS1053_I2SRate(uint8_t speed);
void VS1053_I2SDisable();
//void 	VS1053_SendMusicBytes(uint8_t* music,int quantity);
int 	VS1053_SendMusicBytes(uint8_t* music,uint16_t quantity);
uint16_t	VS1053_GetBitrate();
uint16_t	VS1053_GetSampleRate();
uint16_t	VS1053_GetDecodeTime();
void	VS1053_flush_cancel();
// admix plugin control
void VS1053_SetVolumeLine(int16_t vol);
void VS1053_Admix(bool val);

//Volume control
void VS1053_DisableAnalog(void);
uint8_t 	VS1053_GetVolume();
uint8_t 	VS1053_GetVolumeLinear();
void	VS1053_SetVolume(uint8_t xMinusHalfdB);
void 	VS1053_VolumeUp(uint8_t xHalfdB);
void	VS1053_VolumeDown(uint8_t xHalfdB);
//Treble control
int8_t	VS1053_GetTreble();
void	VS1053_SetTreble(int8_t xOneAndHalfdB);
void	VS1053_TrebleUp(uint8_t xOneAndHalfdB);
void	VS1053_TrebleDown(uint8_t xOneAndHalfdB);
void	VS1053_SetTrebleFreq(uint8_t xkHz);
int8_t	VS1053_GetTrebleFreq(void);
//Bass control
uint8_t	VS1053_GetBass();
void	VS1053_SetBass(uint8_t xdB);
void	VS1053_BassUp(uint8_t xdB);
void	VS1053_BassDown(uint8_t xdB);
void	VS1053_SetBassFreq(uint8_t xTenHz);
uint8_t	VS1053_GetBassFreq(void);
// Spacial
uint8_t	VS1053_GetSpatial();
void VS1053_SetSpatial(uint8_t num);
// reduce the chip consumption
void VS1053_LowPower();
// normal chip consumption
void VS1053_HighPower();
void VS1053_WriteVS10xxRegister(unsigned short addr,unsigned short val);
void VS1053_WriteRegister(uint8_t addressbyte,
		uint8_t highbyte, uint8_t lowbyte);
void VS1053_task(void *pvParams);
#endif /* VS1053_H_ */
