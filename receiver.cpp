// Nama File : receiver.cpp
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <string>
#include <iostream>
#include <thread>
#include <queue>
#include "transmitterFrame.h"
#include "receiverFrame.h"
#include "dcomm.h"

using namespace std;

struct CompareFrame {
    bool operator()(TransmitterFrame & frame1, TransmitterFrame & frame2) {
        // return "true" if "p1" is ordered before "p2", for example:
        return frame1.getFrameNumber() < frame2.getFrameNumber();
    }
};

priority_queue<TransmitterFrame, vector<TransmitterFrame>, CompareFrame > buffer;

// Kamus Global
struct sockaddr_in serverAddr, clientAddr;
struct sockaddr_storage serverStorage;
socklen_t addr_size, client_addr_size;


void sendNAK(int frameNum, int udpSocket) {
	ReceiverFrame ack(frameNum);
	ack.setAck(NAK);
	sendto(udpSocket,ack.toBytes(),ack.getBytesLength(),0,(struct sockaddr *)&serverStorage,addr_size);
}

void sendACK(int frameNum, int udpSocket) {
	ReceiverFrame ack(frameNum);
	ack.setAck(ACK);
	sendto(udpSocket,ack.toBytes(),ack.getBytesLength(),0,(struct sockaddr *)&serverStorage,addr_size);
}


void processMsg(int udpSocket) {
	TransmitterFrame frame;
	int num = 0;
	while(!buffer.empty()) {
		if(buffer.top().getFrameNumber() == num) {
			frame = buffer.top();	// blm ada operator= 
			buffer.pop();
			num = (num+1) % (WINDOW_SIZE-1);
			cout << frame.getData() << endl;
		} 
	}
}

bool isAllTrue(bool approved[], int length) {
	for(int i=0; i<length; i++) {
		if(!approved[i]) return false;
	}
	return true;
}

void setAllFalse(bool approved[], int length) {
	for(int i=0; i<WINDOW_SIZE; i++) {
		approved[i] = false;
	}
}

// Menerima message
void rcvMsg(int udpSocket) {
	char msg[100];
	bool approved[WINDOW_SIZE];
	setAllFalse(approved, WINDOW_SIZE);
	while (true) {
		recvfrom(udpSocket,msg,100,0,(struct sockaddr *)&serverStorage, &addr_size);
		TransmitterFrame frame(msg);
		if(frame.isError()) {
			sendNAK(frame.getFrameNumber(), udpSocket);
		} else {
			sendACK(frame.getFrameNumber(), udpSocket);
			if(!approved[frame.getFrameNumber()]) {
				buffer.push(frame);
				approved[frame.getFrameNumber()] = true;
				if(isAllTrue(approved, WINDOW_SIZE)) {
					setAllFalse(approved, WINDOW_SIZE);
				}
			}
		}
	}
} 

// Melakukan konfigurasi koneksi berdasarkan port tertentu
void configureSetting(int portNum) {
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(portNum);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
} 

int main (int argc, char* argv[]) {
	if (argc != 2) {
		cout << "Usage : ./receiver <port number>" << endl;
	} else {
		int udpSocket, portNum;
		char IP[30];
		portNum = atoi(argv[1]);
	  	udpSocket = socket(PF_INET, SOCK_DGRAM, 0);

	  	configureSetting(portNum);
		bind(udpSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
		
		// Inisialisasi ukuran variabel yang akan digunakan
		addr_size = sizeof serverStorage;

		thread th1(rcvMsg, udpSocket);
		thread th2(processMsg, udpSocket);

		th1.join();
		th2.join();
	}
	
	return 0;
}