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
    // Em seu routes.c, o body é uma string literal.
    // Se você estivesse alocando o body dinamicamente, você o liberaria aqui.
    // Para literais de string, definir como NULL é bom.
    if (state->body) {
        err_t err = tcp_write(tpcb, state->body, state->body_len, TCP_WRITE_FLAG_COPY);
        state->body = NULL;
        return err;
    }
    close_connection(tpcb, state); // Fechar a conexão depois que todos os dados forem enviados
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
        // Conexão fechada pelo cliente
        close_connection(tpcb, (connection_state_t *)arg);
        return ERR_OK;
    }

    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    connection_state_t *state = (connection_state_t *)arg;

    // Assegurar que o buffer de cabeçalhos não fique cheio
    size_t copy_len = p->tot_len < sizeof(state->headers) ? p->tot_len : sizeof(state->headers) - 1;
    pbuf_copy_partial(p, state->headers, copy_len, 0);
    state->headers[copy_len] = '\0'; // Null-terminate the received data

    http_response_t response;
    init_http_response(&response);
    
    handle_route(state->headers, &response);

     // Buffer temporário para a linha de status e cabeçalhos
    char http_response_buffer[MAX_HEADERS_SIZE + 256]; // Cabeçalhos + Linha de Status + \r\n\r\n
    int offset = 0;

    // 1. Linha de Status
    offset += sprintf(http_response_buffer + offset, "HTTP/1.1 %d %s\r\n",
                      response.status_code, response.status_message);

    // 2. Adicionar cabeçalhos coletados em http_response.headers
    offset += sprintf(http_response_buffer + offset, "%s", response.headers);

    // 3. Adicionar Content-Length (se não foi explicitamente adicionado em routes.c)
    if (!strstr(response.headers, "Content-Length")) {
        offset += sprintf(http_response_buffer + offset, "Content-Length: %zu\r\n", response.body_len);
    }

    // 4. Linha em branco para separar cabeçalhos do corpo
    offset += sprintf(http_response_buffer + offset, "\r\n");

    // Enviar cabeçalhos e a linha de status
    err_t wr_err = tcp_write(tpcb, http_response_buffer, offset, TCP_WRITE_FLAG_COPY);
    if (wr_err != ERR_OK) {
        printf("Error writing HTTP headers: %d\n", wr_err);
        free_http_response(&response); // <<< IMPORTANTE: Liberar memória em caso de erro
        close_connection(tpcb, state);
        pbuf_free(p);
        return wr_err;
    }

    // Enviar o corpo
    if (response.body && response.body_len > 0) {
        wr_err = tcp_write(tpcb, response.body, response.body_len, TCP_WRITE_FLAG_COPY);
        if (wr_err != ERR_OK) {
            printf("Error writing HTTP body: %d\n", wr_err);
            free_http_response(&response); // <<< IMPORTANTE: Liberar memória em caso de erro
            close_connection(tpcb, state);
            pbuf_free(p);
            return wr_err;
        }
    }

    tcp_output(tpcb);

    // Limpeza: Liberar a memória alocada para o corpo da resposta
    free_http_response(&response);

    // Definir retorno de chamada para fechar a conexão depois que os dados forem enviados
    tcp_sent(tpcb, on_sent_close_connection);
    // Importante: Confirme os dados recebidos
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
    // Caso queira um poll callback quanto a timeouts
    // tcp_poll(newpcb, tcp_server_poll, POLL_TIME_S * 2);
    // E o Callback de Erro
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
    printf("HTTP server starting on port %d\n", HTTP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        printf("Failed to create PCB\n");
        return;
    }

    if (tcp_bind(pcb, IP_ANY_TYPE, HTTP_PORT) != ERR_OK) {
        printf("Failed to bind TCP\n");
        tcp_close(pcb);
        return;
    }

    struct tcp_pcb *listen_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!listen_pcb) {
        printf("Failed to listen for TCP connections\n");
        tcp_close(pcb);
        return;
    }

    tcp_accept(listen_pcb, tcp_server_accept);
}