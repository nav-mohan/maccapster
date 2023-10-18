#if !defined(GUI_H)
#define GUI_H

#import <Cocoa/Cocoa.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nullable, strong) NSWindow *window;
@property (nonnull, nonatomic, strong) NSTextField *latLabel;
@end

#endif // GUI_H
