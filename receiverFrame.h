#ifndef _RECEIVERFRAME_
#define _RECEIVERFRAME_

#include "dcomm.h"
#include <stdio.h>
#include "crc16.h"

class ReceiverFrame {
private:
    char ack;
    char frameNumber;

public:
    ReceiverFrame(int frameNumber) {
        this->frameNumber = frameNumber;
    }

    ReceiverFrame(char * frame) {
        //// SETTING ACK
        this->setAck(frame[0]);

        ////SETTING FRAME NUMBER
        this->setFrameNumber(frame[1]);
    }

    char getAck() { return this->ack; }
    void setAck(char newAck) { this->ack = newAck; }

    char getFrameNumber() { return this->frameNumber; }
    void setFrameNumber(char newNumber) { this->frameNumber = newNumber; }

    char * toBytes() {
        char * o = new char[1 + 1 + 4];
        o[0] = this->ack;
        o[1] = this->frameNumber;
        sprintf(o, "%s%x", o, calc_crc16(o));
        return o;
    }

    void printBytes() {
        char * buffer = this->toBytes();
        for(int j = 0; buffer[j] != 0; j++)
            printf("%02X ", buffer[j]);
        printf("\n");
    }
};

#endif