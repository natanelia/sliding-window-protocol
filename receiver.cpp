// Nama File : receiver.cpp
// Deskripsi : membuat receiver untuk menerima pesan dengan menggunakan 
// sliding window protocol dan selective repeat ARQ

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

vector<TransmitterFrame> buffer;

// Menghapus buffer 
void delBuffer(vector<TransmitterFrame> &buffer, int frameNum, TransmitterFrame& result) {
	for(vector<TransmitterFrame>::iterator i=buffer.begin(); i<buffer.end(); i++) {
		if(i->getFrameNumber() == frameNum) {
			result = *i;
			buffer.erase(i);
			break;
		}
	}
}

// mengecek apakah ada frame yang frame numbernya valid ato nggak
bool isElement(vector<TransmitterFrame> buffer, int frameNum) {
	for(vector<TransmitterFrame>::iterator i=buffer.begin(); i<buffer.end(); i++) {
		if(i->getFrameNumber() == frameNum) {
			return true;
		}
	}	
	return false;
}

// Kamus Global
struct sockaddr_in serverAddr, clientAddr;
struct sockaddr_storage serverStorage;
socklen_t addr_size, client_addr_size;

// Mengirim NAK
void sendNAK(int frameNum, int udpSocket) {
	char temp = char(frameNum);
	ReceiverFrame ack(temp);
	ack.setAck(NAK);
	printf("NAK %d\n", frameNum);
	sendto(udpSocket,ack.toBytes(),ack.getBytesLength(),0,(struct sockaddr *)&serverStorage,addr_size);
}

// Mengirim ACK
void sendACK(int frameNum, int udpSocket) {
	char temp = char(frameNum);
	ReceiverFrame ack(temp);
	ack.setAck(ACK);
	printf("ACK %d\n", frameNum);
	sendto(udpSocket,ack.toBytes(),ack.getBytesLength(),0,(struct sockaddr *)&serverStorage,addr_size);
}

// memproses message yang ada di buffer.
void processMsg(int udpSocket) {
	TransmitterFrame frame;
	int num = 1;
	while(true) {
		if(!buffer.empty()) {
			if(isElement(buffer, num)) {
				delBuffer(buffer, num, frame);
				if(num > BUFFER_SIZE) num = 1;
				else num++;
				printf("Message ke-%d: %s\n", frame.getFrameNumber(), frame.getData());
			} 		
		}
	}
}

// mengecek apakah seluruh elemen dari approved true
bool isAllTrue(bool approved[], int length) {
	for(int i=0; i<length; i++) {
		if(!approved[i]) return false;
	}
	return true;
}

// mengeset semua nilai array approved dengan nilai false
void setAllFalse(bool approved[], int length) {
	for(int i=0; i<length; i++) {
		approved[i] = false;
	}
}

// Menerima message dan mengirim ACK atau NAK bila frame benar atau salah
void rcvMsg(int udpSocket) {
	char msg[100];
	bool approved[WINDOW_SIZE+1];
	setAllFalse(approved, WINDOW_SIZE);
	while (true) {
		recvfrom(udpSocket,msg,300,0,(struct sockaddr *)&serverStorage, &addr_size);
		TransmitterFrame frame(msg);
		if(!frame.isError()) {
			sendACK(frame.getFrameNumber(), udpSocket);
			if(!approved[frame.getFrameNumber()]) {
				buffer.push_back(frame);
				approved[frame.getFrameNumber()] = true;
				if(isAllTrue(approved, WINDOW_SIZE+1)) {
					setAllFalse(approved, WINDOW_SIZE+1);
				}
			}
		} else {
			sendNAK(frame.getFrameNumber(), udpSocket);
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