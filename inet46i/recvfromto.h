#ifndef INET46I_RECVFROMTO_H
#define INET46I_RECVFROMTO_H

#include <sys/socket.h>

#include "sockaddr46.h"


ssize_t recvfromto(
  int sockfd, void * restrict buf, size_t len, int flags,
  struct sockaddr * restrict src_addr, socklen_t * restrict addrlen,
  union sockaddr_in46 * restrict dst_addr, void * restrict spec_dst);


#endif /* INET46I_RECVFROMTO_H */
