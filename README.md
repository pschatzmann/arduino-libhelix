# A MP3 and AAC Decoder using Helix

I am providing the [Helix MP3 decoder from RealNetworks](https://en.wikipedia.org/wiki/Helix_Universal_Server) as a simple Arduino Library. The Helix decoders are based on 16bits integers, so they are a perfect fit to be used in Microcontrollers.

MP3 is a compressed audio file formats based on PCM. A 2.6 MB wav file can be compressed down to 476 kB MP3.

This project can be used stand alone or together with the [arduino-audio_tools library](https://github.com/pschatzmann/arduino-audio-tools). It can also be used from non Arduino based systems with the help of cmake.

The Helix MP3 decoder provides Layer 3 support for MPEG-1, MPEG-2, and MPEG-2.5. It supports variable bit rates, constant bit rates, and stereo and mono audio formats. 

## API Example

The API provides the decoded data to a __Arduino Stream__ or alternatively to a callback function. The platform dependent I2S implementations usually implement I2S as subclass of Stream, so you can just provide the corresponding I2S object to the constructor.

Here is a MP3 example using the callback:

```
#include "MP3DecoderHelix.h"
#include "music_mp3.h"

using namespace libhelix;

void dataCallback(MP3FrameInfo &info, int16_t *pcm_buffer, size_t len, void*) {
    for (int i=0; i<len; i+=info.nChans){
        for (int j=0;j<info.nChans;j++){
            Serial.print(pcm_buffer[i+j]);
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
    Serial.println("writing...");
    mp3.write(music_data, music_len);    

    // restart from the beginning
    delay(2000);
    mp3.begin();
}

```

## Installation

For Arduino, you can download the library as zip and call include Library -> zip library. Or you can git clone this project into the Arduino libraries folder e.g. with

```
cd  ~/Documents/Arduino/libraries
git clone https://github.com/pschatzmann/arduino-libhelix.git
```

This project can also be built and executed on your desktop with cmake:

```
cd arduino-libhelix
mkdir build
cd build
cmake ..
make
```

## Parameters

The decoder needs to allocate a buffer to store a frame and the decoded data. The following default values have been defined in ConfigHelix.h: 

```
#define MP3_MAX_OUTPUT_SIZE 1024 * 5
#define MP3_MAX_FRAME_SIZE 1024 * 2 
#define AAC_MAX_OUTPUT_SIZE 1024 * 8 
#define AAC_MAX_FRAME_SIZE 2100 
```

These values are working with standard bitrates, but in some exceptional cases (with extreme (220kbs) or insane (320kbs) bitrates) you might need to increase these values e.g. to
```
#define MP3_MAX_OUTPUT_SIZE 1024 * 10
#define MP3_MAX_FRAME_SIZE 3200
```

Alternatively you can set the values in your sketch by calling the following methods:
```
setMaxPCMSize(int size)
setMaxFrameSize(int size)
```

## Memory Management

On the ESP32 we support PSRAM: just activate it in the Arduino Tools menu and all the memory will be allocated in PSRAM.

## Logging

You can define the log level as Debug, Info, Warning, Error
```
LOGLEVEL_HELIX = LogLevelHelix::Info;
```



## Documentation

- The [Class Documentation can be found here](https://pschatzmann.github.io/arduino-libhelix/html/annotated.html)
- I also suggest that you have a look at [my related blogs](https://www.pschatzmann.ch/home/tag/codecs/)

I recommend to use this library together with my [Arduino Audio Tools](https://github.com/pschatzmann/arduino-audio-tools). 
This is just one of many codecs that I have collected so far: Further details can be found in the [Encoding and Decoding Wiki](https://github.com/pschatzmann/arduino-audio-tools/wiki/Encoding-and-Decoding-of-Audio) of the Audio Tools.

### Final Comments

This library is based on a adapted version of libhelix from [Earle Phil Hower's ESP8288Audio library](https://github.com/earlephilhower/ESP8266Audio)  
The decoder code is from the Helix project and licensed under RealNetwork's [RPSL license](https://github.com/pschatzmann/arduino-libhelix/blob/main/src/libhelix-mp3/RPSL.txt). For commercial use you might still going to need the usual AAC licensing.


