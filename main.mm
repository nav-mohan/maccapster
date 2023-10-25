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

#include "mslogger.hpp"

#include "gui.h"

boost::asio::io_context ioContext;
Client clientConn1(ioContext);
Client clientConn2(ioContext);
XmlHandler xmlHandler;
RS232Util relayControl;
GeoUtil geoUtil;
UserSettings settings;

// std::ofstream capturedOutput("output.txt");
int main(int argc, char *argv[])
{
    // std::cout << "Press <RETURN> to start" << std::endl;
    // getchar();
    MsLogger<DEBUG>::get_instance().log_to_stdout("CONFIGURATION");
    MsLogger<INFO>::get_instance().log_to_file("CONFIGURATION",DEBUG);

    MsLogger<DEBUG>::get_instance().log_to_stdout("SERVER1 " + settings.server1_ + ":" + settings.port1_);
    MsLogger<INFO>::get_instance().log_to_file("SERVER1 " + settings.server1_ + ":" + settings.port1_,DEBUG);

    MsLogger<DEBUG>::get_instance().log_to_stdout("SERVER2 " + settings.server2_ + ":" + settings.port2_);
    MsLogger<INFO>::get_instance().log_to_file("SERVER2 " + settings.server2_ + ":" + settings.port2_,DEBUG);

    MsLogger<DEBUG>::get_instance().log_to_stdout("COORDINATES " + settings.latitude_ + "," + settings.longitude_);
    MsLogger<INFO>::get_instance().log_to_file("COORDINATES " + settings.latitude_ + "," + settings.longitude_,DEBUG);


    clientConn1.set_endpoint(settings.server1_,settings.port1_);
    clientConn1.setTimerFrequency(atof(settings.heartbeatFreq_.c_str()));
    MsLogger<DEBUG>::get_instance().log_to_stdout("SET ENDPOINT 1");
    MsLogger<INFO>::get_instance().log_to_file("SET ENDPOINT 1",DEBUG);

    clientConn2.set_endpoint(settings.server2_,settings.port2_);
    clientConn2.setTimerFrequency(atof(settings.heartbeatFreq_.c_str()));
    MsLogger<DEBUG>::get_instance().log_to_stdout("SET ENDPOINT 2");
    MsLogger<INFO>::get_instance().log_to_file("SET ENDPOINT 2",DEBUG);

    geoUtil.set_station(settings.latitude_,settings.longitude_);
    MsLogger<DEBUG>::get_instance().log_to_stdout("SET LAT/LON");
    MsLogger<INFO>::get_instance().log_to_file("SET LAT/LON ",DEBUG);

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
    MsLogger<DEBUG>::get_instance().log_to_stdout("CONFIGURED PLAYBACKQUEUE");
    MsLogger<INFO>::get_instance().log_to_file("CONFIGURED PLAYBACKQUEUE",DEBUG);

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
        MsLogger<INFO>::get_instance().log_to_stdout("DOWNLOAD COMPLETED " + filename);
        MsLogger<INFO>::get_instance().log_to_file("DOWNLOAD COMPLETED " + filename);
        playbackQueue.Push(filename);
    };
    DWQueue downloadQueue(ioContext,std::move(lambda),std::move(certbuffer));
    MsLogger<DEBUG>::get_instance().log_to_stdout("CONFIGURED DOWNLOADQUEUE");
    MsLogger<INFO>::get_instance().log_to_file("CONFIGURED DOWNLOADQUEUE",DEBUG);

    xmlHandler.decodeToFile = [&](std::string&& encodedData, const std::string& filename){
        decode_and_write(std::move(encodedData),filename);
        MsLogger<INFO>::get_instance().log_to_stdout("DECODING COMPLETED " + filename);
        MsLogger<INFO>::get_instance().log_to_file("DECODING COMPLETED " + filename);
        playbackQueue.Push(filename);
    };
    MsLogger<DEBUG>::get_instance().log_to_stdout("CONFIGURED XMLHANDLER DECODER");
    MsLogger<INFO>::get_instance().log_to_file("CONFIGURED XMLHANDLER DECODER",DEBUG);
    
    xmlHandler.enqueueDwnld = [&](const std::string& uri, const std::string& filename){
        Downloadable d;
        d.filename_ = filename;
        ParseURL(uri,d.host_,d.target_,d.port_);
        downloadQueue.Push(d);
    };
    MsLogger<DEBUG>::get_instance().log_to_stdout("CONFIGURED XMLHANDLER ENQUEUDW");
    MsLogger<INFO>::get_instance().log_to_file("CONFIGURED XMLHANDLER ENQUEUDW",DEBUG);

    xmlHandler.checkArea = [&](std::string &&boundary){
        geoUtil.set_alertArea(std::move(boundary));
        return geoUtil.is_inside();
    };
    MsLogger<DEBUG>::get_instance().log_to_stdout("CONFIGURED XMLHANDLER CHECKAREA");
    MsLogger<INFO>::get_instance().log_to_file("CONFIGURED XMLHANDLER CHECKAREA",DEBUG);

    TextToSpeechQueue *ttsq = [[TextToSpeechQueue alloc]init];
    PBQueue *pbq = &playbackQueue;
    ttsq->onComplete = ^(NSString* filename){
        MsLogger<INFO>::get_instance().log_to_stdout("TTS JOB DONE " + std::string(filename.UTF8String));
        MsLogger<INFO>::get_instance().log_to_file("TTS JOB DONE " + std::string(filename.UTF8String));
        pbq->Push([filename UTF8String]);
    };
    MsLogger<DEBUG>::get_instance().log_to_stdout("CONFIGURED XMLHANDLER TTSQUEUE");
    MsLogger<INFO>::get_instance().log_to_file("CONFIGURED XMLHANDLER TTSQUEUE",DEBUG);

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
    MsLogger<DEBUG>::get_instance().log_to_stdout("CONFIGURED XMLHANDLER ENQUEUETTS");
    MsLogger<INFO>::get_instance().log_to_file("CONFIGURED XMLHANDLER ENQUEUETTS",DEBUG);

    clientConn1.appendXml = [&](std::string& host, const std::vector<char>&& tempBuffer, size_t size){
        xmlHandler.append(host,std::move(tempBuffer),size);
    };
    MsLogger<DEBUG>::get_instance().log_to_stdout("CONFIGURED CLIENTCONN1 APPENDXML");
    MsLogger<INFO>::get_instance().log_to_file("CONFIGURED CLIENTCONN1 APPENDXML",DEBUG);
    
    clientConn2.appendXml = [&](std::string& host, const std::vector<char>&& tempBuffer, size_t size){
        xmlHandler.append(host,std::move(tempBuffer),size);
    };
    MsLogger<DEBUG>::get_instance().log_to_stdout("CONFIGURED CLIENTCONN2 APPENDXML");
    MsLogger<INFO>::get_instance().log_to_file("CONFIGURED CLIENTCONN2 APPENDXML",DEBUG);

    clientConn1.start();
    MsLogger<INFO>::get_instance().log_to_stdout("STARTED CLIENTCONN1");
    MsLogger<INFO>::get_instance().log_to_file("STARTED CLIENTCONN1");
    // clientConn2.start();
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