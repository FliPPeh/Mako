#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>

#define SOCKET_SENDFLNBUF_MAX 1024
#define SOCKET_HOSTNAME_MAX 256


const char *socket_addrstr(int fam, struct sockaddr *sa);
const char *socket_addrcanon(struct sockaddr *sa, socklen_t len);

int socket_connect(const char *host, const char *svc);
int socket_disconnect(int fd);

ssize_t socket_send(int fd, void *buf, size_t maxsize);
ssize_t socket_sendall(int fd, void *buf, size_t minsize);

ssize_t socket_recv(int fd, void *buf, size_t maxsize);
ssize_t socket_recvall(int fd, void *buf, size_t minsize);

int socket_sendfln(int fd, const char *fmt, ...);

#endif /* defined _SOCKET_H_ */
