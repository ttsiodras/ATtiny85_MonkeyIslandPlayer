MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR := $(patsubst %/,%,$(dir ${MKFILE_PATH}))
BUILD_DIR := ${CURRENT_DIR}/tmp

SRC:=beeperJukeboxTiny.ino
ALL_SRC:=$(wildcard *.ino *.h) contrib/songs_in_huffman.h

ELF:=tmp/${SRC}.elf
HEX:=tmp/${SRC}.hex

BASE:=/usr/share/arduino
USER_BASE:=$(HOME)/.arduino15
USER_LIBS:=$(HOME)/Arduino/libraries
# BOARD:=attiny:avr:ATtinyX5:cpu=attiny85,clock=internal8
BOARD:=ATTinyCore:avr:attinyx5:LTO=enable,TimerClockSource=default,chip=85,clock=8internal,eesave=aenable,bod=disable,millis=enabled
# -vid-pid=1A86_7523
HARDWARE:=-hardware ${BASE}/hardware -hardware ${USER_BASE}/packages 
TOOLS:=-tools ${BASE}/tools-builder -tools ${USER_BASE}/packages
LIBRARIES=-built-in-libraries ${BASE}/lib
LIBRARIES+=-libraries ${USER_LIBS}  # Where U8g2 comes from
WARNINGS:=-warnings all -logger human

ARDUINO_BUILDER_OPTS=${HARDWARE} ${TOOLS} ${LIBRARIES}
ARDUINO_BUILDER_OPTS+=-fqbn=${BOARD} ${WARNINGS}
ARDUINO_BUILDER_OPTS+=-verbose -build-path ${BUILD_DIR} 
# ARDUINO_BUILDER_OPTS+=-prefs=build.extra_flags=-save-temps

binary:	codegen ${ELF}  ## Build the binary.

codegen:  ## Code generation with the Huffman compressed melodies
	$(MAKE) -C contrib/ all

${ELF}:	${ALL_SRC}
	@mkdir -p ${BUILD_DIR}
	arduino-builder -compile ${ARDUINO_BUILDER_OPTS} ${SRC} 2>&1 | tee build.log

tags:	${ALL_SRC}  ## Create tags for fast navigation
	ctags -R . ${USER_LIBS} ${USER_BASE}

clean:  ## Remove all outputs of the build.
	rm -rf ${BUILD_DIR} build.log tags
	$(MAKE) -C contrib/ clean

upload:	${ELF} ## Upload to board
	avrdude -C${USER_BASE}/packages/arduino/tools/avrdude/6.3.0-arduino9/etc/avrdude.conf -v -pattiny85 -cstk500v1 -P/dev/ttyUSB0 -b19200 -Uflash:w:${HEX}:i
	avr-size ${ELF}

stats:	${ELF}  ## Show stats about the built binary
	avr-nm --print-size -t d ${ELF} \
	    | c++filt | sort -n -k 2 | awk '{a+=$$2; print a " " $$0;}' | grep -v u8x8 | tail -20
	avr-size ${ELF}

help: ## Display this help section
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z0-9_-]+:.*?## / {printf "\033[36m%-18s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)
.DEFAULT_GOAL := help

define newline # a literal \n


endef

# Makefile debugging trick:
# call print-VARIABLE to see the runtime value of any variable
# (hardened a bit against some special characters appearing in the output)
print-%:
	@echo '$*=$(subst ','\'',$(subst $(newline),\n,$($*)))'

sizes:
	avr-nm --print-size -t decimal ${ELF} | c++filt  | sort -n -k 2

.PHONY: help clean stats print-* codegen
