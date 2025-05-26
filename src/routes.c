/**
 * -----------------------------------------------
 * Arquivo: routes.c 
 * Projeto: pico_access_point_with_routes
 * -----------------------------------------------
 * 
 * Descrição: 
 *      Este módulo define as rotas tratadas pelo servidor HTTP.
 *      Ele interpreta o início da requisição HTTP recebida e
 *      define a resposta apropriada em HTML, com status e tipo.
 */
#include "routes.h"
#include <string.h>
#include <stdio.h>

/**
 * [Descrição]: Manipula a rota com base na requisição HTTP recebida.
 * [Parâmetros]: 
 *  - const char *request: string contendo os headers HTTP da requisição;
 *  - http_response_t *response: estrutura onde será definida a resposta HTTP;
 * [Notas]: 
 *  - Suporta as seguintes rotas:
 *      - `GET /` ou `GET /index`: retorna página inicial.
 *      - `GET /ligar`: retorna página de confirmação e pode ativar GPIO.
 *      - `GET /desligar`: retorna página de confirmação e pode desativar GPIO.
 *      - Qualquer outra rota resulta em erro 404.
 */
void handle_route(const char *request, http_response_t *response) {
    if (strncmp(request, "GET / ", 6) == 0 || strncmp(request, "GET /index", 10) == 0) {
        response->body = "<html><body><h1>Página Inicial</h1></body></html>";
        response->body_len = strlen(response->body);
        strcpy(response->content_type, "text/html; charset=utf-8");
        response->status_code = 200;

    } else if (strncmp(request, "GET /ligar", 10) == 0) {
        response->body = "<html><body><h1>Dispositivo Ligado</h1></body></html>";
        response->body_len = strlen(response->body);
        strcpy(response->content_type, "text/html; charset=utf-8");
        response->status_code = 200;

        // Aqui você pode ligar GPIO etc.

    } else if (strncmp(request, "GET /desligar", 13) == 0) {
        response->body = "<html><body><h1>Dispositivo Desligado</h1></body></html>";
        response->body_len = strlen(response->body);
        strcpy(response->content_type, "text/html; charset=utf-8");
        response->status_code = 200;

        // Aqui você pode desligar GPIO etc.

    } else {
        response->body = "<html><body><h1>404 - Página não encontrada</h1></body></html>";
        response->body_len = strlen(response->body);
        strcpy(response->content_type, "text/html; charset=utf-8");
        response->status_code = 404;
    }
}
