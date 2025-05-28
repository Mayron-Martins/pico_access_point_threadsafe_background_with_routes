/**
 * -----------------------------------------------
 * Author: Mayron Martins da Silva
 * Arquivo: routes.c 
 * Projeto: pico_access_point_with_routes
 * -----------------------------------------------
 * 
 * Descrição: 
 *      Este módulo define as rotas tratadas pelo servidor HTTP.
 *      Ele interpreta o início da requisição HTTP recebida e
 *      define a resposta apropriada em HTML, com status e tipo.
 *      A resposta é montada diretamente a partir de strings C.
 */
#include "routes.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/**
 * [Descrição]: Gera e retorna o conteúdo HTML completo como string alocada.
 * [Parâmetros]: 
 *  - nenhum
 * [Notas]: 
 *  - A string retornada é alocada dinamicamente com `malloc`.
 *  - O chamador é responsável por liberar a memória alocada com `free`.
 */
static char* get_html_content() {
    const char* html_template =
        "<!DOCTYPE html>\n"
        "<html lang=\"pt-BR\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>Minha Rota Inicial (Embutida)</title>\n"
        "    <style>\n"
        "        body {\n"
        "            font-family: Arial, sans-serif;\n"
        "            background-color: #f4f4f4;\n"
        "            color: #333;\n"
        "            margin: 0;\n"
        "            display: flex;\n"
        "            justify-content: center;\n"
        "            align-items: center;\n"
        "            min-height: 100vh;\n"
        "            flex-direction: column;\n"
        "        }\n"
        "        .container {\n"
        "            background-color: #fff;\n"
        "            padding: 30px;\n"
        "            border-radius: 8px;\n"
        "            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);\n"
        "            text-align: center;\n"
        "        }\n"
        "        h1 {\n"
        "            color: #0056b3;\n"
        "        }\n"
        "        p {\n"
        "            line-height: 1.6;\n"
        "        }\n"
        "        .footer {\n"
        "            margin-top: 20px;\n"
        "            font-size: 0.8em;\n"
        "            color: #666;\n"
        "        }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <h1>Bem-vindo à Rota Inicial!</h1>\n"
        "        <p>Este é o conteúdo da sua página inicial, servido diretamente do código C.</p>\n"
        "        <p>Sem arquivos HTML externos!</p>\n"
        "    </div>\n"
        "    <div class=\"footer\">\n"
        "        <p>&copy; 2025 Minha Aplicação. Todos os direitos reservados.</p>\n"
        "    </div>\n"
        "</body>\n"
        "</html>\n";

    size_t len = strlen(html_template);
    char* html_content = (char*)malloc(len + 1);
    if (html_content) {
        strcpy(html_content, html_template);
    }
    return html_content;
}

/**
 * [Descrição]: Define a resposta HTTP com base no HTML fornecido.
 * [Parâmetros]: 
 *  - http_response_t *response: ponteiro para a estrutura de resposta;
 *  - char *html_content: conteúdo HTML alocado dinamicamente;
 * [Notas]: 
 *  - Se o conteúdo for válido, define status 200 e cabeçalhos.
 *  - Em caso de erro, retorna status 500 e corpo com mensagem de erro.
 */
static void set_response(http_response_t *response, char *html_content){
    if (html_content) {
        set_response_status(response, 200, "OK");
        add_response_header(response, "Content-Type", "text/html; charset=utf-8");
        add_response_header(response, "Content-Length", "%lu", strlen(html_content));
        set_response_body(response, html_content);
    } else {
        set_response_status(response, 500, "Internal Server Error");
        add_response_header(response, "Content-Type", "text/plain");
        set_response_body(response, "Erro ao carregar a página inicial.");
    }
}

/**
 * [Descrição]: Manipula a rota com base na requisição HTTP recebida.
 * [Parâmetros]: 
 *  - const char *request: string contendo os headers HTTP da requisição;
 *  - http_response_t *response: estrutura onde será definida a resposta HTTP;
 * [Notas]: 
 *  - Suporta as seguintes rotas:
 *      - `GET /` ou `GET /index`: retorna a página inicial com HTML embutido.
 *      - Qualquer outra rota resulta em erro 404 com texto simples.
 */
void handle_route(const char *request, http_response_t *response) {
    if (strncmp(request, "GET / ", 6) == 0 || strncmp(request, "GET /index", 10) == 0) {
        char *html_content = get_html_content();
        set_response_body(response, html_content);

    } else {
        set_response_status(response, 404, "Not Found");
        add_response_header(response, "Content-Type", "text/plain");
        set_response_body(response, "Página não encontrada.");
    }
}
