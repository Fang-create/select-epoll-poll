#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>

#define EPOLL_SIZE 1024

int main(int argc, const char *argv[])
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("socket");
		return -1;
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(8888);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	int ret = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if(ret != 0){
		perror("bind");
		return -1;
	}
	

	if(listen(sockfd, 5) < 0){
		perror("listen");
		return -1;
	}
	//
	int epfd = epoll_create(1);
	struct epoll_event ev, events[EPOLL_SIZE] = {0};

	ev.events = EPOLLIN;
	ev.data.fd = sockfd;
	epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);  //添加到epoll中

	int i = 0;
	while(1){
		int nready = epoll_wait(epfd, events, EPOLL_SIZE, -1);
		if(nready == -1){
			perror("epoll_wait");
			break;
		}
		for(i = 0;i < nready; i++){
			if(events[i].data.fd == sockfd){
				struct sockaddr_in clientaddr;
				int len = sizeof(struct sockaddr_in);
				int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
				if(clientfd <= 0) continue;

				char str[1024] = {0};
				printf("recvived from %s at port %d, clientfd:%d\n", inet_ntop(AF_INET, &clientaddr.sin_addr, str, sizeof(str)),
									ntohs(clientaddr.sin_port), clientfd);
				//设置可读出触发和边沿触发
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = clientfd;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);   //加入到select中
			}else{
				int clientfd = events[i].data.fd;

				char buffer[1024] = {0};
				int ret = recv(clientfd, buffer, 1024, 0);
				if(ret < 0){
					//如果是多线程将数据读走了，就会返回< 0,判断是否是多线程读走,多线程一定加此判断
					if(errno == EAGAIN || errno == EWOULDBLOCK){
						printf("read all data\n");
					}
					
					close(clientfd);

					ev.events = EPOLLIN | EPOLLET;
					ev.data.fd = clientfd;
					epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);
				}else if(ret == 0){  //收到对方的FIN
					printf("disconnect %d\n", i);
					close(clientfd);
					ev.events = EPOLLIN | EPOLLET;
					ev.data.fd = clientfd;
					epoll_ctl(epfd, EPOLL_CTL_DEL, clientfd, &ev);
				}else{
					printf("Recv : %s, len = %d\n", buffer, ret);
					send(clientfd, buffer, strlen(buffer), 0);
				}
			}
		}
	}
	printf("exit main()\n");
	close(sockfd);
	return 0;
}
