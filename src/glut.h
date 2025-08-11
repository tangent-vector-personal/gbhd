// Copyright 2011 Theresa Foley. All rights reserved.
//
// glut.h

#ifndef GBHD_GLUT_H
#define GBHD_GLUT_H

#include "opengl.h"

#ifdef __APPLE__
#include <GLUT/GLUT.h>
#else
#include <GL/glut.h>
#endif

#ifdef _MSC_VER
//#pragma comment(lib, "glut32.lib")
#endif

#endif // GBHD_GLUT_H
