#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include <netinet/in.h> // struct sockaddr_in, htons()

#define MAX_PACKET  2048

#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT1    5000
#define SERVER_PORT2    5001

#define MAX_BACKLOG     2

#define MAX_POLL        MAX_BACKLOG + 1
#define TIMEOUT_POLL    3 * 1000

#define TCP             1
#define UDP             2

typedef union _sendInfo_t
{
    int fd;
    struct sockaddr_in addr;
} sendInfo_t;

typedef struct _packet_t
{
    int tcpUdp;
    sendInfo_t info;
    int length;
    char data[MAX_PACKET];
} packet_t;

int CheckEmptyPoll(struct pollfd* polls, int pollSize);
int AcceptClient(struct pollfd* polls, int pollSize);
int ReceivePacket(int fd, packet_t* packet);
int RemoveClient(struct pollfd* poll);

#endif // _TCPSERVER_H_