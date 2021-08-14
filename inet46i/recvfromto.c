#define _GNU_SOURCE

#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "private/macro.h"
#include "sockaddr46.h"
#include "recvfromto.h"


ssize_t recvfromto(
    int sockfd, void * restrict buf, size_t len, int flags,
    struct sockaddr * restrict src_addr, socklen_t * restrict addrlen,
    union sockaddr_in46 * restrict dst_addr, void * restrict spec_dst) {
  return_if_not (dst_addr != NULL || spec_dst != NULL)
    recvfrom(sockfd, buf, len, flags, src_addr, addrlen);

  char cmbuf[1024];
  struct iovec iov = {.iov_base = buf, .iov_len = len};
  struct msghdr mh = {
    .msg_name = src_addr, .msg_namelen = addrlen == NULL ? 0 : *addrlen,
    .msg_iov = &iov, .msg_iovlen = 1,
    .msg_control = cmbuf, .msg_controllen = sizeof(cmbuf),
  };

  ssize_t recvlen = recvmsg(sockfd, &mh, flags);
  return_if_fail (recvlen >= 0) recvlen;

  for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&mh); cmsg != NULL;
       cmsg = CMSG_NXTHDR(&mh, cmsg)) {
    if (cmsg->cmsg_level == IPPROTO_IP) {
      if (cmsg->cmsg_type == IP_PKTINFO) {
        struct in_pktinfo *pi = (void *) CMSG_DATA(cmsg);
        if (dst_addr != NULL) {
          dst_addr->sa_family = AF_INET;
          memcpy(&dst_addr->v4.sin_addr, &pi->ipi_addr, sizeof(pi->ipi_addr));
          dst_addr->sa_scope_id = pi->ipi_ifindex;
        }
        if (spec_dst != NULL) {
          memcpy(spec_dst, &pi->ipi_spec_dst, sizeof(pi->ipi_spec_dst));
        }
        break;
      }
    } else if (cmsg->cmsg_level == IPPROTO_IPV6) {
      if (cmsg->cmsg_type == IPV6_PKTINFO) {
        struct in6_pktinfo *pi = (void *) CMSG_DATA(cmsg);
        if (dst_addr != NULL) {
          dst_addr->sa_family = AF_INET6;
          memcpy(&dst_addr->v6.sin6_addr, &pi->ipi6_addr,
                 sizeof(pi->ipi6_addr));
          dst_addr->sa_scope_id = pi->ipi6_ifindex;
        }
        if (spec_dst != NULL) {
          memcpy(spec_dst, &pi->ipi6_addr, sizeof(pi->ipi6_addr));
        }
        break;
      }
    }
  }
  return recvlen;
}
