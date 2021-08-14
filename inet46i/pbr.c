#define _GNU_SOURCE

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "private/macro.h"
#include "in46.h"
#include "recvfromto.h"
#include "route.h"
#include "socket46.h"
#include "sockaddr46.h"
#include "pbr.h"


bool sockaddrpr_match (
    const struct SockaddrPolicy *policy, const union sockaddr_in46 *addr) {
  const union in46_addr *host;
  in_port_t port = sockaddr46_get(&addr->sock, (void **) &host);

  if (policy->prefix != 0 && (
        addr->sa_family != AF_INET ||
        IN6_IS_ADDR_V4MAPPED(&policy->network6) ||
        IN6_IS_ADDR_V4COMPAT(&policy->network6)
      ) && (
        addr->sa_family == AF_INET ? host->v4.s_addr != 0 :
          !IN6_IS_ADDR_UNSPECIFIED(&host->v6))) {
    return_if_not (inet_innetwork(
      addr->sa_family, host, addr->sa_family == AF_INET ?
        (const void *) &policy->network : (const void *) &policy->network6,
      policy->prefix)) false;
  }
  if (port != 0) {
    return_if_not (policy->port_max == 0 ?
      policy->port == 0 || port == policy->port :
      policy->port_min <= port && port <= policy->port_max) false;
  }
  if (policy->scope_id != 0 && addr->sa_scope_id != 0) {
    return_if_not (policy->scope_id == addr->sa_scope_id) false;
  }
  return true;
}


struct SocketPolicyRoute *socketpr_resolve (
    const struct SocketPolicyRoute *routes, const struct Socket5Tuple *conn) {
  for (int i = 0; routes[i].func != NULL; i++) {
    continue_if_not (sockaddrpr_match(&routes[i].src, &conn->src));
    continue_if_not (sockaddrpr_match(&routes[i].dst, &conn->dst));
    return (struct SocketPolicyRoute *) routes + i;
  }
  return NULL;
}


int socketpbr_accept4 (int sockfd, const struct SocketPBR *pbr, int flags) {
  struct Socket5Tuple conn;
  conn.src.sa_scope_id = 0;
  conn.dst.sa_scope_id = 0;

  socklen_t srclen = sizeof(conn.src);
  int socket_child = accept4(sockfd, &conn.src.sock, &srclen, flags);
  return_if_fail (socket_child >= 0) pbr->nothing_ret;

  if (!pbr->dst_required) {
    memset(&conn.dst, 0, sizeof(conn.dst));
  } else {
    if (pbr->dst_addr.sa_family == AF_UNSPEC) {
      socklen_t dstlen = sizeof(conn.dst);
      getsockname(socket_child, &conn.dst.sock, &dstlen);
    } else {
      memcpy(&conn.dst, &pbr->dst_addr, sizeof(union sockaddr_in46));
    }
  }

  SocketPolicyRouteFunc func;
  void *context;
  if (pbr->single_route) {
    func = pbr->func;
    context = pbr->context;
  } else {
    const struct SocketPolicyRoute *route = socketpr_resolve(pbr->routes, &conn);
    return_if_fail (route != NULL) pbr->nothing_ret;
    func = route->func;
    context = route->context;
  }
  return ((SocketPolicyRouteStreamFunc) func)(context, socket_child, &conn);
}


int socketpbr_accept (int sockfd, const struct SocketPBR *pbr) {
  return socketpbr_accept4(sockfd, pbr, 0);
}


static int getsockport (int sockfd) {
  struct sockaddr addr;
  socklen_t addrlen = sizeof(addr);
  return_if_fail (getsockname(sockfd, &addr, &addrlen) == 0) -1;
  return sockaddr46_get(&addr, NULL);
}


int socketpbr_recv (int sockfd, const struct SocketPBR *pbr, int flags) {
  struct Socket5Tuple conn;
  conn.src.sa_scope_id = 0;
  memset(&conn.dst, 0, sizeof(conn.dst));
  union in46_addr spec_dst;

  char buf[pbr->buflen];

  socklen_t srclen = sizeof(conn.src);
  int buflen = recvfromto(
    sockfd, buf, pbr->recvlen > 0 ? pbr->recvlen : pbr->buflen, flags,
    &conn.src.sock, &srclen,
    pbr->dst_required ? &conn.dst : NULL, pbr->dst_required ? &spec_dst : NULL);
  return_if_fail (buflen > 0) pbr->nothing_ret;
  if (pbr->dst_required) {
    conn.dst.sa_port = pbr->dst_port != 0 ? pbr->dst_port : getsockport(sockfd);
  }

  SocketPolicyRouteFunc func;
  void *context;
  if (pbr->single_route) {
    func = pbr->func;
    context = pbr->context;
  } else {
    const struct SocketPolicyRoute *route =
      socketpr_resolve(pbr->routes, &conn);
    return_if_fail (route != NULL) pbr->nothing_ret;
    func = route->func;
    context = route->context;
  }
  return ((SocketPolicyRouteDgramFunc) func)(
    context, buf, buflen, sockfd, &conn, &spec_dst);
}


int socketpbr_read (int sockfd, const struct SocketPBR *pbr) {
  return socketpbr_recv(sockfd, pbr, 0);
}


int socketpbr_set (int sockfd, struct SocketPBR *pbr, int af) {
  (void) pbr;

  if (af == 0) {
    struct sockaddr addr;
    socklen_t addrlen = sizeof(addr);
    return_if_fail (getsockname(sockfd, &addr, &addrlen) == 0) -1;
    af = addr.sa_family;
  }

  should (af == AF_INET || af == AF_INET6) otherwise {
    errno = EAFNOSUPPORT;
    return -1;
  }

  int enabled = 1;
  return setsockopt(
    sockfd, af == AF_INET ? IPPROTO_IP : IPPROTO_IPV6,
    af == AF_INET ? IP_PKTINFO : IPV6_RECVPKTINFO,
    &enabled, sizeof(enabled));
}
