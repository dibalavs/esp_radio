idf_component_register(SRCS "MerusAudio.c"
                    INCLUDE_DIRS "include"
					"../audio_player/include"
					"../../main/include"
					PRIV_REQUIRES esp_adc driver nvs_flash
					)
