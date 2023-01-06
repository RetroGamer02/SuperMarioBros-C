SuperMarioBros-C 3DS
================

An attempt to translate the original Super Mario Bros. for the NES to readable C/C++.

MitchellSternke took the `smbdis.asm` disassembly of Super Mario Bros. and successfully converted it to C++ using an automated codegen program MitchellSternke wrote (which can be found in the `codegen/` subdirectory of the repo). Right now, it looks very similar to the original disassembly and is fairly dense code, but it works! Check out `source/SMB/SMB.cpp` if you're curious.

Many thanks to doppelganger (doppelheathen@gmail.com), who wrote the original comprehensive Super Mario Bros. disassembly. This can be found in the `docs/` folder of the repo.

![Demo gif](https://github.com/MitchellSternke/SuperMarioBros-C/raw/master/demo.gif)

*looks and plays just like the original*

Building
--------

**Dependencies**
- C++11 compiler
- SDL
- Make

Build
```
The project uses the devkitpro 3ds dev environment. When you have installed git on your system you can clone the repository by type in git clone https://github.com/RetroGamer02/SuperMarioBros-C.git. You will probably need the DevkitARM Patch but please make a backup of your DevkitARM folder first! Run msys2 and type make.
```

Running
-------

This requires an *unmodified* copy of the `Super Mario Bros. (JU) (PRG0) [!].nes` ROM to run. Without this, the game won't have any graphics, since the CHR data is used for rendering. By default, the program will look for this file in the 3ds/SMB directory.

Architecture
------------

The game consists of a few parts:
- The decompiled original Super Mario Bros. source code in C++
- An emulation layer, consisting of
  - Core NES CPU functionality (RAM, CPU registers, call stack, and emulation of unique 6502 instructions that don't have C++ equivalents)
  - Picture Processing Unit (PPU) emulation (for video)
  - Audio Processing Unit (APU) emulation (for sound/music)
  - Controller emulation
- SDL library for cross-platform video/audio/input

Essentially, the game is a statically recompiled version of Super Mario Bros. for modern platforms. The only part of the NES that doesn't have to be emulated is the CPU, since most instructions are now native C++ code.

The plan is to eventually ditch all of the emulation layer and convert code that relies upon it. Once that's done, this will be a true cross-platform version of Super Mario Bros. which behaves identically to the original. It could then be easily modified and extended with new features!

License
-------

TODO
