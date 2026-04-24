/* GET requests */
const char *raw_get = 
    "GET / HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "Connection: close\r\n"
    "\r\n";

/* GET with path and query string */
const char *raw_get_with_query =
    "GET /api/users?id=123&name=john HTTP/1.1\r\n"
    "Host: api.example.com\r\n"
    "User-Agent: Mozilla/5.0\r\n"
    "Accept: application/json\r\n"
    "Connection: keep-alive\r\n"
    "\r\n";

/* GET with multiple headers */
const char *raw_get_full_headers =
    "GET /index.html HTTP/1.1\r\n"
    "Host: www.example.com\r\n"
    "User-Agent: curl/7.68.0\r\n"
    "Accept: */*\r\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "Connection: keep-alive\r\n"
    "Cache-Control: no-cache\r\n"
    "\r\n";

/* GET HTTP/1.0 */
const char *raw_get_http10 =
    "GET /page.html HTTP/1.0\r\n"
    "Host: example.com\r\n"
    "\r\n";