/**
 * -----------------------------------------------
 * Author: Mayron Martins da Silva
 * Arquivo: http_response.c 
 * Projeto: epico_access_point_with_routes
 * -----------------------------------------------
 * 
 * Descrição: 
 *      Este módulo fornece funções utilitárias para criar,
 *      configurar e liberar uma estrutura de resposta HTTP,
 *      incluindo status, cabeçalhos e corpo.
 */
#include "http_response.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/**
 * [Descrição]: Inicializa uma estrutura `http_response_t` com valores padrão.
 * [Parâmetros]: 
 *  - http_response_t *response: ponteiro para a estrutura a ser inicializada;
 * [Notas]: 
 *  - Zera os campos e define ponteiros como NULL.
 */
void init_http_response(http_response_t *response) {
    if (response) {
        response->status_code = 0;
        response->status_message = NULL;
        response->headers[0] = '\0';
        response->headers_len = 0;
        response->body = NULL;
        response->body_len = 0;
    }
}

/**
 * [Descrição]: Define o status da resposta HTTP.
 * [Parâmetros]: 
 *  - http_response_t *response: ponteiro para a resposta;
 *  - int code: código de status (ex: 200, 404);
 *  - const char *message: mensagem opcional associada (ex: "OK");
 * [Notas]: Apenas armazena os valores, sem validar o código.
 */
void set_response_status(http_response_t *response, int code, const char *message) {
    if (response) {
        response->status_code = code;
        response->status_message = message;
    }
}

/**
 * [Descrição]: Adiciona um cabeçalho HTTP à resposta com formatação.
 * [Parâmetros]: 
 *  - http_response_t *response: ponteiro para a resposta;
 *  - const char *key: nome do cabeçalho (ex: "Content-Type");
 *  - const char *format: string de formatação (como printf);
 *  - ...: argumentos adicionais para a formatação;
 * [Notas]: 
 *  - Usa buffer temporário interno para formatar o valor.
 *  - Garante que não ultrapasse o tamanho do buffer de cabeçalhos.
 */
void add_response_header(http_response_t *response, const char *key, const char *format, ...) {
    if (response) {
        char value_buffer[256]; // Buffer temporário para o valor do cabeçalho formatado
        va_list args;
        va_start(args, format);
        int written = vsnprintf(value_buffer, sizeof(value_buffer), format, args);
        va_end(args);

        size_t remaining_space = sizeof(response->headers) - response->headers_len;

        if (written > 0 && written < sizeof(value_buffer)) {
            int written_result = snprintf(response->headers + response->headers_len,
                                          remaining_space,
                                          "%s: %s\r\n",
                                          key, value_buffer);
            if (written_result > 0 && written_result < remaining_space) {
                response->headers_len += written_result;
            } else if (written_result >= remaining_space) { //Excedeu o espaço disponível.
                printf("WARNING: Header truncated or too long for response.headers buffer.\n");
                response->headers_len = sizeof(response->headers) - 1; // Preencher o buffer até o final
            }
        }
    }
}

/**
 * [Descrição]: Define o corpo da resposta HTTP, alocando memória dinamicamente.
 * [Parâmetros]: 
 *  - http_response_t *response: ponteiro para a resposta;
 *  - const char *body: conteúdo a ser usado como corpo da resposta;
 * [Notas]: 
 *  - Substitui o corpo anterior, se houver, liberando a memória anterior.
 *  - Em caso de falha na alocação, `body` é setado como NULL.
 */
void set_response_body(http_response_t *response, const char *body) {
    if (response) {
        // Liberar corpo anterior, se houver, para evitar vazamento
        if (response->body) {
            free(response->body);
            response->body = NULL;
        }

        if (body) {
            response->body_len = strlen(body);
            // Alocar nova memória e copiar o conteúdo
            response->body = (char *)malloc(response->body_len + 1);
            if (response->body) {
                strcpy(response->body, body);
            } else {
                response->body_len = 0;
                // Tratar erro de alocação de memória se necessário
            }
        } else {
            response->body_len = 0;
            response->body = NULL;
        }
    }
}

/**
 * [Descrição]: Libera os recursos associados a uma resposta HTTP.
 * [Parâmetros]: 
 *  - http_response_t *response: ponteiro para a resposta a ser limpa;
 * [Notas]: 
 *  - Libera apenas o campo `body`, que pode ter sido alocado dinamicamente.
 *  - Redefine os demais campos para valores seguros.
 */
void free_http_response(http_response_t *response) {
    if (response) {
        // Apenas libera o corpo se ele foi alocado dinamicamente
        if (response->body) {
            free(response->body);
            response->body = NULL;
        }
        // Redefinir outros campos para um estado inicial seguro
        response->status_code = 0;
        response->status_message = NULL;
        response->headers[0] = '\0';
        response->headers_len = 0;
        response->body_len = 0;
    }
}