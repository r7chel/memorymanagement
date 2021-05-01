#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
static bool nread(int fd, int len, uint8_t *buf) {
  int readbytes = 0;
  return false;
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {
  return false;
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block) {
  uint8_t header[HEADER_LEN];
  uint16_t len;
  int offset = 0;
  if(nread(fd, HEADER_LEN, header)){
    memcpy(&len, offset+header, len);
    offset += len;
    memcpy(op, 2+header, *op);
    offset += *op;
    memcpy(ret, 6+header, *ret);
    len = ntohs(len);
    *op = ntohl(*op);
    *ret = ntohs(*ret);
    return true;
  }
  else {
    return false;
  }
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block) {
}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port) {
  struct sockaddr_in caddr;
  //make socket
  cli_sd = socket(PF_INET, SOCK_STREAM, 0);
  if (cli_sd == -1){
    printf("Error on socket creation [%s]\n", strerror(errno));
    return false;
  }
  //address information 
  caddr.sin_port = htons(port);
  caddr.sin_family = AF_INET;
  if(inet_aton(ip, &caddr.sin_addr)==0){
    return false;
  }
  return true;
}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {
  close(cli_sd);
  cli_sd = -1;
}

/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block) {
}
