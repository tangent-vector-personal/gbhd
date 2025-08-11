// Copyright 2011 Theresa Foley. All rights reserved.
//
// opengl.h

#ifndef GBHD_OPENGL_H
#define GBHD_OPENGL_H

#if 0

// Note: switching rendering logic to use something
// other than OpenGL... exact answer TBD.

#ifdef WIN32
#include <Windows.h>
#include <GL/glew.h>
#endif

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/GL.h>
#else
#include <GL/GL.h>
#endif

#ifdef _MSC_VER
#pragma comment(lib, "glew32.lib")
#endif

#endif

#endif // GBHD_OPENGL_H
