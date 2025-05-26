#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "lwip/err.h"
#include "lwip/tcp.h"
#include "http_response.h"

void http_server_start(void);

#endif // HTTP_SERVER_H
