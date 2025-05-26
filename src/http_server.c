/**
 * -----------------------------------------------
 * Arquivo: http_server.c 
 * Projeto: pico_access_point_with_routes
 * -----------------------------------------------
 * 
 * Descrição: 
 *      Este módulo implementa um servidor HTTP básico para o
 *      Raspberry Pi Pico W operando em modo Access Point (AP).
 *      Ele processa requisições TCP recebidas na porta 80,
 *      analisa os headers HTTP, trata as rotas e envia
 *      respostas com base no conteúdo definido em `routes.c`.
 */

#include "http_server.h"
#include "http_utils.h"
#include "routes.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define HTTP_PORT 80

typedef struct {
    struct tcp_pcb *client_pcb;
    char headers[512];
    int header_len;
    const char *body;
    int body_len;
} connection_state_t;

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

/**
 * [Descrição]: Fecha uma conexão TCP e libera a memória alocada para o estado.
 * [Parâmetros]: 
 *  - struct tcp_pcb *tpcb: controle de bloco do protocolo TCP;
 *  - connection_state_t *state: ponteiro para estado da conexão;
 * [Notas]: A função também limpa os argumentos de callback.
 */
static void close_connection(struct tcp_pcb *tpcb, connection_state_t *state) {
    if (state) free(state);
    tcp_arg(tpcb, NULL);
    tcp_close(tpcb);
}

/**
 * [Descrição]: Callback executado após envio completo dos dados.
 * [Parâmetros]: 
 *  - void *arg: ponteiro para o estado da conexão;
 *  - struct tcp_pcb *tpcb: ponteiro para o socket TCP;
 *  - u16_t len: número de bytes enviados;
 * [Notas]: Fecha a conexão após finalização do envio.
 */
static err_t on_sent_close_connection(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    close_connection(tpcb, (connection_state_t *)arg);
    return ERR_OK;
}

/**
 * [Descrição]: Callback de envio TCP quando o pacote foi entregue com sucesso.
 * [Parâmetros]: 
 *  - void *arg: ponteiro para o estado da conexão;
 *  - struct tcp_pcb *tpcb: socket do cliente;
 *  - u16_t len: número de bytes enviados;
 * [Notas]: Envia o corpo da resposta, se ainda não tiver sido enviado.
 */
static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    connection_state_t *state = (connection_state_t *)arg;
    // In your routes.c, the body is a string literal.
    // If you were allocating the body dynamically, you would free it here.
    // For string literals, setting to NULL is fine.
    if (state->body) {
        // If there's still data to send, send it. This logic is a bit
        // simplified given your use of on_sent_close_connection.
        // For larger bodies, you might need more sophisticated chunking.
        err_t err = tcp_write(tpcb, state->body, state->body_len, TCP_WRITE_FLAG_COPY);
        state->body = NULL; // Indicate body has been sent
        return err;
    }
    close_connection(tpcb, state); // Close connection after all data is sent
    return ERR_OK;
}

/**
 * [Descrição]: Callback chamado quando dados são recebidos do cliente.
 * [Parâmetros]: 
 *  - void *arg: ponteiro para o estado da conexão;
 *  - struct tcp_pcb *tpcb: socket do cliente;
 *  - struct pbuf *p: buffer contendo os dados recebidos;
 *  - err_t err: código de erro, se houver;
 * [Notas]: Trata a requisição HTTP, prepara e envia a resposta.
 */
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        // Connection closed by client
        close_connection(tpcb, (connection_state_t *)arg);
        return ERR_OK;
    }

    // Check for errors
    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    connection_state_t *state = (connection_state_t *)arg;

    // Ensure we don't overflow our headers buffer
    size_t copy_len = p->tot_len < sizeof(state->headers) ? p->tot_len : sizeof(state->headers) - 1;
    pbuf_copy_partial(p, state->headers, copy_len, 0);
    state->headers[copy_len] = '\0'; // Null-terminate the received data

    http_response_t response;
    // Initialize content_type to avoid uninitialized data issues if routes.c doesn't set it
    strcpy(response.content_type, "text/plain");
    response.status_code = 500; // Default to internal server error
    response.body = "Internal Server Error";
    response.body_len = strlen(response.body);

    handle_route(state->headers, &response);

    state->body = response.body;
    state->body_len = response.body_len;
    state->header_len = build_http_headers(state->headers, sizeof(state->headers),
                                            response.status_code, response.content_type, response.body_len);

    err_t wr_err = tcp_write(tpcb, state->headers, state->header_len, TCP_WRITE_FLAG_COPY);
    if (wr_err != ERR_OK) {
        printf("Error writing HTTP headers: %d\n", wr_err);
        close_connection(tpcb, state);
        pbuf_free(p);
        return wr_err;
    }

    // Only send the body if there is one
    if (state->body && state->body_len > 0) {
        wr_err = tcp_write(tpcb, state->body, state->body_len, TCP_WRITE_FLAG_COPY);
        if (wr_err != ERR_OK) {
            printf("Error writing HTTP body: %d\n", wr_err);
            close_connection(tpcb, state);
            pbuf_free(p);
            return wr_err;
        }
    }

    tcp_output(tpcb); // send data immediately

    // Set callback to close connection after data is sent
    tcp_sent(tpcb, on_sent_close_connection);
    // Important: Acknowledge the received data
    tcp_recved(tpcb, p->tot_len);

    pbuf_free(p);
    return ERR_OK;
}

/**
 * [Descrição]: Callback chamado ao aceitar uma nova conexão TCP.
 * [Parâmetros]: 
 *  - void *arg: argumento opcional (não usado aqui);
 *  - struct tcp_pcb *newpcb: novo socket para a conexão aceita;
 *  - err_t err: código de erro, se houver;
 * [Notas]: Aloca e inicializa o estado da conexão, registrando os callbacks.
 */
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK) {
        printf("TCP accept error: %d\n", err);
        return err;
    }

    connection_state_t *state = calloc(1, sizeof(connection_state_t));
    if (!state) {
        printf("Failed to allocate connection state\n");
        return ERR_MEM;
    }

    state->client_pcb = newpcb;
    tcp_arg(newpcb, state);
    tcp_recv(newpcb, tcp_server_recv);
    tcp_sent(newpcb, tcp_server_sent);
    // You might also want a poll callback for timeouts, similar to main2.c
    // tcp_poll(newpcb, tcp_server_poll, POLL_TIME_S * 2);
    // And an error callback
    // tcp_err(newpcb, tcp_server_err);
    return ERR_OK;
}

/**
 * [Descrição]: Inicia o servidor HTTP e escuta por conexões na porta 80.
 * [Parâmetros]: 
 *  - nenhum
 * [Notas]: 
 *      - Deve ser chamado após a inicialização da rede no modo AP.
 *      - Usa `tcp_accept` para registrar o callback de conexões.
 */
void http_server_start(void) {
    // IMPORTANT: Remove cyw43_arch_init() and cyw43_arch_enable_sta_mode()
    // These should be handled once in main.c before calling network_setup.

    printf("HTTP server starting on port %d\n", HTTP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        printf("Failed to create PCB\n");
        return; // No return value to indicate error in void function, consider changing signature
    }

    // Bind to the correct interface for AP mode (NULL for any interface, including AP)
    if (tcp_bind(pcb, IP_ANY_TYPE, HTTP_PORT) != ERR_OK) { // Use IP_ANY_TYPE to bind to any available interface
        printf("Failed to bind TCP\n");
        tcp_close(pcb); // Close PCB if binding fails
        return;
    }

    struct tcp_pcb *listen_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!listen_pcb) {
        printf("Failed to listen for TCP connections\n");
        tcp_close(pcb); // Close PCB if listen fails
        return;
    }
    // No specific argument needed for the server_pcb if not maintaining server-wide state
    // tcp_arg(listen_pcb, NULL); // Or pass a pointer to a server state struct if needed
    tcp_accept(listen_pcb, tcp_server_accept);
}