#include "router.h"
#include "handlers.h"
#include <string.h>
#include <stdio.h>

/* -----------------------------------------------------------------------
 * Route table
 * --------------------------------------------------------------------- */

typedef void (*HandlerFn)(const ParsedRequest *, char *, int);

//this rep a single route mapping
typedef struct {
    const char *method;   //http method string or null for any
    const char *path;     // exact uri path to match
    HandlerFn   handler;    //the function to execute once a match
} Route;


/*
    static routing table 
    routes are evaluated sequentially, first match found will be executed
    null marks end of table
*/
static const Route route_table[] = {
    { "GET",  "/",        handle_index    },
    { "GET",  "/echo",    handle_echo     },
    { "POST", "/echo",    handle_post_echo},
    { "GET",  "/headers", handle_headers  },
    { "GET",  "/health",  handle_health   },
    { NULL,   NULL,       NULL            }   /* sentinel */
};

/*
    follows thre levels:
    1.path and method match: executes handler
    2. path matches but method doesnt: return 405
    3. no match: return 404 not found
*/
void router_dispatch(const ParsedRequest *req, char *out, int out_size) {
    if (!req || !out || out_size <= 0) return;

    int path_matched = 0;

    //go through static route table until sentinel is reached(null)
    for (int i = 0; route_table[i].path != NULL; i++) {
        if (strcmp(req->path, route_table[i].path) != 0) continue;

        path_matched = 1;

        if (route_table[i].method == NULL ||
            strcmp(req->method, route_table[i].method) == 0) {
            route_table[i].handler(req, out, out_size);
            return;
        }
    }
    //check if method matches or if the route allows any method
    if (path_matched) {
        handle_method_not_allowed(req, out, out_size);
    } else {
        handle_not_found(req, out, out_size);
    }
}