#ifndef INCLUDE_PROTURA_NET_PROTO_UDP_H
#define INCLUDE_PROTURA_NET_PROTO_UDP_H

#include <protura/types.h>

struct protocol;
struct packet;

void udp_rx(struct protocol *, struct packet *packet);
int udp_tx(struct packet *packet);

void udp_init(void);

struct udp_socket_private {
};

#endif
