// A thin wrapper around MPEGEncoder and AACEncoder to encapsulate them both within a std::variant

#include "encoder.hpp"
#include <variant>

// MPEGEncoder<ENCODERBITRATE::LOW, ENCODERSAMPLERATE::MEDIUM, ENCODERCHANNEL::STEREO> mpegEnc;
MPEGEncoder<ENCODERBITRATE::LOW, ENCODERSAMPLERATE::MEDIUM, ENCODERCHANNEL::MONO> mpegEnc;

AACEncoder<ENCODERBITRATE::LOW, ENCODERSAMPLERATE::MEDIUM, ENCODERCHANNEL::STEREO> aacEnc;
// AACEncoder<ENCODERBITRATE::MEDIUM, ENCODERSAMPLERATE::MEDIUM, ENCODERCHANNEL::MONO> aacEnc;


// we need this for the default constructor of a std::variant
struct DummyEncoder
{
    DummyEncoder(){printf("DUMMY::CONSTRUCT\n");}
    ~DummyEncoder(){printf("DUMMY::DESTORY\n");}
    constexpr int       DoEncodeMono(const void *pcmBuffer, const uint32_t pcmBufferSize){return 0;}
    constexpr int       DoEncodeInterleaved(const void *pcmBuffer, const uint32_t pcmBufferSize){return 0;}
    constexpr int       DoFlush(){return 0;}
    unsigned char       m_encBuffer[2];

};

// we are using unique_ptrs (or heap-allocated pointers) to prevent std::variant from destroying 
using AllPossibleEncoderPtrs = std::variant<
    std::unique_ptr<DummyEncoder>, // this prevents the default constructor of a std::variant from constructing an actual encoder
    std::unique_ptr< MPEGEncoder<ENCODERBITRATE::LOW,       ENCODERSAMPLERATE::MEDIUM, ENCODERCHANNEL::MONO>>,
    std::unique_ptr< AACEncoder <ENCODERBITRATE::LOW,       ENCODERSAMPLERATE::MEDIUM, ENCODERCHANNEL::MONO>>,
    std::unique_ptr< AACEncoder <ENCODERBITRATE::MEDIUM,    ENCODERSAMPLERATE::MEDIUM, ENCODERCHANNEL::MONO>>
>;

AllPossibleEncoderPtrs EncoderFactory(const std::string encoderType)
{
    AllPossibleEncoderPtrs encPtr;
    if(encoderType == "MPEG MED")
    {
        auto mpegMedPtr = std::make_unique<MPEGEncoder<ENCODERBITRATE::LOW,ENCODERSAMPLERATE::MEDIUM,ENCODERCHANNEL::MONO>>();
        encPtr = std::move(mpegMedPtr);
    }
    if(encoderType == "MPEG LOW")
    {
        auto mpegLowPtr = std::make_unique<MPEGEncoder<ENCODERBITRATE::LOW,ENCODERSAMPLERATE::MEDIUM,ENCODERCHANNEL::MONO>>();
        encPtr = std::move(mpegLowPtr);
    }
    if(encoderType == "AAC MED")
    {
        auto aacMedPtr = std::make_unique<AACEncoder<ENCODERBITRATE::MEDIUM,ENCODERSAMPLERATE::MEDIUM,ENCODERCHANNEL::MONO>>();
        encPtr = std::move(aacMedPtr);
    }

    return encPtr;
}
