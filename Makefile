PROJECT_NAME     := blinky_pca10059_mbr
TARGETS          := nrf52840_xxaa
OUTPUT_DIRECTORY := _build

SDK_ROOT := ../../../../../..
PROJ_DIR := ../../..

$(OUTPUT_DIRECTORY)/nrf52840_xxaa.out: \
  LINKER_SCRIPT  := ./config/blinky_gcc_nrf52.ld

# Source files common to all targets
SRC_FILES += \
 ./modules/nrfx/mdk/gcc_startup_nrf52840.S \
 ./components/libraries/log/src/nrf_log_frontend.c \
 ./components/libraries/log/src/nrf_log_str_formatter.c \
 ./components/boards/boards.c \
 ./components/libraries/util/app_error.c \
 ./components/libraries/util/app_error_handler_gcc.c \
 ./components/libraries/util/app_error_weak.c \
 ./components/libraries/util/app_util_platform.c \
 ./components/libraries/util/nrf_assert.c \
 ./components/libraries/atomic/nrf_atomic.c \
 ./components/libraries/balloc/nrf_balloc.c \
 ./external/fprintf/nrf_fprintf.c \
 ./external/fprintf/nrf_fprintf_format.c \
 ./components/libraries/memobj/nrf_memobj.c \
 ./components/libraries/ringbuf/nrf_ringbuf.c \
 ./components/libraries/strerror/nrf_strerror.c \
 ./modules/nrfx/soc/nrfx_atomic.c \
 ./main.c \
 ./modules/nrfx/mdk/system_nrf52840.c \

# Include folders common to all targets
INC_FOLDERS += \
 ./components \
 ./modules/nrfx/mdk \
 ./components/softdevice/mbr/headers \
 ./components/libraries/strerror \
 ./components/toolchain/cmsis/include \
 ./components/libraries/util \
 ./config \
 ./components/libraries/balloc \
 ./components/libraries/ringbuf \
 ./modules/nrfx/hal \
 ./components/libraries/bsp \
 ./components/libraries/log \
 ./modules/nrfx \
 ./components/libraries/experimental_section_vars \
 ./components/libraries/delay \
 ./integration/nrfx \
 ./components/drivers_nrf/nrf_soc_nosd \
 ./components/libraries/atomic \
 ./components/boards \
 ./components/libraries/memobj \
 ./external/fprintf \
 ./components/libraries/log/src \

# Libraries common to all targets
LIB_FILES += \

# Optimization flags
OPT = -O3 -g3
# Uncomment the line below to enable link time optimization
#OPT += -flto

# C flags common to all targets
CFLAGS += $(OPT)
CFLAGS += -DBOARD_PCA10059
CFLAGS += -DBSP_DEFINES_ONLY
CFLAGS += -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DFLOAT_ABI_HARD
CFLAGS += -DMBR_PRESENT
CFLAGS += -DNRF52840_XXAA
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -Wall -Werror
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# keep every function in a separate section, this allows linker to discard unused ones
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin -fshort-enums

# C++ flags common to all targets
CXXFLAGS += $(OPT)
# Assembler flags common to all targets
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
ASMFLAGS += -DBOARD_PCA10059
ASMFLAGS += -DBSP_DEFINES_ONLY
ASMFLAGS += -DCONFIG_GPIO_AS_PINRESET
ASMFLAGS += -DFLOAT_ABI_HARD
ASMFLAGS += -DMBR_PRESENT
ASMFLAGS += -DNRF52840_XXAA

# Linker flags
LDFLAGS += $(OPT)
LDFLAGS += -mthumb -mabi=aapcs -L ./modules/nrfx/mdk -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
# let linker dump unused sections
LDFLAGS += -Wl,--gc-sections
# use newlib in nano version
LDFLAGS += --specs=nano.specs

nrf52840_xxaa: CFLAGS += -D__HEAP_SIZE=8192
nrf52840_xxaa: CFLAGS += -D__STACK_SIZE=8192
nrf52840_xxaa: ASMFLAGS += -D__HEAP_SIZE=8192
nrf52840_xxaa: ASMFLAGS += -D__STACK_SIZE=8192

# Add standard libraries at the very end of the linker input, after all objects
# that may need symbols provided by these libraries.
LIB_FILES += -lc -lnosys -lm


.PHONY: default help

# Default target - first one defined
default: nrf52840_xxaa

# Print all targets that can be built
help:
	@echo following targets are available:
	@echo		nrf52840_xxaa
	@echo		flash      - flashing binary

TEMPLATE_PATH :=./components/toolchain/gcc


include $(TEMPLATE_PATH)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

.PHONY: flash

# Flash the program
DFU_PORT ?= /dev/ttyACM0

flash: default
	@echo Flashing: $(OUTPUT_DIRECTORY)/nrf52840_xxaa.hex
	nrfutil pkg generate $(OUTPUT_DIRECTORY)/app.zip \
	    --hw-version 52 \
	    --sd-req 0 \
	    --application $(OUTPUT_DIRECTORY)/nrf52840_xxaa.hex \
	    --application-version 1 \
	#    --softdevice ./components/softdevice/s113/hex/s113_nrf52_7.2.0_softdevice.hex \
	#    --sd-id 0x0102 \
	      >/dev/null
	nrfutil dfu usb-serial \
	    --package $(OUTPUT_DIRECTORY)/app.zip \
	    --port $(DFU_PORT)
