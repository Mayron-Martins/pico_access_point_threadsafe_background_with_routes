/**
 * Author: Mayron Martins da Silva
 * Descrição: Projeto para criação de access point utilizando a SDK do Raspberry Pi Pico W,
 * com criação de servidor HTTP, definição de rotas e execução de página HTML com CSS. 
 */
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "http_server.h"
#include "setup.h"


int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    if (cyw43_arch_init()) {
        printf("Erro ao iniciar Wi-Fi\n");
        return 1;
    }

    // Cria Access Point com SSID e senha
    cyw43_arch_enable_ap_mode("EVACUATION_ALARM", "senha123", CYW43_AUTH_WPA2_AES_PSK);
    printf("Access Point iniciado: EVACUATION_ALARM\n");

    //Iniciar configurações de rede (DNS, DHCP e HTTP)
    network_setup();

    sleep_ms(2000); //Intervalo para estabilização

    while (true) {
        cyw43_arch_poll(); // processa eventos do Wi-Fi/lwIP
        sleep_ms(1);
    }

    cyw43_arch_deinit();
    return 0;
}
