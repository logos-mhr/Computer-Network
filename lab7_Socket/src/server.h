#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <cstring>
#include <vector>
#include <mutex>
#include <ctime>

#include <pthread.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_MAX 256

void connection_handle(int cfd);
void send_msg(int sock, const std::string& message);

typedef std::pair<std::string, int> ip_port;

class socketServer
{
private:
    int sockfd;
    sockaddr_in sin;
    
    const int connection_max = 16;

    static void* thread_handle(void* cfd)
    {
        connection_handle(*(int*)cfd);
        return nullptr;
    }
public:
    socketServer();
    ~socketServer();
    void run();
};

#endif