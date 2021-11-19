#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1000000
#define LISTENQ 16
void handle_socket(int fd);   
void load_file(int fd, char* buffer, int ret);
struct{
	char *ext;
	char *filetype;
} extension[] = {
	{"gif", "image/gif" },
	{"jpg", "image/jpeg"},
	{"jpeg","image/jpeg"},
	{"png", "image/png" },
	{"zip", "image/zip" },
	{"gz",  "image/gz"  },
    {"tar", "image/tar" },
    {"htm", "text/html" },
    {"html","text/html" },
    {"exe","text/plain" },
    {0,0}
};

int main(){
	int listenfd, connfd, length, pid;
	struct sockaddr_in servaddr, cliaddr;
	char buffer[BUFSIZE];
	signal(SIGCLD, SIG_IGN);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(8080);

	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);
	
	fprintf(stderr, "Waiting connect...\n");
	
	while(1){
		length = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &length);
		
		if((pid = fork()) < 0)
			exit(1);
		else{
			if(pid == 0){
				close(listenfd);
				handle_socket(connfd);
			}
			else
				close(connfd);
		}
	}
}

void handle_socket(int fd){
	char buffer[BUFSIZE];
	int ret, i, file_fd;
	char *ptr, *fstr;
	ret = read(fd, buffer, BUFSIZE);
	fstr = (char *)0;

	load_file(fd, buffer, ret);
	/* 字串處理 */
	if(ret > 0 && ret < BUFSIZE)
		buffer[ret] = 0;
	else
		buffer[0] = 0;
	for(i=0;i<ret;i++){
		if(buffer[i] == '\r' || buffer[i] == '\n')
			buffer[i] = 0;
	}
	//printf("%s\n", buffer);
	if(strncmp(buffer, "post ", 5) && strncmp(buffer, "POST ", 5) && strncmp(buffer, "get ", 4) && strncmp(buffer, "GET ", 4)){
		exit(1);
	}

	ptr = strstr(buffer, " HTTP/");
	*ptr = 0;
	ret = strlen(buffer);
	//printf("[%s]\n", buffer);
	//exit(1);
	for(i=0;extension[i].ext!=0;i++) {
        int len = strlen(extension[i].ext);
        if(!strncmp(&buffer[ret-len], extension[i].ext, len)) {
            fstr = extension[i].filetype;
            break;
        }
    }
	if(fstr == 0) {
        fstr = extension[i-1].filetype;
    }
	ptr = strchr(buffer, '/');
	ptr++;
	if((file_fd = open(&buffer[ptr-buffer], O_RDONLY)) == -1){
		write(fd, "Failed to open file.", 20);
	}
	sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
    write(fd, buffer, strlen(buffer));
	while ((ret = read(file_fd, buffer, BUFSIZE))>0) {
        write(fd, buffer, ret);
    }

    exit(1);

	return;
}

void load_file(int fd, char* buffer, int ret){
	printf("----------------------\n");
	printf("%s\n", buffer);
	printf("======================\n");
	return;
}
