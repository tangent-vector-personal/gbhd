// Copyright (C) 2011 Theresa Foley. All Rights Reserved.
//
// AppDelegate.m

#import "AppDelegate.h"

@implementation AppDelegate

@synthesize window;
@synthesize basicOpenGLView;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Insert code here to initialize your application
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
    return YES;
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
    if( basicOpenGLView != NULL )
        [basicOpenGLView openFile:filename];
    return YES;
}

@end
