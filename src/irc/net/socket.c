#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "irc/net/socket.h"
#include "util/log.h"


const char *socket_addrstr(int fam, struct sockaddr *sa)
{
    static char ip[INET6_ADDRSTRLEN];

    if (fam == AF_INET)
        inet_ntop(AF_INET,
                &(((struct sockaddr_in *)sa)->sin_addr), ip, sizeof(ip));
    else if (fam == AF_INET6)
        inet_ntop(AF_INET6,
                &(((struct sockaddr_in6 *)sa)->sin6_addr), ip, sizeof(ip));

    return ip;
}

const char *socket_addrcanon(struct sockaddr *sa, socklen_t len)
{
    static char canon[SOCKET_HOSTNAME_MAX];

    if (getnameinfo(sa, len, canon, sizeof(canon), NULL, 0, 0) != 0)
        return NULL;

    return canon;
}

int socket_connect(const char *host, const char *svc)
{
    struct addrinfo hints;
    struct addrinfo *resolv = NULL;
    struct addrinfo *p = NULL;

    int fd = -1;
    int gai_err = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    log_info("Looking up '%s'...", host);

    if ((gai_err = getaddrinfo(host, svc, &hints, &resolv))) {
        log_error("Unable to look up '%s': %s",
                host, gai_strerror(gai_err));

        goto exit;
    }

    for (p = resolv; p != NULL; p = p->ai_next) {
        const char *addrstr = socket_addrstr(p->ai_family, p->ai_addr);
        const char *addrcanon = socket_addrcanon(p->ai_addr, p->ai_addrlen);

        if (!addrcanon)
            log_info("Attempting '%s:%s'...", addrstr, svc);
        else
            log_info("Attempting '%s:%s' (%s:%s)...",
                    addrcanon, svc, addrstr,   svc);

        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            log_info("   Unable to create socket: %s", strerror(errno));

            continue;
        }

        /* TODO: bind */

        if (connect(fd, p->ai_addr, p->ai_addrlen) < 0) {
            log_info("   Unable to connect: %s", strerror(errno));

            close(fd);
            continue;
        }

        log_info("Connected to %s:%s!", addrcanon, svc);
        goto exit;
    }

    fd = -1;

exit:
    freeaddrinfo(resolv);

    return fd;
}

int socket_disconnect(int fd)
{
    return close(fd);
}

ssize_t socket_send(int fd, void *buf, size_t maxsize)
{
    return send(fd, buf, maxsize, 0);
}

ssize_t socket_sendall(int fd, void *buf, size_t minsize)
{
    ssize_t sent = 0;
    ssize_t tosend = (ssize_t)minsize;

    while (sent < tosend) {
        ssize_t out = socket_send(fd,
                (unsigned char *)buf + sent, (size_t)tosend);

        if (out < 0) {
            return -1;
        } else if (out == 0) {
            break;
        } else {
            tosend -= (size_t)out;
            sent += out;
        }
    }

    return sent;
}

ssize_t socket_recv(int fd, void *buf, size_t maxsize)
{
    return recv(fd, buf, maxsize, 0);
}

ssize_t socket_recvall(int fd, void *buf, size_t minsize)
{
    ssize_t recvd = 0;
    ssize_t torecv = (ssize_t)minsize;

    while (recvd < torecv) {
        ssize_t in = socket_recv(fd,
                (unsigned char *)buf + recvd, (size_t)torecv);

        if (in < 0) {
            return -1;
        } else if (in == 0) {
            break;
        } else {
            recvd += in;
            torecv -= in;
        }
    }

    return recvd;
}

ssize_t socket_sendfln(int fd, const char *fmt, ...)
{
    char buffer[SOCKET_SENDFLNBUF_MAX] = {0};
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer) - 3, fmt, args);
    va_end(args);

    strcat(buffer, "\r\n");

    return socket_sendall(fd, buffer, strlen(buffer));
}
