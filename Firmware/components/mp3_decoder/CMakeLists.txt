idf_component_register(SRCS "mp3_decoder.c"
                    INCLUDE_DIRS "include"
					"../audio_player/include"
					"../audio_renderer/include"
					"../../main/include"
					PRIV_REQUIRES driver fifo common mad
					)
