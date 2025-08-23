/*
客户端IO多路复用
准备阶段：清空并重新设置要监视的文件描述符集合
等待阶段：调用select阻塞等待，直到标准输入或socket准备就绪
处理阶段：根据准备就绪的文件描述符类型分别处理：
1、如果是socket准备就绪：接收并显示服务器消息
2、如果是标准输入准备就绪：读取并发送用户输入到服务器
循环执行：处理完后继续下一轮监视，实现并发I/O处理
这种设计使得客户端可以同时处理用户输入和服务器响应，避免了传统阻塞I/O中必须等待一个操作完成才能进行下一个操作的问题。
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
#include<sys/select.h>

#define MAXLINE 4096

int main()
{
    int sockfd;
    char recvline[MAXLINE], sendline[MAXLINE];
    struct sockaddr_in servaddr;
    fd_set read_fds; //声明一个文件描述符集合
    int max_fd;	//最大文件描述符值

    //初始化服务器地址结构
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;	//设置地址族为IPv4
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //设置IP地址为任意地址（INADDR_ANY）
    servaddr.sin_port = htons(8000); //设置端口号为8000
    
    //创建socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("[client] main failed! create socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(1);
    }

    //连接服务器
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        printf("[client] main failed! connect error: %s(errno: %d)\n", strerror(errno), errno);
        exit(1);
    }

    while (1)
    {
        //清空文件描述符集合，确保开始时集合为空，避免之前可能残留的数据影响当前操作
        FD_ZERO(&read_fds);
        
        //将标准输入(STDIN_FILENO)加入监视集合，告诉select需要监视标准输入是否有数据可读
        FD_SET(STDIN_FILENO, &read_fds);
        
        //将socket加入监视集合，告诉select需要监视socket是否有数据可读
        FD_SET(sockfd, &read_fds);
        
        //确定最大文件描述符值，为select系统调用提供正确的参数，确保检查范围覆盖所有监视的文件描述符
        max_fd = (STDIN_FILENO > sockfd) ? STDIN_FILENO : sockfd;

        //使用select等待任一文件描述符准备就绪，返回值为已就绪的文件描述符数量
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
        {
            perror("[client] main failed! select error");
            exit(1);
        }

        //检查是否有来自服务器的数据
        if (FD_ISSET(sockfd, &read_fds))
        {
            int recv_len;
            if ((recv_len = recv(sockfd, recvline, MAXLINE-1, 0)) < 0) //返回值：实际接收的字节数，出错返回-1，连接关闭返回0
            {
                perror("[client] main failed! recv error");
                exit(1);
            }
            else
            {
                recvline[recv_len] = '\0';
                printf("received message from server: %s", recvline);
            }
        }

        //检查是否有来自标准输入的数据
        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            if (fgets(sendline, MAXLINE, stdin) != NULL)
            {
                int send_len;
                if ((send_len = send(sockfd, sendline, strlen(sendline), 0)) < 0)
                {
                    printf("[client] main failed! send message error: %s(errno: %d)\n", strerror(errno), errno);
                    exit(1);
                }
            }
        }
    }
    
    close(sockfd);
    exit(0);
}