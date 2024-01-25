TARGET   = grabxms

BUILD    = dos
SYSTEM   = dos
DEBUG    = all
DLEVEL   = 0

INCLUDE  = .

O        = obj
C        = cpp
A        = asm

AS       = fasm.exe
CC       = wpp.exe
LD       = wlink.exe
CFLAGS   = -ms -0 -i=$(INCLUDE) -d$(DLEVEL) -bt=$(BUILD)
LFLAGS   =
AFLAGS   =

OBJS     = $(TARGET).$(O)
OBJSTR   = file {$(OBJS)}

all: $(TARGET).exe

$(TARGET).exe: $(OBJS) .symbolic
	%write $(TARGET).lnk debug $(DEBUG)
	%write $(TARGET).lnk name $(TARGET)
	%write $(TARGET).lnk $(OBJSTR) 
	%write $(TARGET).lnk system $(SYSTEM)
	$(LD) @$(TARGET).lnk $(LFLAGS)
	del $(TARGET).lnk

.$(C).$(O):
	$(CC) $< $(CFLAGS)

.$(A).$(O):
	$(AS) $< $(AFLAGS)
	
# clean all
clean: .symbolic
	del *.$(O)
	del $(TARGET).exe
	del *.errr