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
	if(strncmp(buffer, "post ", 5) && strncmp(buffer, "POST ", 5) && strncmp(buffer, "get ", 4) && strncmp(buffer, "GET ", 4)){
		exit(1);
	}

	ptr = strstr(buffer, " HTTP/");
	*ptr = 0;
	ret = strlen(buffer);
	
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
	char boundary[BUFSIZE], data[BUFSIZE], name[BUFSIZE];
	char *ptr, *qtr;
	int boundary_len = 0, data_len = 0, name_len;
	printf("========start=========\n");
	printf("%s\n", buffer);                                                                                                              
    printf("=========end==========\n");

	if((ptr = strstr(buffer, "boundary=")) == 0)
         return;
	
	ptr += 9;
	while(*ptr != '\r' && *ptr != '\n'){
		boundary[boundary_len ++] = *ptr;
		ptr ++;
	}
	boundary[boundary_len] = '\0';

	if((ptr = strstr(buffer, "filename=\"")) == 0)
		return;
	ptr += 10;
	strcpy(name, "load_file/");
	name_len = strlen(name);
	while(*ptr != '\"'){
		name[name_len ++] = *ptr;
		ptr ++;
	}
	name[name_len] = '\0';

	ptr = strstr(ptr , "\n");
	ptr = strstr(++ptr , "\n");
	ptr = strstr(++ptr , "\n");
	ptr++;
	
	qtr = strstr(ptr, boundary);
	qtr -= 4;
	*qtr = '\0';
	
	int file_fd = open(name ,O_CREAT | O_WRONLY | O_TRUNC | O_SYNC , S_IRWXO | S_IRWXU | S_IRWXG);
	write(file_fd ,ptr ,strlen(ptr));
	close(file_fd);

	return;
}
