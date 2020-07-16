
#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include <winsock2.h>


#define DEFAULT_PORT 8080

char *name;
	int sockfd;

	int new_sockfd;


	struct sockaddr_in reader_addr; 

	struct sockaddr_in writer_addr;
	int writer_len = sizeof(writer_addr);
	
	int port = DEFAULT_PORT;
int main(int argc, char* argv[]){

	int number;

	char buf[0x400];

	WORD versionWanted = MAKEWORD(1, 1);
	WSADATA wsaData;
	WSAStartup(versionWanted, &wsaData);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0){

		printf("socket error\n");
		printf("sockfd : 0x%X\n", sockfd);

		perror("socket");
		printf("0x%X\n", errno);

		while(1){}
		return 0;
	}

	DWORD timeout = 5000;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

	memset(&reader_addr, 0, sizeof(reader_addr));

	reader_addr.sin_family = AF_INET;

	reader_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//reader_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
	
	
	
	if(argc > 1)
		port = atoi(argv[1]);
	else
		port = DEFAULT_PORT;
	printf("setting port to: %i\n", port);
	reader_addr.sin_port = htons(port);

	if(bind(sockfd, (struct sockaddr *)&reader_addr, sizeof(reader_addr)) < 0){

		printf("bind error\n");

		perror("bind");
		while(1){}
		return 0;
	}

	if(listen(sockfd, 128) < 0){

		close(sockfd);
		printf("listen error\n");

		perror("listen");

		while(1){}
		return 0;
	}

	while(1){
		new_sockfd = accept(sockfd,(struct sockaddr *)&writer_addr, &writer_len);

		if(new_sockfd < 0){
			break;
		}

		int received = 0;

		do {
			memset(buf, 0x00, sizeof(buf));
			received = recv(new_sockfd, buf, sizeof(buf) - 1, 0);
			printf("%s", buf);
		} while (received > 0);

		closesocket(new_sockfd);
	}

	close(sockfd);

	WSACleanup();

	return 0;
}