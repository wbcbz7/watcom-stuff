copt=/onatmir /s /5r /fp5 /fpi87 /zp1
aopt=/ml /m5
libopt=/b /n
#cdebopt = /d2
#adebopt = /zd

.silent
.erase

$(dest).lib: $(objects)
  echo $@
  %create $(dest).tmp
  for %i in ($(objects)) do @%append $(dest).tmp +%i
  wlib /q $(libopt) $(dest).lib @$(dest).tmp
  del $(dest).tmp

.cpp.obj:
  echo $@
  wpp386 /zq $(copt) $(cdebopt) $<

.asm.obj:
  echo $@
  tasm /t $(aopt) $(adebopt) $<

clean: .symbolic
  if exist $(dest).tmp del $(dest).tmp
  for %i in ($(objects)) do if exist %i del %i

cleanall: .symbolic
  %make clean
  if exist $(dest).lib del $(dest).lib
