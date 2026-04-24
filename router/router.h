#ifndef ROUTER_H
#define ROUTER_H

#include "../parser/request.h"

void router_dispatch(const HttpRequest *req, char *out, int out_size);

#endif /* ROUTER_H */

