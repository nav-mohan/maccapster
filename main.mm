#include "client.hpp"
#include "xmlhandler.hpp"
#include "rs232util.hpp"
#include "geoutil.hpp"
#include "macttsqueue.h"
#include "playbackutil.hpp"
#include "playbackqueue.hpp"
#include "decodeutil.hpp"
#include "downloadqueue.hpp"
#include "encoder.hpp"
#include "encoderQueue.hpp"

#include "helpers.mm"
#include "state.h"

#include "mslogger.hpp"

#include "gui.h"

boost::asio::io_context ioContext;
Client clientConn1(ioContext);
Client clientConn2(ioContext);
XmlHandler xmlHandler;
RS232Util relayControl;
GeoUtil geoUtil;
UserSettings settings;
MPEGEncoder<ENCODERBITRATE::MEDIUM, ENCODERSAMPLERATE::MEDIUM, ENCODERCHANNEL::MONO> mpegEnc;


// std::ofstream capturedOutput("output.txt");
int main(int argc, char *argv[])
{
    EncQueue encoderQueue;
    encoderQueue.Encode = [&](const std::string filename){
        FILE *fp = fopen("sample.wav","rb");
        FILE *fpout = fopen("sample.mp3","wb");
        int bytesRead = fread(mpegEnc.m_pcmBuffer,1,4098,fp);
        while(bytesRead > 0)
        {
            printf("READ %d BTES\n",bytesRead);
            mpegEnc.DoEncodeInterleaved(mpegEnc.m_pcmBuffer,bytesRead);
            int bytesWritten = fwrite(mpegEnc.m_encBuffer,1,mpegEnc.m_bytesEncoded,fpout);
            printf("WROTE %d BTES %s\n",bytesWritten,mpegEnc.m_encBuffer);

            bytesRead = fread(mpegEnc.m_pcmBuffer,1,4098,fp);
        }
        fclose(fp);
        fclose(fpout);
    };  
    encoderQueue.Encode("sample.wav");

    printf("DONE ENCODING\n");

    basic_log("CONFIGURATION",DEBUG);
    basic_log("SERVER1 " + settings.server1_ + ":" + settings.port1_,DEBUG);
    basic_log("SERVER2 " + settings.server2_ + ":" + settings.port2_,DEBUG);
    basic_log("COORDINATES " + settings.latitude_ + "," + settings.longitude_,DEBUG);


    clientConn1.set_endpoint(settings.server1_,settings.port1_);
    clientConn1.setTimerFrequency(atof(settings.heartbeatFreq_.c_str()));
    basic_log("SET ENDPOINT 1",DEBUG);

    clientConn2.set_endpoint(settings.server2_,settings.port2_);
    clientConn2.setTimerFrequency(atof(settings.heartbeatFreq_.c_str()));
    basic_log("SET ENDPOINT 2",DEBUG);

    geoUtil.set_station(settings.latitude_,settings.longitude_);
    basic_log("SET LAT/LON ",DEBUG);

    PBQueue playbackQueue;
    PBUtil playbackUtil;
    playbackQueue.PlayFirst = [&](){
        relayControl.set_pin();
        playbackUtil.OpenFile(getResourcePath("siren.wav"));
    };
    playbackQueue.PlayLast = [&](){
        relayControl.clear_pin();
    };
    playbackQueue.Play = [&](const std::string f){
        playbackUtil.OpenFile(f);
    };
    basic_log("CONFIGURED PLAYBACKQUEUE",DEBUG);

    std::vector<std::string>certfiles = {getResourcePath("cacert.pem")};
    std::string certbuffer;
    std::string buffer;
    buffer.resize(1048576);
    for(const std::string& fname : certfiles)
    {
        FILE *certfile = fopen(fname.c_str(), "rb");
        int bytes_read = fread(buffer.data(), 1, 1048576, certfile);
        certbuffer += buffer;
        fclose(certfile);
    }
    auto lambda = [&](std::string filename){
        basic_log("DOWNLOAD COMPLETED " + filename);
        playbackQueue.Push(filename);
    };
    DWQueue downloadQueue(ioContext,std::move(lambda),std::move(certbuffer));
    basic_log("CONFIGURED DOWNLOADQUEUE",DEBUG);

    xmlHandler.decodeToFile = [&](std::string&& encodedData, const std::string& filename){
        decode_and_write(std::move(encodedData),filename);
        basic_log("DECODING COMPLETED " + filename);
        playbackQueue.Push(filename);
    };
    basic_log("CONFIGURED XMLHANDLER DECODER",DEBUG);
    
    xmlHandler.enqueueDwnld = [&](const std::string& uri, const std::string& filename){
        Downloadable d;
        d.filename_ = filename;
        ParseURL(uri,d.host_,d.target_,d.port_);
        downloadQueue.Push(d);
    };
    basic_log("CONFIGURED XMLHANDLER ENQUEUDW",DEBUG);

    xmlHandler.checkArea = [&](std::string &&boundary){
        geoUtil.set_alertArea(std::move(boundary));
        return geoUtil.is_inside();
    };
    basic_log("CONFIGURED XMLHANDLER CHECKAREA",DEBUG);

    TextToSpeechQueue *ttsq = [[TextToSpeechQueue alloc]init];
    PBQueue *pbq = &playbackQueue;
    ttsq->onComplete = ^(NSString* filename){
        basic_log("TTS JOB DONE " + std::string(filename.UTF8String));
        pbq->Push([filename UTF8String]);
    };
    basic_log("CONFIGURED XMLHANDLER TTSQUEUE",DEBUG);

    xmlHandler.enqueueTTS = [&](std::string&& ttsText, std::string& filename, const std::string& language){
        std::string output = std::move(prepareTextForTTS(std::move(ttsText)));
        const char *t = output.c_str();
        const char *f = filename.c_str();
        const char *l = language.c_str();

        NSString* nst = [NSString stringWithUTF8String:t];
        NSString *nsf = [NSString stringWithUTF8String:f];
        NSString *nsl = [NSString stringWithUTF8String:l];

        NSLog(@"%@",nsf);
        NSLog(@"%@",nsl);
        // NSLog(@"%@",nst);
        Speechable *sp = [[Speechable alloc] initWithTextFilenameLanguage:nst filename:nsf language:nsl];
        [ttsq enqueueText : sp];
        [sp release];
    };
    basic_log("CONFIGURED XMLHANDLER ENQUEUETTS",DEBUG);

    clientConn1.appendXml = [&](std::string& host, const std::vector<char>&& tempBuffer, size_t size){
        xmlHandler.append(host,std::move(tempBuffer),size);
    };
    basic_log("CONFIGURED CLIENTCONN1 APPENDXML",DEBUG);
    
    clientConn2.appendXml = [&](std::string& host, const std::vector<char>&& tempBuffer, size_t size){
        xmlHandler.append(host,std::move(tempBuffer),size);
    };
    basic_log("CONFIGURED CLIENTCONN2 APPENDXML",DEBUG);

    clientConn1.start();
    clientConn2.start();
    basic_log("STARTED CLIENT CONNECTIONS");
    // exit(1);
    std::thread t1([&](){
        ioContext.run();
    });    
    // Create the application instance
    // NSApplication *application = [NSApplication sharedApplication];
    
    // Create the app delegate
    // AppDelegate *delegate = [[AppDelegate alloc] init];
    
    // Set the delegate for the application
    // [application setDelegate:delegate];
    // [application run];
    [[NSRunLoop mainRunLoop]run];
    t1.join();
    return 0;
}