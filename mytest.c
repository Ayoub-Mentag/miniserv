#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

int fds[10];
char *msgs[10];
char msg[42];
char buffer[100];
int fdMax, fdId = 0;
fd_set fdall, fdread, fdwrite;

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

void error_(char *msg)
{
    write(2, msg, strlen(msg));
    exit(1);
}

void sentoall(int fd, char *message)
{
    for (int i = 0; i <= fdMax ; i++)
    {
        if (FD_ISSET(i, &fdwrite) && i != fd)
            send(i, message, strlen(message), 0);
    }
}

void sendMsg(int fd)
{
    char *str;
    while (extract_message(&msgs[fd], &str))
    {
        sprintf(msg, "client %d: ", fds[fd]);
        sentoall(fd, (char *) &msg);
        sentoall(fd, str);
        free(str);
    }
}

void addClient(int fd)
{
    FD_SET(fd, &fdall);
    fds[fd] = fdId++;
    fdMax = fd > fdMax ? fd : fdMax;
    sprintf(msg, "server: client %d just arrived\n", fds[fd]);
    sentoall(fd, (char *) msg);
}

void deleteClient(int fd)
{
    sprintf(msg, "server: client %d just left\n", fds[fd]);
    sentoall(fd, (char *) msg);
    FD_CLR(fd, &fdall);
    free(msgs[fd]);
    msgs[fd] = NULL;
    close(fd);
}


int main(int argc, char **argv)
{
    int sockfd, connfd;
    unsigned int len;
	struct sockaddr_in servaddr, cli; 

    if (argc != 2) error_("Wrong number of arguments");

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) error_("Fatal error");
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		error_("Fatal error");

	if (listen(sockfd, 10) != 0) error_("Fatal error");
    
    FD_ZERO(&fdall);
    FD_SET(sockfd, &fdall);
    printf("server: listening on socket fd %d\n", sockfd);
    fdMax = sockfd;

    while (1)
    {
        fdwrite = fdread = fdall;
        if (select(fdMax + 1, &fdread, &fdwrite, NULL, NULL) < 0)
            error_("Fatal error");

        for (int i = 0; i <= fdMax; i++) {
            if (FD_ISSET(i, &fdread)) {
                if (i == sockfd) {
                    len = sizeof(cli);
                    connfd = accept(i ,(struct sockaddr *) &cli, &len);
                    if (connfd < 0) exit(0);
                    addClient(connfd);
                    break;
                } else {
                    int r = recv(i, &buffer, 99, 0);
                    if (r <= 0) {
                        deleteClient(i);
                        printf("client %d disconnected--------\n", fds[i]);
                    }
                    else {
                        buffer[r] = 0;
                        msgs[i] = str_join(msgs[i], (char *)&buffer);
                        sendMsg(i);
                    }
                    break;
                }
            }
        }
    }
}