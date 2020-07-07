
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int sockfd;

void do_sigio(int signo)
{
	char buffer[256];
	struct sockaddr_in cliaddr;

	int client_len = sizeof(struct sockaddr_in);

	int len = recvfrom(sockfd, buffer, 256, 0, (struct sockaddr *)&cliaddr, (socklen_t *)&client_len);
	printf("Listen Message : %s\r\n", buffer);
	int slen = sendto(sockfd, buffer, len, 0, (struct sockaddr *)&cliaddr,  client_len);

}
int main(int argc, const char *argv[])
{
	struct sigaction sigio_action;
	
	sigio_action.sa_flags = 0;
	sigio_action.sa_handler = do_sigio;

	sigaction(SIGIO, &sigio_action, NULL);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
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
	fcntl(sockfd, F_SETOWN, getpid());

	int flags = fcntl(sockfd, F_GETFL, 0);
	flags |= O_ASYNC | O_NONBLOCK;
	fcntl(sockfd, F_SETFL, flags);

	while(1){
		sleep(1);
	}
	return 0;
}
