                        ....BLASH by b-state!.....                
                          ( da final version )

              ( no kewl ascii logo, so here is a placeholder )

                  first at retro demo @ demosplash 2o15

             code\music - wbc, gfx...what? no gfx - yes demo :) 

  ---------------------------------------------------------------------------

  runnable........................................(use lowres key)..p100\12mb
  watchable....................................................p133-p166\16mb
  recommended....................................................p200mmx\16mb
  c00000l................................................celeron-300a :)\32mb

  it runs at 8mb of ram but it is pretty unstable so at least 16mb is needed!  
  also vga and monitor is required. fast pci\agp vga recommended
  uses 320x200 256 color 60hz mode, letterboxing is normal
  as usual supports gus\interwave\soundblaster\covox and even pc-honker ;)

  it took me about 2 weeks for finishing because I catched a couple of evil
  and sucking bugs while fixing, so I wonder how it works after all :)
  (ничего не поделаешь, это сигавно :)

 - white ugly border on first effect - I used reverse palette for this effect
   and YEEAH, the usually-pure-black zero color stayed white. FFFUUU!
 - in freedir tunnel\planes I'm interpolating picture in 312x200 instead of
   320x200 for speedup and in order to avoid some glicthes on screen. I used
   some regs in CRTC for image centering but after debugging I accidentally 
   killed "mov dx, 0x3d4" in inline asm, so...yes, it will write some values
   to unknown I\O ports with unpredictable results :)
   fixed now ;)
 - some glitchy syncho fixed
 - greetings list expanded
 - when I assembled it, some parts started to trash some tables\textures, i.e.
   I catched some bugs in freedir planes texture (strange white dots) and some
   fades used color palettes from other effects! :)
   but this is fixed in party version already
 - timer synchonisation added! uses RTC because USMP internal timers sucks.
   now it looks nice even on P133!

   but it still does have some problems on machines slower than p100, if
   demo seems to hang but music is still played press escape and restart with
   lowres key (see below). I got crashes only on luckystar p55ce board
   with p120@75 (originally remarked p100 ;) BUT i.e quake runs for hours
   without problems so I don't know why it crashes. maybe really mobo probs?

   anyway, if you want to run it on 486......do it! ;)

  command line keys
      setup  - manual sound setup
    notimer  - disable timer syncro (recommended only on cel300++ ;)
  noretrace  - disable retrace syncro (ACHTUNG TEARING!!!)
       70hz  - 70hz failsafe mode (if 60hz one SUDDENLY does not work)
     lowres  - use special 160x200 FAST mode:
               ahh, it sucks! but can be vital for p75-p100
               demo will ask you which mode should be used
               hardware is faster but 100% works only on matrox, S3 cards will
               also work but with minor palette glitches (but it WORKS ON S3!)
               fake mode is slower than hardware but works on every vga!

 also some secret keys and real hidden part somewhere. 16+ :)

 DOES NOT WORK UNDER WINDOZE - use DOSBox (cycles=max and memsize=16 at least)

 special thanks to diver\4d and sq\skrju for moral support
 HUGE thx to superplek and gargaj for helping me fixing dat interpolator :D
 and thx to type one \ tfl-tdv for info about 320x200 60hz mode
 thanx(?) to FreddyV\useless for sound system (but next time i will use IMS :)

 and GREEEEETIIIIIIINNNNNNNZ! (in random order):

 (b)yterapers - cncd - hornet - dc5 - crtc - desire - bon^2 - rift - haujobb 
  mercury - disaster area - tpolm - fairlight - dhs - genesis project - trsi

  consciosness - demarche - z brothers - 4th dimension - triebkraft - skrju
    thesuper - fishbone - kabardcomp - hooy-program - gemba - all others

 and to people (I'm so lazy that I just copied greetslist from VOID.NFO :)

     legend - rez - destop - distance - optimus - sensenstahl - hellmood
  g0blinish - denpopov - orbitaldecay - trixter - phoenix - scali - gargaj :)
  nuyk - kotsoft - n1k-0 - diver4d - prof4d - psndcj - ts-labs - vbi - robus
 nodeus - mmcm - blastoff - wbr - buddy - sq - kowalski - kakos_nonos - evovxn
  john norton irr - bitl - f0x - manwe - breeze - trefi - tiboh - buyan - psb
   cj splinter - scalesmann - c-jeff - factor6 - brightentayle - moroz1999
 busy - mikezt - noro - evilpaul - ellvis - koshi - riskej - gasman - yerzmyey
              serzhsoft - lamer pinky - denis grachev - introspec
 
 ..and you! :)

 fuckings goes to AAA, dnik75, karbofos and the infamous LORD VADEeR \ mayhem

 that's all!                                           wbw, wbcbz7, 2o.11.2o15

 P.S. ПЯТНАДЦАТЬ смайлов! пля, меня походу Black Cat покусал (или serzhsoft :)