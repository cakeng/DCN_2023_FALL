// NXC Data Communications http_functions.h for HTTP server
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11


///// DO NOT MODIFY THIS FILE!! ////

#ifndef HTTP_FUNCTIONS_H
#define HTTP_FUNCTIONS_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "signal.h"
#include "stdint.h"

#define MAX_HTTP_MSG_HEADER_SIZE 4096 // Maximum size of HTTP GET message

int server_engine (int server_port);
int server_routine (int server_port);

// TA implementation of server_engine.
// Use this to check if your implementation is correct.
int server_engine_ans (int server_port);

// TA implementation of server_routine.
// Use this to check if your implementation is correct.
// This function will print the HTTP requests and responses,
// but you DO NOT have to print them for your implementation.
int server_routine_ans (int client_sock);

// Your implementation of server_engine.
int server_engine (int server_port);
// Your implementation of server_routine.
int server_routine (int client_sock);



/// HTTP MANIPULATION ///
// Refer to http_util.c for details.

// All pointer locations should be dynamically allocated.
typedef struct http_field_t
{
    char *field;
    char *val;
} http_field_t;

// All pointer locations should be dynamically allocated.
typedef struct http_t
{
    char *method;
    char *path;
    char *version;
    char *status;
    size_t body_size;
    void *body_data;
    int field_count;
    int max_field_count;
    http_field_t *fields;
} http_t;

// Create an empty HTTP struct
// Returns NULL if not successful.
http_t *init_http ();

// Create an empty HTTP response message header with version and status.
// Skips arguments if NULL.
// Returns NULL if not successful.
http_t *init_http_with_arg (char *method, char *path, char *version, char *status);

// Deep-copy http struct.
// Returns pointer to copied http_t struct if successful, NULL if not.
http_t *copy_http (http_t *http);

// Free http struct.
void free_http (http_t *http);

// Find value of field in a HTTP struct.
// Returns pointer to value if successful, NULL if not.
char *find_http_field_val (http_t *http, char *field);

// Add a field to HTTP struct.
// Returns 0 if successful, -1 if not.
int add_field_to_http (http_t *http, char *field, char *val);

// Remove a field from HTTP struct.
// Returns 0 if successful, -1 if not.
int remove_field_from_http (http_t *http, char *field);

// Add a body to HTTP struct. Includes addition of Content-Length field.
// Skips if body_size is 0 or body_data is NULL.
// Returns 0 if successful, -1 if not.
int add_body_to_http (http_t *http, size_t body_size, void *body_data);

// Remove a body from HTTP struct.
// Returns 0 if successful, -1 if not.
int remove_body_from_http (http_t *http);

// Parses the header of a HTTP string.
// Returns parsed http_t struct if successful, NULL if not.
http_t *parse_http_header (char *request);

// Format http_t struct to buffer. Dynamically allocates memory for buffer.
// Returns number of bytes written if successful, -1 if not.
ssize_t write_http_to_buffer (http_t *http, void** buffer_ptr);



/// UTILITIES ///
// Refer to http_util.c for details.

// Print http_t struct to stdout.
void print_http_header (http_t *http);

// Encode data to base64.
char *base64_encode(char *data, size_t input_length) ;

// Alloc memory for string and copy it.
// Returns pointer to copied string if successful, NULL if not.
char *copy_string (char *string);

// Write size bytes from buffer to sock.
ssize_t write_bytes (int sock, char *buffer, size_t size);

// Read size bytes from sock to buffer.
ssize_t read_bytes (int sock, char *buffer, size_t size);

// Read a file and return its contents.
// Returns the size of the file if successful, -1 if not.
ssize_t read_file (void** output, char *file_path);

// Get file extension from file path.
// Returns pointer to file extension if successful, NULL if not.
// Does not include the dot, and not allocate memory.
char *get_file_extension (char *file_path);


#endif // HTTP_FUNCTIONS_H