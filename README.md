# libinet46i

Use the same code to process sockets of IPv4 (`AF_INET`), IPv6 (`AF_INET6`) and the interface name (`SO_BINDTODEVICE`).

Docs to be added. Feel free to ask for it.

## Why there is a policy-based socket routing?

You may want to 'listen' to a specific interface as if an IP address, however Linux won't allow you to open two sockets binding to 0.0.0.0 listening to the same port. Routing must be done in user programs.
