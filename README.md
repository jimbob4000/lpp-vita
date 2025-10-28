<p align="center">
	<img src="https://github.com/gnmmarechal/lpp-vita/raw/master/banner.png?raw=true"/>
</p>

# Fork of Lua Player Plus Vita 

### Overview
This fork of [Lua Player Plus Vita](https://github.com/Rinnegatamante/lpp-vita) by Rinnegatamante and other contributors.

The purpose of the fork is to extend functionality for [RetroFlow Launcher](https://github.com/jimbob4000/RetroFlow-Launcher).

### Changes
For the most part changes have been sandboxed into the 'luaExtended.cpp' file so other functions are untouched and can be updated to match the main Lua Player Plus Vita repo.

Example uses of new functions can be found in version 8+ of [RetroFlow Launcher](https://github.com/jimbob4000/RetroFlow-Launcher).

An additional library file has been added for copyicons. See my fork of [copyicons](https://github.com/jimbob4000/copyicons) for more information. Crecdit to cy33hc for the original copyicons.  

Reading sfo files from PSP and PS1 games was made possible by using the code from [vita-launcher](https://github.com/cy33hc/vita-launcher) by cy33hc.


## Compiling the source

To correctly compile lpp-vita you'll need **vitasdk** installed and correctly set in your PATH environment variable. You'll also need the following libraries: **zlib**, **libmpg123**, **libogg**, **libvorbis**, **libsndfile**, **vita2d**, 
**libftpvita**, **libpng**, **libjpeg-turbo**, **freetype**, **libspeexdsp**, **libopus**, **lua-jit**, **libdl**. You can install most of them with **vita-portlibs** and [EasyRPG Vita toolchain](https://ci.easyrpg.org/view/Toolchains/job/toolchain-vita/).<br><br>

Lua Player Plus Vita supports different flags to enable some features:<br>
**-DSKIP_ERROR_HANDLING** disables error handling for faster code execution.<br>
**-DPARANOID** enables extra internal error handling.<br>

## About Lua Player Plus Vita 
Lua Player Plus Vita is the first lua interpreter made for the Sony PlayStation Vita.

The interpreter currently runs under LuaJIT 2.1 with slight additions from Lua 5.2.4 (for example the bit32 lib for bitwise operations). It has also a debug FTP server to correct your errors in runtime.

Official documentation: [http://rinnegatamante.github.io/lpp-vita/](http://rinnegatamante.github.io/lpp-vita/)

For the more information about Lua Player Plus Vita please see the [original repo](https://github.com/Rinnegatamante/lpp-vita).

## Credits
* **cy33hc** for vita launcher and copyicons, his work has been inspiring.
* Special thanks to **Rinnegatamante** for Lua Player Plus Vita plus for his help and support.
* vitasdk contributors.
* **xerpi** for vita2d and debug FTP code.
* **gnmmarechal** for testing the interpreter.
* **hyln9** for vita-luajit.
* **frangarcj** for the help during 3D rendering feature addition.
* **TheFloW** for some snippets i used and some ideas i borrowed.
* **Misledz** for the Lua Player Plus logo.
* **Arkanite** for providing a sample for sceAvPlayer used to implement the Video module.
* **EasyRPG Team** for the Audio Decoder used for Sound module.
* **lecram** for gifdec usd for animated GIFs support.

