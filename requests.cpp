#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params, char *token, 
                            char **cookies, int cookies_count)
{
    char *message = new char[BUFLEN]();
    char *line = new char[LINELEN]();

    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i != cookies_count - 1) 
                strcat(line, "; ");
        }
        compute_message(message, line);
    }
    
    compute_message(message, "");
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char *token, char **body_data, 
                            int body_data_fields_count, char **cookies, int cookies_count)
{
    char *message = new char[BUFLEN]();
    char *line = new char[LINELEN]();
    char *body_data_buffer = new char[LINELEN]();

    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    for (int i = 0; i < body_data_fields_count; i++) {
        if (i != 0) {
            strcat(body_data_buffer, "&");
        }
        strcat(body_data_buffer, body_data[i]);
    }

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    sprintf(line, "Content-Length: %lu", strlen(body_data_buffer));
    compute_message(message, line);

    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i != cookies_count - 1) 
                strcat(line, "; ");
        }
        compute_message(message, line);
    }

    compute_message(message, "");

    memset(line, 0, LINELEN);
    strcat(message, body_data_buffer);

    free(line);
    return message;
}

char *compute_delete_request(char *host, char *url, char* content_type, char *token, 
                              char **cookies, int cookies_count)
{
    char *message = new char[BUFLEN]();
    char *line = new char[LINELEN]();

    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);
    
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }

    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    sprintf(line, "Content-Length: 0");
    compute_message(message, line);

    // Step 4 (optional): add cookies
    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i != cookies_count - 1) 
                strcat(line, "; ");
        }
        compute_message(message, line);
    }

    compute_message(message, "");

    memset(line, 0, LINELEN);

    free(line);
    return message;
}
