#include "client.h"

std::mutex mt;

int main()
{
    std::ios::sync_with_stdio(false);
    socketClient client;
    client.run();
}

socketClient::socketClient()
{
    sockfd = -1;
    key_t msgkey = ftok("/",'a');
    msqid = msgget(msgkey, IPC_CREAT | 0666);
}

socketClient::~socketClient()
{
    close(sockfd);
}

void socketClient::run()
{
    help();
    while (true)
    {
        std::cout<<"> ";
        std::string line;
        getline(std::cin, line);
        // split
        std::regex whitespace("\\s+");
        std::vector<std::string> words(std::sregex_token_iterator(line.begin(), line.end(), whitespace, -1),
                                        std::sregex_token_iterator());
        if (words[0] == "")
        {
            continue;
        }
        else if (words[0] == "connect")
        {
            if (-1 != sockfd)
            {
                std::cout<<RED<<"Connected!\n"<<NORMAL;
            }
            else
            {
                connect(words[1], std::stoi(words[2]));
            }
        }
        else if (words[0] == "close")
        {
            if (-1 == sockfd)
            {
                std::cout<<RED<<"No connection.\n"<<NORMAL;
            }
            else
            {
                char buffer = 50;
                send(sockfd, &buffer, sizeof(buffer), 0);
                mt.lock();
                pthread_cancel(connection_thread);
                mt.unlock();
                close(sockfd);
                sockfd = -1;
                std::cout<<GREEN<<"Connection closed.\n"<<NORMAL;
            }
        }
        else if (words[0] == "gettime")
        {
            char buffer = 1;
            messageStruct timemsg;
            send(sockfd, &buffer, sizeof(buffer), 0);
            msgrcv(msqid, &timemsg, BUFFER_MAX, 11, 0);
            time_t time = atol(timemsg.text);
            std::cout<<YELLOW<<" Server time: "<<ctime(&time)<<NORMAL;
            
        }
        else if (words[0] == "getservername")
        {
            char buffer = 2;
            send(sockfd, &buffer, sizeof(buffer), 0);
            messageStruct namemsg;
            msgrcv(msqid, &namemsg, BUFFER_MAX, 12, 0);
            std::cout<<YELLOW<<"Server host name: "<<namemsg.text<<'\n'<<NORMAL;
        }
        else if (words[0] == "getclients")
        {
            char buffer = 3;
            send(sockfd, &buffer, sizeof(buffer), 0);
            messageStruct peermsg;
            msgrcv(msqid, &peermsg, BUFFER_MAX, 13, 0);
            
            char* ptr = peermsg.text;
            int count = 0, flag = 1;
            std::cout<<YELLOW;
            while (*ptr)
            {
                if ('^' == *ptr)
                {
                    std::cout<<' ';
                }
                else if ('$' == *ptr)
                {
                    std::cout<<'\n';
                    flag = 1;
                }
                else
                {
                    if (flag)
                    {
                        std::cout<<count++<<' ';
                        flag = 0;
                    }
                    std::cout<<(*ptr);
                }
                ptr++;
            }
            std::cout<<NORMAL;
        }
        else if (words[0] == "send")
        {
            char buffer[BUFFER_MAX] = {0};
            buffer[0] = 4;
            sprintf(buffer + strlen(buffer), "%s", words[1].c_str());
            sprintf(buffer + strlen(buffer), "^");
            sprintf(buffer + strlen(buffer), "%s", words[2].c_str());
            sprintf(buffer + strlen(buffer), "$");
            for (int i = 3; i < words.size(); ++i)
            {
                sprintf(buffer + strlen(buffer), "%s", words[i].c_str());
                if (i != words.size())
                {
                    sprintf(buffer + strlen(buffer), " ");
                }
                else
                {
                    sprintf(buffer + strlen(buffer), "\n");
                }
            }
            send(sockfd, buffer, BUFFER_MAX, 0);
            messageStruct statusmsg;
            msgrcv(msqid, &statusmsg, BUFFER_MAX, 14, 0);
            std::cout<<YELLOW<<statusmsg.text<<NORMAL;
        }
        else if (words[0] == "receive")
        {
            char buffer[BUFFER_MAX] = {0};
            buffer[0] = 5;
            sprintf(buffer + strlen(buffer), "receive");
            send(sockfd, buffer, BUFFER_MAX, 0);
        }
        else if (words[0] == "exit")
        {
            if (-1 != sockfd)
            {
                char buffer = 50;
                send(sockfd, &buffer, sizeof(buffer), 0);
                close(sockfd);
                std::cout<<GREEN<<"Socket "<<sockfd<<" closed.\n"<<NORMAL;
            }
            std::cout<<YELLOW<<"Bye.\n"<<NORMAL;
            exit(0);
        }
        else if (words[0] == "help")
        {
            help();
        }
        else
        {
            std::cout<<"Illegal input!\n";
        }
    }
}

void socketClient::connect(const std::string& ip_addr, const int& port)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip_addr.c_str());
    ::connect(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr));
    pthread_create(&connection_thread, nullptr, thread_handle, &sockfd);
}

void socketClient::help()
{
    std::cout<<"Please input a command:\n"
             <<"- connect [IP] [port]\n"
             <<"- close\n"
             <<"- gettime\n"
             <<"- getservername\n"
             <<"- getclients\n"
             <<"- send [IP] [port] [content]\n"
             <<"- exit\n"
             <<"- help\n";
}

void connection_handle(int cfd)
{
    char buffer[BUFFER_MAX];
    recv(cfd, buffer, BUFFER_MAX, 0);
    std::cout<<GREEN<<buffer<<NORMAL<<"> ";
    fflush(stdout);

    messageStruct msg;
    key_t key = ftok("/",'a');
    int msqid = msgget(key, IPC_CREAT | 0666);
    while (1)
    {
        memset(buffer, 0, BUFFER_MAX);
        recv(cfd, buffer, BUFFER_MAX, 0);
        if (20 == buffer[0])
        {
            std::cout<<buffer + 1<<'\n';
            continue;
        }
        msg.type = buffer[0];
        strcpy(msg.text, buffer + 1);
        msgsnd(msqid, &msg, BUFFER_MAX, 0);
    }
}