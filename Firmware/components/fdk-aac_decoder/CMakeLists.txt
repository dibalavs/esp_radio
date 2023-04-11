idf_component_register(SRCS "fdk_aac_decoder.c"
                    INCLUDE_DIRS "./include"
					"../common/include"	
					"../../main/include"
					"../audio_player/include"
					"../audio_renderer/include"
					PRIV_REQUIRES  fdk-aac-oreo-m8 driver
					)
