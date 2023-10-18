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
  uint32_t pcmDataSize;

  WavHeader(int majorVersion)
  {
    if(majorVersion == 13)
    {
      this->audioFormat = 1;
      this->bitsPerSample = 16;
    }
    else 
    {
      this->audioFormat = 3;
      this->bitsPerSample = 32;
    }
  }
  WavHeader &setNumChannels(uint16_t nc)
  {
    this->numChannels = nc;
    return *this;
  }
  WavHeader& setSampleRate(uint32_t sr) 
  {
    this->sampleRate = sr;
    return *this;
  }
  WavHeader& setBitsPerSample(uint16_t bps)
  {
    this->bitsPerSample = bps;
    this->blockAlign = bps/8;
    this->bytesPerSec = this->blockAlign * this->sampleRate;
    if(bps == 32) this->audioFormat = 3;
    if(bps == 16) this->audioFormat = 1;
    return *this;
  }
  WavHeader& setPcmDataSize(uint32_t ds)
  {
    this->pcmDataSize = ds;
    this->chunkSize = ds + 36;
    return *this;
  }
};