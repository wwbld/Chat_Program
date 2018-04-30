#include "network.h"

fd_set fdvar;
int amount = 0;

struct FLAG1 *createFlag1(char *handle) {
   uint16_t packet_len = 4;
   uint8_t handle_len = strlen(handle);
   struct FLAG1 *send = (struct FLAG1*)
                 calloc(sizeof(struct FLAG1), 1);

   send->flag = 1;
   send->handle_len = handle_len;
   send = (struct FLAG1*)realloc(send, 4+handle_len);
   memcpy(((char *)send)+4, handle, handle_len);
   send->packet_len = htons(packet_len+handle_len);

   return send;
}

int myclientSetup(char *handle, char *host_name, char *port) {
   int socket_num, len1, len2;
   char buf[MAXBUF];
   struct sockaddr_in remote;
   struct hostent *hp;
   struct FLAG1 *sendPacket = NULL;
   struct header *recvPacket = NULL;

   if ((socket_num = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("socket failed.\n");
      exit(-1);
   }

   remote.sin_family = AF_INET;

   if ((hp = gethostbyname(host_name)) == NULL) {
      printf("Error getting hostname: %s\n", host_name);
      exit(-1);
   }
   memcpy((char*)&remote.sin_addr, (char*)hp->h_addr, hp->h_length);
   remote.sin_port = htons(atoi(port));
   if (connect(socket_num, (struct sockaddr*)&remote, 
                           sizeof(struct sockaddr_in)) < 0) {
      perror("connect failed.\n");
      exit(-1);
   }

   sendPacket = createFlag1(handle); 
   if ((len1 = send(socket_num, 
               (char *)sendPacket, 
               ntohs(sendPacket->packet_len), 0)) < 0) {
      perror("send failed.\n");
      exit(-1);
   }

   if ((len2 = recv(socket_num, buf, BUFSIZE, 0)) < 0) {
      perror("recv failed.\n");
      exit(-1);
   }

   recvPacket = (struct header *)buf;
   if (recvPacket->flag == 2) {
   } else if (recvPacket->flag == 3) {
      printf("Handle already in use: %s\n", handle);
      exit(-1);
   } else {
      perror("Unknown reply, something wrong.\n");
      exit(-1);
   }

   printf("$:");
   fflush(stdout);

   return socket_num;
}

void sendListRequest(int socket_num) {
   int len = 0;
   struct header *sendPacket = (struct header *)
                         malloc(sizeof(struct header));
   sendPacket->packet_len = htons(3);
   sendPacket->flag = 10;
   len = send(socket_num, sendPacket, 3, 0);
   if (len != 3) {
      perror("send failed.\n");
      exit(-1);
   }
}

void sendEndRequest(int socket_num) {
   int len = 0;
   struct header *sendPacket = (struct header *) 
                  malloc(sizeof(struct header));
   sendPacket->packet_len = htons(3);
   sendPacket->flag = 8;
   len = send(socket_num, sendPacket, 3, 0);
   if (len != 3) {
      perror("senf failed.\n");
      exit(-1);
   }
}

int getHandleNum(char *buf) {
   char *buf1 = malloc(strlen(buf));
   char *temp;

   memcpy(buf1, buf, strlen(buf));
   temp = strtok(buf1+2, " ");
   if (atoi(temp) >= 1 &&
       atoi(temp) <= 9) {
      return atoi(temp)+1;
   }
   free(buf1);

   return 1;
}

void sendMessage(int socket_num, char *handle, char *buf) {
   char *subbuf[30], *temp = calloc(1, 4000), temp1[250];
   const char s[2] = " ";
   int idx = 0, len = 0, used = 0, i = 0, count = 0;
   int handle_num = 0, handle_len = 0, remain = 0;
   int temp2 = getHandleNum(buf), ndx = 0;;
   struct FLAG5 *finalPacket = NULL;
   struct FLAG5 *sendPacket = (struct FLAG5 *)
                        malloc(sizeof(struct FLAG5));

   temp = strtok(buf+2, s);
   while (temp != NULL) {
      len = strlen(temp);
      subbuf[idx] = calloc(1, sizeof(char)*len);
      memcpy(*(subbuf+idx), temp, len);
      idx++;
      if (++ndx == temp2) {
         break;
      }
      temp = strtok(NULL, s);
   }
   temp = strtok(NULL, "\0");

   if ((atoi(subbuf[0]) >= 1) &&
       (atoi(subbuf[0]) <= 9)) {
      handle_num = atoi(subbuf[0]);
      used++;
   } else {
      handle_num = 1;
   }
 
   sendPacket->packet_len = 4;
   sendPacket->flag = 5;
   handle_len = strlen(handle);
   sendPacket->handle_len = handle_len;
   sendPacket = (struct FLAG5*)realloc(sendPacket, 4+handle_len+1);
   memcpy(((char*)sendPacket)+4, handle, handle_len);
   sendPacket->packet_len = sendPacket->packet_len + handle_len;
   memcpy(((char*)sendPacket)+4+handle_len, &handle_num, 1);
   sendPacket->packet_len = sendPacket->packet_len+1;

   for (i = 0; i < handle_num; i++) {
      handle_len = strlen(subbuf[used]);
      sendPacket = (struct FLAG5*)realloc(sendPacket, 
                   sendPacket->packet_len + handle_len + 1);   
      memcpy(((char*)sendPacket)+sendPacket->packet_len,
             &handle_len, 1);
      memcpy(((char*)sendPacket)+sendPacket->packet_len+1,
             subbuf[used], handle_len);
      memcpy(temp1, ((char*)sendPacket)+sendPacket->packet_len+1, handle_len);
      sendPacket->packet_len = sendPacket->packet_len+handle_len+1;
      used++;
   }
   if (temp == NULL) {
      finalPacket = (struct FLAG5*)malloc(sendPacket->packet_len+1);
      memcpy(finalPacket, sendPacket, sendPacket->packet_len-1);

      finalPacket = (struct FLAG5*)realloc(finalPacket,
                   finalPacket->packet_len);
      memcpy(((char*)finalPacket)+finalPacket->packet_len,
             "\n", 1);
      finalPacket->packet_len = finalPacket->packet_len+1;    
      finalPacket->packet_len = htons(finalPacket->packet_len);
      len = send(socket_num, finalPacket, ntohs(finalPacket->packet_len), 0);
      if (len < 0) {
         perror("send failed.\n");
         exit(-1);
      } 
   } else if (*temp == '\n') {
      finalPacket = (struct FLAG5*)malloc(sendPacket->packet_len);
      memcpy(finalPacket, sendPacket, sendPacket->packet_len);
      finalPacket->packet_len = htons(finalPacket->packet_len);
      len = send(socket_num, finalPacket, ntohs(finalPacket->packet_len), 0);
      if (len < 0) {
         perror("send failed.\n");
         exit(-1);
      } 
   } else {
      remain = strlen(temp);
      while (remain > 999) {
         finalPacket = (struct FLAG5*)malloc(sendPacket->packet_len);
         memcpy(finalPacket, sendPacket, sendPacket->packet_len);
         finalPacket = (struct FLAG5*)realloc(finalPacket,
                      finalPacket->packet_len+1000);
         memcpy(((char*)finalPacket)+finalPacket->packet_len,
                 temp+count*999, 999);
         memcpy(((char*)finalPacket)+finalPacket->packet_len+999,
                 "\n", 1);
         finalPacket->packet_len = finalPacket->packet_len+1000;
         finalPacket->packet_len = htons(finalPacket->packet_len);
         len = send(socket_num, finalPacket, ntohs(finalPacket->packet_len), 0);
         if (len < 0) {
            perror("send failed.\n");
            exit(-1);
         }
         remain-=999;
         count++;
      }
      if (remain <= 999) {
         finalPacket = (struct FLAG5*)malloc(sendPacket->packet_len);
         memcpy(finalPacket, sendPacket, sendPacket->packet_len);
         finalPacket = (struct FLAG5*)realloc(finalPacket,
                      finalPacket->packet_len+remain);
         memcpy(((char*)finalPacket)+finalPacket->packet_len,
                 temp+count*999, remain);
         finalPacket->packet_len = finalPacket->packet_len+remain;
         finalPacket->packet_len = htons(finalPacket->packet_len);
         len = send(socket_num, finalPacket, ntohs(finalPacket->packet_len), 0);
         if (len < 0) {
            perror("send failed.\n");
            exit(-1);
         }
      }
   }
   free(finalPacket);
   printf("$:");
   fflush(stdout);
}

void sendBroadcast(int socket_num, char *handle, char *buf) {
   struct FLAG5 *sendPacket = (struct FLAG5*)
                malloc(sizeof(struct FLAG5));
   struct FLAG5 *finalPacket;
   char text[MAXBUF];
   char *temp = malloc(strlen(buf));
   int len = 0, remain = 0, count = 0;
   strcpy(temp, buf+3);
   sendPacket->packet_len = 4+strlen(handle); 
   sendPacket->flag = 4;
   sendPacket->handle_len = strlen(handle);
   sendPacket = (struct FLAG5*)realloc(sendPacket, 
                4+strlen(handle)+strlen(buf+3)+1);
   memcpy(((char*)sendPacket)+4, handle, strlen(handle));
   memcpy(text, buf+3, strlen(buf+3));
   text[strlen(buf+3)] = '\0';
   remain = strlen(text);
   while (remain > 999) {
      finalPacket = (struct FLAG5*)malloc(sendPacket->packet_len);
      memcpy(finalPacket, sendPacket, sendPacket->packet_len);
      finalPacket = (struct FLAG5*)realloc(finalPacket,
                   finalPacket->packet_len+1000);
      memcpy(((char*)finalPacket)+finalPacket->packet_len,
            temp+count*999, 999);
      memcpy(((char*)finalPacket)+finalPacket->packet_len+999,
              "\n", 1);
      finalPacket->packet_len = finalPacket->packet_len+1000;
      finalPacket->packet_len = htons(finalPacket->packet_len);
      len = send(socket_num, finalPacket, ntohs(finalPacket->packet_len), 0);
      if (len < 0) {
         perror("send failed.\n");
         exit(-1);
      }
         remain-=999;
         count++; 
   } 
   if (remain <= 999) {
      finalPacket = (struct FLAG5*)malloc(sendPacket->packet_len);
      memcpy(finalPacket, sendPacket, sendPacket->packet_len);
      finalPacket = (struct FLAG5*)realloc(finalPacket,
                   finalPacket->packet_len+remain);
      memcpy(((char*)finalPacket)+finalPacket->packet_len,
              temp+count*999, remain);
      finalPacket->packet_len = finalPacket->packet_len+remain;
      finalPacket->packet_len = htons(finalPacket->packet_len);
      len = send(socket_num, finalPacket, ntohs(finalPacket->packet_len), 0);
      if (len < 0) {
         perror("send failed.\n");
         exit(-1);
      }
   }
   printf("$:");
   fflush(stdout);
}

void runClientWithInput(int socket_num, char *handle) {
   char *buf = calloc(BUFSIZE, 1);
   fgets(buf, BUFSIZE, stdin);
   if (buf[0] != '%') {
      printf("$:");
      fflush(stdout);
   } else {
      switch(buf[1]) {
         case 'M':
         case 'm':
            sendMessage(socket_num, handle, buf);
            break;
         case 'B':
         case 'b':
            sendBroadcast(socket_num, handle, buf);
            break;
         case 'L':
         case 'l':
            sendListRequest(socket_num);
            break;
         case 'E':
         case 'e':
            sendEndRequest(socket_num);
            break;
         default:
            printf("Incorrect input.\n");
            printf("$:");
            fflush(stdout);
            break;
      }
   }
   free(buf);   
}

int doFlag11(char *buf) {
   struct FLAG11 *recvPacket = (struct FLAG11 *)buf;
   printf("Number of clients: %d\n", ntohl(recvPacket->amount));
   
   return ntohl(recvPacket->amount);
}

void doFlag12(char *buf, int socket_num) {
   struct FLAG12 *recvPacket = (struct FLAG12 *)buf;
   int handle_len = recvPacket->handle_len;
   char mybuf[MAXBUF], *locate;

   locate = ((char *)recvPacket)+4;
   memcpy(mybuf, locate, handle_len);
   mybuf[handle_len] = '\0';
   printf("   %s\n", mybuf); 
   locate+=handle_len;
   amount--;
   if (amount==0) {
      printf("$:");
      fflush(stdout);
   }
}

void doFlag7(char *buf) {
   struct FLAG7 *recvPacket = (struct FLAG7 *)buf;
   int handle_len = recvPacket->handle_len;
   char name[250];

   memcpy(name, buf+4, handle_len);
   name[handle_len] = '\0';
   printf("Client with handle %s does not exist\n", name);
   printf("$:");
   fflush(stdout);
}

void doFlag5(char *buf) {
   char name[250];
   int handle_len = buf[3];
   int curLoc = 0;

   memcpy(name, buf+4, handle_len);
   name[handle_len] = '\0';
   curLoc = 4+handle_len+1;
   handle_len = buf[curLoc];
   curLoc+=handle_len+1;
   printf("\n%s: %s", name, buf+curLoc);
   printf("$:");
   fflush(stdout);
}

void doFlag4(char *buf) {
   char name[MAXNAME];
   int handle_len = buf[3];
   memcpy(name, buf+4, handle_len);
   name[handle_len] = '\0';
   printf("\n%s: %s", name, buf+4+handle_len);
   printf("$:");
   fflush(stdout);
}

int runClientWithPacket(int socket_num, char *handle) {
   char buf[MAXBUF];
   int len1 = 0, len2 = 0;
   int packet_len = 0;
   
   if ((len1 = recv(socket_num, buf, 2, 0)) <= 0) {
     close(socket_num);
     exit(1);
   }
   memcpy(&packet_len, buf, 2);
   if ((len2 = recv(socket_num, buf+2, ntohs(packet_len)-2, 0)) < 0) {
      perror("recv failed.\n");
      exit(-1);
   }
   switch (buf[2]) {
      case 4:
         doFlag4(buf);
         break;
      case 5:
         doFlag5(buf);
         break;
      case 7:
         doFlag7(buf);
         break;
      case 9:
         return 0;
      case 11:
         amount = doFlag11(buf);
         break;
      case 12:
         doFlag12(buf, socket_num);
         break;
   }  
   
   return 1;
}

int runClient(int socket_num, char *handle) {
   int turn = 1;
   struct timeval timeout; 

   while (turn) {     
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
      FD_ZERO(&fdvar);
      FD_SET(0, &fdvar);
      FD_SET(socket_num, &fdvar);        
      if (select(socket_num+1, (fd_set*)&fdvar, 
                (fd_set*)0, (fd_set*)0, &timeout) < 0) {
         perror("select failed.\n");
         exit(-1);
      }
      if (FD_ISSET(0, &fdvar)) {
         runClientWithInput(socket_num, handle);        
      } else if (FD_ISSET(socket_num, &fdvar)) {
         turn = runClientWithPacket(socket_num, handle);
      } 
   }

   return 0;
}

int main(int argc, char **argv) {
   int socket_num = 0;
   if (argc != 4) {
      printf("Usage: %s handle server-name server-port\n", argv[0]);
      exit(1);
   }
   socket_num = myclientSetup(argv[1], argv[2], argv[3]);
   runClient(socket_num, argv[1]);

   close(socket_num);
   return 0;
}
