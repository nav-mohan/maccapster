#include "client.hpp"
#include "xmlhandler.hpp"
#include "rs232util.hpp"
#include "geoutil.hpp"
#include "macttsqueue.h"
#include "playbackutil.hpp"
#include "playbackqueue.hpp"
#include "decodeutil.hpp"
#include "downloadqueue.hpp"
#include "encoderFactory.hpp"
#include "encoderQueue.hpp"
#include "archiveQueue.hpp"
#include "zipArchiver.hpp"
#include "dailyWorker.hpp"

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
EncQueue encoderQueue;
ArchiveQueue archiveQueue;
ZipArchiver zipArchiver;
// DailyWorker dailyWorker;


// std::ofstream capturedOutput("output.txt");
int main(int argc, char *argv[])
{
    MsLogger<INFO>::set_log_dir(getStorageDirPath()); // All specializations <INFO>,<DEBUG>,<TRACE> will write to the same file. 
    auto mpegEnc = std::move(EncoderFactory("MPEG MED"));
    encoderQueue.Encode = [&](const std::string filename){
        std::visit([&filename](auto&& encoderVariant)
        {
            basic_log("START ENCODING " + filename,DEBUG);
            std::string outfilename = filename.substr(0, filename.rfind('.')) + ".mp3";
            FILE *fp = fopen(filename.c_str(),"rb");
            FILE *fpout = fopen(outfilename.c_str(),"wb");
            
            const int bufferSize = 2048; // FDK-AAC is finicky about the buffer-size. it maxes out at 2048 for MONO and 4096 for STEREO sticking to 2048 bytes works for MONO and STEREO
            void *pcmBufferQueue = malloc(bufferSize); 
            int bytesRead = 0;
            memset(pcmBufferQueue,0,bufferSize); 
            
            fseek(fp,44,SEEK_SET); // skip the WAV header for now but we should use the WAV-Header to switch between MONO and STEREO
            
            while((bytesRead = fread(pcmBufferQueue,1,bufferSize,fp)) > 0)
            {
                if(int bytesEncoded = encoderVariant->DoEncodeMono(pcmBufferQueue,bytesRead))
                    int bytesWritten = fwrite(encoderVariant->m_encBuffer,1,bytesEncoded,fpout);
            }
            if ( int flushedBytes = encoderVariant->DoFlush() ) 
                fwrite(encoderVariant->m_encBuffer, 1, flushedBytes, fpout);

            fclose(fp);
            fclose(fpout);
            basic_log("FINISHED ENCODING " + filename,DEBUG);
        },mpegEnc);
    };  
    encoderQueue.OnFinish = [](){
        // check which all WAV files have been succesfully encoded and delete the WAV files
        std::vector<const std::string> history = encoderQueue.GetHistory();
        basic_log("DONE ENCODING " + std::to_string(history.size()) + "WAV FILE(S)",INFO);
        for(const auto& wavSrc : history)
        {
            std::string mpegSrc = wavSrc.substr(0, wavSrc.rfind('.')) + ".mp3";
            std::string aacSrc = wavSrc.substr(0, wavSrc.rfind('.')) + ".aac";
            if(std::filesystem::exists(mpegSrc) || std::filesystem::exists(aacSrc)) 
            {
                basic_log("DELETING WAV FILE " + wavSrc , DEBUG);
                deleteFile(wavSrc);
            }
        }
    };


    archiveQueue.DoArchiving = [&](const std::string& directoryPath){
        const std::string archivePath = directoryPath + ".zip";
        zipArchiver.CompressDirectoryToZip(archivePath,directoryPath);
    };
    archiveQueue.OnFinishArchiving = [&](){
        basic_log("DONE ZIPPING ALL FOLDERS",INFO);
    };
 
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
    playbackQueue.OnFinish = [&](){
        relayControl.clear_pin();
        // convert all these WAV files into MP3 or AAC
        std::vector<const std::string> history = playbackQueue.GetHistory();
        for(const auto & wavFile: history)
        {
            const std::string extension = wavFile.substr(wavFile.rfind('.'));
            if(extension != "WAV" && extension != "wav") continue; // Only WAV files must be encoded. Downloaded MP3 files shoudln't be encoded.
            encoderQueue.Push(wavFile);
        }
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
        d.filename_ = getStorageDirPath() + "/" + filename;
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
        filename = getStorageDirPath() + "/" + filename;
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

    struct WaitForAllQueues
    {
        bool operator()()
        {
            if([ttsq_ getQueueSize]) 
            {
                printf("ttsq is busy\n");
                return  false;
            }
            if(encoderQueue.GetHistory().size()) 
            {
                printf("encoderQueue is busy\n");
                return false;
            }
            if(dwq_.GetHistory().size()) 
            {
                printf("downloadQueue is busy\n");
                return false;
            }
            if(pbq_.GetHistory().size()) 
            {
                printf("playbackQueue is busy\n");
                return false;
            }

            printf("Daily worker done waiting for all queues\n");
            return true;            
        }

        // the list of external-queues that the DailyWorker must wait for. This list could be easily expanded.
        const TextToSpeechQueue * ttsq_;
        const PBQueue           & pbq_;
        const DWQueue           & dwq_;

        WaitForAllQueues(const TextToSpeechQueue * t, const PBQueue & p, const DWQueue & d )
            : ttsq_(t) , pbq_(p) , dwq_(d){}
    };

    WaitForAllQueues waiter(ttsq, playbackQueue, downloadQueue);
    DailyWorker<WaitForAllQueues> dailyWorker(waiter);
    dailyWorker.SetScheduleTime(18,41,40);
    dailyWorker.DoTask = [&](){
        printf("DAILY TASK\n");
        // take the .mp3, .aac, .xml files inside Storage/ folder (except .log files) and move it into a folder with today's date
        // then zip that folder 
        dailyWorker.SetScheduleTime(21,25,00);
    };

    xmlHandler.writeToFile = [](std::string content, std::string filename){
        FILE *xmllogfile = fopen((getStorageDirPath() + "/" + filename +".xml").c_str(),"w");
        fwrite(content.c_str(), 1, content.size(), xmllogfile);
        fclose(xmllogfile);
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