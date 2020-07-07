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
#include <sys/select.h>

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
	serv_addr.sin_port = htons(6016);
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

	//select
	fd_set rdfs, rset;

	FD_ZERO(&rdfs);
	FD_SET(sockfd, &rdfs);

	int maxfd = sockfd;  //最大的fd，用于遍历的个数

	printf("init success.\n");
	while (1)
	{
		//rset保存原来的rset
		rset = rdfs;
		int nready = select(maxfd + 1, &rset, NULL, NULL, NULL);   //5---null为阻塞，
		if(nready < 0) {
			perror("select");
			continue;
		}
		//accept
		if(FD_ISSET(sockfd, &rset)){
			struct sockaddr_in clientaddr;
			int len = sizeof(struct sockaddr_in);
			int clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
			if(clientfd < 0) continue;

			char str[1024] = {0};
			printf("recvived from %s at port %d, sockfd:%d, clientfd:%d\n", inet_ntop(AF_INET, &clientaddr.sin_addr, str, sizeof(str)),
									ntohs(clientaddr.sin_port), sockfd, clientfd);
			FD_SET(clientfd, &rdfs);   //加入到select中
			if(clientfd > maxfd)maxfd = clientfd;
			printf("sockfd: %d, maxfd: %d\n", clientfd, maxfd);
			if(--nready == 0) continue;
		}

		int i = 0;
		//从listen 的sockfd + 1开始遍历，sockfd是accept的
		for(i = sockfd + 1;i <= maxfd; i++){
			if(FD_ISSET(i, &rset)){
				
				char buffer[1024] = {0};
				int ret = recv(i, buffer, 1024, 0);
				if(ret < 0){
					//如果是多线程将数据读走了，就会返回< 0,判断是否是多线程读走,多线程一定加此判断
					if(errno == EAGAIN || errno == EWOULDBLOCK){
						printf("read all data\n");
					}else{
						FD_CLR(i, &rdfs);
						close(i);
					}	
				}else if(ret == 0){  //收到对方的FIN
					printf("disconnect %d\n", i);
					FD_CLR(i, &rdfs);
					close(i);
				}else{
					printf("Recv : %s, len = %d\n", buffer, ret);
				}
				if(--nready == 0) break;
			}

		}
	}
	close(sockfd);
	return 0;
}
