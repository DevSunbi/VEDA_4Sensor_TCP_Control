#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXDATASIZE 100
int sockfd = -1;
int main(int argc, char *argv[])
{
   int sockfd, numbytes; // 클라이언트 소켓 디스크립터, 받아올 데이터의 크기 선언 
   socklen_t addr_len; // 서버 구조체 크기
   char buf[MAXDATASIZE]; // 문자열 수신 버퍼
   struct hostent *he; // 호스트 정보 구조체
   struct sockaddr_in server_addr; // 서버 주소 구조체

   if(argc != 2) {
       fprintf(stderr, "usage : client hostname \n");
       exit(1);
   }
   if((he = gethostbyname(argv[1])) == NULL) {
       perror("gethostbyname");
       exit(1);
   }
   if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
       perror("socket");
       exit(1);
   }
   server_addr.sin_family = AF_INET; // IPv4 명시
   server_addr.sin_port = htons(60000); // 포트 번호 명시
   //htons는 네트워크 바이트 주소에서 호스트 바이트 주소로 변경
   server_addr.sin_addr = *((struct in_addr *)he->h_addr); // 32비트 IP 주소 명시
   printf("[ %s ]\n",(char*) inet_ntoa(server_addr.sin_addr)); // IP주소 출력
   //inet_ntoa는 32비트 IP주소를 10진수 표기법으로 변환
   memset(&(server_addr.sin_zero), '\0',8); // 서버 구조체 0으로 초기화 
   if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))== -1) { // 서버 연결 요청
       perror("connect");
       exit(1);
   }
   if((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) { // 서버로부터 메시지 수신
       perror("recv");
       exit(1);
   }
   buf[numbytes] = '\0'; // 문자열 끝 표시(EOF)
   printf("Received : %s\n", buf); // 수신한 메시지 출력
   close(sockfd); // 소켓 닫기
   return 0;
}

void sigint_handler(int signo) {
    
}