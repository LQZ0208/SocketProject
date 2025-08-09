/*
服务器I/O多路复用
准备阶段：初始化监听socket，创建文件描述符集合，将监听socket加入监视集合
等待阶段：调用select阻塞等待，直到有新连接请求或已连接客户端有数据可读
处理阶段：根据准备就绪的文件描述符类型分别处理：
1、如果是监听socket准备就绪：接受新客户端连接，将新客户端socket加入监视集合
2、如果是客户端socket准备就绪：接收客户端数据，处理后回显给客户端
循环执行：处理完后继续下一轮监视，实现并发处理多个客户端连接
这种设计使得服务器可以同时处理多个客户端连接，避免了传统阻塞I/O中必须等待一个客户端操作完成才能处理下一个客户端的问题。
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/select.h>  // 用于select系统调用

#define MAXLINE 4096
#define LISTENQ 1024    // 监听队列长度

int main()
{
    int listenfd, connectfd;
    struct sockaddr_in servaddr;
    char buff[MAXLINE];
    int n;
    fd_set read_fds, all_fds;  //当前读集合和所有文件描述符集合
    int max_fd, i;
    int client_fds[FD_SETSIZE]; //客户端文件描述符数组
    int maxIndex;

    //创建监听socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("[server] main failed! create socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(1);
    }

    //初始化服务器地址结构
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;  //设置地址族为IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //设置IP地址为任意地址（INADDR_ANY）
    servaddr.sin_port = htons(8000); //设置端口号为8000

    //绑定socket
    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
    {
        printf("[server] main failed! bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(1);
    }

    //开始监听连接
    if (listen(listenfd, LISTENQ) == -1)
    {
        printf("[server] main failed! listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(1);
    }

    //初始化客户端文件描述符数组
    for (i = 0; i < FD_SETSIZE; i++)
        client_fds[i] = -1;  //-1表示空闲位置
    
    maxIndex = -1;  //初始时没有客户端连接
    
    //初始化文件描述符集合
    FD_ZERO(&all_fds);
    FD_SET(listenfd, &all_fds);  //将监听socket加入集合（监听socket永不移除）
    max_fd = listenfd;  //当前最大文件描述符

    printf("======waiting for client request======\n");
    
    while (1)
    {
        read_fds = all_fds;  //每次循环都复制all_fds到read_fds
        
        //使用select等待任一文件描述符准备就绪
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("[server] main failed! select error");
            exit(1);
        }

        //检查是否有新连接请求
        if (FD_ISSET(listenfd, &read_fds))
        {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            
            //接受新连接
            if ((connectfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addr_len)) == -1)
            {
                perror("[server] main failed! accept connection error");
                continue;
            }
            
            printf("new client connected: %s:%d\n", 
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            
            //将新客户端加入client_fds数组
            for (i = 0; i < FD_SETSIZE; i++)
            {
                if (client_fds[i] < 0)
                {
                    client_fds[i] = connectfd;  //保存客户端文件描述符
                    if (i > maxIndex)
                        maxIndex = i;  //更新最大索引
                    break;
                }
            }
            
            //如果数组已满
            if (i == FD_SETSIZE)
            {
                printf("[server] main failed! too many clients\n");
                close(connectfd);
                continue;
            }
            
            //将新客户端加入监视集合
            FD_SET(connectfd, &all_fds);
            if (connectfd > max_fd)
                max_fd = connectfd;  //更新最大文件描述符
        }

        //检查现有客户端是否有数据
        for (i = 0; i <= maxIndex; i++)
        {
            if ((connectfd = client_fds[i]) < 0)
                continue;  //跳过无效的文件描述符
                
            if (FD_ISSET(connectfd, &read_fds))
            {
                //接收客户端数据
                if ((n = recv(connectfd, buff, MAXLINE, 0)) <= 0)
                {
                    //客户端关闭连接或发生错误
                    if (n == 0)
                    {
                        printf("client disconnected\n");
                    }
                    else
                    {
                        perror("[server] main failed! receive error");
                    }
                    
                    close(connectfd);
                    FD_CLR(connectfd, &all_fds);  //从监视集合中移除
                    client_fds[i] = -1;  //标记为空闲
                }
                else
                {
                    buff[n] = '\0';
                    printf("received message from client: %s", buff);
                    
                    //将消息回显给客户端
                    if (send(connectfd, buff, n, 0) < 0)
                    {
                        perror("[server] main failed! send error");
                    }
                }
            }
        }
    }
    
    close(listenfd);
    return 0;
}