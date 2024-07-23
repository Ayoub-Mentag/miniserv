#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

fd_set fdall, fdread, fdwrite;
int fdMax = 0;

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void sendToAllClients(int fd, char *msg) {
	for (int i = 0; i <= fdMax; i++) {
		if (FD_ISSET(i, &fdall) && i != fd) {
			write(i, msg, strlen(msg));
		}
	}
}

void deleteClient(int fd) {
    FD_CLR(fd, &fdall);
	sendToAllClients(fd, "server: client just left\n");
}

int main() {
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(8081); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n");
	if (listen(sockfd, 10) != 0) {
		printf("cannot listen\n"); 
		exit(0); 
	}
	FD_ZERO(&fdall);
	FD_SET(sockfd, &fdall);
	fdMax = sockfd;
	while (1) {
		FD_ZERO(&fdread);
		FD_ZERO(&fdwrite);
		fdread = fdwrite = fdall;
		select(fdMax + 1, &fdread, &fdwrite, 0, 0);
		for (int i = 0; i <= fdMax; i++) {
			if (FD_ISSET(i, &fdread)) {
				if (i == sockfd) {
					len = sizeof(cli);
					connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
					if (connfd < 0) { 
						printf("server acccept failed...\n"); 
						exit(0); 
					} 
					else
						printf("server acccept the client...\n");
					FD_SET(connfd, &fdall);
					fdMax = connfd;
				}
				else {
					char buffer[100];
					bzero(buffer, 100);
					int r = read(i, buffer, 100);
					if (r <= 0) {
						deleteClient(i);
					}
				}
			}
		}
	}
}