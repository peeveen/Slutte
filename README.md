# Slutte!

This is very old source code from an emulator that I wrote in 1998, for the 1987 arcade game [Bionic Commando](https://en.wikipedia.org/wiki/Bionic_Commando_(1987_video_game)) (named "Top Secret" in Japan). This code became the basis for a driver that I submitted to the [MAME](https://www.mamedev.org/) project, way back at version [0.34b1](https://wiki.mamedev.org/index.php/MAME_0.34b1).

Everything below here is from the `SLUTTE!.TXT` file.

```
Slutte! (Beefheart version)
By Steven Frew (steven.frew@howden.com)
Official Slutte! website: http://www.geocities.com/SiliconValley/Haven/9015/
----------------------------------------------------------------------------

NOTE: The changes in this version mean that your current .CFG file might
      cause the emulator to freeze shortly after the startup screen. If this
      happens, delete your .CFG file and reconfigure.

What's New
----------
Beefheart version.
Player 2 controls now work.
Added a menu interface for setting the dipswitches.
Fixed a problem with the highscore saving.
Started work on emulating the palette intensity values, so fades kinda work.
  I don't think the intermediate colours are right, but it's a start.
StarScream v0.24 implemented. Neill's bugfixes have made the parachuting
  bonus objects move more correctly.

History
-------
18/03/98  Beefheart
12/03/98  America
10/03/98  Ahab
04/03/98  Initial release

Disclaimer
----------
If Slutte! gives you any grief, don't blame me. It's free, please don't sell
it for cash, please don't distribute the ROMs with it, thou shalt not covet
thy neighbours ass, etc ... In short, I assume no responsibility for
anything. I am quite literally irresponsible.

What Slutte! does
-----------------
Slutte! emulates the 1987 Capcom arcade game, "Bionic Commando".

What Slutte! doesn't
--------------------
So far, there are a number of known problems. These are the most prominent
ones.

1.  Graphic priority (the information determining which graphics appear in
    front of others) needs a lot more work.
2.  No sound or music.
3.  Startup self-test screen needs a hardcoded palette.
4.  Currently, Slutte! runs in a 256-colour display mode. Bionic Commando
    uses a 512 colour palette, so some graphics don't look right. This really
    only affects the text and small sections of levels 3 and 5.

Requirements
------------
Probably about 12Mb.
Some other stuff.

How to use Slutte!
------------------
First of all, you need a set of Bionic Commando ROMs. These should exist in
a directory called "BionicC". The ROMs you need are:

Name             Size
TSU_01B.ROM      32,768      Z80 code (for YM2151). Currently not emulated.
TSU_02B.ROM      65,536      68000 code
TSU_03B.ROM      65,536
TSU_04B.ROM      65,536
TSU_05B.ROM      65,536
TSU_08.ROM       32,768      Character set
TS_11.ROM        32,768      SCROLL1 tiles
TS_12.ROM        32,768
TS_13.ROM        32,768
TS_16.ROM        32,768
TS_17.ROM        32,768
TS_18.ROM        32,768
TS_23.ROM        32,768
TS_24.ROM        32,768
TSU_06.ROM       32,768      SCROLL2 tiles
TSU_07.ROM       32,768
TSU_09.ROM       32,768      Sprites
TSU_10.ROM       32,768
TSU_14.ROM       32,768
TSU_15.ROM       32,768
TSU_19.ROM       32,768
TSU_20.ROM       32,768
TSU_21.ROM       32,768
TSU_22.ROM       32,768

Then you can run Slutte! with some options:

-? or /?        Displays a quick bit of info about these options.
-res xxx yyy    Specifies a VESA 2.0 resolution to use. You MUST have a VESA
                2.0 driver (or compatible graphics card) in order to use this
                option. The only VESA driver I know of is "SciTech Display
                Doctor", and a trial version of this is available on the net
                from www.scitechsoft.com. Bionic Commando runs in a 256x240
                resolution, so use a resolution close to that if you can. If
                you get a "VBE 2.0 not available" error, then you DON'T HAVE
                A VESA 2.0 DRIVER, and you should use the -modex option (see
                next).
-modex          Use a 320x240 ModeX display. This should work with all VGA
                graphics cards. This is the first time I've used ModeX, and
                it's not perfect: there's a bit of "shearing", even with the
                vsync enabled. This might get better as I investigate ModeX a
                little more thoroughly.
-k              Use keyboard controls. If you haven't used keyboard before,
                it'll automatically ask you to define the keys you want to
                use. Keyboard is selected as default.
-j              Use joystick controls.
-v ON/OFF       Enable or disable vsync. Default is OFF. I originally thought
                that with this on, the game ran a little too slowly, but I've
                since compared the in-game time counter with the actual
                arcade machine, and it seems to be just about spot on. If
                vsync is disabled, Slutte! runs as fast as possible.
-definekeys     Allows you to redefine your keys before the emulator starts.
-calibrate      Allows you to calibrate your joystick if Slutte! is reading
                it incorrectly. Values are written to Slutte!.CLB, so you
                only need to do this once.

When you run the emulator, it'll save your options into Slutte!.CFG. If you
want to use the same options again next time, just run Slutte! without any
parameters. Slutte! automatically reads Slutte!.CFG when it starts up, and
then overrides any options that it finds in that file with command line
options. Slutte! ought to have no problem with .CFG files from previous
versions, but if your old config doesn't work with a newer version, delete
Slutte!.CFG and reconfigure.

While the emulator is running, the following keys do the following things:

1             Insert a coin in the player 1 slot
2             Insert a coin in the player 2 slot
3             Start a one player game.
4             Start a two player game. Currently, player 2 uses the same
              controls as player 1.
F12           Grab the screen as a PCX file.
Tab           Go to the dipswitch menu. While in the menu, cursor keys change
              the settings, and TAB takes you back to the game. Dipswitch
              values are saved into Slutte!.CFG when you exit the emulator.
Pause         Pause the game. Any key to restart.
Escape        Quit the emulator.

Credits
-------
Thanks to everyone who has sent me messages of encouragement since Slutte!
started.

Thanks to Jason Fourier and Jason Sizemore for betatesting.

Incredible amounts of gratitude to Shawn Hargreaves for the Allegro library.

Biggest thanks of all to Neill Corlett for his outstanding StarScream 68000
emulator.
```