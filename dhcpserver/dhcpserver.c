/**
 * -----------------------------------------------
 * Arquivo: dhcpserver.c 
 * Projeto: pico_access_point_with_routes
 * -----------------------------------------------
 * 
 * Descrição: 
 *      Este módulo implementa um servidor DHCP simples
 *      compatível com o protocolo descrito na RFC 2131.
 *      Ele fornece IPs dinâmicos a clientes conectados
 *      à rede criada pelo Raspberry Pi Pico W.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "cyw43_config.h"
#include "dhcpserver.h"
#include "lwip/udp.h"

#define DHCPDISCOVER    (1)
#define DHCPOFFER       (2)
#define DHCPREQUEST     (3)
#define DHCPDECLINE     (4)
#define DHCPACK         (5)
#define DHCPNACK        (6)
#define DHCPRELEASE     (7)
#define DHCPINFORM      (8)

#define DHCP_OPT_PAD                (0)
#define DHCP_OPT_SUBNET_MASK        (1)
#define DHCP_OPT_ROUTER             (3)
#define DHCP_OPT_DNS                (6)
#define DHCP_OPT_HOST_NAME          (12)
#define DHCP_OPT_REQUESTED_IP       (50)
#define DHCP_OPT_IP_LEASE_TIME      (51)
#define DHCP_OPT_MSG_TYPE           (53)
#define DHCP_OPT_SERVER_ID          (54)
#define DHCP_OPT_PARAM_REQUEST_LIST (55)
#define DHCP_OPT_MAX_MSG_SIZE       (57)
#define DHCP_OPT_VENDOR_CLASS_ID    (60)
#define DHCP_OPT_CLIENT_ID          (61)
#define DHCP_OPT_END                (255)

#define PORT_DHCP_SERVER (67)
#define PORT_DHCP_CLIENT (68)

#define DEFAULT_LEASE_TIME_S (24 * 60 * 60) // in seconds

#define MAC_LEN (6)
#define MAKE_IP4(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))

typedef struct {
    uint8_t op; // message opcode
    uint8_t htype; // hardware address type
    uint8_t hlen; // hardware address length
    uint8_t hops;
    uint32_t xid; // transaction id, chosen by client
    uint16_t secs; // client seconds elapsed
    uint16_t flags;
    uint8_t ciaddr[4]; // client IP address
    uint8_t yiaddr[4]; // your IP address
    uint8_t siaddr[4]; // next server IP address
    uint8_t giaddr[4]; // relay agent IP address
    uint8_t chaddr[16]; // client hardware address
    uint8_t sname[64]; // server host name
    uint8_t file[128]; // boot file name
    uint8_t options[312]; // optional parameters, variable, starts with magic
} dhcp_msg_t;

/**
 * [Descrição]: Cria um novo socket UDP e registra o callback de recebimento.
 * [Parâmetros]: 
 *  - struct udp_pcb **udp: ponteiro para o socket a ser alocado;
 *  - void *cb_data: ponteiro com dados que serão repassados ao callback;
 *  - udp_recv_fn cb_udp_recv: função callback para recebimento UDP;
 * [Notas]: Retorna 0 em caso de sucesso e -ENOMEM em caso de falha.
 */
static int dhcp_socket_new_dgram(struct udp_pcb **udp, void *cb_data, udp_recv_fn cb_udp_recv) {
    // family is AF_INET
    // type is SOCK_DGRAM

    *udp = udp_new();
    if (*udp == NULL) {
        return -ENOMEM;
    }

    // Register callback
    udp_recv(*udp, cb_udp_recv, (void *)cb_data);

    return 0; // success
}

/**
 * [Descrição]: Libera o socket UDP previamente criado.
 * [Parâmetros]: 
 *  - struct udp_pcb **udp: ponteiro para o socket UDP;
 * [Notas]: O ponteiro é setado como NULL após liberação.
 */
static void dhcp_socket_free(struct udp_pcb **udp) {
    if (*udp != NULL) {
        udp_remove(*udp);
        *udp = NULL;
    }
}

/**
 * [Descrição]: Associa o socket UDP a uma porta local.
 * [Parâmetros]: 
 *  - struct udp_pcb **udp: ponteiro para o socket UDP;
 *  - uint16_t port: porta local (geralmente 67 para servidor DHCP);
 * [Notas]: Retorna o código de erro do lwIP (padrão `err_t`).
 */
static int dhcp_socket_bind(struct udp_pcb **udp, uint16_t port) {
    // TODO convert lwIP errors to errno
    return udp_bind(*udp, IP_ANY_TYPE, port);
}

/**
 * [Descrição]: Envia um pacote UDP para o cliente.
 * [Parâmetros]: 
 *  - struct udp_pcb **udp: ponteiro para o socket;
 *  - struct netif *nif: interface de rede usada;
 *  - const void *buf: buffer com os dados a serem enviados;
 *  - size_t len: tamanho do buffer;
 *  - uint32_t ip: endereço IP destino (em big endian);
 *  - uint16_t port: porta destino;
 * [Notas]: Se `nif` for NULL, a função usa o `udp_sendto` padrão.
 */
static int dhcp_socket_sendto(struct udp_pcb **udp, struct netif *nif, const void *buf, size_t len, uint32_t ip, uint16_t port) {
    if (len > 0xffff) {
        len = 0xffff;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL) {
        return -ENOMEM;
    }

    memcpy(p->payload, buf, len);

    ip_addr_t dest;
    IP4_ADDR(ip_2_ip4(&dest), ip >> 24 & 0xff, ip >> 16 & 0xff, ip >> 8 & 0xff, ip & 0xff);
    err_t err;
    if (nif != NULL) {
        err = udp_sendto_if(*udp, p, &dest, port, nif);
    } else {
        err = udp_sendto(*udp, p, &dest, port);
    }

    pbuf_free(p);

    if (err != ERR_OK) {
        return err;
    }

    return len;
}

/**
 * [Descrição]: Busca por uma opção DHCP específica.
 * [Parâmetros]: 
 *  - uint8_t *opt: ponteiro para a área de opções DHCP;
 *  - uint8_t cmd: código da opção desejada;
 * [Notas]: Retorna ponteiro para a opção encontrada ou NULL.
 */
static uint8_t *opt_find(uint8_t *opt, uint8_t cmd) {
    for (int i = 0; i < 308 && opt[i] != DHCP_OPT_END;) {
        if (opt[i] == cmd) {
            return &opt[i];
        }
        i += 2 + opt[i + 1];
    }
    return NULL;
}

/**
 * [Descrição]: Escreve uma opção DHCP com dados genéricos.
 * [Parâmetros]: 
 *  - uint8_t **opt: ponteiro para ponteiro de escrita;
 *  - uint8_t cmd: código da opção;
 *  - size_t n: tamanho dos dados;
 *  - const void *data: ponteiro para os dados;
 * [Notas]: A posição `*opt` é atualizada após escrita.
 */
static void opt_write_n(uint8_t **opt, uint8_t cmd, size_t n, const void *data) {
    uint8_t *o = *opt;
    *o++ = cmd;
    *o++ = n;
    memcpy(o, data, n);
    *opt = o + n;
}

/**
 * [Descrição]: Escreve uma opção DHCP do tipo uint8_t.
 * [Parâmetros]: 
 *  - uint8_t **opt: ponteiro para ponteiro de escrita;
 *  - uint8_t cmd: código da opção;
 *  - uint8_t val: valor de 1 byte;
 * [Notas]: Usado para opções simples como tipo da mensagem.
 */
static void opt_write_u8(uint8_t **opt, uint8_t cmd, uint8_t val) {
    uint8_t *o = *opt;
    *o++ = cmd;
    *o++ = 1;
    *o++ = val;
    *opt = o;
}

/**
 * [Descrição]: Escreve uma opção DHCP do tipo uint32_t.
 * [Parâmetros]: 
 *  - uint8_t **opt: ponteiro para ponteiro de escrita;
 *  - uint8_t cmd: código da opção;
 *  - uint32_t val: valor de 4 bytes;
 * [Notas]: Útil para IPs, tempo de lease, etc.
 */
static void opt_write_u32(uint8_t **opt, uint8_t cmd, uint32_t val) {
    uint8_t *o = *opt;
    *o++ = cmd;
    *o++ = 4;
    *o++ = val >> 24;
    *o++ = val >> 16;
    *o++ = val >> 8;
    *o++ = val;
    *opt = o;
}

/**
 * [Descrição]: Callback principal do servidor DHCP que processa pacotes recebidos.
 * [Parâmetros]: 
 *  - void *arg: ponteiro para `dhcp_server_t`;
 *  - struct udp_pcb *upcb: ponteiro para PCB do socket;
 *  - struct pbuf *p: buffer de pacote recebido;
 *  - const ip_addr_t *src_addr: endereço IP do remetente;
 *  - u16_t src_port: porta do remetente;
 * [Notas]: Processa mensagens DHCPDISCOVER e DHCPREQUEST e envia DHCPOFFER ou DHCPACK.
 */
static void dhcp_server_process(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *src_addr, u16_t src_port) {
    dhcp_server_t *d = arg;
    (void)upcb;
    (void)src_addr;
    (void)src_port;

    // This is around 548 bytes
    dhcp_msg_t dhcp_msg;

    #define DHCP_MIN_SIZE (240 + 3)
    if (p->tot_len < DHCP_MIN_SIZE) {
        goto ignore_request;
    }

    size_t len = pbuf_copy_partial(p, &dhcp_msg, sizeof(dhcp_msg), 0);
    if (len < DHCP_MIN_SIZE) {
        goto ignore_request;
    }

    dhcp_msg.op = DHCPOFFER;
    memcpy(&dhcp_msg.yiaddr, &ip4_addr_get_u32(ip_2_ip4(&d->ip)), 4);

    uint8_t *opt = (uint8_t *)&dhcp_msg.options;
    opt += 4; // assume magic cookie: 99, 130, 83, 99

    uint8_t *msgtype = opt_find(opt, DHCP_OPT_MSG_TYPE);
    if (msgtype == NULL) {
        // A DHCP package without MSG_TYPE?
        goto ignore_request;
    }

    switch (msgtype[2]) {
        case DHCPDISCOVER: {
            int yi = DHCPS_MAX_IP;
            for (int i = 0; i < DHCPS_MAX_IP; ++i) {
                if (memcmp(d->lease[i].mac, dhcp_msg.chaddr, MAC_LEN) == 0) {
                    // MAC match, use this IP address
                    yi = i;
                    break;
                }
                if (yi == DHCPS_MAX_IP) {
                    // Look for a free IP address
                    if (memcmp(d->lease[i].mac, "\x00\x00\x00\x00\x00\x00", MAC_LEN) == 0) {
                        // IP available
                        yi = i;
                    }
                    uint32_t expiry = d->lease[i].expiry << 16 | 0xffff;
                    if ((int32_t)(expiry - cyw43_hal_ticks_ms()) < 0) {
                        // IP expired, reuse it
                        memset(d->lease[i].mac, 0, MAC_LEN);
                        yi = i;
                    }
                }
            }
            if (yi == DHCPS_MAX_IP) {
                // No more IP addresses left
                goto ignore_request;
            }
            dhcp_msg.yiaddr[3] = DHCPS_BASE_IP + yi;
            opt_write_u8(&opt, DHCP_OPT_MSG_TYPE, DHCPOFFER);
            break;
        }

        case DHCPREQUEST: {
            uint8_t *o = opt_find(opt, DHCP_OPT_REQUESTED_IP);
            if (o == NULL) {
                // Should be NACK
                goto ignore_request;
            }
            if (memcmp(o + 2, &ip4_addr_get_u32(ip_2_ip4(&d->ip)), 3) != 0) {
                // Should be NACK
                goto ignore_request;
            }
            uint8_t yi = o[5] - DHCPS_BASE_IP;
            if (yi >= DHCPS_MAX_IP) {
                // Should be NACK
                goto ignore_request;
            }
            if (memcmp(d->lease[yi].mac, dhcp_msg.chaddr, MAC_LEN) == 0) {
                // MAC match, ok to use this IP address
            } else if (memcmp(d->lease[yi].mac, "\x00\x00\x00\x00\x00\x00", MAC_LEN) == 0) {
                // IP unused, ok to use this IP address
                memcpy(d->lease[yi].mac, dhcp_msg.chaddr, MAC_LEN);
            } else {
                // IP already in use
                // Should be NACK
                goto ignore_request;
            }
            d->lease[yi].expiry = (cyw43_hal_ticks_ms() + DEFAULT_LEASE_TIME_S * 1000) >> 16;
            dhcp_msg.yiaddr[3] = DHCPS_BASE_IP + yi;
            opt_write_u8(&opt, DHCP_OPT_MSG_TYPE, DHCPACK);
            printf("DHCPS: client connected: MAC=%02x:%02x:%02x:%02x:%02x:%02x IP=%u.%u.%u.%u\n",
                dhcp_msg.chaddr[0], dhcp_msg.chaddr[1], dhcp_msg.chaddr[2], dhcp_msg.chaddr[3], dhcp_msg.chaddr[4], dhcp_msg.chaddr[5],
                dhcp_msg.yiaddr[0], dhcp_msg.yiaddr[1], dhcp_msg.yiaddr[2], dhcp_msg.yiaddr[3]);
            break;
        }

        default:
            goto ignore_request;
    }

    opt_write_n(&opt, DHCP_OPT_SERVER_ID, 4, &ip4_addr_get_u32(ip_2_ip4(&d->ip)));
    opt_write_n(&opt, DHCP_OPT_SUBNET_MASK, 4, &ip4_addr_get_u32(ip_2_ip4(&d->nm)));
    opt_write_n(&opt, DHCP_OPT_ROUTER, 4, &ip4_addr_get_u32(ip_2_ip4(&d->ip))); // aka gateway; can have multiple addresses
    opt_write_n(&opt, DHCP_OPT_DNS, 4, &ip4_addr_get_u32(ip_2_ip4(&d->ip))); // this server is the dns
    opt_write_u32(&opt, DHCP_OPT_IP_LEASE_TIME, DEFAULT_LEASE_TIME_S);
    *opt++ = DHCP_OPT_END;
    struct netif *nif = ip_current_input_netif();
    dhcp_socket_sendto(&d->udp, nif, &dhcp_msg, opt - (uint8_t *)&dhcp_msg, 0xffffffff, PORT_DHCP_CLIENT);

ignore_request:
    pbuf_free(p);
}

/**
 * [Descrição]: Inicializa o servidor DHCP.
 * [Parâmetros]: 
 *  - dhcp_server_t *d: ponteiro para estrutura do servidor DHCP;
 *  - ip_addr_t *ip: IP base do servidor (gateway);
 *  - ip_addr_t *nm: máscara de sub-rede;
 * [Notas]: Cria o socket, registra callback e inicia escuta na porta 67.
 */
void dhcp_server_init(dhcp_server_t *d, ip_addr_t *ip, ip_addr_t *nm) {
    ip_addr_copy(d->ip, *ip);
    ip_addr_copy(d->nm, *nm);
    memset(d->lease, 0, sizeof(d->lease));
    
    if (dhcp_socket_new_dgram(&d->udp, d, dhcp_server_process) != 0) {
        printf("dhcp server: failed to create socket\n");
        return;
    }
    
    if (dhcp_socket_bind(&d->udp, PORT_DHCP_SERVER) != 0) {
        printf("dhcp server: failed to bind socket\n");
        dhcp_socket_free(&d->udp);
        return;
    }
    
    printf("dhcp server: successfully started on port %d\n", PORT_DHCP_SERVER);
}

void dhcp_server_deinit(dhcp_server_t *d) {
    dhcp_socket_free(&d->udp);
}
