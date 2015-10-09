
#include <iostream>
#include <fstream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "dcomm.h"

#define BUFSIZE 2

using namespace std;
struct sockaddr_in myaddr;  /* our address */
struct sockaddr_in remaddr; /* remote address */
socklen_t addrlen = sizeof(remaddr);        /* length of addresses */
int recvlen;            /* # bytes received */
int fd;             /* our socket */
char buf[BUFSIZE];  /* # bytes in acknowledgement message */

void *childCodes(void *threadArgs) {
    for (;;) {
        recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
        if (recvlen >=  0) {
            buf[recvlen] = 0;   // expect a printable string - terminate it
        }
    }
}

int main(int argc, char *argv[])
{
	if (argc <= 3) {
		cout << "ERROR: No address, port, or file name" << endl;
		return 0;
	}

    pthread_t childThread;

	const string CLIENT_ADDRESS = argv[1];
	const int CLIENT_PORT = atoi(argv[2]);
	char *server = argv[1];

	cout << "Membuat socket untuk koneksi ke " << CLIENT_ADDRESS << ":" << CLIENT_PORT << " ..." << endl;

	// membuat socket UDP
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		cout << "ERROR: cannot create socket" << endl;
		return 1;
	}


    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(0);

    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return 0;
    }       

	// mendefinisikan address tujuan pengiriman
	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(CLIENT_PORT);
	if (inet_aton(server, &remaddr.sin_addr)==0) {
		cout << "ERROR: socketing to destination failed" << endl;
		return 1;
	}

	// membuat proses child buat baca XON/XOFF
    pthread_create(&childThread, NULL, childCodes, NULL);

    // proses parent
    int c = 0;
	fstream inputFile (argv[3], fstream::in);
	if (inputFile.is_open())
	{
		char inputChar;
		while (inputFile >> noskipws >> inputChar) {
            usleep(10000);
			if (buf[0] == XOFF) {
				cout << "XOFF diterima." << endl;
                cout << "Menunggu XON..." << endl;
				while (buf[0] == XOFF) {
                    usleep(10000);
				}
				cout << "XONN diterima." << endl;
			}
            char sendBuf[1];
            sendBuf[0] = inputChar;
			if (sendto(fd, sendBuf, 1, 0, (struct sockaddr *)&remaddr, addrlen) >= 0)
				cout << "Mengirim byte ke-" << ++c << ": ‘" << inputChar << "’" << endl;
		}


        char sendBuf[1];
        sendBuf[0] = Endfile;
        sendto(fd, sendBuf, 1, 0, (struct sockaddr *)&remaddr, addrlen);
            //cout << "Mengirim byte ke-" << ++c << ": EOF" << endl;

		inputFile.close();
	} else {
		cout << "Unable to open file" << endl;
	};
    close(fd);

    return 0;
}
