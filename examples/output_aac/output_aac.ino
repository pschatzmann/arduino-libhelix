
/**
 * @file output_aac.ino
 * @author Phil Schatzmann
 * @brief We just display the decoded audio data on the serial monitor
 * @version 0.1
 * @date 2021-07-18
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "AACDecoderHelix.h"
#include "BabyElephantWalk60_aac.h"

using namespace libhelix;

AACDecoderHelix aac;

void dataCallback(AACFrameInfo &info, int16_t *pcm_buffer, size_t len,
                  void *ref) {
  for (size_t i = 0; i < len; i += info.nChans) {
    for (int j = 0; j < info.nChans; j++) {
      Serial.print(pcm_buffer[i + j]);
      Serial.print(" ");
    }
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  aac.setDataCallback(dataCallback);
  aac.begin();
}

void loop() {
  Serial.println("writing...");
  aac.write(BabyElephantWalk60_aac, BabyElephantWalk60_aac_len);

  // restart from the beginning
  delay(2000);
  aac.begin();
}
