#include "gui.h"
// NEED TO HANDLE FILE READ - WRITE PERMISSION WITHNS THE FOLDEr
// it cant seem to write the wav file or mp3 file to a dir 
// it needs write permission


@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {

    // Create the window
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 600, 400)
                                              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"Capster"];
    
    // Create latitude label
    self.latLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20,370,80,20)];
    [self.latLabel setEditable:NO];
    [self.latLabel setSelectable:NO];
    [self.latLabel setBordered:NO];
    [self.latLabel setStringValue:@"Latitude"];
    [self.window.contentView addSubview:self.latLabel];

    [self.window makeKeyAndOrderFront:nil];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
    return YES;
}
@end
