 todo: ну всякий там стафф ;)


 system?

 p133 - minimum, p200mmx - optimal, cel300a - rulez, pii450 - overkill :)
 
 needs about 20-25 mb of free extended memory, so get at least 32 mb
 (64 mb is even better cuz i didn't know if former will suffice or not)

 although it should work under mustdie85\98 please run it from plain DOS so it
 can work A LOT faster and will not suffer from memory swapping and crashes.
 it will NOT work under win2k\xp because of incomplete DPMI support (such as
 linear memory mapping required for VESA LFB modes) in NTVDM. sorry :(

 should work fine under DOSBox, set cpu=pentium, memsize=32 and cycles=300000

 video?

 fast vesa 2.0 compatible card with LFB is required (no banked fallback yet)
 if not, install univbe or s3vbe20 (for s3 cards). >= 2mb vram recommended.

 this prod runs in 640x350 internal mode with both 8bpp and 32bpp color depths,
 so at start it will prompt you which mode should it use. generally any mode
 with 640x(something) resolution is supported but 640x480 @ 60hz is preferred.

 if possible, choose 640x480 mode with 8bpp for palette mode and with 32bpp
 for truecolor mode. if you can't, choose 24\16\15bpp mode or even a 8bpp
 grayscale fallback (though it is not recommended ;)

 banked and mode-x fallback modes will be supported in final version (maybe :)

 sound?

 by ims 0.6 by pascal\cubic (niklas, you can blame me, you know for what :)

 supports gus\interwave\sb\awe\ess\wss so you won't got nosound :)

     >>AWE32\64 USERS - run "MIXERSET.EXE /BA:192;192;50" RIGHT NOW!<< 
 because of some bass issues when emu8k mode is used (fine when in sb16 mode)

 for gus\interwave at least 1 mb of sound ram is required, for awe32\64 - 2 mb

 interpolation is disabled by default for speed reasons, you can turn it on

 known bugs?

 - tearing in 15\16\24bpp modes - triple buffering is not used if available so
   you may notice some tearing and other glitches -> use 32bpp mode instead

 .. ну там еще чота


 - strong butthurt if you are _low _voltage and _dumb - nevermind ;)


17.o9.2o16 -> addtional note
this is not an actual readme from demo, it's just a sketch ;)
truecolor stuff was planned but not included in demo, since it work WAY too slow :)