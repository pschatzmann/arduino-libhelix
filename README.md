# A MP3 and AAC Decoder using Helix

I am providing the [Helix MP3 decoder from RealNetworks](https://en.wikipedia.org/wiki/Helix_Universal_Server) as a simple Arduino Library. The Helix decoders are based on 16bits integers, so they are a perfect fit to be used in Microcontrollers.

MP3 is a compressed audio file formats based on PCM. A 2.6 MB wav file can be compressed down to 476 kB MP3.

This project can be used stand alone or together with the [arduino-audio_tools library](https://github.com/pschatzmann/arduino-audio-tools). It can also be used from non Arduino based systems with the help of cmake.

The Helix MP3 decoder provides Layer 3 support for MPEG-1, MPEG-2, and MPEG-2.5. It supports variable bit rates, constant bit rates, and stereo and mono audio formats. 

### API Example

The API provides the decoded data to a Arduino Stream or alternatively to a callback function. Here is a MP3 example using the callback:

```
#include "MP3DecoderHelix.h"
#include "music_mp3.h"

using namespace libhelix;

void dataCallback(MP3FrameInfo &info, int16_t *pwm_buffer, size_t len) {
    for (int i=0; i<len; i+=info.channels){
        for (int j=0;j<info.channels;j++){
            Serial.print(pwm_buffer[i+j]);
            Serial.print(" ");
        }
        Serial.println();
    }
}

MP3DecoderHelix mp3(dataCallback);

void setup() {
    Serial.begin(115200);
    mp3.begin();
}

void loop() {
    Serial.println("writing...")
    mp3.write(music_data, muslic_len);    

    // restart from the beginning
    delay(2000);
    mp3.begin();
}
```


### Documentation

The [class documentation can be found here](https://pschatzmann.github.io/arduino-libhelix/html/annotated.html)


