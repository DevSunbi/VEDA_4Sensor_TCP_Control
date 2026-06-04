#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>

#define MAXDATASIZE 100
#define PORT 8000
int sockfd = -1;
char server_ip[128] = "";

void sigint_handler(int signo);
void send_command(const char *host, const char *cmd);
int is_numeric(const char *str);

int main(int argc, char *argv[])
{
   struct hostent *he;
   signal(SIGINT, sigint_handler);
   signal(SIGTSTP, SIG_IGN);
   signal(SIGQUIT, SIG_IGN);

   if(argc!=2) {
    fprintf(stderr, "usage : client hostname \n");
    exit(1);
   }

   strncpy(server_ip, argv[1], sizeof(server_ip) - 1);

   if((he=gethostbyname(argv[1]))==NULL) {
    perror("gethostbyname");
    exit(1);
   }

   printf("[Connected to Server IP: %s]\n", (char*)inet_ntoa(*((struct in_addr *)he->h_addr)));

   int choice;
   char line[128];
   while(1) {
    printf("\n1 LED  2 Buzzer  3 7-Segment  4 Photoresistor  5 Quit\n");
    printf("Select menu: ");

    if(fgets(line, sizeof(line), stdin)==NULL) {
        break;
    }
    if(sscanf(line, "%d", &choice) != 1) {
        printf("Invalid input. Plz enter a number between 1 and 5\n");
        continue;
    }

    if(choice == 1) {
        printf("Enter state ON/MID/OFF/Brightness value 0-1024: ");
        char state[64];
        if(fgets(line, sizeof(line), stdin) != NULL) {
            if(sscanf(line, "%s", state) == 1) {
                char cmd[128];

                if(strcasecmp(state, "on") == 0 || strcasecmp(state, "mid") == 0 || strcasecmp(state, "off") == 0) {
                    snprintf(cmd, sizeof(cmd), "led/%s", state);
                    send_command(argv[1], cmd);
                } else if(is_numeric(state)) {
                    int val = atoi(state);

                    if(val >= 0 && val <= 1024) {
                        snprintf(cmd, sizeof(cmd), "led/brightness?value=%d", val);
                        send_command(argv[1], cmd);
                    } else {
                        printf("Invalid brightness value. Must be between 0 and 1024\n");
                    }
                } else {
                    printf("Invalid LED State or Brightness value.\n");
                }
                 
            }
        }
    }
    else if(choice == 2) {
        printf("Enter Buzzer note (do, re, mi, fa, sol, la, si, do2, off) : ");
        char note[64];
        
        if(fgets(line, sizeof(line), stdin) != NULL) {
            if(sscanf(line, "%s", note) == 1) {
                char cmd[128];
                snprintf(cmd, sizeof(cmd), "buzz/%s", note);
                send_command(argv[1], cmd);
            }
        }
    }
    else if(choice == 3) {
        printf("Enter segment command (start/off): ");
        char seg_cmd[64];
        
        if(fgets(line, sizeof(line), stdin)!=NULL) {
            if(sscanf(line, "%s", seg_cmd) == 1) {
                char cmd[128];
                snprintf(cmd, sizeof(cmd), "segment/%s", seg_cmd);

                send_command(argv[1], cmd);
            }
        }
    }
    else if(choice == 4) {
        printf("1 Read Analog(ADC) 2 Read Digital 3 Start Auto LED Control 4 Stop Auto LED Control 5 Go Back\n");
        printf("Select Option: ");

        int opt;
        if(fgets(line, sizeof(line), stdin) != NULL) {
            if(sscanf(line, "%d", &opt) == 1) {
                if(opt == 1) {
                    send_command(argv[1], "pr");
                } else if(opt == 2) {
                    send_command(argv[1], "pr/digital");
                } else if(opt == 3) {
                    send_command(argv[1], "pr/auto/start");
                } else if(opt == 4) {
                    send_command(argv[1], "pr/auto/stop");
                } else if(opt == 5) {
                    // Go back to main menu
                    printf("Going back to main menu...\n");
                } else {
                    printf("Invalid option.\n");
                }
            }
        }
    }
    else if (choice == 5) {
        printf("Stopping auto control (if running) and quitting program...\n");
        send_command(server_ip, "pr/auto/stop");
        break;
    }
   }
   return 0;
}

void sigint_handler(int signo) {
    (void)signo;
    printf("\n Ctrl+C Detected : Stopping auto control and exiting...\n");
    if (strlen(server_ip) > 0) {
        send_command(server_ip, "pr/auto/stop");
    }
    if (sockfd != -1) {
        close(sockfd);
        printf("[Resource Cleanup] Closed active socket descriptor: %d\n", sockfd);
        sockfd = -1;
    }
    exit(0);
}

void send_command(const char *host, const char *cmd) {
    struct hostent *he;
    struct sockaddr_in server_addr;
    char req[512];
    char buf[4086];


    if((he = gethostbyname(host)) == NULL) {
        perror("gethostbyname");
        return;
    }

        if((sockfd=socket(AF_INET, SOCK_STREAM, 0))==-1) {
            perror("socket");
            return;
        }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr = *((struct in_addr *) he -> h_addr);

    memset(&(server_addr.sin_zero), '\0', 8);

    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect");
        close(sockfd);
        sockfd = -1;
        return;
    }

    snprintf(req, sizeof(req), 
            "GET /%s HTTP/1.1\r\n"
            "HOST: %s\r\n"
            "Connection: close\r\n\r\n", cmd, host);
    
    if(send(sockfd, req, strlen(req), 0) == -1) {
        perror("send");
        close(sockfd);
        sockfd = -1;
        return;
    }
    int total_bytes = 0;
    int read_bytes;

    while((read_bytes = recv(sockfd, buf+total_bytes, sizeof(buf) - total_bytes - 1, 0)) > 0) {
        total_bytes += read_bytes;
        if(total_bytes >= (int)sizeof(buf) - 1) {
            break;
        }
    }

    if(read_bytes == -1) {
        perror("recv");
        close(sockfd);
        sockfd = -1;
        return;
    }

    buf[total_bytes] = '\0';
    char *body = strstr(buf, "\r\n\r\n");
    if(body!=NULL) {
        printf("[Server Response]: %s\n", body+4);
    } else {
        printf("[Server Response]: %s\n", buf);
    }
    close(sockfd);
    sockfd = -1;
}

int is_numeric(const char *str) {
    if(str == NULL || *str == '\0') return 0;

    for(int i = 0; str[i] != '\0'; i++) {
        if(!isdigit((unsigned char)str[i])) return 0;
    }
    return 1;
}