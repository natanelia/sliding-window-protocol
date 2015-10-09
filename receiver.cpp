/*
 File : T1_rx.cpp
*/ 
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <iostream>
#include <fstream>
#include <cstring> 
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include "dcomm.h"

/* Delay to adjust speed of consuming buffer, in milliseconds */ 
#define DELAY 1

/* Define receiver buffer size */ 
#define RXQSIZE 8 

using namespace std;

Byte rxbuf[RXQSIZE]; //Receive Buffer
QTYPE rcvq = { 0, 0, 0, RXQSIZE, rxbuf };
QTYPE *rxq = &rcvq;
Byte sent_xonxoff = XON;
bool send_xon = false,send_xoff = false;

/* Socket */

int sockfd; // listen on sock_fd 

int recvlen;			/* # bytes received */

/* Address*/
struct sockaddr_in myaddr;	/* our address */
struct sockaddr_in remaddr;	/* remote address */
socklen_t addrlen = sizeof(remaddr);		/* length of addresses */

/* Global Counter*/
int globalCounterChild = 0;
int globalCounterParent = 0;

/* Functions declaration */
static Byte *rcvchar(int sockfd, QTYPE *queue);
static Byte *q_get(QTYPE *, Byte *);
void *childCodes(void *threadArgs);
char *findIPV4Address();

/* Main Program */
int main(int argc, char *argv[]){
	pthread_t childThread;
	Byte c;
	char* myaddress;

	/* create a UDP socket */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}

	//Find my address
	myaddress = findIPV4Address();

	/* Insert code here to bind socket to the port number given in argv[1]. */
	
	//Convert the char* argv to port (integer)
	int port = atoi(argv[1]);
	int IPV4address = atoi(myaddress);

	cout << "Receiver " << port << endl;

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(port);

	cout << "Binding pada " << myaddress << ":" << port << "..." <<endl;

	if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("Binding gagal");
		return 0;
	}
	
	/* Initialize XON/XOFF flags */

	/* Create child process */
	pthread_create(&childThread, NULL, childCodes, NULL);
	
	/*** IF PARENT PROCESS ***/
	while (true) {
		c = *(rcvchar(sockfd, rxq));
		// Quit on end of file
		if (c == Endfile) {
			pthread_join(childThread, NULL);
		}
	}
	close(sockfd);
	return 0;
}

void *childCodes(void *threadArgs) {
	while (true) {
		Byte *data;
		// Call q_get
		Byte *out = q_get(rxq, data);/*
		while (q_get(rxq, data) == NULL) {
			cout << rxq->count << endl;
			sleep(1);
		}*/
		
		//Quit on end of file
		if (out != NULL) { 
			if (*out == Endfile) {
				exit(0);	
			}
		}
	
		// Can introduce some delay here. 
		//usleep(DELAY);
		sleep(DELAY);
	}
}

static Byte *rcvchar(int sockfd, QTYPE *queue){

	/* Insert code here. 

	Read a character from socket and put it to the receive buffer. 

	If the number of characters in the receive buffer is above certain 

	level, then send XOFF and set a flag (why?). 

	Return a pointer to the buffer where data is put. 

	*/

	//Minimum Upper Limit must be smaller than Receiver Buffer Size 
	int minUpperLimit = 6;
	char a[RXQSIZE];	
	int retId = queue->rear;

	recvlen = recvfrom(sockfd, a , strlen(a), 0, (struct sockaddr *)&remaddr, &addrlen);

	if (recvlen < 0) {
		cout << "ERROR: recvfrom failed" << endl;
	} else {
		queue->data[queue->rear] = a[0];
		queue->count++;
		globalCounterParent++;
		//increment rear circularly
		if (queue->rear >= RXQSIZE - 1) {
			queue->rear = 0;
		} else {
			++queue->rear;
		}
		if (queue->data[retId] != Endfile)
				cout << "Menerima byte ke-" << globalCounterParent << "." << endl;
	}


	if (queue->count >= minUpperLimit) {
		char b[RXQSIZE];
		b[0] = XOFF;
		cout <<	"Buffer > minimum upperlimit. Mengirim XOFF." << endl;		
		if (sendto(sockfd, b, strlen(b), 0, (struct sockaddr *)&remaddr, addrlen) < 0) {
			perror("send to error");
		}
		send_xoff = true;
		send_xon = false;
	}

	return &queue->data[retId];
}

		/* q_get returns a pointer to the buffer where data is read or NULL if 

		* buffer is empty. 

		*/

static Byte *q_get(QTYPE *queue, Byte *data){
		 /* 
		 Insert code here. 

		 Retrieve data from buffer, save it to "current" and "data" 

		 If the number of characters in the receive buffer is below certain 

		 level, then send XON. 

		 Increment front index and check for wraparound. 

		 */

	if (queue->count == 0) {
		data = NULL;
		return NULL;
	}

	Byte *current;

	int maxLowerLimit = 3;

	int retId;
	bool valid = false;


	do {
		if (queue->count > 0) {
			(*data) = queue->data[queue->front];
			current = data;
			retId = queue->front;
			//increment queue->front
			if (queue->front >= RXQSIZE - 1) {
				queue->front = 0;
			} else {
				++queue->front;
			}
			--queue->count;
			globalCounterChild++;

			if (((*data) > 32 || (*data) == Endfile) && (*data) != CR && (*data) != LF) {
				valid = true;
			} 

			if (*data != Endfile)
				cout << "Mengkonsumsi byte ke-" << globalCounterChild << ":" << "\"" << (*data) << "\"" << endl;
		}
	} while (!valid);

	if (queue->count < maxLowerLimit && send_xoff) {
		char a[RXQSIZE];
		a[0] = XON;

		if(sendto(sockfd, a, strlen(a), 0, (struct sockaddr *)&remaddr, addrlen) < 0) {
			cout << "ERROR" << endl;
		}				
		send_xon = true;
		send_xoff = false;
		cout <<	"Buffer < maximum lowerlimit. Mengirim XON." << endl;
	}

	return &queue->data[retId];

}

char* findIPV4Address(){	
	struct ifaddrs *ifAddrStruct=NULL;
	struct ifaddrs *ifa=NULL;
	void *tmpAddrPtr=NULL;
	char *myaddress = new char[INET_ADDRSTRLEN];

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
	    if (!ifa->ifa_addr) {
	        continue;
	    }

	    if (ifa->ifa_addr->sa_family == AF_INET) { 
	    	// check whether is is IPV4 and it is valid IPV4 or not
	        tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
	        char addressBuffer[INET_ADDRSTRLEN];
	        inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
	        if (strcmp (addressBuffer,"127.0.0.1") == 0){
	        	//localhost IP
	        	continue;
	        }
	        else{
	        	//External host IP
		        for (int i = 0 ; i < INET_ADDRSTRLEN;i++)
		        	myaddress[i] = addressBuffer[i];
	        }	        
	    } 
	}

	if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
	
	return myaddress;
}
