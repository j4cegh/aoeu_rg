# aoeu's rhythm game

A "online" rhythm game programmed in C.
It has Ouendan and Mania modes so far.

# Compiling
If you're on Windows, you can use Visual Studio 2019 and it will work out of the box, nothing else is required.

On Linux, cd into the linux/ directory and type "make d".
Make sure all the following libraries but the specific Windows ones are installed.

Libraries used:
 - SDL2
 - SDL2_Image
 - SDL2_TTF
 - CSFML (System and Network, not actually used for anything other than online)
 - Dirent (windows)
 - Discord RPC (windows)
 - Sodium
 - BASS (easiest to install on arch linux)
 - BASS FX (easiest to install on arch linux)

If you're not on Arch, you need to install BASS and BASS FX manually, sorry.
