struct WavHeader
{
  uint8_t RIFF[4] = {'R', 'I', 'F', 'F'};
  uint32_t chunkSize;
  uint8_t WAVE[4] = {'W', 'A', 'V', 'E'};

  // fmt
  uint8_t fmt[4]            = {'f', 'm', 't', ' '};
  uint32_t fmtSize          = 16;    // bytes
  uint16_t audioFormat      = 3; // F32PCM
  uint16_t numChannels      = 1; // mono
  uint32_t sampleRate       = 22050;      // Hertz
  uint32_t bytesPerSec      = 4*22050;     // sampleRate * sampleWidth
  uint16_t blockAlign       = 4;  // 32-bit mono
  uint16_t bitsPerSample    = 32;

  // data
  uint8_t data[4] = {'d', 'a', 't', 'a'};
  uint32_t dataSize;
};