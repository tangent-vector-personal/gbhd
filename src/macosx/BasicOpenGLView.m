// Copyright (C) 2011 Tim Foley. All Rights Reserved.
//
// BasicOpenGLView.m

#import "BasicOpenGLView.h"

#include <mach/mach.h>
#include <mach/mach_time.h>

@implementation BasicOpenGLView

// pixel format definition
+ (NSOpenGLPixelFormat*) basicPixelFormat
{
    NSOpenGLPixelFormatAttribute attributes [] = {
        NSOpenGLPFAWindow,
        NSOpenGLPFADoubleBuffer,	// double buffered
        NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)16, // 16 bit depth buffer
        NSOpenGLPFAStencilSize, (NSOpenGLPixelFormatAttribute)8,
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
    int width = rectView.size.width;
    int height = rectView.size.height;
	
    glViewport (0, 0, width, height);
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
#if NOPE
    [[self openGLContext] makeCurrentContext];
    [[self openGLContext] update];
    
    
	// setup viewport and prespective
	[self resizeGL]; // forces projection matrix update (does test for size changes)
	// [self updateModelView];  // update model view matrix for object

	// clear our drawable
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
	NSRect rectView = [self bounds];
    int width = rectView.size.width;
    int height = rectView.size.height;
    
    extern void DrawCore( int width, int height );
    DrawCore( width, height );
    
	
	// model view and projection matricies already set

    [[self openGLContext] flushBuffer];
#endif
}

// ---------------------------------

static CVReturn DisplayLinkCallback(
    CVDisplayLinkRef displayLink,
    const CVTimeStamp* now,
    const CVTimeStamp* outputTime,
    CVOptionFlags flagsIn,
    CVOptionFlags* flagsOut,
    void* displayLinkContext )
{
    CVReturn result = [(BasicOpenGLView*)displayLinkContext getFrameForTime:outputTime];
    return result;
}

// set initial OpenGL state (current context is set)
// called after context is created
- (void) prepareOpenGL
{
    // Set V-Sync
    GLint swapInt = 1;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
    
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    CVDisplayLinkSetOutputCallback(displayLink,
        &DisplayLinkCallback, self);
    
    CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
    CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
    CVDisplayLinkStart(displayLink);

	glShadeModel(GL_SMOOTH);    
}

// ---------------------------------

- (void)dealloc
{
    // Release the display link
    CVDisplayLinkRelease(displayLink);

    [super dealloc];
}

// ---------------------------------

- (CVReturn)getFrameForTime:(const CVTimeStamp*)outputTime
{
    [[self openGLContext] makeCurrentContext];
    [[self openGLContext] update];
    
 
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
    
       
          
                
	// setup viewport and prespective
	[self resizeGL]; // forces projection matrix update (does test for size changes)
	// [self updateModelView];  // update model view matrix for object

	// clear our drawable
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
	NSRect rectView = [self bounds];
    int width = rectView.size.width;
    int height = rectView.size.height;
    
    extern void DrawCore( int width, int height );
    DrawCore( width, height );
    
	
	// model view and projection matricies already set

    [[self openGLContext] flushBuffer];

    return kCVReturnSuccess;
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
}

- (IBAction)open:(id)sender
{
    NSOpenPanel* openDialog = [NSOpenPanel openPanel];
    [openDialog setCanChooseFiles:YES];
    [openDialog setCanChooseDirectories:NO];
    [openDialog setResolvesAliases:YES];
    [openDialog setAllowsMultipleSelection:NO];
    
    [openDialog setPrompt:@"Open"];
    [openDialog setAllowedFileTypes:[NSArray arrayWithObject:@"gb"]];
    [openDialog setAllowsOtherFileTypes:NO];
    
    [openDialog
        beginSheetModalForWindow:[self window]
        completionHandler:^(NSInteger result)
        {
            if( result == NSOKButton )
            {   
                NSURL* url = [openDialog URL];
                [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:url];
                
                NSString* path = [url path];
                [self openFile:path];
            }
        }];
}

- (void)openFile:(NSString*)path
{
    const char* str = [path UTF8String];
    extern void InitCore(const char* path);
    InitCore(str);    
}

- (IBAction)setMediaFolder:(id)sender
{
    NSOpenPanel* setMediaFolderDialog = [NSOpenPanel openPanel];
    [setMediaFolderDialog setCanChooseFiles:NO];
    [setMediaFolderDialog setCanChooseDirectories:YES];
    [setMediaFolderDialog setResolvesAliases:YES];
    [setMediaFolderDialog setAllowsMultipleSelection:NO];
    
    [setMediaFolderDialog setPrompt:@"Select"];
    
    [setMediaFolderDialog
        beginSheetModalForWindow:[self window]
        completionHandler:^(NSInteger result)
        {
            if( result == NSOKButton )
            {
                NSURL* url = [setMediaFolderDialog URL];
                NSString* path = [url path];
                const char* str = [path UTF8String];
                
                extern void SetMediaFolderCore(const char* path);
                SetMediaFolderCore(path);
            }
        }];
}

@end

