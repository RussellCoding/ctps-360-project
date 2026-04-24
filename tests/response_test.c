#include "./response_test.h"

void test_function_resp_init(void){
    HttpResponse res;

    resp_init(&res, 200); // Initialize response

    TEST_ASSERT_EQUAL_INT(200, res.status_code);
    TEST_ASSERT_NULL(res.body);
    TEST_ASSERT_EQUAL_INT(0, res.body_length);
    TEST_ASSERT_EQUAL_INT(0, res.header_count);
}

/* Free heap memory owned by resp (body if body_owned, nothing else). */
void test_function_resp_free(void){
    HttpResponse res;
    resp_init(&res, 200);
    
    // Set body info
    res.body = malloc(128 + 1); // some arbitrary length
    strcpy(res.body, "sample body");
    res.body_length = strlen("sample body");
    res.body_owned = 1;

    // Free response
    resp_free(&res);
    // Make sure values are in line
    TEST_ASSERT_NULL(res.body);
    TEST_ASSERT_EQUAL_INT(0, res.body_owned);
    TEST_ASSERT_EQUAL_INT(0, res.body_length);
}

/*
 * Add or overwrite a header (case-insensitive name match).
 * Returns 0 on success, -1 if header table is full or args are too long.
 */
void test_function_resp_add_header(void){
    HttpResponse res;
    resp_init(&res, 200);
    
    // Test simple headers
    int result = resp_add_header(&res, "Content-Type", "text/plain");
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, res.header_count);
    TEST_ASSERT_EQUAL_STRING("Content-Type", res.headers[0].name);
    TEST_ASSERT_EQUAL_STRING("text/plain", res.headers[0].value);
    
    // Test overwriting
    result = resp_add_header(&res, "content-type", "application/json");
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, res.header_count); // Still 1 header
    TEST_ASSERT_EQUAL_STRING("application/json", res.headers[0].value);
    
    // Test adding multiple headers
    result = resp_add_header(&res, "X-Custom", "value123");
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(2, res.header_count);
    
    // Test header name too long
    char longName[MAX_RESP_HEADER_NAME + 2];
    memset(longName, 'x', MAX_RESP_HEADER_NAME + 1);
    longName[MAX_RESP_HEADER_NAME + 1] = '\0';
    result = resp_add_header(&res, longName, "value");
    TEST_ASSERT_EQUAL_INT(-1, result); // Should fail
    
    // Test header value too long
    char longValue[MAX_RESP_HEADER_VAL + 2];
    memset(longValue, 'x', MAX_RESP_HEADER_VAL + 1);
    longValue[MAX_RESP_HEADER_VAL + 1] = '\0';
    result = resp_add_header(&res, "X-Long", longValue);
    TEST_ASSERT_EQUAL_INT(-1, result); // Should fail
    
    resp_free(&res);
}

/* Convenience: set Content-Type header. */
void test_function_resp_set_content_type(void){
    HttpResponse res;
    resp_init(&res, 200);
    
    // Test setting content type
    int result = resp_set_content_type(&res, "application/json");
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, res.header_count);
    TEST_ASSERT_EQUAL_STRING("Content-Type", res.headers[0].name);
    TEST_ASSERT_EQUAL_STRING("application/json", res.headers[0].value);
    
    // Test updating content type
    result = resp_set_content_type(&res, "text/html");
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, res.header_count); // Still 1 header
    TEST_ASSERT_EQUAL_STRING("text/html", res.headers[0].value);
    
    resp_free(&res);
}


/*
 * Set body from a C string (makes an internal copy).
 * Also sets Content-Length and Content-Type automatically.
 * Returns 0 on success, -1 on allocation failure.
 */
void test_function_resp_set_body_str(void){
    HttpResponse res;
    resp_init(&res, 200);
    
    // Test setting body from string
    const char *body_text = "{\"ok\":true}";
    int result = resp_set_body_str(&res, body_text, "application/json");
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_NOT_NULL(res.body);
    TEST_ASSERT_EQUAL_STRING(body_text, res.body);
    TEST_ASSERT_EQUAL_INT(strlen(body_text), res.body_length);
    TEST_ASSERT_EQUAL_INT(1, res.body_owned); // We own this memory
    
    // Verify Content-Type and Content-Length headers were set
    const char *content_type = NULL;
    const char *content_length = NULL;
    for (int i = 0; i < res.header_count; i++) {
        if (strcasecmp(res.headers[i].name, "Content-Type") == 0)
            content_type = res.headers[i].value;
        if (strcasecmp(res.headers[i].name, "Content-Length") == 0)
            content_length = res.headers[i].value;
    }
    TEST_ASSERT_NOT_NULL(content_type);
    TEST_ASSERT_EQUAL_STRING("application/json", content_type);
    TEST_ASSERT_NOT_NULL(content_length);
    
    resp_free(&res);
}

/*
 * Set body from raw bytes (makes an internal copy).
 * Returns 0 on success, -1 on allocation failure.
 */
void test_function_resp_set_body_bytes(void){
    HttpResponse res;
    resp_init(&res, 200);
    
    // Test setting body from raw bytes
    const char *data = "\x00\x01\x02\x03\x04"; // Binary data with null bytes
    size_t data_len = 5;
    int result = resp_set_body_bytes(&res, data, data_len, "application/octet-stream");
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_NOT_NULL(res.body);
    TEST_ASSERT_EQUAL_INT(data_len, res.body_length);
    TEST_ASSERT_EQUAL_INT(1, res.body_owned);
    
    // Verify the binary data was copied correctly
    for (size_t i = 0; i < data_len; i++) {
        TEST_ASSERT_EQUAL_INT((unsigned char)data[i], (unsigned char)res.body[i]);
    }
    
    // Verify Content-Type header was set
    int found = 0;
    for (int i = 0; i < res.header_count; i++) {
        if (strcasecmp(res.headers[i].name, "Content-Type") == 0) {
            TEST_ASSERT_EQUAL_STRING("application/octet-stream", res.headers[i].value);
            found = 1;
        }
    }
    TEST_ASSERT_EQUAL_INT(1, found);
    
    resp_free(&res);
}

/*
 * Serialize the response into a heap-allocated byte buffer ready to
 * write to a socket.
 *
 * On success: *out points to the buffer, *out_len is its length.
 *             Caller must free(*out).
 * Returns 0 on success, -1 on allocation failure.
 */
void test_function_resp_serialize(void){
    HttpResponse res;
    resp_init(&res, 200);
    resp_set_body_str(&res, "Hello World", "text/plain");
    resp_add_header(&res, "X-Custom", "value");
    
    char *out = NULL;
    size_t out_len = 0;
    
    int result = resp_serialize(&res, &out, &out_len);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT(out_len > 0);
    
    // Verify the serialized output contains expected parts
    char *buffer_str = malloc(out_len + 1);
    memcpy(buffer_str, out, out_len);
    buffer_str[out_len] = '\0';
    
    TEST_ASSERT(strstr(buffer_str, "200") != NULL); // Status code
    TEST_ASSERT(strstr(buffer_str, "Hello World") != NULL); // Body
    TEST_ASSERT(strstr(buffer_str, "Content-Type") != NULL); // Headers
    TEST_ASSERT(strstr(buffer_str, "text/plain") != NULL);
    TEST_ASSERT(strstr(buffer_str, "X-Custom") != NULL); // Custom header
    TEST_ASSERT(strstr(buffer_str, "value") != NULL);
    
    free(buffer_str);
    free(out);
    resp_free(&res);
}

/* Build a plain-text error response (e.g. 404, 400) in one call. */
void test_function_resp_make_error(void){
    HttpResponse res;
    resp_make_error(&res, 404, "Resource not found");
    
    TEST_ASSERT_EQUAL_INT(404, res.status_code);
    TEST_ASSERT_NOT_NULL(res.body);
    TEST_ASSERT(strstr(res.body, "Resource not found") != NULL);
    
    // Verify Content-Type is text/html
    int found = 0;
    for (int i = 0; i < res.header_count; i++) {
        if (strcasecmp(res.headers[i].name, "Content-Type") == 0) {
            TEST_ASSERT_EQUAL_STRING("text/html; charset=utf-8", res.headers[i].value);
            found = 1;
        }
    }
    TEST_ASSERT_EQUAL_INT(1, found);
    
    resp_free(&res);
}

/* Build a JSON response in one call. */
void test_function_resp_make_json(void){
    HttpResponse res;
    const char *json_body = "{\"status\":\"ok\",\"code\":42}";
    resp_make_json(&res, 200, json_body);
    
    TEST_ASSERT_EQUAL_INT(200, res.status_code);
    TEST_ASSERT_NOT_NULL(res.body);
    TEST_ASSERT_EQUAL_STRING(json_body, res.body);
    
    // Verify Content-Type is application/json
    int found = 0;
    for (int i = 0; i < res.header_count; i++) {
        if (strcasecmp(res.headers[i].name, "Content-Type") == 0) {
            TEST_ASSERT_EQUAL_STRING("application/json", res.headers[i].value);
            found = 1;
        }
    }
    TEST_ASSERT_EQUAL_INT(1, found);
    
    resp_free(&res);
}

/* Build a redirect response (301 or 302). */
void test_function_resp_make_redirect(void){
    // Test 301 permanent redirect
    HttpResponse res;
    resp_make_redirect(&res, 301, "/new-location");
    
    TEST_ASSERT_EQUAL_INT(301, res.status_code);
    
    // Verify Location header
    int found = 0;
    for (int i = 0; i < res.header_count; i++) {
        if (strcasecmp(res.headers[i].name, "Location") == 0) {
            TEST_ASSERT_EQUAL_STRING("/new-location", res.headers[i].value);
            found = 1;
        }
    }
    TEST_ASSERT_EQUAL_INT(1, found);
    
    resp_free(&res);
    
    // Test 302 temporary redirect
    resp_make_redirect(&res, 302, "https://example.com");
    TEST_ASSERT_EQUAL_INT(302, res.status_code);
    
    found = 0;
    for (int i = 0; i < res.header_count; i++) {
        if (strcasecmp(res.headers[i].name, "Location") == 0) {
            TEST_ASSERT_EQUAL_STRING("https://example.com", res.headers[i].value);
            found = 1;
        }
    }
    TEST_ASSERT_EQUAL_INT(1, found);
    
    resp_free(&res);
}

/* Build a 204 No Content response. */
void test_function_resp_make_no_content(void){
    HttpResponse res;
    resp_make_no_content(&res);
    
    TEST_ASSERT_EQUAL_INT(204, res.status_code);
    TEST_ASSERT_NULL(res.body); // No body for 204
    TEST_ASSERT_EQUAL_INT(0, res.body_length);
    
    resp_free(&res);
}


void test_function_resp_status_text(void){
    // Test common status codes
    TEST_ASSERT_EQUAL_STRING("OK", resp_status_text(200));
    TEST_ASSERT_EQUAL_STRING("Created", resp_status_text(201));
    TEST_ASSERT_EQUAL_STRING("No Content", resp_status_text(204));
    TEST_ASSERT_EQUAL_STRING("Moved Permanently", resp_status_text(301));
    TEST_ASSERT_EQUAL_STRING("Found", resp_status_text(302));
    TEST_ASSERT_EQUAL_STRING("Bad Request", resp_status_text(400));
    TEST_ASSERT_EQUAL_STRING("Unauthorized", resp_status_text(401));
    TEST_ASSERT_EQUAL_STRING("Forbidden", resp_status_text(403));
    TEST_ASSERT_EQUAL_STRING("Not Found", resp_status_text(404));
    TEST_ASSERT_EQUAL_STRING("Internal Server Error", resp_status_text(500));
    TEST_ASSERT_EQUAL_STRING("Not Implemented", resp_status_text(501));
    
    // Test unknown status code
    const char *unknown = resp_status_text(999);
    TEST_ASSERT_NOT_NULL(unknown);
    // Should return "Unknown" or similar for unrecognized codes
}

void setUp(void){

}
void tearDown(void){

}

int main(){
    UNITY_BEGIN();
    RUN_TEST(test_function_resp_init);
    RUN_TEST(test_function_resp_free);
    RUN_TEST(test_function_resp_add_header);
    RUN_TEST(test_function_resp_set_content_type);
    RUN_TEST(test_function_resp_set_body_str);
    RUN_TEST(test_function_resp_set_body_bytes);
    RUN_TEST(test_function_resp_serialize);
    RUN_TEST(test_function_resp_make_error);
    RUN_TEST(test_function_resp_make_json);
    RUN_TEST(test_function_resp_make_redirect);
    RUN_TEST(test_function_resp_make_no_content);
    RUN_TEST(test_function_resp_status_text);
    return UNITY_END();
}