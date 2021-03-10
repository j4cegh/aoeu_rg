# aoeu's rhythm game

A "online" rhythm game programmed in C.
It has Ouendan and Mania modes so far.

# Compiling
If you're on Windows, you can use Visual Studio 2019 and it will work out of the box, nothing else is required.

On Linux, type "make d -j4".
Make sure all the following libraries but the specific Windows ones are installed.

If res/font.otf isn't present, the game will segfault.

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

Install libraries on debian based OSes:
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev libcsfml-dev libsodium-dev libglew-dev

If you're on linux, use the bass_install.sh script to install bass and bass_fx.
If you're on arch, just use the AUR to install bass and bass_fx.
