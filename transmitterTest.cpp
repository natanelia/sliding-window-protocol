// Nama File : transmitter.cpp
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <thread>
#include <fstream>

#define Endfile 26;

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
void sendMsg(int clientSocket, string file) {
  int n, nBytes;
  char ch;
  fstream fin(file, fstream::in);
  n = 0;
  fin >> noskipws >> ch;
  while (!fin.eof()) {
    sleep(1);
    if(strcmp(sign,"XOFF") != 0) {
      n++;
      cout << "Mengirim byte ke-" << n << ": " << ch << endl;
      sendto(clientSocket,&ch,1,0,(struct sockaddr *)&serverAddr,addr_size);
      fin >> noskipws >> ch;
    } else if(strcmp(sign,"XOFF") == 0){
      cout << "Menunggu XON..." << endl;
    }
  }
  // Mengirim flag EOF
  ch = Endfile;
  sendto(clientSocket,&ch,1,0,(struct sockaddr *)&serverAddr,addr_size);
  //cout << "*** PENGIRIMAN FILE SELESAI ***" << endl;
  exit(0);
}

// Melakukan konfigurasi koneksi berdasarkan port dan IP tertentu
void configureSetting(char IP[], int portNum) {
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(portNum);
  serverAddr.sin_addr.s_addr = inet_addr(IP);
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);  
}

int main(int argc, char* argv[]) {
  if (argc != 4) {
    cout << "Usage : ./transmitter <IP address> <port number> <file name>" << endl;
  } else {
    int clientSocket, portNum;
    string NAMA_FILE;

    portNum = atoi(argv[2]);
    NAMA_FILE = argv[3];

    // Membuat UPD socket
    clientSocket = socket(PF_INET, SOCK_DGRAM, 0);

    cout << "Membuat socket untuk koneksi ke " << argv[1] << ":" << portNum << endl;
    configureSetting(argv[1], portNum);
    //sendto(clientSocket,argv[1],strlen(argv[1])+1,0,(struct sockaddr *)&serverAddr,addr_size);
    // Inisialisasi ukuran variabel yang akan digunakan
    addr_size = sizeof serverAddr;  

    sendto(clientSocket,argv[1],20,0,(struct sockaddr *)&serverAddr,addr_size);

    cout << "Membaca dari file '"<< NAMA_FILE <<"' ..."<< endl;

    // Inisialisasi sign dengan XON
    strcpy(sign,"XON");

    thread th1(sendMsg, clientSocket, NAMA_FILE);
    thread th2(recvSign, clientSocket);

    th1.join();
    th2.join();
  }

  return 0;
}