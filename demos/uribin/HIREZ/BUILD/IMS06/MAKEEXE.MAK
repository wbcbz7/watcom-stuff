!ifdef libpath
!ifneq libpath
cwc11include=/i=$(libpath)
!endif
!endif
copt=/onatmir /s /5r /fp5 /fpi87 /zp1 $(cwc11include)
aopt=/ml /m5
lsys=dos4g
lopt=stack=8192
cdebopt = /d2
adebopt = /zd
ldebopt = all

.silent
.erase

$(dest).exe: $(objects) $(libraries)
  echo $@
  %create $*.tmp
  for %i in ($(ldebopt)) do %append $*.tmp debug %i
  for %i in ($(objects)) do %append $*.tmp file %i
  for %i in ($(libpath)) do %append $*.tmp libpath %i
  for %i in ($(libraries)) do %append $*.tmp library %i
  for %i in ($(lopt)) do %append $*.tmp option %i
  %append $*.tmp option { map quiet caseexact dosseg }
  %append $*.tmp name $(dest)
  %append $*.tmp system $(lsys)
  wlink @$*.tmp
  del $*.tmp

.cpp.obj: .autodepend
  echo $@
  wpp386 /zq $(copt) $(cdebopt) $<

.asm.obj:
  echo $@
  tasm /t $(aopt) $(adebopt) $<

!ifdef libraries
!ifneq libraries
$(libraries):
  %null
!endif
!endif

clean: .symbolic
  if exist $(dest).tmp del $(dest).tmp
  if exist $(dest).map del $(dest).map
  for %i in ($(objects)) do if exist %i del %i

cleanall: .symbolic
  %make clean
  if exist $(dest).exe del $(dest).exe

ifdef libpath
ifeq libpath
.lib: $(libpath)
endif
endif

