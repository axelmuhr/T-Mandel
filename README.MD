About
-----
This is the source of the "CSA Mandelzoom" tool, created by Computer System Architects (CSA) around 1989.

It maps an existing Transputer network and loads workers onto every T4xx/T8xx found. Integer as well as floatingpoint workers are provided. Graphical output is either CGS, EGA or VGA currently at 16 colors max.

Prerequisites
-------------
* At least one Transputer (does not make sense without one)
* A x86 PC running [MS|Free|Open|PC]DOS - yes, it's DOS only
* Borland C-compiler (I use BC++ 3.1)
* LSC Transputer cross C-Compiler (use this [link to download](http://www.classiccmp.org/transputer/software/languages/ansic/lsc/lsc-V89.1.tar.gz)

Enhancements
------------
This Version was pushed to the 3.0 level, as the graphics output was *heavily* altered:
+ intro'd `optarg` to replace the oldschool argv parsing 
* CGA, Hercules and EGA support were removed from the code
* VGA 640x480/16 is now standard
* The /v switch enables VESA VBE graphics. '/vshow' displays all available 8bit (256 color) modes, default is 640x480/256


Beyond this, there are still all other mods from 2.x, like
* a high-precision timer, mainly used for benchmarking
* /x for verbose Transputer-comms output (for debugging)

But still there are many things to do:
- [x] Better graphics support (higher resoluton, more colors)
- [x] Rubberband function in VBE not working
- [ ] Better Transputer detection (crashed if T2xx or C004 are found)
- [ ] Better Timer-Lib. This current is an eval version, manufacturer vanished, can't remove nag-screen.
- [ ] Enhance link adaptor support (currently B004 interface only)
- [ ] Mouse control?
- [ ] ...

Learn more about the tool at [this page](http://www.geekdot.com/basic-transputer-tools)

Several benchmarking results can be found [here](http://www.geekdot.com/lies-damn-lies-and-benchmarks)

In doubt enter "man /?" for options. 
