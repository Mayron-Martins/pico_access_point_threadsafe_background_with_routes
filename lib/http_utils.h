#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

#include <stddef.h>

int build_http_headers(char *buffer, size_t max_len, int status_code, const char *content_type, size_t content_length);

#endif // HTTP_UTILS_H
