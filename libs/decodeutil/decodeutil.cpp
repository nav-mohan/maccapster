#include "decodeutil.hpp"

void decode_and_write(std::string&& encodedData, const std::string& filename)
{
    std::string data(std::move(encodedData));
    const unsigned char *input = (const unsigned char *)data.c_str();
    int len = data.size();
    size_t outputSize = len*3/4;
    MsLogger<INFO>::get_instance().log_to_stdout("DecodeUtil::decode_and_write() len=" + std::to_string(len) + " outputSize=" + std::to_string(outputSize));
    MsLogger<INFO>::get_instance().log_to_file("DecodeUtil::decode_and_write() len=" + std::to_string(len) + " outputSize=" + std::to_string(outputSize));
    char *out = (char*)malloc(outputSize + 5);
    char *result = out;
    signed char vals[4];

    while(len > 0) 
    {
        if(len < 4)
        {
            free(result);
            MsLogger<INFO>::get_instance().log_to_stdout("DecodeUtil::decode_and_write() INVALID");
            MsLogger<INFO>::get_instance().log_to_file("DecodeUtil::decode_and_write() INVALID");
            return; /* Invalid Base64 data */
        }

        vals[0] = base64decode[*input++];
        vals[1] = base64decode[*input++];
        vals[2] = base64decode[*input++];
        vals[3] = base64decode[*input++];

        if(vals[0] < 0 || vals[1] < 0 || vals[2] < -1 || vals[3] < -1) {
            len -= 4;
            continue;
        }

        *out++ = vals[0]<<2 | vals[1]>>4;
        /* vals[3] and (if that is) vals[2] can be '=' as padding, which is
           looked up in the base64decode table as '-1'. Check for this case,
           and output zero-terminators instead of characters if we've got
           padding. */
        if(vals[2] >= 0)
            *out++ = ((vals[1]&0x0F)<<4) | (vals[2]>>2);
        else
            *out++ = 0;

        if(vals[3] >= 0)
            *out++ = ((vals[2]&0x03)<<6) | (vals[3]);
        else
            *out++ = 0;

        len -= 4;
    }
    // printf("out = %s\n",out);
    FILE *fp = fopen(filename.c_str(),"wb");
    fwrite(result,1,outputSize,fp);
    fclose(fp);
    free(result);
}