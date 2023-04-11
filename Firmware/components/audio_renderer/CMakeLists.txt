idf_component_register(SRCS "audio_renderer.c"
                    INCLUDE_DIRS "include"
					"../../main/include"
					PRIV_REQUIRES driver nvs_flash common fifo fdk-aac_decoder main audio_player MerusAudio
					)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=misleading-indentation" APPEND)