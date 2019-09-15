# Makefile


CC       = gcc
CFLAGS   = -march=native -Ofast -pipe -std=c99 -Wall
includes = _HMod/Hex-MuxerHMod.c _HMod/CHIPCore.c

FFMPEG_LIBS = libavdevice   \
              libavformat   \
              libavfilter   \
              libavcodec    \
              libswresample \
              libswscale    \
              libavutil

# export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig
CFLAGS := $(shell pkg-config --cflags $(FFMPEG_LIBS)) $(CFLAGS)
LDLIBS := $(shell pkg-config --libs   $(FFMPEG_LIBS)) $(LDLIBS) -lm


.PHONY: Hex-Muxer

Hex-Muxer: Hex-Muxer.c
	$(CC) $(CFLAGS) Hex-Muxer.c -o Hex-Muxer $(includes) $(LDLIBS)

help:
	@echo "/*****************************************************************************"
	@echo " * help : Hex-Muxer - v1.0 - 07.11.2017"
	@echo " *****************************************************************************"
	@echo " * + test  : Testlauf"
	@echo " * + clean : Aufräumen"
	@echo " *****************************************************************************/"

clean:
	find . -maxdepth 1 ! -name "*.c" ! -name "*.man" ! -name "COPYING" \
		! -name "Makefile" -type f -delete

test:
	./Hex-Muxer Testset/jellyfish-25-mbps-hd-h264.mkv Test_out.mkv 1920 1080 7 0 0

