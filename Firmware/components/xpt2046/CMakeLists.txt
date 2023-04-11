idf_component_register(SRCS "xpt2046.c"
                    INCLUDE_DIRS "."
					PRIV_REQUIRES main driver nvs_flash u8g2 ucglib audio_renderer common
					)
