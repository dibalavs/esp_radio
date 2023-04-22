/******************************************************************************
 *
 * Copyright 2017 karaw(http://www.karawin.fr)
 *
*******************************************************************************/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define TAG "GPIO"
#include "driver/gpio.h"
#include "gpio.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "app_main.h"
#include "eeprom.h"


static SemaphoreHandle_t muxnvs= NULL;
const char hardware[] = {"hardware"};
const char option_space[] = {"option_space"};
const char gpio_space[] = {"gpio_space"};
const char label_space[] = {"label_space"};

// open and read the gpio hardware setting
//
esp_err_t open_partition(const char *partition_label, const char *namespace,nvs_open_mode open_mode,nvs_handle *handle)
{
	esp_err_t err;
	if (muxnvs == NULL) muxnvs=xSemaphoreCreateMutex();
	xSemaphoreTake(muxnvs, portMAX_DELAY);
	err = nvs_flash_init_partition(partition_label);
	if (err != ESP_OK) {ESP_LOGD(TAG,"Hardware partition not found"); return err;}
//	ESP_ERROR_CHECK(nvs_open_from_partition(partition_label, namespace, open_mode, handle));
	err = nvs_open_from_partition(partition_label, namespace, open_mode, handle);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG,"Namespace %s not found, ERR: %x",namespace,err);
		nvs_flash_deinit_partition(partition_label);
		xSemaphoreGive(muxnvs);
	}
	return err;
}
void close_partition(nvs_handle handle,const char *partition_label)
{
	nvs_commit(handle); // if a write is pending
	nvs_close(handle);
	nvs_flash_deinit_partition(partition_label);
	xSemaphoreGive(muxnvs);
}

void gpio_get_label(char** label)
{
	size_t required_size;
	nvs_handle hardware_handle;
	*label = NULL;
	if (open_partition(hardware, label_space,NVS_READONLY,&hardware_handle)!= ESP_OK)
	{
		ESP_LOGD(TAG,"get label");
		return;
	}
	nvs_get_str(hardware_handle, "L_LABEL", NULL, &required_size);
	if (required_size >1)
	{
		*label = kmalloc(required_size);
		nvs_get_str(hardware_handle, "L_LABEL", *label, &required_size);
		ESP_LOGV(TAG,"Label: \"%s\"\n Required size: %d",*label,required_size);
	}
	close_partition(hardware_handle,hardware);
}

void gpio_get_comment(char** label)
{
	size_t required_size;
	nvs_handle hardware_handle;
	*label = NULL;
	if (open_partition(hardware, label_space,NVS_READONLY,&hardware_handle)!= ESP_OK)
	{
		ESP_LOGD(TAG,"get comment");
		return;
	}
	nvs_get_str(hardware_handle, "L_COMMENT", NULL, &required_size);
	if (required_size >1)
	{
		*label = kmalloc(required_size);
		nvs_get_str(hardware_handle, "L_COMMENT", *label, &required_size);
		ESP_LOGV(TAG,"Label: \"%s\"\n Required size: %d",*label,required_size);
	}
	close_partition(hardware_handle,hardware);
}

void option_get_audio_output(output_mode_t *oom)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*oom = I2S;
	if (open_partition(hardware, option_space,NVS_READONLY,&hardware_handle)!= ESP_OK)
	{
		ESP_LOGD(TAG,"get_audio");
		return;
	}
	err = nvs_get_u8(hardware_handle, "O_AUDIO",(uint8_t *) oom);
	if (err != ESP_OK) ESP_LOGD(TAG,"get_audio err 0x%x",err);
	close_partition(hardware_handle,hardware);
}

void option_get_lcd_info(uint8_t *enca,uint8_t* rt)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	uint8_t typ,rot;
	// init default
	*enca = g_device->lcd_type;
	*rt = ((g_device->options32)&T_ROTAT)?1:0;
	if (open_partition(hardware, option_space,NVS_READONLY,&hardware_handle)!= ESP_OK)
	{
		ESP_LOGD(TAG,"lcd_info");
		return;
	}

	err = nvs_get_u8(hardware_handle, "O_LCD_TYPE",(uint8_t *) &typ);
	err |= nvs_get_u8(hardware_handle, "O_LCD_ROTA",(uint8_t *) &rot);
	if (typ != 255) *enca = typ;
	if (rot != 255) *rt = rot;
	if (*rt) *rt = 1;
	if (err != ESP_OK) ESP_LOGD(TAG,"oget_lcd_info err 0x%x",err);
	close_partition(hardware_handle,hardware);
}
void option_set_lcd_info(uint8_t enca, uint8_t rt)
{
	esp_err_t err;
	nvs_handle hardware_handle;

	if (open_partition(hardware, option_space,NVS_READWRITE,&hardware_handle)!= ESP_OK)
	{
		ESP_LOGD(TAG,"set lcd_info");
		return;
	}

	err = nvs_set_u8(hardware_handle, "O_LCD_TYPE",enca);
	err |= nvs_set_u8(hardware_handle, "O_LCD_ROTA",rt?1:0);
	if (err != ESP_OK) ESP_LOGD(TAG,"oset_lcd_info err 0x%x",err);

	close_partition(hardware_handle,hardware);
}

void option_get_ddmm(uint8_t *enca)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	uint8_t dmm;
	// init default
	*enca = ((g_device->options32)&T_DDMM)?1:0;;

	if (open_partition(hardware, option_space,NVS_READONLY,&hardware_handle)!= ESP_OK)
	{
		ESP_LOGD(TAG,"ddmm");
		return;
	}

	err = nvs_get_u8(hardware_handle, "O_DDMM_FLAG",(uint8_t *) &dmm);

	if (err != ESP_OK) ESP_LOGD(TAG,"oget_ddmm err 0x%x",err);
	else{
		if (dmm != 255) *enca = dmm;
		if (*enca) *enca = 1;
	}

	close_partition(hardware_handle,hardware);
}
void option_set_ddmm(uint8_t enca)
{
	esp_err_t err;
	nvs_handle hardware_handle;

	if (open_partition(hardware, option_space,NVS_READWRITE,&hardware_handle)!= ESP_OK)
	{
		ESP_LOGD(TAG,"set_ddmm");
		return;
	}

	err = nvs_set_u8(hardware_handle, "O_DDMM_FLAG",enca?1:0);
	if (err != ESP_OK) ESP_LOGD(TAG,"oset_ddmm err 0x%x",err);

	close_partition(hardware_handle,hardware);
}


void option_set_lcd_out(uint32_t enca)
{
	esp_err_t err;
	nvs_handle hardware_handle;

	if (open_partition(hardware, option_space,NVS_READWRITE,&hardware_handle)!= ESP_OK)
	{
		ESP_LOGD(TAG,"set_lcd_out");
		return;
	}

	err = nvs_set_u32(hardware_handle, "O_LCD_OUT",enca);
	if (err != ESP_OK) ESP_LOGD(TAG,"oset_lcd_out err 0x%x",err);

	close_partition(hardware_handle,hardware);
}
void option_set_lcd_stop(uint32_t enca)
{
	esp_err_t err;
	nvs_handle hardware_handle;

	if (open_partition(hardware, option_space,NVS_READWRITE,&hardware_handle)!= ESP_OK)
	{
		ESP_LOGD(TAG,"set_lcd_stop");
		return;
	}

	err = nvs_set_u32(hardware_handle, "O_LCD_STOP",enca);
	if (err != ESP_OK) ESP_LOGD(TAG,"oset_lcd_stop err 0x%x",err);

	close_partition(hardware_handle,hardware);
}

void option_get_lcd_out(uint32_t *enca, uint32_t *encb)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	uint32_t lout;

	// init default lcd_out
//		*enca = g_device->lcd_out;
	*enca = 0;
	// init default lcd_stop
	*encb = 0;

	if (open_partition(hardware, option_space,NVS_READWRITE,&hardware_handle)!= ESP_OK)
	{
		ESP_LOGD(TAG,"lcd_out");
		return;
	}

	err = nvs_get_u32(hardware_handle, "O_LCD_OUT",(uint32_t *) &lout);
	if (err == ESP_ERR_NVS_NOT_FOUND)
		err = nvs_set_u32(hardware_handle, "O_LCD_OUT",*enca);
	if (err != ESP_OK)
	{
		ESP_LOGD(TAG,"oget_lcd_out err 0x%x",err);
	}
	else
	{
		if (lout == 255) lout = 0; // special case
		*enca = lout;
	}

	err = nvs_get_u32(hardware_handle, "O_LCD_STOP",(uint32_t *) &lout);
	if (err == ESP_ERR_NVS_NOT_FOUND)
		err = nvs_set_u32(hardware_handle, "O_LCD_STOP",*encb);
	if (err != ESP_OK)
	{
		ESP_LOGI(TAG,"oget_lcd_STOP err 0x%x",err);
	}
	else
	{
		if (lout == 255) lout = 0; // special case
		*encb = lout;
	}
	close_partition(hardware_handle,hardware);
}

void gpio_get_ir_signal(gpio_num_t *ir)
{
	esp_err_t err;
	nvs_handle hardware_handle;
	// init default
	*ir = PIN_IR_SIGNAL;

	if (open_partition(hardware, gpio_space,NVS_READONLY,&hardware_handle)!= ESP_OK)
	{
		ESP_LOGD(TAG,"ir");
		return;
	}

	err = nvs_get_u8(hardware_handle, "P_IR_SIGNAL",(uint8_t *) ir);
	if (err != ESP_OK) ESP_LOGD(TAG,"g_get_ir_signal err 0x%x",err);

	close_partition(hardware_handle,hardware);
}

uint8_t gpioToChannel(uint8_t gpio)
{
	if (gpio == GPIO_NONE) return GPIO_NONE;
	if (gpio >= 38) return (gpio-36);
	else return (gpio-28);
}

bool gpio_get_ir_key(nvs_handle handle,const char *key, uint32_t *out_value1 , uint32_t *out_value2)
{
	// init default
	bool ret = false;
	*out_value1 = 0;
	*out_value2 = 0;
	size_t required_size;
	nvs_get_str(handle, key, NULL, &required_size);
	if (required_size >1)
	{
		char* string = kmalloc(required_size);
		nvs_get_str(handle, key, string, &required_size);
		sscanf(string,"%x %x",out_value1,out_value2);
//		ESP_LOGV(TAG,"String \"%s\"\n Required size: %d",string,required_size);
		free (string);
		ret = true;
	}
	ESP_LOGV(TAG,"Key: %s, value1: %x, value2: %x, ret: %d",key,*out_value1,*out_value2,ret);

	return ret;
}
