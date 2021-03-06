/*
* File : dcomm.h
*/
#ifndef _DCOMM_H_
#define _DCOMM_H_
/* ASCII Const */
#define SOH 1       /* Start of Header Character */
#define STX 2       /* Start of Text Character */
#define ETX 3       /* End of Text Character */
#define ENQ 5       /* Enquiry Character */
#define ACK 6       /* Acknowledgement */
#define BEL 7       /* Message Error Warning */
#define CR 13       /* Carriage Return */
#define LF 10       /* Line Feed */
#define NAK 21      /* Negative Acknowledgement */
#define Endfile 26  /* End of file character */
#define ESC 27      /* ESC key */

/* Const */
#define BYTESIZE 256    /* The maximum value of a byte */
#define MAXLEN 1024     /* Maximum messages length */

#define WINDOW_SIZE 5
#define BUFFER_SIZE (WINDOW_SIZE * 2)
#define ACK_TIMEOUT 3

typedef unsigned char Byte;

typedef struct QTYPE
{
    unsigned int count;
    unsigned int front;
    unsigned int rear;
    unsigned int maxsize;
    Byte *data;
} QTYPE;

typedef struct MESGB
{
    unsigned int soh;
    unsigned int stx;
    unsigned int etx;
    Byte checksum;
    Byte msgno;
    Byte *data;
} MESGB;
#endif