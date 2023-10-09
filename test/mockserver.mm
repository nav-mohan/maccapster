#import <Cocoa/Cocoa.h>
#import "mockserver.h"

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) NSTextField *portLabel;
@property (nonatomic, strong) NSTextField *portInput;
@property (nonatomic, strong) NSButton *startStopButton;
@property (nonatomic, copy) void (^completionHandler)(const char *portText);
@property (nonatomic) BOOL isRunning;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notifciation 
{
    self.isRunning = FALSE;
    self.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0,0,300,200)
        styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable 
        backing : NSBackingStoreBuffered
        defer: NO];
    [self.window setTitle:@"Capster Server"];

    self.portLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20,60,100,10)];
    [self.portLabel setEditable:NO];
    [self.portLabel setSelectable:NO];
    [self.portLabel setBordered:NO];
    [self.portLabel setStringValue:@"Port Number"];
    [self.window.contentView addSubview:self.portLabel];

    self.portInput = [[NSTextField alloc] initWithFrame:NSMakeRect(20,20,50,20)];
    [self.portInput setPlaceholderString:@"8080"];
    [self.portInput setEditable:YES];
    [self.portInput setSelectable:YES];
    [self.window.contentView addSubview:self.portInput];

    self.startStopButton = [[NSButton alloc] initWithFrame:NSMakeRect(125,40,75,40)];
    [self.startStopButton setTitle:@"Start"];
    [self.startStopButton setAction:@selector(startStopButtonClicked:)];
    [self.startStopButton setTarget:self];
    [self.window.contentView addSubview:self.startStopButton];


    [self.window makeKeyAndOrderFront:nil];
}

-(void)startStopButtonClicked:(id)sender
{
    if(self.isRunning)
    {
        [self.startStopButton setTitle:@"Start"];
        stop_server();
        self.isRunning = FALSE;
        return;
    }
    NSOpenPanel *openPanel = [NSOpenPanel openPanel];
    [openPanel setCanChooseDirectories:NO];
    [openPanel setCanChooseFiles:YES];
    [openPanel setAllowsMultipleSelection:YES];
    [openPanel setAllowedFileTypes:@[@"xml"]];

    NSInteger result = [openPanel runModal];
    
    if(result != NSModalResponseOK) {
        printf("Failed to get valid files\n");
        return;
    }
    clear_files();
    NSArray<NSURL*> *selectedURLs = [openPanel URLs];
    for(NSURL *url in selectedURLs)
    {
        const char *filePath = [[url path] UTF8String];
        add_file(filePath);
    }

    const char *portText = [self.portInput.stringValue UTF8String];

    printf("PORT: %s\n",portText);
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        start_server(portText);
    });
    self.isRunning = TRUE;
    [self.startStopButton setTitle:@"Stop"];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
    return YES;
}

@end

int main()
{
    NSApplication *application = [NSApplication sharedApplication];

    AppDelegate *delegate = [[AppDelegate alloc] init];

    [application setDelegate:delegate];

    [application run];

    return 0;
}