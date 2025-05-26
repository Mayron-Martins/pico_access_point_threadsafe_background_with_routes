#ifndef HTTP_RESPONSES_H
#define HTTP_RESPONSES_H

#include <stddef.h>

typedef struct {
    const char *body;
    size_t body_len;
    char content_type[64];
    int status_code;
} http_response_t;


#endif // HTTP_RESPONSES_H