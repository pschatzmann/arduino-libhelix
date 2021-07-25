
#ifdef EMULATOR
#include "Arduino.h"
#include "output_mp3.ino"

int main(){
    setup();
    while(true){
        loop();
    }
    return -1;
}

#endif