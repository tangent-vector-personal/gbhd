// Copyright (C) 2011 Tim Foley. All Rights Reserved.
//
// BasicOpenGLView.h

#import <Cocoa/Cocoa.h>
#import <CoreVideo/CoreVideo.h>

#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/glu.h>

struct GameBoyState;

@interface BasicOpenGLView : NSOpenGLView
{
    CVDisplayLinkRef displayLink;
    struct GameBoyState* gb;
    NSRecursiveLock* lock;
}

+ (NSOpenGLPixelFormat*) basicPixelFormat;

- (void)resizeGL;

- (void)keyDown:(NSEvent *)theEvent;
- (void)keyUp:(NSEvent *)theEvent;

- (void) prepareOpenGL;
- (void) update;

- (void)dealloc;

- (void)drawRect:(NSRect)theRect;

- (BOOL) acceptsFirstResponder;
- (BOOL) becomeFirstResponder;
- (BOOL) resignFirstResponder;

- (id) initWithFrame: (NSRect) frameRect;
- (void) awakeFromNib;

- (IBAction)open:(id)sender;
- (void)openFile:(NSString*)name;

- (IBAction)setMediaFolder:(id)sender;
- (void)handleSetMediaFolder:(int)result;

@end
