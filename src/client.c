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

volatile sig_atomic_t
server_running = 1;

void send_command(const char *host, const char *cmd);
int is_numeric(const char *str);
void handle_shutdown_signal(int sig);
void cleanup_client(void);

void cleanup_client(void)
{
    if (strlen(server_ip) > 0) {
        send_command(server_ip, "pr/auto/stop");
    }

    if (sockfd != -1) {
        close(sockfd);
        sockfd = -1;
    }
}

int main(int argc, char *argv[])
{
   struct hostent *he;
   struct sigaction sa;
   sa.sa_handler = handle_shutdown_signal;
   sigemptyset(&sa.sa_mask);
   sa.sa_flags = 0;
   sigaction(SIGINT, &sa, NULL);
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

   printf("[Resolved Server IP: %s]\n", (char*)inet_ntoa(*((struct in_addr *)he->h_addr)));

   int choice;
   char line[128];
   while(server_running) {
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
        printf("Enter segment command (start/off/0-9/show/0-9): ");
        char seg_cmd[64];
        
        if(fgets(line, sizeof(line), stdin)!=NULL) {
            if(sscanf(line, "%s", seg_cmd) == 1) {
                int valid = 0;
                if (strcasecmp(seg_cmd, "start") == 0 || strcasecmp(seg_cmd, "off") == 0) {
                    valid = 1;
                } else if (is_numeric(seg_cmd) && strlen(seg_cmd) == 1) {
                    int val = atoi(seg_cmd);
                    if (val >= 0 && val <= 9) valid = 1;
                } else if (strncmp(seg_cmd, "show/", 5) == 0) {
                    char *val_str = seg_cmd + 5;
                    if (is_numeric(val_str) && strlen(val_str) == 1) {
                        int val = atoi(val_str);
                        if (val >= 0 && val <= 9) valid = 1;
                    }
                }

                if (!valid) {
                    printf("Invalid segment command! (Allowed: start, off, 0-9, show/0-9)\n");
                    continue;
                }

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
        printf("\nStopping auto control (if running) and quitting program...\n");
        server_running = 0;
        break;
    }
   }
   cleanup_client();
   return 0;
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

    int retries = 3;
    int delay_sec = 1;
    while (1) {
        if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == 0) {
            break;
        }
        perror("connect");
        close(sockfd);
        sockfd = -1;

        if (--retries <= 0) {
            fprintf(stderr, "Failed to connect to server after multiple attempts.\n");
            return;
        }
        printf("Connection failed. Retrying in %d seconds...\n", delay_sec);
        sleep(delay_sec);
        delay_sec *= 2;

        if((sockfd=socket(AF_INET, SOCK_STREAM, 0))==-1) {
            perror("socket");
            return;
        }
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

void handle_shutdown_signal(int sig) {
    (void)sig;
    server_running=0;
}