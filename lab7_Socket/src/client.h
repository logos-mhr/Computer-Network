#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <vector>
#include <regex>
#include <mutex>

#include <pthread.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/msg.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_MAX 256

struct messageStruct
{
    long type;
    char text[BUFFER_MAX];
};

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define NORMAL "\033[0m"

void connection_handle(int cfd);

class socketClient
{
private:
    int sockfd;
    sockaddr_in serverAddr;
    pthread_t connection_thread;
    int msqid;

    void help();
    void connect(const std::string& ip_addr, const int& port);

    static void* thread_handle(void* cfd)
    {
        connection_handle(*(int*)cfd);
        return nullptr;
    }

public:
    socketClient();
    ~socketClient();
    
    void run();
};

#endif