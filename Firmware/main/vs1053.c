/**
***********************************************************************************************************************
  * @file    VS1053.c
  * @author  Piotr Sperka
  * @date    07.08.2015
  * @brief   This file provides VS1053 usage and control functions. Based on VS1003 library by Przemyslaw Stasiak.
  * Copyright 2017 karawin (http://www.karawin.fr) for KaRadio32
  * added control treble, bass and spacialisation
  ***********************************************************************************************************************
*/

/** @addtogroup VS1053
  * @{
  */
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define CHUNK	32
#include "vs1053.h"
#include "gpio.h"
#include "eeprom.h"
#include <string.h>
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include <math.h>
#include "interface.h"
#include "app_main.h"
#include "audio_player.h"
#include "spiram_fifo.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern void  LoadUserCodes(void);

#define TAG "vs1053"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define SET 0
#define RESET 1

#define RXNE    0x01
#define TXE     0x02
#define BSY     0x80

#define VS_WRITE_COMMAND 	0x02
#define VS_READ_COMMAND 	0x03
#define SPI_MODE        	0x00
#define SPI_STATUSVS      	0x01
#define SPI_BASS        	0x02
#define SPI_CLOCKF      	0x03
#define SPI_DECODE_TIME 	0x04
#define SPI_AUDATA      	0x05
#define SPI_WRAM        	0x06
#define SPI_WRAMADDR    	0x07
#define SPI_HDAT0       	0x08
#define SPI_HDAT1       	0x09
#define SPI_AIADDR      	0x0a
#define SPI_VOL         	0x0b
#define SPI_AICTRL0     	0x0c
#define SPI_AICTRL1     	0x0d
#define SPI_AICTRL2     	0x0e
#define SPI_AICTRL3     	0x0f
#define SM_DIFF         	0x01
#define SM_JUMP         	0x02
#define SM_LAYER12			0x02
#define SM_RESET        	0x04
#define SM_CANCEL           0x08
#define SM_OUTOFWAV     	0x08
#define SM_PDOWN        	0x10
#define SM_TESTS        	0x20
#define SM_STREAM       	0x40
#define SM_PLUSV        	0x80
#define SM_DACT         	0x100
#define SM_SDIORD       	0x200
#define SM_SDISHARE     	0x400
#define SM_SDINEW       	0x800
#define SM_ADPCM        	0x1000
#define SM_ADPCM_HP     	0x2000
#define SM_LINE1            0x4000
#define para_endFillByte    0x1E06

int vsVersion = -1; // the version of the chip
//	SS_VER is 0 for VS1001, 1 for VS1011, 2 for VS1002, 3 for VS1003, 4 for VS1053 and VS8053, 5 for VS1033, 7 for VS1103, and 6 for VS1063.

static spi_device_handle_t vsspi;  // the evice handle of the vs1053 spi
static spi_device_handle_t hvsspi;  // the device handle of the vs1053 spi high speed

SemaphoreHandle_t vsSPI = NULL;
SemaphoreHandle_t hsSPI = NULL;

static void VS1053_ControlReset(uint8_t State);
static uint16_t VS1053_ReadRegister(uint8_t addressbyte);
static void VS1053_ResetChip();
static uint16_t VS1053_MaskAndShiftRight(uint16_t Source, uint16_t Mask, uint16_t Shift);
static void VS1053_regtest();

uint8_t spi_take_semaphore(SemaphoreHandle_t isSPI) {
	if(isSPI) if(xSemaphoreTake(isSPI, portMAX_DELAY)) return 1;
	return 0;
}

void spi_give_semaphore(SemaphoreHandle_t isSPI) {
	if(isSPI) xSemaphoreGive(isSPI);
}

int getVsVersion() { return vsVersion;}

bool VS1053_HW_init()
{
	uint32_t freq =spi_get_actual_clock(APB_CLK_FREQ, 1400000, 128);
	ESP_LOGI(TAG,"VS1053 LFreq: %d",freq);
	spi_device_interface_config_t devcfg={
        .clock_speed_hz=freq,               //Clock out at x Hz
		.command_bits = 8,
		.address_bits = 8,
		.dummy_bits = 0,
		.duty_cycle_pos = 0,
		.cs_ena_pretrans = 0,
		.cs_ena_posttrans = 1,
		.flags = 0,
        .mode = 0,                         //0 = (0-CPOL, 0-CPHA)
        .spics_io_num = PIN_NUM_XCS,               //XCS pin
        .queue_size = 1,                          //We want to be able to queue x transactions at a time
        .pre_cb = NULL,
		.post_cb = NULL
	};

	//slow speed
	ESP_ERROR_CHECK(spi_bus_add_device(SPI_NO, &devcfg, &vsspi));

	//high speed
	freq =spi_get_actual_clock(APB_CLK_FREQ, 6100000, 128);
	ESP_LOGI(TAG,"VS1053 HFreq: %d",freq);
	devcfg.clock_speed_hz = freq;
	devcfg.spics_io_num= PIN_NUM_XDCS;               //XDCS pin
	devcfg.command_bits = 0;
	devcfg.address_bits = 0;
	ESP_ERROR_CHECK(spi_bus_add_device(SPI_NO, &devcfg, &hvsspi));

	//Initialize non-SPI GPIOs
	gpio_config_t gpio_conf;
	gpio_conf.mode = GPIO_MODE_OUTPUT;
	gpio_conf.pull_up_en =  GPIO_PULLUP_DISABLE;
	gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_conf.pin_bit_mask = ((uint64_t)(((uint64_t)1)<<PIN_NUM_RST));
	ESP_ERROR_CHECK(gpio_config(&gpio_conf));

	gpio_conf.mode = GPIO_MODE_INPUT;
	gpio_conf.pull_up_en =  GPIO_PULLUP_DISABLE;
	gpio_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	gpio_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_conf.pin_bit_mask = ((uint64_t)(((uint64_t)1)<<PIN_NUM_DREQ));
	ESP_ERROR_CHECK(gpio_config(&gpio_conf));

	//gpio_set_direction(dreq, GPIO_MODE_INPUT);
	//gpio_set_pull_mode(dreq, GPIO_PULLDOWN_ENABLE); //usefull for no vs1053 test
	return true;
}


static void VS1053_ControlReset(uint8_t State){
	gpio_set_level(PIN_NUM_RST, State);
}

uint8_t CheckDREQ() {
	return gpio_get_level(PIN_NUM_DREQ);
}
#define TMAX 4096
void  WaitDREQ() {
	uint16_t  time_out = 0;
	while(gpio_get_level(PIN_NUM_DREQ) == 0 && time_out++ < TMAX)
	{
		taskYIELD();
	}
}

void VS1053_spi_write_char(uint8_t *cbyte, uint16_t len)
{
	esp_err_t ret;
    spi_transaction_t t;

    memset(&t, 0, sizeof(t));       //Zero out the transaction
	t.tx_buffer = cbyte;
    t.length= len*8;
    //t.rxlength=0;
	while(gpio_get_level(PIN_NUM_DREQ) == 0 )taskYIELD();
	spi_take_semaphore(hsSPI);
    ret = spi_device_transmit(hvsspi, &t);  //Transmit!
	if (ret != ESP_OK) ESP_LOGE(TAG,"err: %d, VS1053_spi_write_char(len: %d)",ret,len);
	spi_give_semaphore(hsSPI);
//	while(gpio_get_level(dreq) == 0 );
}

void VS1053_WriteRegister(uint8_t addressbyte, uint8_t highbyte, uint8_t lowbyte)
{
    spi_transaction_t t;
	esp_err_t ret;

	if (vsVersion == 0) return;

    memset(&t, 0, sizeof(t));       //Zero out the transaction
	t.flags |= SPI_TRANS_USE_TXDATA;
	t.cmd = VS_WRITE_COMMAND;
	t.addr = addressbyte;
	t.tx_data[0] = highbyte;
	t.tx_data[1] = lowbyte;
    t.length= 16;
	WaitDREQ();
	spi_take_semaphore(vsSPI);
//    ESP_ERROR_CHECK(spi_device_transmit(vsspi, &t));  //Transmit!
    ret = spi_device_transmit(vsspi, &t);  //Transmit!
	if (ret != ESP_OK) ESP_LOGE(TAG,"err: %d, VS1053_WriteRegister(%d,%d,%d)",ret,addressbyte,highbyte,lowbyte);
	spi_give_semaphore(vsSPI);
	WaitDREQ();
}

void VS1053_WriteRegister16(uint8_t addressbyte, uint16_t value)
{
    spi_transaction_t t;
	esp_err_t ret;

	if (vsVersion == 0) return;

    memset(&t, 0, sizeof(t));       //Zero out the transaction
	t.flags |= SPI_TRANS_USE_TXDATA;
	t.cmd = VS_WRITE_COMMAND;
	t.addr = addressbyte;
	t.tx_data[0] = (value>>8)&0xff;
	t.tx_data[1] = value&0xff;
    t.length= 16;
	WaitDREQ();
	spi_take_semaphore(vsSPI);
//    ESP_ERROR_CHECK(spi_device_transmit(vsspi, &t));  //Transmit!
    ret = spi_device_transmit(vsspi, &t);  //Transmit!
	if (ret != ESP_OK) ESP_LOGE(TAG,"err: %d, VS1053_WriteRegister16(%d,%d)",ret,addressbyte,value);
	spi_give_semaphore(vsSPI);
	WaitDREQ();

}

static uint16_t VS1053_ReadRegister(uint8_t addressbyte){
	uint16_t result;
    spi_transaction_t t;
	esp_err_t ret;
	if (vsVersion == 0) return 0;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
	t.length=16;
	t.flags |= SPI_TRANS_USE_RXDATA	;
	t.cmd = VS_READ_COMMAND;
	t.addr = addressbyte;
	WaitDREQ();
	spi_take_semaphore(vsSPI);
	ret = spi_device_transmit(vsspi, &t);  //Transmit!
	if (ret != ESP_OK) ESP_LOGE(TAG,"err: %d, VS1053_ReadRegister(%d), read: %d",ret,addressbyte,(uint32_t)*t.rx_data);
	result =  (((t.rx_data[0]&0xFF)<<8) | ((t.rx_data[1])&0xFF)) ;
//	ESP_LOGI(TAG,"VS1053_ReadRegister data: %d %d %d %d",t.rx_data[0],t.rx_data[1],t.rx_data[2],t.rx_data[3]);
	spi_give_semaphore(vsSPI);
	WaitDREQ();
	return result;
}


void VS1053_WriteVS10xxRegister(unsigned short addr,unsigned short val)
{
	VS1053_WriteRegister((uint8_t)addr&0xff, (uint8_t)((val&0xFF00)>>8), (uint8_t)(val&0xFF));
}


static void VS1053_ResetChip(){
	VS1053_ControlReset(SET);
	vTaskDelay(10);
	VS1053_ControlReset(RESET);
	vTaskDelay(10);
	if (CheckDREQ() == 1) return;
	vTaskDelay(100);
}


static uint16_t VS1053_MaskAndShiftRight(uint16_t Source, uint16_t Mask, uint16_t Shift){
	return ( (Source & Mask) >> Shift );
}

static void VS1053_regtest()
{
	int MP3Status = VS1053_ReadRegister(SPI_STATUSVS);
	int MP3Mode = VS1053_ReadRegister(SPI_MODE);
	int MP3Clock = VS1053_ReadRegister(SPI_CLOCKF);
	ESP_LOGI(TAG,"SCI_Status  = 0x%X",MP3Status);
	ESP_LOGI(TAG,"SCI_Mode (0x4800) = 0x%X",MP3Mode);
	ESP_LOGI(TAG,"SCI_ClockF = 0x%X",MP3Clock);
	ESP_LOGI(TAG,"VS Version (VS1053 is 4) = %d",vsVersion);
	//The 1053B should respond with 4. VS1001 = 0, VS1011 = 1, VS1002 = 2, VS1003 = 3, VS1054 = 4
}

void VS1053_I2SRate(uint8_t speed){ // 0 = 48kHz, 1 = 96kHz, 2 = 128kHz
    if (speed > 2) speed = 0;
	if (vsVersion != 4) return;
	VS1053_WriteRegister16(SPI_WRAMADDR, 0xc040); //address of GPIO_ODATA is 0xC017
	VS1053_WriteRegister16(SPI_WRAM, 0x0008|speed); //
	VS1053_WriteRegister16(SPI_WRAMADDR, 0xc040); //address of GPIO_ODATA is 0xC017
	VS1053_WriteRegister16(SPI_WRAM, 0x000C|speed); //
	ESP_LOGI(TAG,"I2S Speed: %d",speed);
}
void VS1053_DisableAnalog(){
	// disable analog output
	VS1053_WriteRegister16(SPI_VOL,0xFFFF);
}

// reduce the chip consumption
void VS1053_LowPower(){
	VS1053_WriteRegister16(SPI_CLOCKF,0x0000); //
}

// normal chip consumption
void VS1053_HighPower(){
	if (vsVersion == 4) // only 1053
		VS1053_WriteRegister16(SPI_CLOCKF,0xB800); // SC_MULT = x1, SC_ADD= x1
	else
		VS1053_WriteRegister16(SPI_CLOCKF,0xb000);
}

// patch if GPIO1 is not wired to gnd
void VS1053_GPIO1()
{
// these 4 lines makes board to run on mp3 mode, no soldering required anymore
		VS1053_WriteRegister16(SPI_WRAMADDR, 0xc017); //address of GPIO_DDR is 0xC017
		VS1053_WriteRegister16(SPI_WRAM, 0x0003); //GPIO_DDR=3
		VS1053_WriteRegister16(SPI_WRAMADDR, 0xc019); //address of GPIO_ODATA is 0xC019
		VS1053_WriteRegister16(SPI_WRAM, 0x0000); //GPIO_ODATA=0
		ESP_LOGI(TAG,"SPI_AUDATA 1 = %x",VS1053_ReadRegister(SPI_AUDATA));
}

// First VS10xx configuration after reset
void VS1053_InitVS()
{
	if (vsVersion == 4) // only 1053b
//		VS1053_WriteRegister(SPI_CLOCKF,0x78,0x00); // SC_MULT = x3, SC_ADD= x2
		VS1053_WriteRegister16(SPI_CLOCKF,0xB800); // SC_MULT = x1, SC_ADD= x1
//		VS1053_WriteRegister16(SPI_CLOCKF,0x8800); // SC_MULT = x3.5, SC_ADD= x1
//		VS1053_WriteRegister16(SPI_CLOCKF,0x9000); // SC_MULT = x3.5, SC_ADD= x1.5
	else
		VS1053_WriteRegister16(SPI_CLOCKF,0xB000);

	VS1053_WriteRegister(SPI_MODE, (SM_SDINEW|SM_LINE1)>>8,SM_RESET);
	VS1053_WriteRegister(SPI_MODE, (SM_SDINEW|SM_LINE1)>>8, SM_LAYER12); //mode
	WaitDREQ();

	VS1053_regtest();

	// enable I2C dac output of the vs1053
	if (vsVersion == 4) // only 1053
	{
		VS1053_WriteRegister16(SPI_WRAMADDR, 0xc017);
		VS1053_WriteRegister16(SPI_WRAM, 0x00F0);
		VS1053_I2SRate(g_device->i2sspeed);
	}
}

bool VS1053_CheckPresent(void)
{
	VS1053_ControlReset(SET);
	vTaskDelay(10);
	VS1053_ControlReset(RESET);
	vTaskDelay(50);
	if (CheckDREQ() == 0) vTaskDelay(50);	// wait a bit more
	//Check DREQ
	if (CheckDREQ() == 0)
	{
		vsVersion = 0;
		ESP_LOGE(TAG,"NO VS1053 detected");
		return false;
	}

	vsVersion = (VS1053_ReadRegister(SPI_STATUSVS) >> 4) & 0x000F;

	return vsVersion == 4;
}

void VS1053_Start(){
	VS1053_ControlReset(SET);
	vTaskDelay(10);
	VS1053_ControlReset(RESET);
	vTaskDelay(50);
	if (CheckDREQ() == 0) vTaskDelay(50);	// wait a bit more
	//Check DREQ
	if (CheckDREQ() == 0)
	{
		vsVersion = 0;
		ESP_LOGE(TAG,"NO VS1053 detected");
		return;
	}

// patch to mp3 mode id needed
//	if (VS1053_ReadRegister(SPI_AUDATA) == 0xac45) // midi mode?
		VS1053_GPIO1();	// patch if GPIO1 is not wired to gnd
	if (VS1053_ReadRegister(SPI_AUDATA) == 0xac45) // try again
	{
		VS1053_ResetChip();
		VS1053_GPIO1();	// patch if GPIO1 is not wired to gnd
	}

	vsVersion = (VS1053_ReadRegister(SPI_STATUSVS) >> 4) & 0x000F; //Mask out only the four version bits
//0 for VS1001, 1 for VS1011, 2 for VS1002, 3 for VS1003, 4 for VS1053 and VS8053,
//5 for VS1033, 7 for VS1103, and 6 for VS1063
	ESP_LOGI(TAG,"VS10xx detection. Version: %x",vsVersion);

	// plugin patch
	if ((vsVersion == 4) && ((g_device->options&T_PATCH)==0))
	{
		LoadUserCodes() ;	// vs1053b patch
		ESP_LOGI(TAG,"SPI_AUDATA 2 = %x",VS1053_ReadRegister(SPI_AUDATA));
		if (VS1053_ReadRegister(SPI_AUDATA) == 0xAC45) //midi mode?
		{
			VS1053_WriteRegister(SPI_AIADDR,0x00,0x50); // reset soft but let  patch loaded
			VS1053_GPIO1();	// patch if GPIO1 is not wired to gnd
			if (VS1053_ReadRegister(SPI_AUDATA) == 0xAC45) // in midi mode
			{	//fed up
				ESP_LOGI(TAG,"midi mode on\n");
				g_device->options |= T_PATCH; // force no patch
				eeprom_save_device_settings(g_device);
				esp_restart();
			}
		}
	}

	VS1053_InitVS();
	// disable analog output
	VS1053_WriteRegister16(SPI_VOL,0xFFFF);
	VS1053_DisableAnalog();
	vTaskDelay(1);
	ESP_LOGI(TAG,"volume: %d",g_device->vol);
	app_set_ivol( g_device->vol);
	VS1053_SetVolume( g_device->vol);
	VS1053_SetTreble(g_device->treble);
	VS1053_SetBass(g_device->bass);
	VS1053_SetTrebleFreq(g_device->freqtreble);
	VS1053_SetBassFreq(g_device->freqbass);
	VS1053_SetSpatial(g_device->spacial);
}

int VS1053_SendMusicBytes(uint8_t* music, uint16_t quantity){
	if(quantity ==0) return 0;
	int oo = 0;

	while(quantity)
	{
		int t = quantity;
		if(t > CHUNK) t = CHUNK;
		VS1053_spi_write_char(&music[oo], t);
		oo += t;
		quantity -= t;
	}
	return oo;
}

// Get volume and convert it in log one
uint8_t VS1053_GetVolume(){
uint8_t i,j;
uint8_t value =  VS1053_ReadRegister(SPI_VOL) & 0x00FF;
	for (i = 0;i< 255; i++)
	{
		j = (log10(255/((float)i+1)) * 105.54571334); // magic no?
		if (value == j )
		  return i;
	}
	return 127;
}

// rough volume
uint8_t VS1053_GetVolumeLinear(){
	return VS1053_ReadRegister(SPI_VOL) & 0x00FF;
}

/**
 * Function sets the same volume level to both channels.
 * @param xMinusHalfdB describes damping level as a multiple
 * 		of 0.5dB. Maximum volume is 0 and silence is 0xFEFE.
 * convert the log one to rough one and set it invs1053
 */
void VS1053_SetVolume(uint8_t xMinusHalfdB){
uint8_t value = (log10(255/((float)xMinusHalfdB+1)) * 105.54571334);
//printf("setvol: %d\n",value);
	if (value == 255) value = 254;
//printf("xMinusHalfdB=%d  value=%d\n",xMinusHalfdB,value);
	VS1053_WriteRegister(SPI_VOL,value,value);
}

/**
 * Functions returns level of treble enhancement.
 * @return Returned value describes enhancement in multiplies
 * 		of 1.5dB. 0 value means no enhancement, 8 max (12dB).
 */
int8_t	VS1053_GetTreble(){
	int8_t  treble = (VS1053_ReadRegister(SPI_BASS) & 0xF000) >> 12;
	if ( (treble&0x08)) treble |= 0xF0; // negative value
	return ( treble);
}

/**
 * Sets treble level.
 * @note If xOneAndHalfdB is greater than max value, sets treble
 * 		to maximum.
 * @param xOneAndHalfdB describes level of enhancement. It is a multiplier
 * 		of 1.5dB. 0 - no enhancement, -8 minimum -12dB , 7 - maximum, 10.5dB.
 * @return void
 */
void VS1053_SetTreble(int8_t xOneAndHalfdB){
	uint16_t bassReg = VS1053_ReadRegister(SPI_BASS);

	if (( xOneAndHalfdB <= 7) && ( xOneAndHalfdB >=-8))
		VS1053_WriteRegister( SPI_BASS, VS1053_MaskAndShiftRight(bassReg,0x0F00,8) | (xOneAndHalfdB << 4), bassReg & 0x00FF );
}

/**
 * Sets low limit frequency of treble enhancer.
 * @note new frequency is set only if argument is valid.
 * @param xkHz The lowest frequency enhanced by treble enhancer.
 * 		Values from 0 to 15 (in kHz)
 * @return void
 */
void VS1053_SetTrebleFreq(uint8_t xkHz){
	uint16_t bassReg = VS1053_ReadRegister(SPI_BASS);
	if ( xkHz <= 15 )
		VS1053_WriteRegister( SPI_BASS, VS1053_MaskAndShiftRight(bassReg,0xF000,8) | xkHz, bassReg & 0x00FF );
}
int8_t	VS1053_GetTrebleFreq(){
	return ( (VS1053_ReadRegister(SPI_BASS) & 0x0F00) >> 8);
}

/**
 * Returns level of bass boost in dB.
 * @return Value of bass enhancement from 0 (off) to 15(dB).
 */
uint8_t	VS1053_GetBass(){
	return ( (VS1053_ReadRegister(SPI_BASS) & 0x00F0) >> 4);
}

/**
 * Sets bass enhancement level (in dB).
 * @note If xdB is greater than max value, bass enhancement is set to its max (15dB).
 * @param xdB Value of bass enhancement from 0 (off) to 15(dB).
 * @return void
 */
void VS1053_SetBass(uint8_t xdB){
	uint16_t bassReg = VS1053_ReadRegister(SPI_BASS);
	if (xdB <= 15)
		VS1053_WriteRegister(SPI_BASS, (bassReg & 0xFF00) >> 8, (bassReg & 0x000F) | (xdB << 4) );
	else
		VS1053_WriteRegister(SPI_BASS, (bassReg & 0xFF00) >> 8, (bassReg & 0x000F) | 0xF0 );
}

/**
 * Sets low limit frequency of bass enhancer.
 * @note new frequency is set only if argument is valid.
 * @param xTenHz The lowest frequency enhanced by bass enhancer.
 * 		Values from 2 to 15 ( equal to 20 - 150 Hz).
 * @return void
 */
void VS1053_SetBassFreq(uint8_t xTenHz){
	uint16_t bassReg = VS1053_ReadRegister(SPI_BASS);
	if (xTenHz >=2 && xTenHz <= 15)
		VS1053_WriteRegister(SPI_BASS, VS1053_MaskAndShiftRight(bassReg,0xFF00,8), (bassReg & 0x00F0) | xTenHz );
}

uint8_t	VS1053_GetBassFreq(){
	return ( (VS1053_ReadRegister(SPI_BASS) & 0x000F) );
}

uint8_t	VS1053_GetSpatial(){
	if (vsVersion != 4) return 0;
	uint16_t spatial = (VS1053_ReadRegister(SPI_MODE) & 0x0090) >>4;
	return ((spatial&1) | ((spatial>>2) & 2));
}

void VS1053_SetSpatial(uint8_t num){
	if (vsVersion != 4) return ;
	uint16_t spatial = VS1053_ReadRegister(SPI_MODE);
	if (num <= 3)
	{
		num = (((num <<2)&8) | (num&1))<<4;
		VS1053_WriteRegister(SPI_MODE, VS1053_MaskAndShiftRight(spatial,0xFF00,8), (spatial & 0x006F) | num );
	}
}

uint16_t VS1053_GetDecodeTime(){
	return VS1053_ReadRegister(SPI_DECODE_TIME);
}

uint16_t VS1053_GetBitrate(){
	uint16_t bitrate = (VS1053_ReadRegister(SPI_HDAT0) & 0xf000) >> 12;
	uint8_t ID = (VS1053_ReadRegister(SPI_HDAT1) & 0x18) >> 3;
	uint16_t res;
	if (ID == 3)
	{	res = 32;
		while(bitrate>13)
		{
			res+=64;
			bitrate--;
		}
		while (bitrate>9)
		{
			res+=32;
			bitrate--;
		}
		while (bitrate>5)
		{
			res+=16;
			bitrate--;
		}
		while (bitrate>1)
		{
			res+=8;
			bitrate--;
		}
	}
	else
	{	res = 8;

		while (bitrate>8)
		{
			res+=16;
			bitrate--;
		}
		while (bitrate>1)
		{
			res+=8;
			bitrate--;
		}
	}
	return res;
}

uint16_t VS1053_GetSampleRate(){
	return (VS1053_ReadRegister(SPI_AUDATA) & 0xFFFE);
}

/* to  stop a  stream */
void VS1053_flush_cancel() {
	int8_t endFillByte ;
	int16_t y;
	uint8_t buf[33];
	// set spimode with SM_CANCEL
	uint16_t spimode = VS1053_ReadRegister(SPI_MODE)| SM_CANCEL;
  // set CANCEL
	VS1053_WriteRegister(SPI_MODE,VS1053_MaskAndShiftRight(spimode,0xFF00,8), (spimode & 0x00FF) );
	// wait CANCEL
	VS1053_WriteRegister16(SPI_WRAMADDR, para_endFillByte);
	endFillByte = (int8_t) (VS1053_ReadRegister(SPI_WRAM) & 0xFF);
	for (y = 0; y < 32; y++) buf[y] = endFillByte;
	y = 0;
	while (VS1053_ReadRegister(SPI_MODE)& SM_CANCEL)
	{
		VS1053_SendMusicBytes( buf, 32);
		if (y++ > 64)
		{
			ESP_LOGE(TAG,"VS1053 Reset");
//			VS1053_Start();
			break;
		}
	}

	for (y = 0; y < 64; y++)
		VS1053_SendMusicBytes( buf, 32); //2080 bytes

}

void VS1053_task(void *pvParams) {
#define VSTASKBUF	1024
	portBASE_TYPE uxHighWaterMark;
	uint8_t  b[VSTASKBUF];
	uint16_t size ,s;

	player_t *player = pvParams;

	while(1) {
		// stop requested, terminate immediately
        if(player->decoder_command == CMD_STOP) {
            break;
        }

		unsigned fsize = spiRamFifoFill();
		size = min(VSTASKBUF, fsize);
/*		if (size > 	VSTASKBUF)
		{
			ESP_LOGE(TAG, "Decoder vs1053 size: %d, fsize: %d, VSTASKBUF: %d .\n",size,fsize,VSTASKBUF );
			size = 	VSTASKBUF;
		}
*/
		if (size > 0)
		{
			spiRamFifoRead((char*)b, size);
			s = 0;
			while(s < size)
			{
				s += VS1053_SendMusicBytes(b+s, size-s);
			}
		} else vTaskDelay(10);
		vTaskDelay(2);
	}

    player->decoder_status = STOPPED;
    player->decoder_command = CMD_NONE;
	spiRamFifoReset();
    ESP_LOGD(TAG, "Decoder vs1053 stopped.\n");
	uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
	ESP_LOGI(TAG,"watermark: %x  %d",uxHighWaterMark,uxHighWaterMark);
	vTaskDelete(NULL);

}


