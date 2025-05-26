/**
 * -----------------------------------------------
 * Arquivo: dnsserver.c 
 * Projeto: pico_access_point_with_routes
 * -----------------------------------------------
 * 
 * Descrição: 
 *      Este módulo implementa um servidor DNS simples que
 *      responde com o IP local para qualquer requisição
 *      recebida. Útil para redirecionar todos os domínios
 *      acessados por um cliente para um IP específico,
 *      como em portais cativos.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdbool.h>

#include "dnsserver.h"
#include "lwip/udp.h"

#define PORT_DNS_SERVER 53
#define DUMP_DATA 0

#define DEBUG_printf(...)
#define ERROR_printf printf

typedef struct dns_header_t_ {
    uint16_t id;
    uint16_t flags;
    uint16_t question_count;
    uint16_t answer_record_count;
    uint16_t authority_record_count;
    uint16_t additional_record_count;
} dns_header_t;

#define MAX_DNS_MSG_SIZE 300

/**
 * [Descrição]: Cria um novo socket UDP e registra o callback.
 * [Parâmetros]: 
 *  - struct udp_pcb **udp: ponteiro para o socket UDP a ser alocado;
 *  - void *cb_data: ponteiro com dados passados ao callback;
 *  - udp_recv_fn cb_udp_recv: função chamada ao receber pacotes;
 * [Notas]: Retorna 0 em caso de sucesso ou -ENOMEM em erro.
 */
static int dns_socket_new_dgram(struct udp_pcb **udp, void *cb_data, udp_recv_fn cb_udp_recv) {
    *udp = udp_new();
    if (*udp == NULL) {
        return -ENOMEM;
    }
    udp_recv(*udp, cb_udp_recv, (void *)cb_data);
    return ERR_OK;
}

/**
 * [Descrição]: Libera um socket UDP previamente criado.
 * [Parâmetros]: 
 *  - struct udp_pcb **udp: ponteiro para o socket a ser liberado;
 * [Notas]: O ponteiro será setado como NULL após liberação.
 */
static void dns_socket_free(struct udp_pcb **udp) {
    if (*udp != NULL) {
        udp_remove(*udp);
        *udp = NULL;
    }
}

/**
 * [Descrição]: Associa o socket UDP a um IP e porta.
 * [Parâmetros]: 
 *  - struct udp_pcb **udp: ponteiro para o socket UDP;
 *  - uint32_t ip: IP local para binding (em big endian);
 *  - uint16_t port: porta para binding (ex: 53 para DNS);
 * [Notas]: Em caso de erro, o socket será liberado automaticamente.
 */
static int dns_socket_bind(struct udp_pcb **udp, uint32_t ip, uint16_t port) {
    ip_addr_t addr;
    IP4_ADDR(&addr, ip >> 24 & 0xff, ip >> 16 & 0xff, ip >> 8 & 0xff, ip & 0xff);
    err_t err = udp_bind(*udp, &addr, port);
    if (err != ERR_OK) {
        ERROR_printf("dns failed to bind to port %u: %d\n", port, err);
        dns_socket_free(udp);
        return err;
    }
    return ERR_OK;
}

#if DUMP_DATA
static void dump_bytes(const uint8_t *bptr, uint32_t len) {
    unsigned int i = 0;

    for (i = 0; i < len;) {
        if ((i & 0x0f) == 0) {
            printf("\n");
        } else if ((i & 0x07) == 0) {
            printf(" ");
        }
        printf("%02x ", bptr[i++]);
    }
    printf("\n");
}
#endif

/**
 * [Descrição]: Envia uma mensagem DNS para o endereço e porta indicados.
 * [Parâmetros]: 
 *  - struct udp_pcb **udp: ponteiro para o socket UDP;
 *  - const void *buf: buffer de dados a ser enviado;
 *  - size_t len: tamanho do buffer;
 *  - const ip_addr_t *dest: endereço IP de destino;
 *  - uint16_t port: porta de destino;
 * [Notas]: Retorna o número de bytes enviados ou erro.
 */
static int dns_socket_sendto(struct udp_pcb **udp, const void *buf, size_t len, const ip_addr_t *dest, uint16_t port) {
    if (len > 0xffff) {
        len = 0xffff;
    }

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL) {
        ERROR_printf("DNS: Failed to send message out of memory\n");
        return -ENOMEM;
    }

    memcpy(p->payload, buf, len);
    err_t err = udp_sendto(*udp, p, dest, port);

    pbuf_free(p);

    if (err != ERR_OK) {
        ERROR_printf("DNS: Failed to send message %d\n", err);
        return err;
    }

#if DUMP_DATA
    dump_bytes(buf, len);
#endif
    return len;
}

/**
 * [Descrição]: Callback que processa mensagens DNS recebidas e envia uma resposta.
 * [Parâmetros]: 
 *  - void *arg: ponteiro para `dns_server_t`;
 *  - struct udp_pcb *upcb: ponteiro para socket UDP;
 *  - struct pbuf *p: buffer com os dados recebidos;
 *  - const ip_addr_t *src_addr: endereço IP do remetente;
 *  - u16_t src_port: porta de origem do remetente;
 * [Notas]: Responde com o IP local a todas as requisições DNS válidas.
 */
static void dns_server_process(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *src_addr, u16_t src_port) {
    dns_server_t *d = arg;
    DEBUG_printf("dns_server_process %u\n", p->tot_len);

    uint8_t dns_msg[MAX_DNS_MSG_SIZE];
    dns_header_t *dns_hdr = (dns_header_t*)dns_msg;

    size_t msg_len = pbuf_copy_partial(p, dns_msg, sizeof(dns_msg), 0);
    if (msg_len < sizeof(dns_header_t)) {
        goto ignore_request;
    }

#if DUMP_DATA
    dump_bytes(dns_msg, msg_len);
#endif

    uint16_t flags = lwip_ntohs(dns_hdr->flags);
    uint16_t question_count = lwip_ntohs(dns_hdr->question_count);

    DEBUG_printf("len %d\n", msg_len);
    DEBUG_printf("dns flags 0x%x\n", flags);
    DEBUG_printf("dns question count 0x%x\n", question_count);

    // flags from rfc1035
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    // |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    // +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    // Check QR indicates a query
    if (((flags >> 15) & 0x1) != 0) {
        DEBUG_printf("Ignoring non-query\n");
        goto ignore_request;
    }

    // Check for standard query
    if (((flags >> 11) & 0xf) != 0) {
        DEBUG_printf("Ignoring non-standard query\n");
        goto ignore_request;
    }

    // Check question count
    if (question_count < 1) {
        DEBUG_printf("Invalid question count\n");
        goto ignore_request;
    }

    // Print the question
    DEBUG_printf("question: ");
    const uint8_t *question_ptr_start = dns_msg + sizeof(dns_header_t);
    const uint8_t *question_ptr_end = dns_msg + msg_len;
    const uint8_t *question_ptr = question_ptr_start;
    while(question_ptr < question_ptr_end) {
        if (*question_ptr == 0) {
            question_ptr++;
            break;
        } else {
            if (question_ptr > question_ptr_start) {
                DEBUG_printf(".");
            }
            int label_len = *question_ptr++;
            if (label_len > 63) {
                DEBUG_printf("Invalid label\n");
                goto ignore_request;
            }
            DEBUG_printf("%.*s", label_len, question_ptr);
            question_ptr += label_len;
        }
    }
    DEBUG_printf("\n");

    // Check question length
    if (question_ptr - question_ptr_start > 255) {
        DEBUG_printf("Invalid question length\n");
        goto ignore_request;
    }

    // Skip QNAME and QTYPE
    question_ptr += 4;

    // Generate answer
    uint8_t *answer_ptr = dns_msg + (question_ptr - dns_msg);
    *answer_ptr++ = 0xc0; // pointer
    *answer_ptr++ = question_ptr_start - dns_msg; // pointer to question
    
    *answer_ptr++ = 0;
    *answer_ptr++ = 1; // host address

    *answer_ptr++ = 0;
    *answer_ptr++ = 1; // Internet class

    *answer_ptr++ = 0;
    *answer_ptr++ = 0;
    *answer_ptr++ = 0;
    *answer_ptr++ = 60; // ttl 60s

    *answer_ptr++ = 0;
    *answer_ptr++ = 4; // length
    memcpy(answer_ptr, &d->ip.addr, 4); // use our address
    answer_ptr += 4;

    dns_hdr->flags = lwip_htons(
                0x1 << 15 | // QR = response
                0x1 << 10 | // AA = authoritative
                0x1 << 7);   // RA = authenticated
    dns_hdr->question_count = lwip_htons(1);
    dns_hdr->answer_record_count = lwip_htons(1);
    dns_hdr->authority_record_count = 0;
    dns_hdr->additional_record_count = 0;

    // Send the reply
    DEBUG_printf("Sending %d byte reply to %s:%d\n", answer_ptr - dns_msg, ipaddr_ntoa(src_addr), src_port);
    dns_socket_sendto(&d->udp, &dns_msg, answer_ptr - dns_msg, src_addr, src_port);

ignore_request:
    pbuf_free(p);
}

/**
 * [Descrição]: Inicializa o servidor DNS.
 * [Parâmetros]: 
 *  - dns_server_t *d: ponteiro para a estrutura do servidor DNS;
 *  - ip_addr_t *ip: endereço IP a ser utilizado como resposta;
 * [Notas]: Associa o socket à porta 53 e registra o callback.
 */
void dns_server_init(dns_server_t *d, ip_addr_t *ip) {
    if (dns_socket_new_dgram(&d->udp, d, dns_server_process) != ERR_OK) {
        ERROR_printf("dns server: failed to create socket\n");
        return;
    }
    
    if (dns_socket_bind(&d->udp, ip4_addr_get_u32(ip_2_ip4(ip)), PORT_DNS_SERVER) != ERR_OK) {
        ERROR_printf("dns server: failed to bind socket\n");
        return;
    }
    
    ip_addr_copy(d->ip, *ip);
    DEBUG_printf("dns server: successfully started on port %d\n", PORT_DNS_SERVER);
}

/**
 * [Descrição]: Finaliza o servidor DNS e libera recursos.
 * [Parâmetros]: 
 *  - dns_server_t *d: ponteiro para a estrutura do servidor DNS;
 * [Notas]: Libera o socket UDP.
 */
void dns_server_deinit(dns_server_t *d) {
    dns_socket_free(&d->udp);
}
