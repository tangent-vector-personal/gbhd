// Copyright (C) 2011 Tim Foley. All Rights Reserved.
//
// BasicOpenGLView.h

#import <Cocoa/Cocoa.h>

#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#import <OpenGL/glu.h>

@interface BasicOpenGLView : NSOpenGLView
{
	NSTimer* timer;
}

+ (NSOpenGLPixelFormat*) basicPixelFormat;

- (void) resizeGL;

- (void) updateObjectRotationForTimeDelta:(CFAbsoluteTime)deltaTime;
- (void)animationTimer:(NSTimer *)timer;

- (void) drawInfo;

- (void)keyDown:(NSEvent *)theEvent;
- (void)keyUp:(NSEvent *)theEvent;

- (void) drawRect:(NSRect)rect;

- (void) prepareOpenGL;

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
