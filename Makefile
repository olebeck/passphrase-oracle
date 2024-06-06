PHONY := all package clean

CC := arm-vita-eabi-gcc
CXX := arm-vita-eabi-g++
STRIP := arm-vita-eabi-strip

PROJECT_TITLE := Passphrase Oracle
PROJECT_TITLEID := PASSORACL

PROJECT := passphrase-oracle
CFLAGS += -Wl,-q
CXXFLAGS += -Wl,-q -std=c++11

SRCS=main.c
OBJS=$(SRCS:%.c=%.o)

LIBS += -lSceNet_stub -lSceVshBridge_stub -lSceRegistryMgr_stub -lSceLibKernel_stub

all: package

package: $(PROJECT).vpk

$(PROJECT).vpk: eboot.bin param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin $(PROJECT).vpk

eboot.bin: $(PROJECT).velf
	vita-make-fself -a 0x2800000000000001 $(PROJECT).velf eboot.bin

param.sfo:
	vita-mksfoex -s TITLE_ID="$(PROJECT_TITLEID)" "$(PROJECT_TITLE)" param.sfo

$(PROJECT).velf: $(PROJECT).elf
	$(STRIP) -g $<
	vita-elf-create $< $@

$(PROJECT).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROJECT).velf $(PROJECT).elf $(PROJECT).vpk param.sfo eboot.bin $(OBJS)