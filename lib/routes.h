#ifndef ROUTES_H
#define ROUTES_H

#include "http_utils.h"
#include "http_response.h"

typedef struct{
    const char *path;
    size_t length;
} route_info_t;

void handle_route(const char *request, http_response_t *response);

#endif // ROUTES_H
