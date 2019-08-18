#ifndef INCLUDE_PROTURA_NET_PROTO_H
#define INCLUDE_PROTURA_NET_PROTO_H

#include <protura/fs/inode.h>
#include <protura/net/sockaddr.h>

struct packet;
struct socket;

enum protocol_type {
    PROTOCOL_RAW,
    PROTOCOL_ICMP,
    PROTOCOL_UDP,
    PROTOCOL_TCP,
};

struct protocol {
    list_node_t proto_entry;
    enum protocol_type protocol_id;
    struct protocol_ops *ops;
};

#define PROTOCOL_INIT(proto, type, op) \
    { \
        .proto_entry = LIST_NODE_INIT((proto).proto_entry), \
        .protocol_id = type, \
        .ops = op, \
    }

static inline void protocol_init(struct protocol *proto, enum protocol_type id, struct protocol_ops *ops)
{
    *proto = (struct protocol)PROTOCOL_INIT(*proto, id, ops);
}

struct protocol_ops {
    void (*packet_rx) (struct protocol *, struct packet *);
    int (*packet_tx) (struct protocol *, struct packet *);

    int (*create) (struct protocol *, struct socket *);
    int (*delete) (struct protocol *, struct socket *);

    int (*sendto) (struct protocol *, struct socket *, struct packet *, const struct sockaddr *, socklen_t);

    int (*bind) (struct protocol *, struct socket *, const struct sockaddr *, socklen_t);
    int (*autobind) (struct protocol *, struct socket *);
    int (*getsockname) (struct protocol *, struct socket *, struct sockaddr *, socklen_t *);

    int (*connect) (struct protocol *, struct socket *, const struct sockaddr *, socklen_t);
    int (*accept) (struct protocol *, struct socket *, struct socket **, struct sockaddr *, socklen_t *);
};

void protocol_register(struct protocol *);
struct protocol *protocol_lookup(enum protocol_type);

#endif