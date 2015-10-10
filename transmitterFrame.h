#ifndef _TRANSMITTERFRAME_
#define _TRANSMITTERFRAME_

#include "dcomm.h"
#include <stdio.h>
#include "crc16.h"

class TransmitterFrame {
private:

    char frameNumber;
    char * data;
    int length;
    bool error;

public:
    TransmitterFrame(int frameNumber) {
        this->length = 0;
        this->frameNumber = frameNumber;
        this->error = false;
    }

    TransmitterFrame(char * frame) {
        this->error = false;
        ////SETTING FRAME NUMBER
        this->setFrameNumber(frame[1]);


        //// SETTING DATA AND LENGTH
        int dataLength = 0;
        for (int i = 3 /* after STX */; frame[i] != ETX && frame[i] != 0; ++i) {
            ++dataLength;
            if (frame[i] == 0) this->error = true;
        }

        char * data = new char[dataLength + 1];
        for (int i = 0; i < dataLength; ++i) {
            data[i] = frame[i + 3];
        }
        this->setData(data, dataLength);

        if (!this->error) {
            char checksum[5]; 
            for (int i = 3 + dataLength + 1 /* after ETX */; i < 3 + dataLength + 5; i++) {
                checksum[i - 3 - dataLength - 1] = frame[i];
            }

            char * framex = this->toBytes();
            char trueChecksum[5];
            for (int i = 3 + dataLength + 1 /* after ETX */; i < 3 + dataLength + 5; i++) {
                trueChecksum[i - 3 - dataLength - 1] = framex[i];
            }

            //check equality    
            for (int i = 0; i < 4 && !this->error; i++) {
                if (checksum[i] != trueChecksum[i]) {
                    this->error = true;
                }
            }
        }
    }

    char getFrameNumber() { return this->frameNumber; }
    void setFrameNumber(char newNumber) { this->frameNumber = newNumber; }

    char * getData() { return this->data; }

    void setData(char * newData, int length) {
        this->data = new char[length + 1];
        for (int i = 0; i < length; ++i) {
            this->data[i] = newData[i];
        }
        this->length = length;
    }

    int getLength() { return this->length; }

    bool isError() { return this->error; }

    char * toBytes() {
        char * o = new char[1 + 1 + 1 + this->length + 1 + 4];
        o[0] = SOH;
        o[1] = this->frameNumber;
        o[2] = STX;
        sprintf(o, "%s%s", o, this->data);
        sprintf(o, "%s%c", o, ETX);
        sprintf(o, "%s%x", o, calc_crc16(o));
        return o;
    }

    int getBytesLength() { return 1 + 1 + 1 + this->length + 1 + 4; }

    void printBytes() {
        char * buffer = this->toBytes();
        for(int j = 0; buffer[j] != 0; j++)
            printf("%02X ", buffer[j]);
    }
};

#endif