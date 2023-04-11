idf_component_register(SRCS "common_buffer.c"
                    INCLUDE_DIRS "include"
					PRIV_REQUIRES fifo
					)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=format" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=format-overflow" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=misleading-indentation" APPEND)
