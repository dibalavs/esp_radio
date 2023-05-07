idf_component_register(SRCS "rda5807.c"
                    INCLUDE_DIRS "include"
					PRIV_REQUIRES driver
					)

idf_build_set_property(COMPILE_OPTIONS "-Wno-strict-aliasing" APPEND)