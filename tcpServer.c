#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // close()
#include <sys/socket.h> // socket()
#include <netinet/in.h> // struct sockaddr_in, htons()
#include <arpa/inet.h> // inet_addr()
#include <poll.h> // poll(), struct pollfd

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

int TcpServer()
{
    // 반환값 사용을 위한 변수들
    int res = 0;
    int pollRe = 0;

    // 수신받은 데이터를 저장할 변수
    packet_t packet;

    // 서버 정보를 저장하는 변수
    int serverFd = 0;
    struct sockaddr_in server;
    int serverOpt = 1;

    // 논블로킹을 위한 poll 객체
    struct pollfd polls[MAX_POLL];

    // 초기화
    memset(&packet, 0, sizeof(packet));
    memset(&server, 0, sizeof(server));

    // 소캣 생성
    serverFd = socket(PF_INET, SOCK_STREAM, 0);
    if(serverFd < 0)
    {
        perror("socket error");
        exit(1);
    }

    // 소캣에 바인딩할 포트의 재사용을 위한 옵션 설정
    res = setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &serverOpt, sizeof(serverOpt));
    if(res < 0)
    {
        perror("setsockopt error");
        exit(1);
    }
    
    // 소캣 정보 설정
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_port = htons(SERVER_PORT1);

    // 바인딩
    res = bind(serverFd, (struct sockaddr*)&server, sizeof(server));
    if(res < 0)
    {
        perror("bind error");
        exit(1);
    }

    // 소캣 listen 상태 설정
    res = listen(serverFd, MAX_BACKLOG);
    if(res < 0)
    {
        perror("listen error");
        exit(1);
    }

    // poll 초기 설정
    polls[0].fd = serverFd;
    polls[0].events = POLLIN;

    for(int i = 1; i < MAX_POLL; i++)
    {
        polls[i].fd = -1;
        polls[i].events = POLLIN | POLLERR | POLLHUP;
    }

    printf("TCP server start!\r\n");

    while (1)
    {
        // 이벤트 발생 확인
        pollRe = poll(polls, MAX_POLL, TIMEOUT_POLL);
        if(pollRe > 0)
        {
            if(polls[0].revents & POLLIN) // 연결 요청이 있는지 확인
            {
                // 동일 IP의 중복 연결에 대한 방안 고민
                res = AcceptClient(polls, MAX_POLL); // 연결 요청 수락
                if(res > 0)
                {
                    printf("accept Client at [%d]", res);
                    pollRe--;
                }
            }
            
            for(int i = 1; i < MAX_POLL; i++) // 클라이언트에서 송신한 데이터가 있는지 확인 및 수신
            {
                // 이렇게 하는 게 좋을까?
                // 데이터 수신 이벤트가 발생하지 않았을 때 반복문을 수행하지 않기 위해서 만들어둠
                if(pollRe == 0)
                    break;

                if(polls[i].revents & POLLIN)
                {
                    res = ReceivePacket(&polls[i], &packet);
                    if(res > 0)
                    {
                        printf("receive data from [%d]client\r\n", i);
                        pollRe--;

                        // 큐에 데이터 전달
                    }
                }
                else if(polls[i].revents & POLLHUP)
                {
                    printf("%d client disconnected\r\n", i);
                    RemoveClient(&polls[i]);
                }
                else
                {
                    printf("%d client error\r\n");
                    RemoveClient(&polls[i]);
                }
            }
        }

        sleep(1);
    }
    

    return 1;
}

/**
 * @brief poll 객체를 확인하여 클라이언트를 더 받을 수 있는지 확인
 * 
 * @param polls 확인할 poll 객체 포인터
 * @param pollSize 확인할 poll 객체의 크기
 * @return int 성공: poll에 비어있는 번호를 반환, 실패: -1 반환
 */
int CheckEmptyPoll(struct pollfd* polls, int pollSize)
{
    int empty = -1;

    for(int i = 1; i < pollSize; i++)
    {
        if(polls[i].fd == -1)
        {
            empty = i;
            break;
        }
    }
    
    return empty;
}

/**
 * @brief 클라이언트의 요청을 수락할 수 있는지 확인하고 해당 클라이언트와 서버를 연결
 * 
 * @param polls 요청 수락 여부 결정과 수락에 사용될 poll 객체 포인터
 * @param pollSize 요청 수락 여부 결정에 사용될 poll 객체의 크기
 * @return int 성공: 수락한 클라이언트와 통신할 소캣의 fd, 실패: -1
 */
int AcceptClient(struct pollfd* polls, int pollSize)
{
    int res = 0;
    int acc = -1;

    struct sockaddr_in client;
    socklen_t clientLen = sizeof(client);

    memset(&client, 0, sizeof(client));

    res = CheckEmptyPoll(polls, pollSize);

    acc = accept(polls[0].fd, (struct sockaddr*)&client, &clientLen);
    if(acc < 0)
    {
        perror("accept error");
    }

    if(res != -1)
    {
        polls[res].fd = acc;
    }
    else
    {
        // 더이상 받을 클라이언트가 없다면 연결 후 바로 연결 해제
        // 큐 관리를 위함 (정말 이 방법이 좋은 방법일까? 그냥 놔둬도 되지 않을까?)
        shutdown(acc, SHUT_RDWR);
        close(acc);
        acc = -1;
    }

    return acc;
}

/**
 * @brief 클라이언트가 송신한 데이터를 수신하고 수신한 데이터의 정보를 저장
 * 
 * @param poll 데이터를 송신한 클라이언트의 소캣 fd
 * @param packet 클라이언트가 송신한 데이터와 메타 데이터를 저장할 객체
 * @return int 수신한 데이터의 크기 
 */
int ReceivePacket(int fd, packet_t* packet)
{
    int res = 0;

    // 고정된 값이라 생각되어서 sizeof를 사용하지 않고 MAX_PACKET를 사용함
    res = recv(fd, packet->data, MAX_PACKET, 0);
    if(res <= 0)
    {
        perror("rece error");
    }
    else
    {
        packet->tcpUdp = TCP;
        packet->info.fd = fd;
        packet->length = res;
    }

    return res;
}
/**
 * @brief 연결 해제 또는 오류가 발생한 클라이언트의 소캣을 정리하고 poll 객체를 초기화
 * 
 * @param poll 연결이 해제된 클라이언트의 소캣 fd를 갖은 poll 객체 포인터
 * @return int 0
 */
int RemoveClient(struct pollfd* poll)
{
    int res = 0;
    int fd = poll->fd;
    
    shutdown(fd, SHUT_RDWR); // 스트림 끊음
    close(fd); // 소켓 정리

    poll->fd = -1;

    return res;
}
