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
#include <thread>
#include "transmitterFrame.h"

#define WINDOW_SIZE 5

using namespace std;



//Variable Global
char sign[5];
struct sockaddr_in serverAddr;
struct sockaddr_storage serverStorage;
socklen_t addr_size;

// Menerima sign yang dikirim ke receiver, apakah XON atau XOFF
void recvSign(int clientSocket) {
  while(1) {
    recvfrom(clientSocket,sign,5,0,NULL, NULL);
    if(strcmp(sign,"XOFF") == 0) {
      cout << "XOFF diterima" << endl;
      while(strcmp(sign,"XOFF") == 0) {
        recvfrom(clientSocket,sign,5,0,NULL, NULL);
      }
      cout << "XON diterima" << endl;
    } 
  }
}

// Mengirim pesan kepada receiver
void sendMsg(int clientSocket, char* ch) {
	int n, nBytes;
  	printf("%s\n", ch);
  	TransmitterFrame frame(1);
	frame.setData(ch, 4);
	frame.printBytes();
	char* msg = frame.toBytes();
	int msgLength = frame.getBytesLength(); 
	sendto(clientSocket,msg,msgLength,0,(struct sockaddr *)&serverAddr,addr_size);  	
}

// Melakukan konfigurasi koneksi berdasarkan port dan IP tertentu
void configureSetting(char IP[], int portNum) {
	  serverAddr.sin_family = AF_INET;
	  serverAddr.sin_port = htons(portNum);
	  serverAddr.sin_addr.s_addr = inet_addr(IP);
	  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
}

void rcvSign(socket) {
	while(true) {
		// Tunggu acknya
		// kalo timeout kirim lagi paketnya 
		// kalo keterimanya NAK, kirim lagi sesuai nomor buffer melalui sendFrame
	}
}

void sendFrame(socket)  {
	while (true) {
		// kalo data sudah habis, break
		// Ambil data sebanyak WINDOW_SIZE
		// send semua data tersebut
		// tunggu respon dari rcvsign apakah semuanya uda di ack atau belum
		// respon juga bisa berbentuk rcvSign minta sendframe ngirim frame lagi
	}
}


int main(int argc, char* argv[]) {
  if (argc != 4) {
    cout << "Usage : ./transmitter <IP address> <port number> <file name>" << endl;
  } else {
    int clientSocket, portNum;
    string NAMA_FILE;

    portNum = atoi(argv[2]);
    NAMA_FILE = argv[3];

    // Membuat UDP socket
    clientSocket = socket(PF_INET, SOCK_DGRAM, 0);

    cout << "Membuat socket untuk koneksi ke " << argv[1] << ":" << portNum << endl;
    configureSetting(argv[1], portNum);
    
    // Inisialisasi ukuran variabel yang akan digunakan
    addr_size = sizeof serverAddr;  

    thread th1(rcvSign, clientSocket);
    thread th2(sendFrame, clientSocket);


    char msg[100];
    // pake string 
    msg[0] = 'H';
    msg[1] = 'a';
    msg[2] = 'l';
    msg[3] = 'o';

    sendMsg(clientSocket,msg);
    th1.join();
    th2.join();
  }

  return 0;
}