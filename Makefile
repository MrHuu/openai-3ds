#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(strip $(TOOLDIR)),)
export TOOLDIR=$(DEVKITPRO)/tools/bin
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# IP address of 3DS used for 3dslink or gdb remote session (Optional)
#---------------------------------------------------------------------------------
REMOTE_IP   := 192.168.1.96

#---------------------------------------------------------------------------------
# ROMFS is the directory which contains the RomFS, relative to the Makefile (Optional)
#---------------------------------------------------------------------------------
TARGET      := openai-3ds
BUILD       := build
SOURCES     := src
DATA        := data
INCLUDES    := $(SOURCES)/include
GRAPHICS    := gfx
ROMFS       := rsrc/romfs
GFXBUILD    := $(ROMFS)/gfx

APP_TITLE         := OpenAI
APP_DESCRIPTION   := OpenAI playground for 3DS
APP_AUTHOR        := MrHuu

APP_PRODUCT_CODE    := CTR-P-OPENAI
APP_UNIQUE_ID       := 0xDF0A1
APP_VERSION_MAJOR   := 1
APP_VERSION_MINOR   := 0
APP_VERSION_MICRO   := 0

APP_RSF             := $(TOPDIR)/rsrc/template.rsf
APP_ICON            := $(TOPDIR)/rsrc/icon.png
BANNER_IMAGE_FILE   := $(TOPDIR)/rsrc/banner.png
BANNER_AUDIO_FILE   := $(TOPDIR)/rsrc/audio_silent.wav

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -O2 -mword-relocations \
			-ffunction-sections \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -D__3DS__ `curl-config --cflags`

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lflite_cmu_us_kal -lflite_cmu_us_slt
LIBS	+= -lflite_cmu_us_kal16 -lflite_cmu_us_rms -lflite_cmu_us_awb
LIBS	+= -lflite_usenglish -lflite_cmulex -lflite
LIBS	+= -lmbedx509 -lmbedtls -lmbedcrypto `curl-config --libs`
LIBS	+= -lsndfile -lpng -ljansson
LIBS	+= -lcitro2d -lcitro3d -lctru -lm -lz

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CTRULIB) $(PORTLIBS) $(TOPDIR)/lib/build

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
SHLISTFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
GFXFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.t3s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
ifeq ($(GFXBUILD),$(BUILD))
#---------------------------------------------------------------------------------
export T3XFILES :=  $(GFXFILES:.t3s=.t3x)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
export ROMFS_T3XFILES	:=	$(patsubst %.t3s, $(GFXBUILD)/%.t3x, $(GFXFILES))
export T3XHFILES		:=	$(patsubst %.t3s, $(BUILD)/%.h, $(GFXFILES))
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_SOURCES   :=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export OFILES_BIN   :=	$(addsuffix .o,$(BINFILES)) \
			$(PICAFILES:.v.pica=.shbin.o) $(SHLISTFILES:.shlist=.shbin.o) \
			$(addsuffix .o,$(T3XFILES))

export OFILES   := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES   :=	$(PICAFILES:.v.pica=_shbin.h) $(SHLISTFILES:.shlist=_shbin.h) \
			$(addsuffix .h,$(subst .,_,$(BINFILES))) \
			$(GFXFILES:.t3s=.h)

export INCLUDE   :=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(PORTLIBS)/include/SDL \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS   :=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) -L$(DEVKITPRO)/libctru/include -L$(PORTLIBS)/lib


	export APP_ICON   := $(TOPDIR)/$(ICON)

	export _3DSXFLAGS += --smdh=$(TARGET).smdh

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: all clean 3dslink gdb

#---------------------------------------------------------------------------------
all: $(BUILD) $(GFXBUILD) $(DEPSDIR) $(ROMFS_T3XFILES) $(T3XHFILES)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):

ifneq ($(GFXBUILD),$(BUILD))
$(GFXBUILD):
	@mkdir -p $@
endif

ifneq ($(DEPSDIR),$(BUILD))
$(DEPSDIR):
	@mkdir -p $@
endif

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).cia $(TARGET).elf $(GFXBUILD)

3dslink: all
	@3dslink -a $(REMOTE_IP) $(OUTPUT).3dsx

gdb: all .gdb_cmd
	@$(DEVKITARM)/bin/arm-none-eabi-gdb $(OUTPUT).elf --command $(BUILD)/.gdb_cmd

.gdb_cmd:
	@if ! [ -f $(BUILD)/.gdb_cmd ]; then \
	touch $(BUILD)/.gdb_cmd; \
	echo target remote $(REMOTE_IP):4003 > $(BUILD)/.gdb_cmd; \
    echo continue >> $(BUILD)/.gdb_cmd; \
	fi

#---------------------------------------------------------------------------------
$(GFXBUILD)/%.t3x	$(BUILD)/%.h	:	%.t3s
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@tex3ds -i $< -H $(BUILD)/$*.h -d $(DEPSDIR)/$*.d -o $(GFXBUILD)/$*.t3x

#---------------------------------------------------------------------------------
else

BANNER_IMAGE_ARG            := -i $(BANNER_IMAGE_FILE)
BANNER_AUDIO_ARG            := -a $(BANNER_AUDIO_FILE)

COMMON_MAKEROM_PARAMS       := -rsf $(APP_RSF) -target t -exefslogo -elf $(OUTPUT).elf -icon $(TARGET).smdh \
			-banner $(TARGET).bnr -DAPP_TITLE="$(APP_TITLE)" -DAPP_PRODUCT_CODE="$(APP_PRODUCT_CODE)" \
			-DAPP_UNIQUE_ID="$(APP_UNIQUE_ID)" -DAPP_SYSTEM_MODE="80MB" -DAPP_SYSTEM_MODE_EXT="Legacy" \
			-major "$(APP_VERSION_MAJOR)" -minor "$(APP_VERSION_MINOR)" -micro "$(APP_VERSION_MICRO)"

ifneq ($(APP_LOGO),)
	APP_LOGO_ID             := Homebrew
	COMMON_MAKEROM_PARAMS   += -DAPP_LOGO_ID="$(APP_LOGO_ID)" -logo $(APP_LOGO)
else
	APP_LOGO_ID             := Nintendo
	COMMON_MAKEROM_PARAMS   += -DAPP_LOGO_ID="$(APP_LOGO_ID)"
endif

ifneq ($(ROMFS),)
	APP_ROMFS               := $(TOPDIR)/$(ROMFS)
	COMMON_MAKEROM_PARAMS   += -DAPP_ROMFS="$(APP_ROMFS)"
	CXXFLAGS                += -DCTR_ROMFS
endif

ifeq ($(OS),Windows_NT)
	MAKEROM      = makerom.exe
	BANNERTOOL   = bannertool.exe
else
	MAKEROM      = $(TOOLDIR)/makerom
	BANNERTOOL   = $(TOOLDIR)/bannertool
endif

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
.PHONY : all

all               : $(OUTPUT).3dsx $(OUTPUT).cia

$(OUTPUT).3dsx    : $(OUTPUT).elf $(TARGET).smdh

$(OFILES_SOURCES) : $(HFILES)

$(OUTPUT).elf     : $(OFILES)

$(OUTPUT).cia     : $(OUTPUT).elf $(TARGET).bnr $(TARGET).smdh
	@$(MAKEROM) -f cia -o $(OUTPUT).cia -DAPP_ENCRYPTED=false $(COMMON_MAKEROM_PARAMS)
	@echo "built ... $(TARGET).cia"

$(TARGET).bnr     : $(BANNER_IMAGE_FILE) $(BANNER_AUDIO_FILE)
	@$(BANNERTOOL) makebanner $(BANNER_IMAGE_ARG) $(BANNER_AUDIO_ARG) -o $(TARGET).bnr > /dev/null

$(TARGET).smdh    : $(APP_ICON)
	@$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i $(APP_ICON) -o $(TARGET).smdh > /dev/null


#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
.PRECIOUS	:	%.t3x
#---------------------------------------------------------------------------------
%.t3x.o	%_t3x.h :	%.t3x
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
# rules for assembling GPU shaders
#---------------------------------------------------------------------------------
define shader-as
	$(eval CURBIN := $*.shbin)
	$(eval DEPSFILE := $(DEPSDIR)/$*.shbin.d)
	echo "$(CURBIN).o: $< $1" > $(DEPSFILE)
	echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u32" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(CURBIN) | tr . _)`.h
	picasso -o $(CURBIN) $1
	bin2s $(CURBIN) | $(AS) -o $*.shbin.o
endef

%.shbin.o %_shbin.h : %.v.pica %.g.pica
	@echo $(notdir $^)
	@$(call shader-as,$^)

%.shbin.o %_shbin.h : %.v.pica
	@echo $(notdir $<)
	@$(call shader-as,$<)

%.shbin.o %_shbin.h : %.shlist
	@echo $(notdir $<)
	@$(call shader-as,$(foreach file,$(shell cat $<),$(dir $<)$(file)))

#---------------------------------------------------------------------------------
%.t3x	%.h	:	%.t3s
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@tex3ds -i $< -H $*.h -d $*.d -o $*.t3x

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
