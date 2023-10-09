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

@implementation TextToSpeechQueue {
    AVSpeechSynthesizer *_synthesizer;
    NSMutableArray<Speechable *> *_queue;
    dispatch_queue_t _dispatchQueue;
    NSCondition *_condition;
    FILE *_outfile;
    NSString* _filename;
    char *_massivebuffer;
    size_t _bytesCopied;
}

- (instancetype)init {
    self = [super init];
    _synthesizer = [[AVSpeechSynthesizer alloc] init];
    _queue = [NSMutableArray array];
    _condition = [[NSCondition alloc]init];
    _dispatchQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    _outfile = nullptr;
    _filename = nullptr;
    dispatch_async(_dispatchQueue,^{[self startProcessingQueue];});
    _massivebuffer = (char*)malloc(16777216);
    _bytesCopied = 44;
    return self;
}

-(void) audioBufferCallback : (AVAudioBuffer*)buffer filename:(NSString*)filename
{
    void *data = buffer.audioBufferList->mBuffers->mData;
    uint32_t bufSize = buffer.audioBufferList->mBuffers->mDataByteSize;

    memcpy(_massivebuffer+_bytesCopied,data,bufSize);
    _bytesCopied += bufSize;

    if(bufSize != 0) return;
    printf("NO MORE DATA\n");

    // do this on complete. use _bytesCopied
    const char *fname = [filename UTF8String];
    AVAudioFormat *format = buffer.format;
    WavHeader wavheader;
    wavheader.dataSize = _bytesCopied - 44;
    wavheader.chunkSize = wavheader.dataSize + 36;
    _outfile = fopen([filename UTF8String],"ab");

    if (format) 
    {
        const AudioStreamBasicDescription *streamDescription = format.streamDescription;
        wavheader.sampleRate = format.sampleRate;
        wavheader.numChannels = format.channelCount;
        wavheader.bitsPerSample = streamDescription->mBitsPerChannel;
        wavheader.blockAlign = wavheader.bitsPerSample/8;
        wavheader.bytesPerSec      = wavheader.blockAlign*wavheader.sampleRate;
        if(wavheader.bitsPerSample == 32)
            wavheader.audioFormat = 3;
        else                        // bitsPerSample == 16
            wavheader.audioFormat = 1;

        printf("fmtSize %d\n", wavheader.fmtSize);
        printf("audioFormat %d\n", wavheader.audioFormat);
        printf("numChannels %d\n", wavheader.numChannels);
        printf("sampleRate %d\n", wavheader.sampleRate);
        printf("bytesPerSec %d\n", wavheader.bytesPerSec);
        printf("blockAlign %d\n", wavheader.blockAlign);
        printf("bitsPerSample %d\n", wavheader.bitsPerSample);

    } else {
        NSLog(@"Audio buffer format is nil.");
    }

    memcpy(_massivebuffer, &wavheader, 44);

    int bytesWritten = fwrite(_massivebuffer,1,_bytesCopied+44,_outfile);
    printf("WROTE %d BYTES %s\n",_bytesCopied + 44,fname);
    fclose(_outfile);

    _bytesCopied = 44;

    return onComplete(filename);
}

- (void)enqueueText:(Speechable *)sp {
    printf("PUSHING\n");
    [_condition lock];
    [_queue addObject:sp];
    [_condition signal];
    [_condition unlock];
    printf("SIGNALLED\n");
}

- (void)startProcessingQueue {
    printf("START LOOP\n");
    while(1)
    {
        [_condition lock];
        while(_queue.count == 0) {printf("WAIT\n");[_condition wait];}
        NSString *text       = [_queue.firstObject->text_ copy];
        NSString *filename   = [_queue.firstObject->filename_ copy];
        NSString *language   = [_queue.firstObject->language_ copy];
        printf("POPPING %s\n",filename.UTF8String);
        [_queue removeObjectAtIndex:0];
        printf("REMOVED %s\n",filename.UTF8String);
        [_condition unlock];
        
        AVSpeechUtterance *utterance    = [AVSpeechUtterance speechUtteranceWithString:text];
        AVSpeechSynthesisVoice *voice   = [AVSpeechSynthesisVoice voiceWithLanguage:language];
        utterance.rate = 0.35;
        [utterance setVoice:voice];
        
        // [_synthesizer writeUtterance:utterance toBufferCallback:^(AVAudioBuffer * buffer) {audioBufferCallback(buffer,filename.UTF8String);}];
        [_synthesizer writeUtterance:utterance toBufferCallback:^(AVAudioBuffer * buffer) {[self audioBufferCallback : buffer filename:filename];}];
        std::cout << "HELLO WORLD" << std::endl; // this will show up before the TTS is done proving that we've succesfully deployed our TTS to another thread. whether or not it works inside that other thread is a whole other problem. 
    }
}

- (void)stopProcessingQueue {
    [_synthesizer stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
}

@end
