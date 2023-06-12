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
	"esp_radio/bus.c"
	"esp_radio/buttons.c"
	"esp_radio/debug_task_stats.c"
	"esp_radio/ext_gpio.c"
	"esp_radio/i2s_redirector.c"
	"esp_radio/merus.c"
	"esp_radio/network.c"
)

set(COMPONENT_ADD_INCLUDEDIRS
	"./include/"
	"../webpage/"
	"./include/esp_radio"
)

idf_component_register(SRCS ${COMPONENT_SRCS}
                       INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
					   REQUIRES i2c_bus mcp23017 RDA5807 esp_adc mdns u8g2 ucglib spi_flash fifo driver esp_wifi nvs_flash audio_renderer audio_player mp3_decoder fdk-aac_decoder MerusAudio common app_update
					   PRIV_REQUIRES wolfssl)

idf_build_set_property(COMPILE_OPTIONS "-Wno-format" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DWOLFSSL_USER_SETTINGS" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DWOLFSSL_STATIC_RSA" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=format-overflow" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-implicit-fallthrough" APPEND)
