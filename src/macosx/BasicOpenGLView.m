// Copyright (C) 2011 Tim Foley. All Rights Reserved.
//
// BasicOpenGLView.m

#import "BasicOpenGLView.h"

#include <mach/mach.h>
#include <mach/mach_time.h>

// ==================================

static CFAbsoluteTime gStartTime = 0.0f;

// set app start time
static void setStartTime (void)
{	
	gStartTime = CFAbsoluteTimeGetCurrent ();
}

// ---------------------------------

// return float elpased time in seconds since app start
static CFAbsoluteTime getElapsedTime (void)
{	
	return CFAbsoluteTimeGetCurrent () - gStartTime;
}

// ===================================

@implementation BasicOpenGLView

// pixel format definition
+ (NSOpenGLPixelFormat*) basicPixelFormat
{
    NSOpenGLPixelFormatAttribute attributes [] = {
        NSOpenGLPFAWindow,
        NSOpenGLPFADoubleBuffer,	// double buffered
        NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)16, // 16 bit depth buffer
        (NSOpenGLPixelFormatAttribute)nil
    };
    return [[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes] autorelease];
}

// ---------------------------------

// handles resizing of GL need context update and if the window dimensions change, a
// a window dimension update, reseting of viewport and an update of the projection matrix
- (void) resizeGL
{
	NSRect rectView = [self bounds];
	
    glViewport (0, 0, rectView.size.width, rectView.size.height);
}

// ---------------------------------

// given a delta time in seconds and current rotation accel, velocity and position, update overall object rotation
- (void) updateObjectRotationForTimeDelta:(CFAbsoluteTime)deltaTime
{
}

// ---------------------------------

// per-window timer function, basic time based animation preformed here
- (void)animationTimer:(NSTimer *)timer
{
    static uint64_t lastTime = 0;
    if( lastTime == 0 )
        lastTime = mach_absolute_time();
    uint64_t thisTime = mach_absolute_time();
    
    uint64_t elapsedTime = thisTime - lastTime;
    lastTime = thisTime;
    
    AbsoluteTime absTime = *((AbsoluteTime*) &elapsedTime);
    Nanoseconds nanoTime = AbsoluteToNanoseconds(absTime);
    
    uint64_t elapsedNanos = *((uint64_t*) &nanoTime);

    int deltaTime = (int) (elapsedNanos / 1000000);
    
    extern void UpdateCore(int deltaTime);
    UpdateCore(deltaTime);
    
	time = CFAbsoluteTimeGetCurrent (); //reset time in all cases
    [self drawRect:[self bounds]]; // redraw now instead dirty to enable updates during live resize
}

#pragma mark ---- Method Overrides ----

enum KeyCodes
{
    kKeyCode_S = 1,
    kKeyCode_T = 17,
    kKeyCode_A = 0,
    kKeyCode_B = 11,
    kKeyCode_I = 34,
    kKeyCode_J = 38,
    kKeyCode_K = 40,
    kKeyCode_L = 37,
};

-(void)keyDown:(NSEvent *)theEvent
{
    extern void KeyDownCore(int c);

    switch( [theEvent keyCode] )
    {
    case kKeyCode_S:
        KeyDownCore('s');
        break;
    case kKeyCode_T:
        KeyDownCore('t');
        break;
    case kKeyCode_A:
        KeyDownCore('a');
        break;
    case kKeyCode_B:
        KeyDownCore('b');
        break;
    case kKeyCode_I:
        KeyDownCore('i');
        break;
    case kKeyCode_J:
        KeyDownCore('j');
        break;
    case kKeyCode_K:
        KeyDownCore('k');
        break;
    case kKeyCode_L:
        KeyDownCore('l');
        break;
    default:
        break;
    }
}

-(void)keyUp:(NSEvent *)theEvent
{
    extern void KeyUpCore(int c);

    switch( [theEvent keyCode] )
    {
    case kKeyCode_S:
        KeyUpCore('s');
        break;
    case kKeyCode_T:
        KeyUpCore('t');
        break;
    case kKeyCode_A:
        KeyUpCore('a');
        break;
    case kKeyCode_B:
        KeyUpCore('b');
        break;
    case kKeyCode_I:
        KeyUpCore('i');
        break;
    case kKeyCode_J:
        KeyUpCore('j');
        break;
    case kKeyCode_K:
        KeyUpCore('k');
        break;
    case kKeyCode_L:
        KeyUpCore('l');
        break;
    default:
        break;
    }
}

// ---------------------------------

- (void) drawRect:(NSRect)rect
{
    [[self openGLContext] makeCurrentContext];
    [[self openGLContext] update];
    
    
    static bool didInit = false;
    if( !didInit )
    {
        extern void InitCore();
        InitCore();
        didInit = true;
    }
    
	// setup viewport and prespective
	[self resizeGL]; // forces projection matrix update (does test for size changes)
	// [self updateModelView];  // update model view matrix for object

	// clear our drawable
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    extern void DrawCore();
    DrawCore();
    
	
	// model view and projection matricies already set

    [[self openGLContext] flushBuffer];
}

// ---------------------------------

// set initial OpenGL state (current context is set)
// called after context is created
- (void) prepareOpenGL
{
    long swapInt = 1;

    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval]; // set to vbl sync

	// init GL stuff here
	glEnable(GL_DEPTH_TEST);

	glShadeModel(GL_SMOOTH);    
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glPolygonOffset (1.0f, 1.0f);
	
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

// ---------------------------------

-(id) initWithFrame: (NSRect) frameRect
{
	NSOpenGLPixelFormat * pf = [BasicOpenGLView basicPixelFormat];

	self = [super initWithFrame: frameRect pixelFormat: pf];
    return self;
}

// ---------------------------------

- (BOOL)acceptsFirstResponder
{
  return YES;
}

// ---------------------------------

- (BOOL)becomeFirstResponder
{
  return  YES;
}

// ---------------------------------

- (BOOL)resignFirstResponder
{
  return YES;
}

// ---------------------------------

- (void) awakeFromNib
{
	setStartTime (); // get app start time
	
	time = CFAbsoluteTimeGetCurrent ();  // set animation time start time

	// start animation timer
	timer = [NSTimer timerWithTimeInterval:(1.0f/60.0f) target:self selector:@selector(animationTimer:) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
	[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSEventTrackingRunLoopMode]; // ensure timer fires during resize
}


@end

