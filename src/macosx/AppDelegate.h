// Copyright (C) 2011 Theresa Foley. All Rights Reserved.
//
// AppDelegate.h


#import <Cocoa/Cocoa.h>

#import "BasicOpenGLView.h"

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    NSWindow *window;
}

@property (assign) IBOutlet NSWindow *window;
@property (assign) IBOutlet BasicOpenGLView *basicOpenGLView;

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication;
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;

@end
