#import "macttsqueue.h"
#import "wavheader.h"

@implementation Speechable {}
-(instancetype)initWithTextFilenameLanguage : (NSString*)text filename:(NSString*)filename language:(NSString*)language
{
    [super init];
    text_ = [text copy];
    filename_ = [filename copy];
    language_ = [language copy];
    return self;
}
@end;

// the TTS queue works on creating only 1 file at a time. 
// the callback is blocking. entering the callback is a serially queued. but startProcessingQueue in of itself is not blocking. 
@implementation TextToSpeechQueue {
    AVSpeechSynthesizer *_synthesizer;
    NSMutableArray<Speechable *> *_queue;
    dispatch_queue_t _dispatchQueue;
    NSCondition *_condition;
    uint32_t _majorVersion; // the audiobuffercallback has a breaking change between MacOS12 and MacOS13
    
    // file properties - dont touch these anywhere outside of the audidbuffercallback. otherwise you'll confuse audioBufferCallback.
    FILE *_outfile; //the current file audioBufferCallback is working on creating
    NSString *_filename; // the filename of the current file audioBufferCallback is working on creating
    size_t _pcmBytesWritten; // total pcm bytes of the current file audioBufferCallback is working on creating
}

- (instancetype)init {

    NSString* versionStr = [[NSProcessInfo processInfo] operatingSystemVersionString];
    NSOperatingSystemVersion versionOS = [[NSProcessInfo processInfo] operatingSystemVersion];
        
    basic_log("MacTTSQueue::init " + std::to_string(versionOS.majorVersion) + "." + std::to_string(versionOS.minorVersion) + "." + std::to_string(versionOS.patchVersion),DEBUG);
    _majorVersion = versionOS.majorVersion;

    self = [super init];
    _synthesizer = [[AVSpeechSynthesizer alloc] init];
    _queue = [NSMutableArray array];
    _condition = [[NSCondition alloc]init];
    _dispatchQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(_dispatchQueue,^{[self startProcessingQueue];});
    return self;
}

- (void)enqueueText:(Speechable *)sp {
    
    // basic_log("MacTTSQueue::enqueueText " + std::string(sp->filename_.UTF8String));
    [_condition lock];
    [_queue addObject:sp];
    [_condition unlock];
    [_condition signal];
}

- (void)startProcessingQueue {
    while(1)
    {
        [_condition lock];
        while(_queue.count == 0) [_condition wait];
        NSString *text       = [_queue.firstObject->text_ copy];
        NSString *filename   = [_queue.firstObject->filename_ copy];
        NSString *language   = [_queue.firstObject->language_ copy];
        
        basic_log("MacTTSQueue::startProcessingQueue " + std::string(filename.UTF8String));
        [_queue removeObjectAtIndex:0];
        [_condition unlock];
        
        AVSpeechUtterance *utterance    = [AVSpeechUtterance speechUtteranceWithString:text];
        AVSpeechSynthesisVoice *voice   = [AVSpeechSynthesisVoice voiceWithLanguage:language];
        utterance.rate = 0.35;
        [utterance setVoice:voice];
        
        [_synthesizer writeUtterance:utterance toBufferCallback:^(AVAudioBuffer * buffer) 
        {
            [self audioBufferCallback : buffer filename:filename ];
        }];
        
        [utterance release];
        [voice release];
        [text release];
        [filename release];
        [language release];
    }
}

-(void) audioBufferCallback : (AVAudioBuffer*)buffer filename:(NSString*)filename
{
    char *data = (char*)buffer.audioBufferList->mBuffers->mData;
    uint32_t bufSize = buffer.audioBufferList->mBuffers->mDataByteSize;
    
    // basic_log("MacTTSQueue::audioBufferCallback " + std::string(filename.UTF8String) + " " + std::to_string(bufSize));
    if(!filename || !_outfile) // create new file
    {
        _filename = filename;
        _outfile = fopen([filename UTF8String],"w");
        _pcmBytesWritten = 0;
        char dummy[sizeof(WavHeader)];
        fwrite(&dummy, 1, sizeof(WavHeader), _outfile); // write dummy WAV header and reserver space
    }

    // write PCM Samples
    _pcmBytesWritten += fwrite(data, 1, bufSize, _outfile);

    // [self stopProcessingQueue]; // calling this here after en-CA TTS will drop the fr-CA TTS. 
    if(_majorVersion > 12 && bufSize != 0) return; // else write the WavHeader, reset the FILE params, and call OnComplete
    
    AVAudioFormat *format = buffer.format;
    WavHeader wavheader(_majorVersion);
    wavheader.setPcmDataSize(_pcmBytesWritten);
    if(format)
    {
        const AudioStreamBasicDescription *streamDescription = format.streamDescription;
        wavheader.setSampleRate(format.sampleRate).setNumChannels(format.channelCount).setBitsPerSample(streamDescription->mBitsPerChannel);
    }
    else 
    {
        basic_log("Audio buffer format is nil. Lets hope the default values based on _majorVersion will work " + std::string(filename.UTF8String) + " " + std::to_string(bufSize));
    }

    fseek(_outfile,0,0);
    int wavHeaderBytesWritten = fwrite(&wavheader, 1, sizeof(WavHeader), _outfile);
    fclose(_outfile);
    _pcmBytesWritten = 0;
    _outfile = nullptr;
    _filename = nullptr;
    wavheader = {0};
    if(wavHeaderBytesWritten != sizeof(WavHeader)) return;
    return onComplete(filename);
}


- (void)stopProcessingQueue {
    [_synthesizer stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
}

// Method to get the queue size
// This is a less-reliable substitute for GetHistory but I'm not sure how 
// to implement that in a way that is compatible with DispatchQueue 
- (NSUInteger)getQueueSize {
    [_condition lock];
    NSUInteger count = _queue.count;
    [_condition unlock];
    return count;
}

@end
