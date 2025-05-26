/**
 * -----------------------------------------------
 * Arquivo: http_utils.c 
 * Projeto: pico_access_point_with_routes
 * -----------------------------------------------
 * 
 * Descrição: 
 *      Este módulo fornece utilitários auxiliares para 
 *      formatação e geração de respostas HTTP.
 *      Atualmente, implementa a função de construção de cabeçalhos HTTP.
 */
#include "http_utils.h"
#include <stdio.h>

/**
 * [Descrição]: Constrói os cabeçalhos HTTP padrão para uma resposta.
 * [Parâmetros]: 
 *  - char *buffer: buffer onde os cabeçalhos serão escritos;
 *  - size_t max_len: tamanho máximo do buffer;
 *  - int status_code: código de status HTTP (ex: 200, 404);
 *  - const char *content_type: tipo do conteúdo (ex: "text/html");
 *  - size_t content_length: tamanho do corpo da resposta em bytes;
 * [Notas]: 
 *  - Retorna o número de caracteres escritos no buffer.
 *  - A string gerada termina com dois "\n" para indicar o fim dos cabeçalhos.
 */
int build_http_headers(char *buffer, size_t max_len, int status_code, const char *content_type, size_t content_length) {
    return snprintf(buffer, max_len,
        "HTTP/1.1 %d OK\n"
        "Content-Length: %zu\n"
        "Content-Type: %s\n"
        "Connection: close\n\n",
        status_code, content_length, content_type);
}
