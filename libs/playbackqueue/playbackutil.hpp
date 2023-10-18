#include <AudioToolbox/AudioToolbox.h>
#include <string>
#include <iostream>

#define READ_BYTES 1024
#define NBUFFERS 3
#define DURATION  1
#define LOOP_DURATION 0.25
#define LOOP_FLUSH_DURATION 2

typedef struct AudioPlayer
{
    AudioFileID                     inFile_;
    SInt64                          pktPos_;
    UInt32                          numPktsToRead_;
    AudioStreamPacketDescription    *pktDesc_;
    Boolean                         isDone_;
} AudioPlayer;

struct PBUtil
{
    AudioPlayer audioPlayer_;
    PBUtil() : audioPlayer_({0}){}
    ~PBUtil(){
        delete audioPlayer_.pktDesc_;
    }
    OSStatus OpenFile(const std::string& filepath);
    OSStatus CalculateBytesForDuration(AudioFileID inFile, AudioStreamBasicDescription inDesc, Float64 secs, UInt32 *outBufSize, UInt32 *outNumPkts);
    static void Callback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inAQBufRef);
    OSStatus CopyEncoderCookie(AudioFileID inFile, AudioQueueRef outAQ);
};

OSStatus PBUtil::CalculateBytesForDuration(AudioFileID inFile, AudioStreamBasicDescription inDesc, Float64 secs, UInt32 *outBufSize, UInt32 *outNumPkts)
{
    OSStatus err;
    UInt32 maxPktSize;
    UInt32 propsize = sizeof(maxPktSize);
    err = AudioFileGetProperty(inFile, kAudioFilePropertyPacketSizeUpperBound, &propsize, &maxPktSize );
    if(err != noErr) return err;
    
    const int maxBufSize = 0x400;
    const int minBufSize = 0x100; 

    if(inDesc.mFramesPerPacket)
    {
        Float64 numPktsForTime = inDesc.mSampleRate/inDesc.mFramesPerPacket*secs;
        *outBufSize = numPktsForTime * maxPktSize;
    }
    else
        *outBufSize = maxBufSize > maxPktSize ? maxBufSize : maxPktSize;
    
    if(*outBufSize > maxBufSize && *outBufSize > maxPktSize)
        *outBufSize = maxBufSize;
    else if(*outBufSize < minBufSize)
        *outBufSize = minBufSize;

    *outNumPkts = (*outBufSize)/maxPktSize;
    return noErr;
}

OSStatus PBUtil::CopyEncoderCookie(AudioFileID inFile, AudioQueueRef outAQ)
{
    UInt32 propsize; 
    OSStatus err;
    err = AudioFileGetPropertyInfo(inFile, kAudioFilePropertyMagicCookieData, &propsize, NULL);
    if(err == noErr && propsize > 0)
    {
        Byte *magicCookie = (Byte*)malloc(sizeof(UInt8)*propsize);
        err = AudioFileGetProperty(inFile, kAudioFilePropertyMagicCookieData, &propsize, magicCookie);
        if(err != noErr) {free(magicCookie);return err;}
        err = AudioQueueSetProperty(outAQ, kAudioQueueProperty_MagicCookie, magicCookie, propsize);
        if(err != noErr) {free(magicCookie);return err;}
        free(magicCookie);
        printf("COPIED COOKIE\n");
    }
    return noErr;
}

void PBUtil::Callback(void *inUserData, AudioQueueRef outAQ, AudioQueueBufferRef outAQBuf)
{
    AudioPlayer *aplayer = (AudioPlayer*)inUserData;
    OSStatus err;
    UInt32 nBytes = READ_BYTES;
    UInt32 nPkts = aplayer->numPktsToRead_;
    err = AudioFileReadPacketData(
        aplayer->inFile_, 
        false,
        &nBytes,
        aplayer->pktDesc_,
        aplayer->pktPos_,
        &nPkts,
        outAQBuf->mAudioData
    );
    if(err != noErr) {printf("AudioFileReadPacketData Faild %d:%s\n",err,strerror(err)); return;}
    if(nPkts > 0)
    {
        outAQBuf->mAudioDataByteSize = nBytes;
        err = AudioQueueEnqueueBuffer(
            outAQ, outAQBuf, 
            (aplayer->pktDesc_ ? nPkts : 0),
            aplayer->pktDesc_
        );
        if(err != noErr) {printf("AudioQueueEnqueueBuffer Faild %d:%s\n",err,strerror(err)); return;}
        aplayer->pktPos_ += nPkts;
    }
    else
    {
        err = AudioQueueStop(outAQ, false);
        if(err != noErr) {printf("AudioQueueStop Faild %d:%s\n",err,strerror(err)); return;}
        aplayer->isDone_ = true;
    }
}

OSStatus PBUtil::OpenFile(const std::string& filepath)
{
    OSStatus err;
    CFURLRef fileURL = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8*)filepath.c_str(),filepath.size(), false);
    err = AudioFileOpenURL(fileURL, kAudioFileReadPermission, 0, &audioPlayer_.inFile_);
    CFRelease(fileURL);
    if(err != noErr) 
    {
        printf("Failed to open file %d:%s\n",err,strerror(err));
        return err;
    }
    
    AudioStreamBasicDescription dataFormat;
    UInt32 propsize = sizeof(dataFormat);
    err = AudioFileGetProperty(audioPlayer_.inFile_, kAudioFilePropertyDataFormat, &propsize, &dataFormat);
    if(err != noErr) {printf("Failed to get file ASBD %d:%s\n",err,strerror(err));return err;}

    AudioQueueRef outAQ;
    err = AudioQueueNewOutput(&dataFormat, Callback, &audioPlayer_, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode, 0, &outAQ);
    // err = AudioQueueNewOutput(&dataFormat, Callback, &audioPlayer_, NULL, kCFRunLoopDefaultMode, 0, &outAQ);
    if(err != noErr) {printf("Failed to create outAQ %d:%s\n",err,strerror(err));return err;}

    UInt32 bufByteSize;
    err = CalculateBytesForDuration(audioPlayer_.inFile_, dataFormat, DURATION, &bufByteSize, &audioPlayer_.numPktsToRead_);
    if(err != noErr) {printf("Failed to CalculateBytesForDuration %d:%s\n",err,strerror(err));return err;}
    printf("Playback duration %d: %d BYTES",DURATION, bufByteSize);

    bool isFormatVBR = (dataFormat.mBytesPerPacket == 0 || dataFormat.mFramesPerPacket == 0);
    if(isFormatVBR)
    {
        audioPlayer_.pktDesc_ = (AudioStreamPacketDescription*)malloc(sizeof(AudioStreamPacketDescription));
        printf("MALLOCED %lu\n",sizeof(AudioStreamPacketDescription));
    }
    else 
        audioPlayer_.pktDesc_ = NULL;
    
    err = CopyEncoderCookie(audioPlayer_.inFile_, outAQ);
    if(err != noErr) {printf("Failed to copy cookie %d:%s\n",err,strerror(err));return err;}

    AudioQueueBufferRef outBufs[NBUFFERS];
    audioPlayer_.isDone_ = false;
    audioPlayer_.pktPos_ = 0;
    for(int i = 0; i < NBUFFERS; i++)
    {
        err = AudioQueueAllocateBuffer(outAQ, bufByteSize, &outBufs[i]);
        if(err != noErr) {printf("Failed to allocate buffer %d:%s\n",err,strerror(err));return err;}
        Callback(&audioPlayer_, outAQ, outBufs[i]);
    }
    err = AudioQueueStart(outAQ, NULL);
    if(err != noErr) {printf("Failed to start outAQ %d:%s\n",err,strerror(err));/**return err;*/}
    do
    {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, LOOP_DURATION, 0);
    } while (!audioPlayer_.isDone_);
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, LOOP_FLUSH_DURATION, 0);

    audioPlayer_.isDone_ = true;
    err = AudioQueueStop(outAQ,TRUE);
    if(err != noErr) {printf("Failed to stop outAQ %d:%s\n",err,strerror(err));return err;}
    for(int i = 0; i < NBUFFERS; i++){
        AudioQueueFreeBuffer(outAQ, outBufs[i]);
    }

    AudioQueueDispose(outAQ, TRUE);
    AudioFileClose(audioPlayer_.inFile_);
    free(audioPlayer_.pktDesc_);
    audioPlayer_={0};
    return 0;
}