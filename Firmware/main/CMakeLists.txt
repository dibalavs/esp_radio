set(COMPONENT_SRCS
	"addon.c"
	"addoncommon.c"
	"addonu8g2.c"
	"addonucg.c"
	"app_main.c"
	"base64.c"
	"eeprom.c"
	"gpio.c"
	"interface.c"
	"irnec.c"
	"libsha1.c"
	"ntp.c"
	"ota.c"
	"servers.c"
	"telnet.c"
	"timezone.c"
	"vs1053.c"
	"vs1053b-patches.c"
	"webclient.c"
	"webserver.c"
	"websocket.c"
	"esp_radio/action_manager.c"
	"esp_radio/app_state.c"
	"esp_radio/bus.c"
	"esp_radio/buttons.c"
	"esp_radio/debug_task_stats.c"
	"esp_radio/ext_gpio.c"
	"esp_radio/i2s_redirector.c"
	"esp_radio/kmalloc.c"
	"esp_radio/merus.c"
	"esp_radio/network.c"
	"esp_radio/webserver.c"
)

set(COMPONENT_ADD_INCLUDEDIRS
	"./include/"
	"../webpage/"
	"./include/esp_radio"
)

idf_component_register(SRCS ${COMPONENT_SRCS}
                       INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
					   REQUIRES
							app_update
							audio_player
							audio_renderer
							common
							driver
							esp_adc
							esp_http_server
							esp_wifi
							fdk-aac_decoder
							fifo
							i2c_bus
							json
							mcp23017
							mdns
							MerusAudio
							mp3_decoder
							nvs_flash
							RDA5807
							spi_flash
							spiffs
							u8g2
							ucglib
					   PRIV_REQUIRES wolfssl)

idf_build_set_property(COMPILE_OPTIONS "-Wno-format" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DWOLFSSL_USER_SETTINGS" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DWOLFSSL_STATIC_RSA" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=format-overflow" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-implicit-fallthrough" APPEND)

spiffs_create_partition_image(files ../spiffs_image FLASH_IN_PROJECT)