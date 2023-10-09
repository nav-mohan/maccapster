#include "client.hpp"
#include "xmlhandler.hpp"
#include "rs232util.hpp"
#include "geoutil.hpp"
#include "macttsqueue.h"
#include "playbackutil.hpp"
#include "playbackqueue.hpp"
#include "decodeutil.hpp"
#include "downloadqueue.hpp"

#include "helpers.mm"
#include "state.h"

#include "gui.h"

boost::asio::io_context ioContext;
Client clientConn1(ioContext);
Client clientConn2(ioContext);
XmlHandler xmlHandler;
RS232Util relayControl;
GeoUtil geoUtil;

// std::ofstream capturedOutput("output.txt");
int main(int argc, char *argv[])
{
    // Create a stringstream to capture the output

    // Redirect std::cout to the stringstream
    // std::streambuf* originalBuffer = std::cout.rdbuf();
    // std::cout.rdbuf(capturedOutput.rdbuf());

    UserSettings settings;

    std::cout << "CONFIGURATION" << std::endl;
    std::cout << "SERVER1 " << settings.server1_ << ":" << settings.port1_ << std::endl;
    std::cout << "SERVER2 " << settings.server2_ << ":" << settings.port2_ << std::endl;
    std::cout << "COORDINATES " << settings.latitude_ << "," << settings.longitude_ << std::endl;

    clientConn1.set_endpoint(settings.server1_,settings.port1_);
    clientConn1.setTimerFrequency(atof(settings.heartbeatFreq_.c_str()));
    std::cout << "SET ENDPOINT 1" << std::endl;
    clientConn2.set_endpoint(settings.server2_,settings.port2_);
    clientConn2.setTimerFrequency(atof(settings.heartbeatFreq_.c_str()));
    std::cout << "SET ENDPOINT 2" << std::endl;

    geoUtil.set_station(settings.latitude_,settings.longitude_);
    std::cout << "SET LAT LON" << std::endl;

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
    std::cout << "CONFIGURED PLAYBACK QUEUE" << std::endl;


    std::cout << "CONFIGURING DOWNLOAD QUEUE" << std::endl;
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
        printf("DW DONE %s\n",filename.c_str());
        playbackQueue.Push(filename);
    };
    DWQueue downloadQueue(ioContext,std::move(lambda),std::move(certbuffer));
    std::cout << "CONFIGURED DOWNLOAD QUEUE" << std::endl;

    xmlHandler.decodeToFile = [&](std::string&& encodedData, const std::string& filename){
        decode_and_write(std::move(encodedData),filename);
        printf("DEC DONE %s\n",filename.c_str());
        playbackQueue.Push(filename);
    };
    std::cout << "CONFIGURED XMLHANDLER DECODER" << std::endl;

    std::string storageFolder = "/Users/nav/Downloads/";
    std::atomic<std::string*> ptrStorageFolder;
    xmlHandler.enqueueDwnld = [&](const std::string& uri, const std::string& filename){
        Downloadable d;
        d.filename_ = filename;
        ParseURL(uri,d.host_,d.target_,d.port_);
        downloadQueue.Push(d);
    };
    std::cout << "CONFIGURED XMLHANDLER ENQUEUDW" << std::endl;
    
    xmlHandler.checkArea = [&](std::string &&boundary){
        geoUtil.set_alertArea(std::move(boundary));
        return geoUtil.is_inside();
    };
    std::cout << "CONFIGURED XMLHANDLER CHECKAREA" << std::endl;

    TextToSpeechQueue *ttsq = [[TextToSpeechQueue alloc]init];
    PBQueue *pbq = &playbackQueue;
    ttsq->onComplete = ^(NSString* filename){
        printf("TTS JOB DONE\n");
        pbq->Push([filename UTF8String]);
    };
    std::cout << "CONFIGURED TTSQUEUE" << std::endl;

    xmlHandler.enqueueTTS = [&](std::string&& ttsText, std::string& filename, const std::string& language){
        const char *t = ttsText.c_str();
        const char *f = filename.c_str();
        const char *l = language.c_str();

        NSString* nst = [NSString stringWithUTF8String:t];
        NSString *nsf = [NSString stringWithUTF8String:f];
        NSString *nsl = [NSString stringWithUTF8String:l];

        NSLog(@"%@",nsf);
        NSLog(@"%@",nsl);
        // NSLog(@"%@",nst);
        Speechable *sp = [[Speechable alloc] initWithTextFilenameLanguage:nst filename:nsf language:nsl];
        // Speechable *sp = [[Speechable alloc] initWithTextFilenameLanguage:@"This is not a test" filename:@"2.wav" language:@"en-CA"];
        [ttsq enqueueText : sp];
    };
    std::cout << "CONFIGURED XMLHANDLER ENQUEUTTS" << std::endl;

    clientConn1.appendXml = [&](std::string& host, const std::vector<char>&& tempBuffer, size_t size){
        xmlHandler.append(host,std::move(tempBuffer),size);
    };
    std::cout << "CONFIGURED CLIENTCONN1 APPENDXML" << std::endl;
    
    clientConn2.appendXml = [&](std::string& host, const std::vector<char>&& tempBuffer, size_t size){
        xmlHandler.append(host,std::move(tempBuffer),size);
    };
    std::cout << "CONFIGURED CLIENTCONN2 APPENDXML" << std::endl;

    clientConn1.start();
    std::cout << "STARTED CLIENTCONN1" << std::endl;
    // clientConn2.start();
    std::thread t1([&](){
        ioContext.run();
    });    
    // Create the application instance
    NSApplication *application = [NSApplication sharedApplication];
    
    // Create the app delegate
    AppDelegate *delegate = [[AppDelegate alloc] init];
    
    // Set the delegate for the application
    [application setDelegate:delegate];
    [application run];
    [[NSRunLoop mainRunLoop]run];
    t1.join();
    return 0;
}