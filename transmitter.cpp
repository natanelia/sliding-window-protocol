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

using namespace std;

//Variable Global
char sign[128];
char contents[1000][128];
char word[128];
vector<TransmitterFrame *> buffer;
vector<TransmitterFrame *> window;
int neffContents = 0, idxContents = 0, clientSocket;
struct sockaddr_in serverAddr;
struct sockaddr_storage serverStorage;
socklen_t addr_size;

// Menerima sign yang dikirim ke receiver, apakah XON atau XOFF
void recvSign(int clientSocket) {
  while(1) {
    recvfrom(clientSocket,sign,5,0,NULL, NULL);
    ReceiverFrame frame(sign);
    if(frame.isError()) {
    	cout << "ERROR " << frame.getFrameNumber() << endl;
    } else if(frame.getAck() == ACK) {
    	window.erase(buffer.begin()+ (frame.getFrameNumber() - '0'));
     	cout << "ACK " << frame.getFrameNumber() << endl;
    } else if (frame.getAck() == NAK) {
    	cout << "NAK " << frame.getFrameNumber() << endl;
    }
  }
}

void *childCodes(void *threadArgs) {
	recvSign(clientSocket);
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
		TransmitterFrame * temp = new TransmitterFrame(i+1);
		temp->setData(contents[idxContents], strlen(contents[idxContents]));
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

// Mengirim pesan kepada receiver
void sendSingleFrame(int clientSocket, TransmitterFrame * frame) {
	char* msg = frame->toBytes();
	int msgLength = frame->getBytesLength(); 
	sendto(clientSocket,msg,msgLength,0,(struct sockaddr *)&serverAddr,addr_size);  	
}

void sendMultipleFrame() {
	for (int i = 0; i < WINDOW_SIZE; i++) {
		buffer[0]->printBytes();
		window.push_back(buffer[0]);
		buffer.erase(buffer.begin());
	}

	//ngurus timeout buat resend
	while (window.size() > 0) {
		for (int i = 0; i < window.size(); i++) {
			sendSingleFrame(clientSocket, window[i]);
		}
		sleep(ACK_TIMEOUT);
	}
}

int main(int argc, char* argv[]) {
  if (argc != 4) {
    cout << "Usage : ./transmitter <IP address> <port number> <file name>" << endl;
  } else {
    int portNum;
    char * NAMA_FILE;

    pthread_t childThread;

    portNum = atoi(argv[2]);
    NAMA_FILE = argv[3];

    // Membuat UDP socket
    clientSocket = socket(PF_INET, SOCK_DGRAM, 0);
    cout << "Membuat socket untuk koneksi ke " << argv[1] << ":" << portNum << endl;
    configureSetting(argv[1], portNum);
    
    // Inisialisasi ukuran variabel yang akan digunakan
    addr_size = sizeof serverAddr;  
	pthread_create(&childThread, NULL, childCodes, NULL);
    readFileAndStore(NAMA_FILE);
    for (int i=0; i<neffContents; i=i+BUFFER_SIZE) {
    	fillBuffer();
    	sendMultipleFrame();
    }
  }

  return 0;
}