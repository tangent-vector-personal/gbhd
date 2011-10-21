// Copyright 2011 Tim Foley. All rights reserved.
//
// opengl.h

#ifndef GBHD_OPENGL_H
#define GBHD_OPENGL_H

#ifdef WIN32
#include <Windows.h>
#include <GL/glew.h>
#endif

#include <GL/GL.h>

#ifdef _MSC_VER
#pragma comment(lib, "glew32.lib")
#endif

#endif // GBHD_OPENGL_H
