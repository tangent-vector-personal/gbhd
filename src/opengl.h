// Copyright 2011 Tim Foley. All rights reserved.
//
// opengl.h

#ifndef GBHD_OPENGL_H
#define GBHD_OPENGL_H

#ifdef WIN32
#include <Windows.h>
#include <GL/glew.h>
#endif

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#else
#include <GL/GL.h>
#endif

#ifdef _MSC_VER
#pragma comment(lib, "glew32.lib")
#endif

#endif // GBHD_OPENGL_H
