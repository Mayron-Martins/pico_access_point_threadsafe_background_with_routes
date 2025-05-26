#ifndef ROUTES_H
#define ROUTES_H

#include "http_utils.h"
#include "http_response.h"

void handle_route(const char *request, http_response_t *response);

#endif // ROUTES_H
