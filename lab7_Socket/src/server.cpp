#include "server.h"

std::vector<std::pair<int, ip_port>> clientList;
std::mutex mt;

int main()
{
    std::ios::sync_with_stdio(false);
    socketServer server;
    server.run();
}

socketServer::socketServer()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(4848); // magic number: my student ID
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sockfd, (sockaddr*)&sin, sizeof(sin));
    listen(sockfd, connection_max);
}

socketServer::~socketServer()
{
    close(sockfd);
}

void socketServer::run()
{
    std::cout<<"Listening\n";
    while (true)
    {
        sockaddr_in client;
        unsigned int clientAddrLength = sizeof(client);
        int connection_fd = accept(sockfd, (sockaddr*)&client, (socklen_t*)&clientAddrLength);
        clientList.push_back(std::pair<int, ip_port>(connection_fd, ip_port(inet_ntoa(client.sin_addr), ntohs(client.sin_port))));
        std::cout<<inet_ntoa(client.sin_addr)<<":"<<ntohs(client.sin_port)<<" connected.\n";
        pthread_t connection_thread;
        pthread_create(&connection_thread, nullptr, thread_handle, &connection_fd);
    }
}

void send_msg(int sock, const std::string& message)
{
    char msg[BUFFER_MAX] = {0};
    msg[0] = 20;
    sprintf(msg + 1, "%s", message.c_str());
    send(sock, msg, strlen(msg), 0);
}

void connection_handle(int cfd)
{
    char hello[] = "hello\n";
    send(cfd, hello, strlen(hello), 0);

    char buffer_recv[BUFFER_MAX] = {0};
    char buffer_send[BUFFER_MAX] = {0};
    while (true)
    {
        recv(cfd, buffer_recv, BUFFER_MAX, 0);
        memset(buffer_send, 0, BUFFER_MAX);
        mt.lock();
        switch (buffer_recv[0])
        {
        case 0:
            {
                for (auto it = clientList.begin(); it != clientList.end(); )
                {
                    if ((*it).first == cfd)
                    {
                        it = clientList.erase(it);
                        break;
                    }
                    else
                    {
                        ++it;
                    }
                }
                break;
            }
        case 1:
            {
                time_t t;
                time(&t);
                std::cout<<cfd<<" gettime: "<<t<<'\n';
                buffer_send[0] = 11;
                sprintf(buffer_send + strlen(buffer_send), "%ld", t);
                send(cfd, buffer_send, strlen(buffer_send), 0);
                break;
            }
        case 2:
            {
                std::cout<<cfd<<" getname"<<'\n';
                buffer_send[0] = 12;
                gethostname(buffer_send + strlen(buffer_send), sizeof(buffer_send) - sizeof(char));
                send(cfd, buffer_send, strlen(buffer_send), 0);
                break;
            }
        case 3:
            {
                std::cout<<cfd<<" getclients"<<'\n';
                buffer_send[0] = 13;
                for (auto& it: clientList)
                {
                    sprintf(buffer_send + strlen(buffer_send), "%s", it.second.first.c_str());
                    sprintf(buffer_send + strlen(buffer_send), "^");
                    sprintf(buffer_send + strlen(buffer_send), "%d", it.second.second);
                    sprintf(buffer_send + strlen(buffer_send), "$");
                }
                send(cfd, buffer_send, strlen(buffer_send), 0);
                break;
            }
        case 4:
            {
                std::string msg(buffer_recv + 1);
                size_t pos0 = msg.find("^"), pos1 = msg.find("$");
                std::string ip_addr = msg.substr(0, pos0);
                int port = atoi(msg.substr((pos0 + 1), pos1 - pos0 - 1).c_str());
                std::string content = msg.substr(pos1 + 1);
                int sock = -1;
                for (auto it = clientList.begin(); it != clientList.end(); ++it)
                {
                    if (it->second.first == ip_addr && it->second.second == port)
                    {
                        sock = it->first;
                        break;
                    }
                }
                std::cout<<cfd<<" send message to "<<ip_addr<<":"<<port<<'\n';
                buffer_send[0] = 14;
                if (-1 == sock)
                {
                    sprintf(buffer_send + 1, "Fail.\n");
                }
                else
                {
                    send_msg(sock, content);
                    sprintf(buffer_send + 1, "Success.\n");
                }
                send(cfd, buffer_send, strlen(buffer_send), 0);
                break;
            }
        case 5:
            {
                break;
            }
        }
        memset(buffer_recv, 0, BUFFER_MAX);
        mt.unlock();
    }
}