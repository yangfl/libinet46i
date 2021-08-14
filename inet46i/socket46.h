#ifndef INET46I_SOCKET46_H
#define INET46I_SOCKET46_H

#include "sockaddr46.h"


__attribute__((warn_unused_result, access(read_only, 1)))
int openaddrz (
  const struct sockaddr *addr, int type, int reuse_port, int scope_id);
__attribute__((warn_unused_result, access(read_only, 1)))
int openaddr (const struct sockaddr *addr, int type, int reuse_port);
__attribute__((warn_unused_result, access(read_only, 1)))
int openaddr46 (const union sockaddr_in46 *addr, int type, int reuse_port);


#endif /* INET46I_SOCKET46_H */
