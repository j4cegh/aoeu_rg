#pragma once

#if defined(_WIN32) || defined(WIN32) || defined(WIN64)
#define USING_WINDOWS
#endif

#if __TINYC__
#define SDL_DISABLE_IMMINTRIN_H
#endif

#ifdef USING_WINDOWS
#ifndef _MSC_VER
#define SDL_MAIN_HANDLED
#endif
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#ifndef USING_WINDOWS
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef USING_WINDOWS
#include <direct.h>
#endif

#define RES_DIR "./res/"
#define BORDER_THICKNESS 1
#define PADDING 2
#define KEY_COUNT SDL_NUM_SCANCODES

#define COMPILE_MSG "compiled on " __DATE__ " at " __TIME__
#define COMPILE_TIME __DATE__ " " __TIME__

#define PI_NUM 3.14159265f

#define ASCII_START 32
#define ASCII_END 126
#define USEFUL_ASCII_CHARS " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
#define ASCII_NUMBERS "0123456789."

#include "timer.h"
#include "dir.h"
#include "color.h"
#include "vec.h"
#include "rect.h"
#include "gl_helper.h"
