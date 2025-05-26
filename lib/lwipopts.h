#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

// =============================================
// 1. Configurações Básicas do Sistema
// =============================================
// allow override in some examples
#ifndef NO_SYS
#define NO_SYS                      1           // Sistema operacional não utilizado
#endif
#ifndef LWIP_SOCKET
#define LWIP_SOCKET                 0           // Desabilita API de sockets
#endif
#if PICO_CYW43_ARCH_POLL
#define MEM_LIBC_MALLOC             1           // Usa malloc padrão quando em modo poll
#else
#define MEM_LIBC_MALLOC             0           // Não usa malloc padrão (incompatível)
#endif
#define MEM_ALIGNMENT               4           // Alinhamento de memória (4 bytes)
#define LWIP_NETCONN                0           // Desabilita API Netconn

// =============================================
// 2. Configurações de Memória e Buffers
// =============================================
#define MEM_SIZE                    4000        // Tamanho total do heap de memória
#define MEMP_NUM_TCP_SEG            32          // Número de segmentos TCP em buffer
#define MEMP_NUM_ARP_QUEUE          10          // Tamanho da fila ARP
#define PBUF_POOL_SIZE              24          // Número de buffers na pool PBUF
#define LWIP_NETIF_TX_SINGLE_PBUF   1           // Usa single PBUF para transmissão

// =============================================
// 3. Protocolos Habilitados/Desabilitados
// =============================================
#define LWIP_ARP                    1           // Habilita ARP
#define LWIP_ETHERNET               1           // Habilita suporte a Ethernet
#define LWIP_ICMP                   1           // Habilita ICMP (ping)
#define LWIP_RAW                    1           // Habilita RAW sockets
#define LWIP_IPV4                   1           // Habilita IPv4
#define LWIP_TCP                    1           // Habilita TCP
#define LWIP_UDP                    1           // Habilita UDP
#define LWIP_DNS                    1           // Habilita resolução DNS
#define LWIP_DHCP                   1           // Habilita cliente DHCP
#define LWIP_TCP_KEEPALIVE          1           // Habilita keepalive TCP

// =============================================
// 4. Configurações de TCP/IP
// =============================================
#define TCP_MSS                     1460        // Maximum Segment Size TCP
#define TCP_WND                     (8 * TCP_MSS) // Janela TCP
#define TCP_SND_BUF                 (8 * TCP_MSS) // Buffer de envio TCP
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))
#define DHCP_DOES_ARP_CHECK         0           // Desabilita verificação ARP no DHCP
#define LWIP_DHCP_DOES_ACD_CHECK    0           // Desabilita verificação ACD no DHCP

// =============================================
// 5. Callbacks e Funcionalidades de Rede
// =============================================
#define LWIP_NETIF_STATUS_CALLBACK  1           // Habilita callbacks de status
#define LWIP_NETIF_LINK_CALLBACK    1           // Habilita callbacks de link
#define LWIP_NETIF_HOSTNAME         1           // Habilita hostname na interface

// =============================================
// 6. Configurações de Checksum e Otimizações
// =============================================
#define LWIP_CHKSUM_ALGORITHM       3           // Algoritmo de checksum (3 = otimizado)

// =============================================
// 7. Configurações de Estatísticas e Debug
// =============================================
#ifndef NDEBUG
#define LWIP_DEBUG                  1           // Habilita debug geral
#define LWIP_STATS                  1           // Habilita estatísticas
#define LWIP_STATS_DISPLAY          1           // Habilita display de estatísticas
#endif

// Configurações individuais de debug (todas desligadas)
#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF
#define MEMP_DEBUG                  LWIP_DBG_OFF
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF

// =============================================
// 8. Configurações de Monitoramento (Desativadas)
// =============================================
#define MEM_STATS                   0           // Desabilita estatísticas de memória
#define SYS_STATS                   0           // Desabilita estatísticas do sistema
#define MEMP_STATS                  0           // Desabilita estatísticas de mem pools
#define LINK_STATS                  0           // Desabilita estatísticas de link

#endif // __LWIPOPTS_H__