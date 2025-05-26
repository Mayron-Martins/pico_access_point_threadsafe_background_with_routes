#ifndef HTTP_RESPONSES_H
#define HTTP_RESPONSES_H

#include <stddef.h>

#define MAX_HEADERS_SIZE 1024

typedef struct {
    int status_code;
    const char *status_message;
    char headers[MAX_HEADERS_SIZE];
    size_t headers_len;
    char *body;
    size_t body_len;
} http_response_t;

void init_http_response(http_response_t *response);

void set_response_status(http_response_t *response, int code, const char *message);

void add_response_header(http_response_t *response, const char *key, const char *format, ...);

void set_response_body(http_response_t *response, const char *body);

void free_http_response(http_response_t *response);


#endif // HTTP_RESPONSES_H