idf_component_register(SRCS 
							"align.c"
							"bit.c"
							"decoder.c"
							"fixed.c"
							"frame.c"
							"huffman.c"
							"layer12.c"
							"layer3.c"
							"stream.c"
							"synth_stereo.c"
							"timer.c"
							"version.c"
                    INCLUDE_DIRS "."
					
					)

idf_build_set_property(COMPILE_OPTIONS "-Wno-error=unused-label" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=return-type" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=missing-braces" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=stringop-overflow" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=pointer-sign" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=parentheses" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=maybe-uninitialized" APPEND)
