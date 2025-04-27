#include <Audio.h>
static audioContext AUDIO;

audioContext *getAudioContext(){
    return &AUDIO;
}

void initAudio(){
    AUDIO.nr52 = 0;
    AUDIO.nr51 = 0;
}

u8 readAudio(u16 address){
    if(address == 0xFF26){
        return AUDIO.nr52;
    }
    else if(address == 0xFF25){
        return AUDIO.nr51;
    }
    else if(address == 0xFF24){
        return AUDIO.nr50;
    }
    return 0;
}

void writeAudio(u16 address, u8 value){
    if(address == 0xFF26){
        if(BIT(value, 7)){
            BIT_SET(AUDIO.nr52, 7, true);
        }
        else{
            BIT_SET(AUDIO.nr52, 7, false);
        }
    }
    else if(address == 0xFF25){
        AUDIO.nr51 = value;
    }
    else if(address == 0xFF24){
        AUDIO.nr50 = value;
    }
}

void tickAudio(){

}