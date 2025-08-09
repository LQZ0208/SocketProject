#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include <unistd.h>

#define DEFAULT_PORT 8000
#define MAXLINE 4096

int main(int argc, char** argv)
{
	int    socket_fd, connect_fd;
	struct sockaddr_in  servaddr;
	char   sendline[4096];
	char   buff[4096];
	int    n;

	//初始化Socket
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	//初始化
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  //IP地址设置成INADDR_ANY,让系统自动获取本机的IP地址。
	servaddr.sin_port = htons(DEFAULT_PORT);       //设置的端口为DEFAULT_PORT

	//将本地地址绑定到所创建的套接字上
	if (bind(socket_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
	{
		printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	//开始监听是否有客户端连接
	if (listen(socket_fd, 10) == -1)
	{
		printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
		exit(0);
	}

	printf("waiting for client's request...\n");

	//多个客户端与服务器建立连接，发送消息
	/*
	while(1)
	{
		//阻塞直到有客户端连接，不然多浪费CPU资源。
		if((connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1)
		{
			printf("accept socket error: %s(errno: %d)",strerror(errno),errno);
			continue;
		}

		//接受客户端传过来的数据
		n = recv(connect_fd, buff, MAXLINE, 0);

		buff[n] = '\0';
		printf("recv msg from client: %s\n", buff);

	}
	*/

	//单个客户端与服务器建立连接，发送消息并回复消息，多次通信

	if ((connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL)) == -1) //阻塞直到有客户端连接，不然多浪费CPU资源。
	{
		printf("accept socket error: %s(errno: %d)", strerror(errno), errno);
	}
	else
	{
		printf("client connected!\n");
	}

	while (1)
	{
		//接受客户端传过来的数据
		n = recv(connect_fd, buff, MAXLINE, 0);
		buff[n] = '\0';
		printf("recv msg from client: %s", buff);

		//给客户端发送消息
		printf("send msg to client:");
		fgets(sendline, 4096, stdin);
		if (send(connect_fd, sendline, strlen(sendline), 0) < 0)
		{
			printf("send msg error: %s(errno: %d)\n", strerror(errno), errno);
			exit(0);
		}

	}


	close(connect_fd);
	close(socket_fd);
}
