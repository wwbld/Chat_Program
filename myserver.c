#include "network.h"

struct ClientList *clientlist;
static struct timeval timeout;
fd_set fdvar;
int count = 0;
int total = 1;

void initClientList() {
   int i = 0;
   clientlist = (struct ClientList*)calloc
                (total, sizeof(struct ClientList));       
   for (; i < total; i++) {                               
      clientlist[i].name = (char*)calloc(MAXNAME, sizeof(char));      
   }
}

int myserverSetup(int port) {
   int server_socket = 0;
   struct sockaddr_in local;
   socklen_t len = sizeof(local);

   server_socket = socket(AF_INET, SOCK_STREAM, 0);
   if (server_socket < 0) {
      perror("socket failed.\n");
      exit(-1);
   }

   local.sin_family = AF_INET;
   local.sin_addr.s_addr = INADDR_ANY;
   local.sin_port = htons(port);

   if (bind(server_socket, (struct sockaddr *)&local, 
                           sizeof(local)) < 0) {
      perror("bind failed.\n");
      exit(-1);
   }
   if (getsockname(server_socket, 
                  (struct sockaddr *)&local, &len) < 0) {
      perror("getsockname failed.\n");
      exit(-1);
   }
   if (listen(server_socket, 1000) < 0) {        
      perror("listen failed.\n");
      exit(-1);
   }

   printf("Server is using port %d\n", ntohs(local.sin_port));
   initClientList();

   return server_socket;
}

int serverAccept(int server_socket) {
   int client_socket = 0;
   
   client_socket = accept(server_socket, 
                         (struct sockaddr *)0, 
                         (socklen_t *)0);
   if (client_socket < 0) {
      perror("accept failed.\n");
      exit(-1);
   }

   return client_socket;
}

int goThroughListAndSet(int server_socket) {
   int max = server_socket, i = 0;
   for (; i < total; i++) {                    
      if (clientlist[i].fdNumber != 0) {
         FD_SET(clientlist[i].fdNumber, &fdvar);
         if (clientlist[i].fdNumber > max) {
            max = clientlist[i].fdNumber;
         }
      }
   }
   return max;  
}

void addNewClient(int newfd) {
   int i = 0;
   for (; i < total; i++) {                
      if (clientlist[i].fdNumber == 0) {
         clientlist[i].fdNumber = newfd;
         break;
      }
   }
   total++;    
   clientlist = (struct ClientList*)realloc(clientlist,
                total*sizeof(struct ClientList)); 
   clientlist[total-1].name = (char*)calloc
                    (MAXNAME, sizeof(char));  
   clientlist[total-1].fdNumber = 0;   
}

int handleExisted(char *name) {
   int i = 0;
   for (; i < total; i++) {   
      if (strcmp(clientlist[i].name, name) == 0) {
         return 1;
      }
   }

   return 0;
}

int handleNotExisted(char *name) {
   int i = 0;
   for (; i < total; i++) {   
      if (strcmp(clientlist[i].name, name) == 0) {
         return 0;
      }
   }
   
   return 1;
}

int findSocketNum(char *name) {
   int num = 0, i = 0;
   for (; i < total; i++) {   
      if (strcmp(clientlist[i].name, name) == 0) {
         num = clientlist[i].fdNumber;
         break;
      }
   }
  
   return num;
}

void addHandleName(char *name, int client_socket) {
   int i = 0;
   for (; i < total; i++) {   
      if (clientlist[i].fdNumber == client_socket) {
         strcpy(clientlist[i].name, name);
      }
   }
}

void removeSocket(int client_socket) {
   int i = 0;
   for (; i < total; i++) {
      if (clientlist[i].fdNumber == client_socket) {
         clientlist[i].fdNumber = 0;
      }
   }
}

void clientSetup(int client_socket, char *buf) {
   struct FLAG1 *packet = (struct FLAG1 *)buf;
   struct header *sendPacket = (struct header*)
                  calloc(1, sizeof(struct header));
   int handle_len = packet->handle_len;
   char *handle_name = (char*)malloc(handle_len+1);
   int len = 0;

   memcpy(handle_name, buf+sizeof(struct FLAG1), handle_len);
   handle_name[handle_len] = '\0';

   sendPacket->packet_len = htons(3);
   if (handleExisted(handle_name) == 0) {
      sendPacket->flag = (uint8_t)2;
      addHandleName(handle_name, client_socket);
   } else {
      removeSocket(client_socket);
      sendPacket->flag = (uint8_t)3;
   }
   if ((len = send(client_socket, 
                  (char *)sendPacket, 3, 0)) < 0) {
      perror("send failed.\n");
      exit(-1);
   }
   count++;
}

void sendHandleAmount(int client_socket) {
   int len = 0;
   struct FLAG11 *sendPacket = (struct FLAG11 *)
                  malloc(sizeof(struct FLAG11));
   sendPacket->packet_len = htons(7);
   sendPacket->flag = 11;
   sendPacket->amount = htonl(count);
   len = send(client_socket, sendPacket, 7, 0);
   if (len != 7) {
      perror("send failed.\n");
      exit(-1);
   }
}

void sendHandleName(int client_socket, char *handle) {
   int len = 0;
   struct FLAG12 *sendPacket = (struct FLAG12 *)
                  malloc(sizeof(struct FLAG12));
   sendPacket->packet_len = htons(4+strlen(handle));
   sendPacket->flag = 12;
   sendPacket->handle_len = strlen(handle);

   sendPacket = (struct FLAG12*)realloc
                (sendPacket, 4+strlen(handle));
   memcpy(((char *)sendPacket)+4, handle, strlen(handle));

   len = send(client_socket, sendPacket, 4+strlen(handle), 0);
   if (len < 0) {
      perror("send failed.\n");
      exit(-1);
   }
}

void clientList(int client_socket, char *buf) {
   int i = 0;
   sendHandleAmount(client_socket);

   for (; i < total; i++) {    
      if (clientlist[i].fdNumber != 0) {
         sendHandleName(client_socket, clientlist[i].name);
      }
   }
}

void clientEnd(int client_socket, char *buf) {
   int i = 0, len = 0;
   struct header *sendPacket = (struct header *)
                  malloc(sizeof(struct header));
   sendPacket->packet_len = htons(3);
   sendPacket->flag = 9;
   len = send(client_socket, sendPacket, 3, 0);
   if (len != 3) {
      perror("send failed.\n");
      exit(-1);
   }
   for (; i < total; i++) {   
      if (clientlist[i].fdNumber == client_socket) {
         clientlist[i].name[0] = '\0';
         clientlist[i].fdNumber = 0;
         count--;
      }
   }
   close(client_socket);
}

void sendFlag7(int client_socket, char *name) {
   struct FLAG7 *sendPacket = (struct FLAG7*)
                 malloc(sizeof(struct FLAG7));
   int len = 0;

   sendPacket->flag = 7;
   sendPacket->handle_len = strlen(name);
   sendPacket->packet_len = htons(4+strlen(name));
   memcpy(((char*)sendPacket)+4, name, strlen(name));
   len = send(client_socket, sendPacket, 4+strlen(name), 0);
   if (len < 0) {
      perror("send failed.\n");
      exit(-1);     
   }
}

int findTextLoc(char *buf, int total) {
   int ret = 0, i = 0, len;
   for (; i < total; i++) {
      len = buf[ret++];
      ret+=len;
   } 
   return ret;
}

void clientSend(int client_socket, char *buf) {
   char mybuf[1500], name[250];
   int handle_len = 0, curLen = 0, handle_total = 0;
   int i = 0, fixedLen = 0, tempLen = 0, packet_len = 0;
   int len = 0, socket_num = 0;
   
   memcpy(mybuf, buf, 4+buf[3]+1);
   mybuf[2] = 5;
   handle_total = buf[4+buf[3]];
   curLen = 4 + buf[3];
   mybuf[curLen++] = 1;
   fixedLen = curLen;
  
   for (; i < handle_total; i++) {
      tempLen = fixedLen;
      handle_len = buf[curLen++];
      memcpy(mybuf+(tempLen++), &handle_len, 1);
      memcpy(mybuf+tempLen, buf+curLen, handle_len);
      memcpy(name, mybuf+tempLen, handle_len);
      tempLen+=handle_len;
      curLen+=handle_len;
      name[handle_len+1] = '\0';
      if (handleNotExisted(name)) {
         sendFlag7(client_socket, name);
      } else {
         socket_num = findSocketNum(name);
         strcpy(mybuf+tempLen, 
                buf+fixedLen+
                findTextLoc(buf+fixedLen, handle_total));

         packet_len = htons(fixedLen + 1 + 
                            handle_len + strlen(mybuf+tempLen) +1);
         memcpy(mybuf, &packet_len, 2);
         len = send(socket_num, mybuf, ntohs(packet_len), 0);
         if (len < 0) {
            perror("send failed.\n");
            exit(-1);
         }
      }
   }
}

void clientBroadcast(int client_socket, char *buf) {
   struct FLAG4 *sendPacket = (struct FLAG4*)
                 malloc(sizeof(struct FLAG4));
   int handle_len = buf[3];
   int text_len = strlen(buf+4+buf[3]);
   int i = 0, len = 0;
 
   sendPacket->flag = 4;
   for (i = 0; i < total; i++) {   
      if (clientlist[i].fdNumber != 0 &&
          clientlist[i].fdNumber != client_socket) {
         sendPacket->handle_len = handle_len;
         sendPacket = (struct FLAG4*)realloc(sendPacket,
                      4+handle_len+text_len+1);
         memcpy(((char*)sendPacket)+4, buf+4, handle_len);
         strcpy(((char*)sendPacket)+4+handle_len,
                buf+4+handle_len);
         ((char*)sendPacket)[4+handle_len+text_len+1] = '\0';
         sendPacket->packet_len = htons(4+handle_len+text_len+1);
         len = send(clientlist[i].fdNumber,
                    sendPacket, ntohs(sendPacket->packet_len), 0);
         if (len < 0) {
            perror("send failed.\n");
            exit(-1);
         }
      }      
   }
}

void clientDoSomething(int client_socket) {
   int message_len1 = 0, message_len2 = 0;;
   int packet_len = 0, i = 0;  
   uint8_t flag = 0;
   char *buf = (char *)calloc(BUFSIZE, sizeof(char));

   message_len1 = recv(client_socket, buf, 2, 0);
   if (message_len1 <= 0) {
      for (; i < total; i++) {
         if (clientlist[i].fdNumber == client_socket) {
            clientlist[i].name[0] = '\0';
            clientlist[i].fdNumber = 0;
            count--;
         }
      }
      close(client_socket);
   } else {
      memcpy(&packet_len, buf, 2);
      message_len2 = recv(client_socket, buf+2, ntohs(packet_len)-2, 0);
      if (message_len2 < 0) {
         perror("recv failed.\n");
         exit(-1);
      }
      flag = ((struct header *)buf)->flag;

      switch(flag) {
         case 1:
            clientSetup(client_socket, buf);
            break;
         case 4:
            clientBroadcast(client_socket, buf);
            break;
         case 5:
            clientSend(client_socket, buf);
            break;
         case 8:
            clientEnd(client_socket, buf);
            break;
         case 10:
            clientList(client_socket, buf);
            break;
      }
   }
}

void runServer(int server_socket) {
   int max = 0, i = 0;
   int new_client_socket = 0;
   timeout.tv_sec = 1;
   timeout.tv_usec = 0;

   while (1) {
      FD_ZERO(&fdvar);
      FD_SET(server_socket, &fdvar);
      max = goThroughListAndSet(server_socket);
      if (select(max+1, (fd_set*)&fdvar, (fd_set*)0, 
                 (fd_set*)0, 0) < 0) {
         perror("select failed.\n");
         exit(-1);     
      }
      if (FD_ISSET(server_socket, &fdvar)) {
         new_client_socket = serverAccept(server_socket);
         addNewClient(new_client_socket);
         FD_SET(new_client_socket, &fdvar);
      }
      for (i = 0; i < total; i++) {   
         if (FD_ISSET(clientlist[i].fdNumber, &fdvar)) {
            clientDoSomething(clientlist[i].fdNumber);
         }
      }
   }
}

int main(int argc, char **argv) {
   int server_socket = 0;
   if (argc == 1) {  
      server_socket = myserverSetup(0);  
   } else if (argc == 2){
      server_socket = myserverSetup(atoi(argv[1])); 
   } else {
      perror("Incorrect input format.\n");
      exit(-1);
   } 
   runServer(server_socket);    
   return 0;
}
