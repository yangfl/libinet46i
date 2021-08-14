#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "private/macro.h"
#include "inet46.h"
#include "sockaddr46.h"
#include "socket46.h"


int openaddrz (
    const struct sockaddr *addr, int type, int reuse_port, int scope_id) {
  int fd = socket(addr->sa_family, type, 0);
  return_if_fail (fd >= 0) -1;

  if (reuse_port) {
    int on = 1;
    goto_if_fail (setsockopt(
      fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == 0) fail;
  }

  goto_if_fail (bind(fd, addr, addr->sa_family == AF_INET ?
    sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)) == 0) fail;

  if (scope_id) {
    struct ifreq ifr;
    goto_if_fail (if_indextoname(scope_id, ifr.ifr_name) != NULL) fail;
    goto_if_fail (setsockopt(
      fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) == 0) fail;
  }

  return fd;

  int err;
fail:
  err = errno;
  close(fd);
  errno = err;
  return -1;
}


int openaddr (const struct sockaddr *addr, int type, int reuse_port) {
  return openaddrz(
    addr, type, reuse_port, addr->sa_family == AF_INET6 ?
      ((const struct sockaddr_in6 *) addr)->sin6_scope_id : 0);
}


int openaddr46 (const union sockaddr_in46 *addr, int type, int reuse_port) {
  return openaddrz(&addr->sock, type, reuse_port, addr->sa_scope_id);
}
