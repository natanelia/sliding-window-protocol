// Nama File : transmitter.cpp
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "dcomm.h"
#include "transmitterFrame.h"
#include "receiverFrame.h"
#include <vector>
#include <time.h>
#include <ctime>
#include <mutex>
#include <thread>

#define NIL -999

using namespace std;

typedef struct {
	int waktu;
	int frameNum;
} timeFrame;

//Variable Global
char sign[128];
char contents[1000][128];
char word[128];
vector<TransmitterFrame> buffer;
vector<TransmitterFrame> window;
vector<timeFrame> timeBuffer;
int neffContents = 0, idxContents = 0, clientSocket;
struct sockaddr_in serverAddr;
struct sockaddr_storage serverStorage;
socklen_t addr_size;
int countSentBuffer = 0;
int countPendingACK = 0;
mutex mtx;



// Mengirim pesan kepada receiver
void sendSingleFrame(int clientSocket, TransmitterFrame frame) {
	char* msg = frame.toBytes();
	int msgLength = frame.getBytesLength(); 
	timeFrame temp;
	temp.waktu = time(0);
	temp.frameNum = frame.getFrameNumber();
	timeBuffer.push_back(temp);
	printf("Message ke-%d: %s\n", frame.getFrameNumber(), frame.getData());
	sendto(clientSocket,msg,300,0,(struct sockaddr *)&serverAddr,addr_size);	
}

int findFrame(int i) {
	for (int j = 0; j < window.size(); j++) {
		if (window[j].getFrameNumber() == i) {
			return j;
		}
	}
	return -999;
}

vector<timeFrame>:: iterator findTimeFrame(int num) {
	for(vector<timeFrame>::iterator it=timeBuffer.begin(); it < timeBuffer.end(); it++) {
		if(it->frameNum == num) return it;
	}
	return timeBuffer.begin();
}

// Menerima sign yang dikirim ke receiver, apakah ACK atau NAK
void recvSign(int clientSocket) {
  while(1) {
    recvfrom(clientSocket,sign,10,0,NULL, NULL);
    ReceiverFrame frame(sign);
    if(frame.getAck() == ACK) {
     	printf("ACK %d\n",frame.getFrameNumber());
     	timeBuffer.erase(timeBuffer.begin());
     	if (window.front().getFrameNumber() == frame.getFrameNumber()) {
     		window.erase(window.begin());
    		for(int i=0; i<countPendingACK+1; i++) {
    			if(!buffer.empty()) {
    				sendSingleFrame(clientSocket, buffer[0]);
	    			mtx.lock();
	    			window.push_back(buffer[0]);
	    			buffer.erase(buffer.begin());
	    			mtx.unlock();
    			}
    		}
    		countPendingACK = 0;
	    } else {
	    	countPendingACK++;
	    	mtx.lock();
	    	window.erase(window.begin() + findFrame(frame.getFrameNumber()));
	    	mtx.unlock();
	    }
    } else if (frame.getAck() == NAK) {
    	printf("NAK %d\n",frame.getFrameNumber());
    	sendSingleFrame(clientSocket, window[findFrame(frame.getFrameNumber())]);
    }
  }
}

void checkTimeout() {
	while (true) {
		if (!timeBuffer.empty()) {
			for (vector<timeFrame>::iterator it=timeBuffer.begin(); it>timeBuffer.end(); it++) {
				int now = time(0);
				if (((it->waktu - now)/double(CLOCKS_PER_SEC)*1000) > ACK_TIMEOUT*1000) {
					sendSingleFrame(clientSocket, findFrame(it->frameNum));
					timeBuffer.erase(it);
				}
			}
		}
	}
}



// Melakukan konfigurasi koneksi berdasarkan port dan IP tertentu
void configureSetting(char IP[], int portNum) {
	  serverAddr.sin_family = AF_INET;
	  serverAddr.sin_port = htons(portNum);
	  serverAddr.sin_addr.s_addr = inet_addr(IP);
	  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
}

void fillBuffer() {
	for (int i = 0; i < BUFFER_SIZE && idxContents < neffContents; i++) {
		TransmitterFrame temp(i+1);
		temp.setData(contents[idxContents], strlen(contents[idxContents]));
		buffer.push_back(temp);
		idxContents++;
	}
}

// Membaca dari file dan mengirimkan paket kepada receiver
void readFileAndStore(char * NAMA_FILE) {
	ifstream fin(NAMA_FILE);
    char ch;
	for (int i = 0; i < 128; i++) {
		word[i] = 0;
	}
    for (int j = 0; j < 1000; j++) {
		for (int i = 0; i < 128; i++) {
			contents[j][i] = 0;
		}
	}

    int nMsg = 0;
	fin >> noskipws >> ch;
	while (!fin.eof()) {
		if (ch == ' ' || ch=='\n' || ch=='\t' || ch=='\v' || ch=='\f' || ch=='\r') {
			for (int i = 0; i < nMsg; i++) {
				contents[neffContents][i] = word[i];
			}
			neffContents++;
			//reset word
			for (int i = 0; i < 128; i++) {
				word[i] = 0;
			}
			nMsg = 0;
		} else {
			word[nMsg++] = ch;
		}
		fin >> noskipws >> ch;
	}
	//masukin ke array pengiriman
	if (!(ch == ' ' || ch=='\n' || ch=='\t' || ch=='\v' || ch=='\f' || ch=='\r')) {
		for (int i = 0; i < nMsg; i++) {
			contents[neffContents][i] = word[i];
		}
		neffContents++;
	}

}

void childCode() {
    // Inisialisasi
    fillBuffer();
    
    mtx.lock();
    for (int i = 0; i < WINDOW_SIZE; i++) {

		sendSingleFrame(clientSocket, buffer[0]);
		window.push_back(buffer[0]);
		buffer.erase(buffer.begin());
	}
	mtx.unlock();
}

int main(int argc, char* argv[]) {
	if (argc != 4) {
    	cout << "Usage : ./transmitter <IP address> <port number> <file name>" << endl;
  	} else {
	    int portNum;
	    char * NAMA_FILE;

	    pthread_t childThread1;
	    pthread_t childThread2;

	    portNum = atoi(argv[2]);
	    NAMA_FILE = argv[3];

	    // Membuat UDP socket
	    clientSocket = socket(PF_INET, SOCK_DGRAM, 0);
	    cout << "Membuat socket untuk koneksi ke " << argv[1] << ":" << portNum << endl;
	    configureSetting(argv[1], portNum);
	    
	    // Inisialisasi ukuran variabel yang akan digunakan
	    addr_size = sizeof serverAddr;  
		
	    readFileAndStore(NAMA_FILE);
		thread th1(recvSign, clientSocket);
		thread th2(childCode);

		th1.join();
		th2.join();
	}


  return 0;
}