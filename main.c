/**
 * Author: Mayron Martins da Silva
 * Descrição: Projeto para criação de access point utilizando a SDK do Raspberry Pi Pico W,
 * com criação de servidor HTTP, definição de rotas e execução de página HTML com CSS. 
 */
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "setup.h"


int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    //Iniciar configurações de rede (DNS, DHCP e HTTP)
    if(network_setup()) return 1;

    while (true) {
        tight_loop_contents();
        sleep_ms(1);
    }

    cyw43_arch_deinit();
    return 0;
}
