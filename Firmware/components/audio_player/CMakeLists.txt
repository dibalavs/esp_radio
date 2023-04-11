idf_component_register(SRCS "audio_player.c"
                    INCLUDE_DIRS "include"
					REQUIRES mp3_decoder fdk-aac_decoder
					PRIV_REQUIRES u8g2 ucglib common fifo audio_renderer main driver
					)
