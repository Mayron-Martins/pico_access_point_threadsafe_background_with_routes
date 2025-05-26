/**
 * -----------------------------------------------
 * Arquivo: setup.c 
 * Projeto: pico_access_point_with_routes
 * -----------------------------------------------
 * 
 * Descrição: 
 *      Este módulo centraliza a configuração da interface de rede
 *      no modo Access Point (AP) do Raspberry Pi Pico W, bem como a 
 *      inicialização dos serviços DHCP, DNS e do servidor HTTP.
 */
#include "setup.h"
#include "pico/cyw43_arch.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "http_server.h"

dhcp_server_t dhcp_server;
dns_server_t dns_server;

/**
 * [Descrição]: Configura a interface de rede Wi-Fi em modo Access Point,
 *              define IP estático e inicializa os servidores DHCP, DNS e HTTP.
 * [Parâmetros]: 
 *  - nenhum
 * [Notas]: 
 *  - Define o IP 192.168.4.1 como gateway e endereço do servidor.
 *  - Configura a interface de rede via `cyw43_arch_lwip_begin/end`.
 *  - O servidor HTTP é iniciado após DHCP e DNS.
 */
void network_setup(void) {
    // Configuração manual dos endereços IP
    ip4_addr_t ap_ip, ap_netmask, ap_gw;
    IP4_ADDR(&ap_gw, 192, 168, 4, 1);
    IP4_ADDR(&ap_netmask, 255, 255, 255, 0);
    IP4_ADDR(&ap_ip, 192, 168, 4, 1);

    // Configuração da interface de rede
    cyw43_arch_lwip_begin();
    netif_set_addr(&cyw43_state.netif[CYW43_ITF_AP], &ap_ip, &ap_netmask, &ap_gw);
    netif_set_up(&cyw43_state.netif[CYW43_ITF_AP]);
    cyw43_arch_lwip_end();

    // Inicialização do DHCP
    dhcp_server_init(&dhcp_server, &ap_gw, &ap_netmask);
    printf("DHCP Server initialized\n");
    
    // Inicialização do DNS
    dns_server_init(&dns_server, &ap_gw);
    printf("DNS Server initialized\n");

    // Start HTTP server (moved from main.c)
    http_server_start();
    printf("HTTP Server started\n");
}