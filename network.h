#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "testing.h"

#define MAXBUF 1500
#define BUFSIZE 5000
#define MAXNAME 250

struct ClientList {
   char *name;
   int fdNumber;
}__attribute__((packed));

struct header {
   uint16_t packet_len;
   uint8_t flag;
}__attribute__((packed));

struct FLAG1 {
   uint16_t packet_len;
   uint8_t flag;
   uint8_t handle_len;
}__attribute__((packed));

struct FLAG4 {
   uint16_t packet_len;
   uint8_t flag;
   uint8_t handle_len;
}__attribute__((packed));

struct FLAG5 {
   uint16_t packet_len;
   uint8_t flag;
   uint8_t handle_len;
}__attribute__((packed));

struct FLAG7 {
   uint16_t packet_len;
   uint8_t flag;
   uint8_t handle_len;
}__attribute__((packed));

struct FLAG11 {
   uint16_t packet_len;
   uint8_t flag;
   uint32_t amount;
}__attribute__((packed));

struct FLAG12 {
   uint16_t packet_len;
   uint8_t flag;
   uint8_t handle_len;
}__attribute__((packed));

