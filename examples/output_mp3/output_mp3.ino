/**
 * @file output_mp3.ino
 * @author Phil Schatzmann
 * @brief We just display the decoded audio data on the serial monitor
 * @version 0.1
 * @date 2021-07-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "MP3DecoderHelix.h"
#include "BabyElephantWalk60_mp3.h"

using namespace libhelix;

MP3DecoderHelix mp3;


void dataCallback(MP3FrameInfo &info, int16_t *pcm_buffer, size_t len, void* ref) {
    for (size_t i=0; i<len; i+=info.nChans){
        for (int j=0;j<info.nChans;j++){
            Serial.print(pcm_buffer[i+j]);
            Serial.print(" ");
        }
        Serial.println();
    }
}

void setup() {
    Serial.begin(115200);
    mp3.setDataCallback(dataCallback);
    mp3.begin();
}

void loop() {
    Serial.println("writing...");
    mp3.write(BabyElephantWalk60_mp3, BabyElephantWalk60_mp3_len);    

    // restart from the beginning
    delay(2000);
    mp3.begin();
}
