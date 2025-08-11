// Copyright (C) 2011 Theresa Foley. All Rights Reserved.
//
// BasicOpenGLView.m

#import "BasicOpenGLView.h"

#include <mach/mach.h>
#include <mach/mach_time.h>

#include "gb.h"

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
    
    kKeyCode_Space = 49,
    kKeyCode_0 = 29,
    kKeyCode_9 = 25,
};

-(void)keyDown:(NSEvent *)theEvent
{
    switch( [theEvent keyCode] )
    {
    case kKeyCode_S:
        GameBoyState_KeyDown(gb, kKey_Start);
        break;
    case kKeyCode_T:
        GameBoyState_KeyDown(gb, kKey_Select);
        break;
    case kKeyCode_A:
        GameBoyState_KeyDown(gb, kKey_A);
        break;
    case kKeyCode_B:
        GameBoyState_KeyDown(gb, kKey_B);
        break;
    case kKeyCode_I:
        GameBoyState_KeyDown(gb, kKey_Up);
        break;
    case kKeyCode_J:
        GameBoyState_KeyDown(gb, kKey_Left);
        break;
    case kKeyCode_K:
        GameBoyState_KeyDown(gb, kKey_Down);
        break;
    case kKeyCode_L:
        GameBoyState_KeyDown(gb, kKey_Right);
        break;
        
    case kKeyCode_Space:
        GameBoyState_TogglePause(gb);
        break;
    case kKeyCode_0:
        GameBoyState_ToggleRenderer(gb);
        break;
    case kKeyCode_9:
        GameBoyState_DumpTiles(gb);
        break;
        
    default:
        break;
    }
}

-(void)keyUp:(NSEvent *)theEvent
{
    switch( [theEvent keyCode] )
    {
    case kKeyCode_S:
        GameBoyState_KeyUp(gb, kKey_Start);
        break;
    case kKeyCode_T:
        GameBoyState_KeyUp(gb, kKey_Select);
        break;
    case kKeyCode_A:
        GameBoyState_KeyUp(gb, kKey_A);
        break;
    case kKeyCode_B:
        GameBoyState_KeyUp(gb, kKey_B);
        break;
    case kKeyCode_I:
        GameBoyState_KeyUp(gb, kKey_Up);
        break;
    case kKeyCode_J:
        GameBoyState_KeyUp(gb, kKey_Left);
        break;
    case kKeyCode_K:
        GameBoyState_KeyUp(gb, kKey_Down);
        break;
    case kKeyCode_L:
        GameBoyState_KeyUp(gb, kKey_Right);
        break;
    default:
        break;
    }
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
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [(BasicOpenGLView*)displayLinkContext drawRect:NSZeroRect];
    [pool release];
    return kCVReturnSuccess;
}

// set initial OpenGL state (current context is set)
// called after context is created
- (void) prepareOpenGL
{
    lock = [[NSRecursiveLock alloc] init];

    // Set V-Sync
    GLint swapInt = 1;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
    
    [[self openGLContext] makeCurrentContext];

	glShadeModel(GL_SMOOTH);
    
    gb = GameBoyState_Create();

    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    CVDisplayLinkSetOutputCallback(displayLink,
        &DisplayLinkCallback, self);
    
    CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
    CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
    CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
    CVDisplayLinkStart(displayLink);
}

// ---------------------------------

- (void) update
{
    [lock lock];
    [super update];
    [lock unlock];
}

// ---------------------------------

- (void)dealloc
{
    // Release the display link
    CVDisplayLinkRelease(displayLink);

    // Next free up the emulator state
    GameBoyState_Release(gb);

    [super dealloc];
}

// ---------------------------------

- (void)drawRect:(NSRect)theRect
{
    [lock lock];

    [[self openGLContext] makeCurrentContext];
    [[self openGLContext] update];
    
    uint64_t machTime = mach_absolute_time();
    AbsoluteTime absTime = *((AbsoluteTime*) &machTime );
    Nanoseconds nanoTime = AbsoluteToNanoseconds(absTime);
    uint64_t nanos = *((uint64_t*) &nanoTime);

    uint64_t nanosecondsPerSecond = 1000000000; // 10^9
    GameBoyState_Update(gb, nanos, nanosecondsPerSecond );
                
	// setup viewport and prespective
	[self resizeGL]; // forces projection matrix update (does test for size changes)
	// [self updateModelView];  // update model view matrix for object

	// clear our drawable
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
	NSRect rectView = [self bounds];
    int width = rectView.size.width;
    int height = rectView.size.height;
    
    GameBoyState_RenderGL( gb, width, height );    
	
    [[self openGLContext] flushBuffer];
    
    [NSOpenGLContext clearCurrentContext];

    [lock unlock];
}

// ---------------------------------

-(id) initWithFrame: (NSRect) frameRect
{
	NSOpenGLPixelFormat * pf = [BasicOpenGLView basicPixelFormat];

	self = [super initWithFrame: frameRect pixelFormat: pf];
    
    gb = NULL;
    
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
    
    GameBoyState_SetGamePath(gb, str);
    
    // If not already started, start now
    GameBoyState_Start(gb);
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
                
                [lock lock];
                [[self openGLContext] makeCurrentContext];
                GameBoyState_SetMediaPath(gb, str);
                [lock unlock];
            }
        }];
}

@end

