#if !defined(TTSQUEUE_H)
#define TTSQUEUE_H

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#include <iostream>
#include <filesystem>

@interface Speechable : NSObject 
{
@public NSString *text_;
@public NSString *filename_;
@public NSString *language_;
}
-(instancetype)initWithTextFilenameLanguage : (NSString*)text filename:(NSString*)filename_ language:(NSString*)language_;
@end;


@interface TextToSpeechQueue : NSObject
{
@public void (^onComplete)(NSString*);
}
// Add a text-to-speech task to the queue
- (void)enqueueText:(Speechable *)s;

// Callback for [_synthesizer writeUtterance]
-(void) audioBufferCallback : (AVAudioBuffer*)buffer filename:(NSString*)filename;

// Start processing the queue (begin speech synthesis)
- (void)startProcessingQueue;

// Stop processing the queue (pause speech synthesis)
- (void)stopProcessingQueue;

@end;

#endif // TTSQUEUE_H
