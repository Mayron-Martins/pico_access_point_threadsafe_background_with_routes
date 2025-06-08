#ifndef SETUP_H
#define SETUP_H

#include "dhcpserver.h"
#include "dnsserver.h"
#include "lwip/ip_addr.h"

extern dhcp_server_t dhcp_server;
extern dns_server_t dns_server;

int network_setup(void);

#endif // SETUP_H