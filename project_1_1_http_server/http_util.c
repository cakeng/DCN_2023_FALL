// NXC Data Communications http_util.c for HTTP server
// Written by Jongseok Park (cakeng@snu.ac.kr)
// 2023. 9. 11


///// DO NOT MODIFY THIS FILE!! ////

#include "http_functions.h"

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

// Encode data to base64.
// From https://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
char *base64_encode(char *data, size_t input_length) 
{
    size_t output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = malloc(output_length);
    if (encoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';

    encoded_data [output_length] = '\0';
    return encoded_data;
}

// Alloc memory for string and copy it.
// Returns pointer to copied string if successful, NULL if not.
char *copy_string (char *string)
{
    if (string == NULL)
    {
        printf("copy_string() NULL parameter error\n");
        return NULL;
    }
    char *copy = (char *) malloc(strlen(string) + 1);
    if (copy == NULL)
    {
        printf("copy_string() malloc() error\n");
        return NULL;
    }
    strcpy(copy, string);
    return copy;
}

// Write size bytes from buffer to sock.
ssize_t write_bytes (int sock, char *buffer, size_t size)
{
    ssize_t bytes_sent = 0;
    ssize_t bytes_remaining = size;
    signal(SIGPIPE, SIG_IGN);
    while (bytes_remaining > 0)
    {
        bytes_sent = write(sock, buffer, bytes_remaining);
        if (bytes_sent == -1)
        {
            printf("write() error\n");
            return -1;
        }
        bytes_remaining -= bytes_sent;
        buffer += bytes_sent;
    }
    return size;
}

// Read size bytes from sock to buffer.
ssize_t read_bytes (int sock, char *buffer, size_t size)
{
    ssize_t bytes_received = 0;
    ssize_t bytes_remaining = size;
    while (bytes_remaining > 0)
    {
        bytes_received = read(sock, buffer, bytes_remaining);
        if (bytes_received == -1)
        {
            printf("read() error\n");
            return -1;
        }
        bytes_remaining -= bytes_received;
        buffer += bytes_received;
    }
    return size;
}

// Read a file and return its contents.
// Returns the size of the file if successful, -1 if not.
ssize_t read_file (void** output, char *file_path)
{
    if (output == NULL || file_path == NULL)
    {
        printf("read_file() NULL parameter error\n");
        return -1;
    }

    FILE *fp = fopen(file_path, "rb");
    if (fp == NULL)
        return -1;

    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size == 0)
    {
        if (*output != NULL)
            free (*output);
        *output = NULL;
        fclose(fp);
        return 0;
    }

    if (*output != NULL)
        free (*output);
    *output = (void *) malloc(file_size);
    if (*output == NULL)
    {
        printf("read_file() malloc() error\n");
        fclose(fp);
        return -1;
    }

    if (fread(*output, 1, file_size, fp) != file_size)
    {
        printf("read_file() fread() error\n");
        fclose(fp);
        free (*output);
        return -1;
    }

    fclose(fp);
    return file_size;
}

// Get file extension from file path.
// Returns pointer to file extension if successful, NULL if not.
// Does not include the dot.
// Does not allocate memory.
char *get_file_extension (char *file_path)
{
    if (file_path == NULL)
    {
        printf("get_file_extension() NULL parameter error\n");
        return NULL;
    }
    char *extension = strrchr(file_path, '.');
    if (extension == NULL)
        return NULL;
    return extension + 1;
}

// Create an empty HTTP header struct
// Returns NULL if not successful.
http_t *init_http ()
{
    http_t *http = (http_t *) malloc(sizeof(http_t));
    if (http == NULL)
    {
        printf("init_http_response() malloc() error\n");
        return NULL;
    }

    http->method = NULL;
    http->path = NULL;
    http->version = NULL;
    http->status = NULL;
    http->body_size = 0;
    http->body_data = NULL;
    http->field_count = 0;
    http->max_field_count = 16;
    http->fields = (http_field_t *) malloc(http->max_field_count * sizeof(http_field_t));
    memset (http->fields, 0, http->max_field_count * sizeof(http_field_t));
    if (http->fields == NULL)
    {
        printf("parse_http_header() fields malloc() error\n");
        free_http (http);
        return NULL;
    }
    return http;
}

// Create an empty HTTP response message header with version and status.
// Skips arguments if NULL.
// Returns NULL if not successful.
http_t *init_http_with_arg (char *method, char *path, char *version, char *status)
{
    if (version == NULL || status == NULL)
    {
        printf("init_http_response() NULL parameter error\n");
        return NULL;
    }

    http_t *response = init_http();
    if (response == NULL)
    {
        printf("init_http_response() init_http() error\n");
        return NULL;
    }

    if (method)
    {
        response->method = copy_string(method);
        if (response->method == NULL)
        {
            printf("init_http_response() method copy error\n");
            goto ERROR;
        }
    }
    if (path)
    {
        response->path = copy_string(path);
        if (response->path == NULL)
        {
            printf("init_http_response() path copy error\n");
            goto ERROR;
        }
    }
    if (version)
    {
        response->version = copy_string(version);
        if (response->version == NULL)
        {
            printf("init_http_response() version copy error\n");
            goto ERROR;
        }
    }
    if (status)
    {
        response->status = copy_string(status);
        if (response->status == NULL)
        {
            printf("init_http_response() status copy error\n");
            goto ERROR;
        }
    }

    return response;
    ERROR:
    free_http(response);
    return NULL;
}

// Deep-copy http struct.
// Returns pointer to copied http_t struct if successful, NULL if not.
http_t *copy_http (http_t *http)
{
    if (http == NULL)
    {
        printf("copy_http() NULL parameter error\n");
        return NULL;
    }

    http_t *copy = init_http_with_arg (http->method, http->path, http->version, http->status);
    if (copy == NULL)
    {
        printf("copy_http() init_http_with_arg() error\n");
        return NULL;
    }

    if (add_body_to_http (copy, http->body_size, http->body_data) == -1)
    {
        printf("copy_http() add_body_to_http() error\n");
        goto ERROR;
    }

    for (int i = 0; i < http->field_count; i++)
    {
        if (add_field_to_http (copy, http->fields[i].field, http->fields[i].val) == -1)
        {
            printf("copy_http() add_field_to_http() error\n");
            goto ERROR;
        }
    }

    return copy;
    ERROR:
    free_http(copy);
    return NULL;
}

// Free http struct.
void free_http (http_t *http)
{
    if (http == NULL)
        return;
    free(http->method);
    free(http->path);
    free(http->version);
    free(http->status);
    free(http->body_data);
    if (http->fields)
    {
        for (int i = 0; i < http->max_field_count; i++)
        {
            free(http->fields[i].field);
            free(http->fields[i].val);
        }
        free (http->fields);
    }
    free(http);
}

// Find value of field in a HTTP struct.
// Returns pointer to value if successful, NULL if not.
char *find_http_field_val (http_t *http, char *field)
{
    if (http == NULL || field == NULL)
    {
        printf("find_http_request_header_field() NULL parameter error\n");
        return NULL;
    }
    for (int i = 0; i < http->field_count; i++)
    {
        if (strcmp(http->fields[i].field, field) == 0)
            return http->fields[i].val;
    }
    return NULL;
}

// Add a field to HTTP struct.
// Returns 0 if successful, -1 if not.
int add_field_to_http (http_t *http, char *field, char *val)
{
    if (http == NULL || field == NULL || val == NULL)
    {
        printf("add_field_to_http() NULL parameter error\n");
        return -1;
    }

    if (find_http_field_val (http, field) != NULL)
    {
        printf("add_field_to_http() field already exists error\n");
        return -1;
    }

    if (http->field_count + 1> http->max_field_count)
    {
        http->max_field_count *= 2;
        http->fields = (http_field_t *) realloc(http->fields, http->max_field_count * sizeof(http_field_t));
        if (http->fields == NULL)
        {
            printf("add_field_to_http() fields realloc() error\n");
            http->max_field_count /= 2;
            return -1;
        }
        memset (http->fields + http->field_count, 0, http->max_field_count / 2 * sizeof(http_field_t));
    }

    http->fields[http->field_count].field = copy_string (field);
    if (http->fields[http->field_count].field == NULL)
    {
        printf("add_field_to_http() field copy error\n");
        return -1;
    }

    http->fields[http->field_count].val = copy_string (val);
    if (http->fields[http->field_count].val == NULL)
    {
        printf("add_field_to_http() val copy error\n");
        return -1;
    }

    http->field_count++;
    return 0;
}

// Remove a field from HTTP struct.
// Returns 0 if successful, -1 if not.
int remove_field_from_http (http_t *http, char *field)
{
    if (http == NULL || field == NULL)
    {
        printf("remove_field_from_http() NULL parameter error\n");
        return -1;
    }

    if (http->field_count == 0)
        return 0;

    int idx = -1;
    for (int i = 0; i < http->field_count; i++)
    {
        if (strcmp(http->fields[i].field, field) == 0)
        {
            idx = i;
            break;
        }
    }
    if (idx == -1)
        return 0;

    free(http->fields[idx].field);
    free(http->fields[idx].val);
    for (int i = idx; i < http->field_count - 1; i++)
    {
        http->fields[i].field = http->fields[i + 1].field;
        http->fields[i].val = http->fields[i + 1].val;
    }
    http->field_count--;
    return 0;
}

// Add a body to HTTP struct. Includes addition of Content-Length field.
// Skips if body_size is 0 or body_data is NULL.
// Returns 0 if successful, -1 if not.
int add_body_to_http (http_t *http, size_t body_size, void *body_data)
{
    if (http == NULL)
    {
        printf("add_body_to_http_response() NULL parameter error\n");
        return -1;
    }
    if (body_size == 0 || body_data == NULL)
        return 0;

    if (http->body_data != NULL || http->body_size != 0)
    {
        printf("add_body_to_http_response() body_data already exists error\n");
        return -1;
    }

    if (find_http_field_val (http, "Content-Length") == NULL)
    {
        char content_length[32] = {0};
        sprintf(content_length, "%lu", body_size);
        if (add_field_to_http (http, "Content-Length", content_length) == -1)
        {
            printf("add_body_to_http_response() add_field_to_http() error\n");
            return -1;
        }
    }

    http->body_size = body_size;
    http->body_data = (void *) malloc(body_size);
    if (http->body_data == NULL)
    {
        printf("add_body_to_http_response() body_data malloc() error\n");
        if (remove_field_from_http (http, "Content-Length") == -1)
            printf("add_body_to_http_response() remove_field_from_http() error\n");
        return -1;
    }
    memcpy(http->body_data, body_data, body_size);

    return 0;
}

// Remove a body from HTTP struct.
// Returns 0 if successful, -1 if not.
int remove_body_from_http (http_t *http)
{
    if (http == NULL)
    {
        printf("remove_body_from_http_response() NULL parameter error\n");
        return -1;
    }

    if (http->body_data == NULL || http->body_size == 0)
        return 0;

    free(http->body_data);
    http->body_data = NULL;
    http->body_size = 0;

    if (remove_field_from_http (http, "Content-Length") == -1)
    {
        printf("remove_body_from_http_response() remove_field_from_http() error\n");
        return -1;
    }

    return 0;
}

// Parses the header of a HTTP string.
// Returns parsed http_t struct if successful, NULL if not.
http_t *parse_http_header (char *request)
{
    if (request == NULL)
    {
        printf("parse_http_header() NULL parameter error\n");
        return NULL;
    }

    http_t *http = init_http();

    char *line = strtok(request, "\r\n");
    char *next_line = line + strlen(line) + 1;
    if (line == NULL)
    {
        printf("parse_http_header() first-line error\n");
        goto ERROR;
    }
    char *token = strtok (line, " ");
    while (token != NULL)
    {
        if (http->method == NULL)
        {
            http->method = copy_string(token);
            if (http->method == NULL)
            {
                printf("parse_http_header() method copy error\n");
                goto ERROR;
            }
        }
        else if (http->path == NULL)
        {
            // Truncate query string and fragment identifier
            char *query_string = strchr(token, '?');
            if (query_string != NULL)
                *query_string = '\0';
            char *fragment_identifier = strchr(token, '#');
            if (fragment_identifier != NULL)
                *fragment_identifier = '\0';
            http->path = copy_string(token);
            if (http->path == NULL)
            {
                printf("parse_http_header() path copy error\n");
                goto ERROR;
            }
        }
        else if (http->version == NULL)
        {
            http->version = copy_string(token);
            if (http->version == NULL)
            {
                printf("parse_http_header() version copy error\n");
                goto ERROR;
            }
        }
        else
        {
            printf("parse_http_header() first-line token error - token: %s\n", token);
            goto ERROR;
        }
        token = strtok (NULL, " ");
    }
    if (http->method == NULL || http->path == NULL || http->version == NULL)
    {
        printf("parse_http_header() first-line error: method: %s, path: %s, version: %s\n",
                http->method? http->method : "NULL", http->path? http->path : "NULL", http->version? http->version : "NULL");
        goto ERROR;
    }

    line = strtok(next_line, "\r\n");
    next_line = line + strlen(line) + 1;
    while (1)
    {
        token = strtok (line, ":");
        if (token == NULL)
        {
            printf("parse_http_header() field token error - token: %s\n", token? token : "NULL");
            goto ERROR;
        }
        char *field = token;
        char *val = strtok (NULL, "\r\n");
        if (add_field_to_http (http, field, val) == -1)
        {
            printf("parse_http_header() add_field_to_http() error\n");
            goto ERROR;
        }
        line = strtok(next_line, "\r\n");
        if (line == NULL)
            break;
        next_line = line + strlen(line) + 1;
    }

    return http;
    ERROR:
    free_http(http);
    return NULL;
}

// Format http_t struct to buffer. Dynamically allocates memory for buffer.
// Returns number of bytes written if successful, -1 if not.
ssize_t write_http_to_buffer (http_t *http, void** buffer_ptr)
{
    size_t buffer_size = 0;
    if (http == NULL || buffer_ptr == NULL)
    {
        printf("write_http_to_buffer() NULL parameter error\n");
        return -1;
    }
    if (http->version == NULL)
    {
        printf("write_http_to_buffer() NULL version error\n");
        return -1;
    }
    buffer_size += strlen(http->version) + 1;
    buffer_size += http->method? strlen(http->method) + 1 : 0;
    buffer_size += http->path? strlen(http->path) + 1 : 0;
    buffer_size += http->status? strlen(http->status) + 1 : 0;
    buffer_size += 1; // \r\n

    for (int i = 0; i < http->field_count; i++)
        buffer_size += strlen(http->fields[i].field) + 2 + strlen(http->fields[i].val) + 2; // field: val\r\n
    buffer_size += 2; // \r\n

    buffer_size += http->body_size;

    if (*buffer_ptr != NULL)
        free(*buffer_ptr);
    *buffer_ptr = (void *) malloc(buffer_size);
    if (*buffer_ptr == NULL)
    {
        printf("write_http_to_buffer() buffer malloc() error\n");
        return -1;
    }
    char *buffer = (char *) *buffer_ptr;
    buffer[0] = '\0';
    
    strcat (buffer, http->method? http->method : "");
    strcat (buffer, http->method? " " : "");
    strcat (buffer, http->path? http->path : "");
    strcat (buffer, http->path? " " : "");
    strcat (buffer, http->version);
    strcat (buffer, " ");
    strcat (buffer, http->status? http->status : "");
    strcat (buffer, "\r\n");
    for (int i = 0; i < http->field_count; i++)
    {
        strcat (buffer, http->fields[i].field);
        strcat (buffer, ": ");
        strcat (buffer, http->fields[i].val);
        strcat (buffer, "\r\n");
    }
    strcat (buffer, "\r\n");
    buffer += strlen(buffer);
    if ((char *) *buffer_ptr + buffer_size != http->body_size + buffer)
    {
        printf("write_http_to_buffer() buffer size error\n");
        free (*buffer_ptr);
        *buffer_ptr = NULL;
        return -1;
    }       
    if (http->body_data)
    {
        memcpy(buffer, http->body_data, http->body_size);
        buffer += http->body_size;
    }
    return buffer_size;
}

// Print http_t struct to stdout.
void print_http_header (http_t *http)
{
    if (http == NULL)
    {
        printf("print_http_header() NULL parameter error\n");
        return;
    }
    printf("\t\tmethod: %s\n", http->method? http->method : "NULL");
    printf("\t\tpath: %s\n", http->path? http->path : "NULL");
    printf("\t\tversion: %s\n", http->version? http->version : "NULL");
    printf("\t\tstatus: %s\n", http->status? http->status : "NULL");
    printf("\t\tbody_size: %lu\n", http->body_size);
    printf("\t\tfield_count: %d\n", http->field_count);
    printf("\t\tfields:\n");
    for (int i = 0; i < http->field_count; i++)
        printf("\t\t\t%s: %s\n", http->fields[i].field? http->fields[i].field : "NULL",
                http->fields[i].val? http->fields[i].val : "NULL");
}
