EE_OBJS	= trxdir3.o
EE_BIN = trxdir3.elf
EE_LIBS = -lkernel -ldraw -lgraph -ldma -lpacket

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS) 

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

emu: $(EE_BIN)
	/mnt/d/git/pcsx2/bin/pcsx2x64-avx2-dev.exe --elf="D:/c++/hwtest/trxdir3/trxdir3.elf"

reset:
	ps2client reset
	ps2client netdump

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
