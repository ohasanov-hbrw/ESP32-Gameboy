#include <DebugFuncs.h>
#include <Bus.h>

static char message[1024] = {0};
static int messageSize = 0;
static int lastMessageSize = 0;

void updateDebug(){
    if (readBus(0xFF02) == 0x81){
        char c = readBus(0xFF01);
        message[messageSize++] = c;
        writeBus(0xFF02, 0);
    }
}

void printDebug(){
    if (message[0]){
        lastMessageSize = messageSize;
        printf("DEBUG: %s\n", message);
    }
}