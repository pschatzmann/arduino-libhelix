#ifdef EMULATOR
#include "Arduino.h"
#include "output_aac.ino"

int main(){
    setup();
    while(true){
        loop();
    }
    return -1;
}

#endif
