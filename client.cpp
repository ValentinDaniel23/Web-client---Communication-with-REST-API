#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <nlohmann/json.hpp>
#include "helpers.h"
#include "requests.h"
#include <iostream>

using namespace std;
using json = nlohmann::json;

#define server_addr (char*)"34.246.184.49"
#define server_port 8080

#define register_route (char*)"/api/v1/tema/auth/register"
#define login_route (char*)"/api/v1/tema/auth/login"
#define enter_library_route (char *)"/api/v1/tema/library/access"
#define get_books_route (char *)"/api/v1/tema/library/books/"
#define logout_route (char *)"/api/v1/tema/auth/logout"

#define version_prefix (char*)"HTTP/1.1 "
#define payload_type (char*)"application/json"

#define OK 200
#define Created 201
#define No_Content 204
#define Bad_Request 400
#define Unauthorized 401
#define Forbidden 403
#define Resource_Not_Found 404
#define Internal_Server_Error 500


char **cookies = NULL;
int cookies_len = 0;

char *library_token = NULL;

int number(string s) {
    if (s.size() > 7) return -1;
    if (s.size() == 0) return -1;
    int n = 0;
    for (int i=0; i<s.size(); i++)
        if (s[i] < '0' || s[i] > '9') return -1;
        else {
            n = n * 10 + s[i] - '0';
        }
    return n;
}

int code(char *response) {
    return atoi(response + strlen(version_prefix));
}

string string_returned(bool nowhitespaces) {
    string sir;
    getline(cin, sir);

    if (sir.empty()) return "";

    // daca nowhitespaces este false, textul nu trebuie sa inceapa sau sa 
    // se termine cu spatiu, sau sa aibe 2 spatii consecutive
    if (sir[0] == ' ') return "";
    if (sir[sir.size() - 1] == ' ') return "";
    for (int i=0; i<sir.size()-1; i++)
        if (sir[i] == ' ' && sir[i+1] == ' ') return "";
    // liniile de mai sus se comenteaza daca se accepta spatii oricum
    
    if (nowhitespaces)
        for (int i=0; i<sir.size(); i++)
            if (sir[i] == ' ') return "";

    return sir;
}

void make_register() {
    cout << "username=";
    string username = string_returned(true);
    cout << "password=";
    string password = string_returned(true);

    if (username == "" || password == "") {
        cout << "ERROR: bad credentials\n";
        return;
    }

    char *message;
    char *response;
    int sockfd;
    sockfd = open_connection(server_addr, server_port, AF_INET, SOCK_STREAM, 0);

    json user_object;
    user_object["username"] = username;
    user_object["password"] = password;
    string json_string = user_object.dump();

    char **body_data = new char *[1];
    body_data[0] = new char[json_string.size() + 1];
    strcpy(body_data[0], json_string.c_str());

    message = compute_post_request(server_addr, register_route, payload_type, NULL, body_data, 1, NULL, 0);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    int code_number = code(response);
    if (code_number == Created) {
        cout << "SUCCESS: new user created\n";
    }
    else if (code_number == Bad_Request)
        cout << "ERROR: bad request, user already taken\n";
    else if (code_number == Internal_Server_Error)
        cout << "ERROR: internal server error\n";
    else
        cout << "ERROR: Unknown code number\n";

    delete[] message;
    delete[] response;
    delete[] body_data[0];
    delete[] body_data;
}

void make_login() {
    cout << "username=";
    string username = string_returned(true);
    cout << "password=";
    string password = string_returned(true);

    if (username == "" || password == "") {
        cout << "ERROR: bad credentials\n";
        return;
    }

    char *message, *response;
    int sockfd;
    sockfd = open_connection(server_addr, server_port, AF_INET, SOCK_STREAM, 0);

    json user_object;
    user_object["username"] = username;
    user_object["password"] = password;
    string json_string = user_object.dump();

    char **body_data = new char *[1];
    body_data[0] = new char[json_string.size() + 1];
    strcpy(body_data[0], json_string.c_str());

    message = compute_post_request(server_addr, login_route, payload_type, NULL, body_data, 1, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    int code_number = code(response);
    if (code_number == OK) {
        cout << "SUCCESS: user logged in\n";

        if (cookies_len) {
            for (int i=0; i<cookies_len; i++) {
                free(cookies[i]);
                cookies[i] = NULL;
            }
            free(cookies);
            cookies = NULL;
            cookies_len = 0;
        }

        char* cookie_start = strstr(response, "Set-Cookie:") + strlen("Set-Cookie: ");
        char* cookie_end = strstr(cookie_start, "\r\n");

        int cookie_len = strlen(cookie_start) - strlen(cookie_end);
        char* cookie_all = new char[cookie_len + 1];
        strncpy(cookie_all, cookie_start, cookie_len);
        cookie_all[cookie_len] = '\0';

        for (int i=0; i<cookie_len; i++)
            if (cookie_all[i] == ';') cookies_len ++;

        cookies_len ++;

        char* delimiter = (char *)"; ";
        char* token = strtok(cookie_all, delimiter);

        cookies = new char *[cookies_len];

        int nr = 0;
        while (token != NULL) {
            cookies[nr] = strdup(token);
            token = strtok(NULL, delimiter);
            nr ++;
        }
        if (library_token != NULL)
            free(library_token);
        library_token = NULL;
    }
    else if (code_number == Bad_Request)
        cout << "ERROR: bad credentials\n";
    else if (code_number == Internal_Server_Error)
        cout << "ERROR: internal server error\n";
    else
        cout << "ERROR: Unknown code number\n";

    delete[] message;
    delete[] response;
    delete[] body_data[0];
    delete[] body_data;
}

void make_enter_library() {
    if (cookies_len == 0) {
        cout << "ERROR: not logged in\n";
        return;
    }

    char *message, *response;
    int sockfd;
    sockfd = open_connection(server_addr, server_port, AF_INET, SOCK_STREAM, 0);

    message = compute_get_request(server_addr, enter_library_route, NULL, NULL, cookies, cookies_len);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    int code_number = code(response);
    if (code_number == OK) {
        cout << "SUCCESS: entered\n";
        char* token_start = strstr(response, "{\"token\"");
        char* token_end = strstr(token_start, "\"}");
        
        int token_len = strlen(token_start) - strlen(token_end) + 2;
        char* token_all = new char[token_len + 1];
        strncpy(token_all, token_start, token_len);
        token_all[token_len] = '\0';

        json token_json = json::parse(token_all);
        string token = token_json["token"];
        
        if (library_token != NULL)
            free(library_token);

        library_token = new char[token_len + 1];
        strcpy(library_token, token.c_str());
    }
    else if (code_number == Unauthorized)
        cout << "ERROR: bad credentials\n";
    else
        cout << "Unknown code number\n";

    delete[] message;
    delete[] response;
}

void make_get_books() {
    if (cookies_len == 0) {
        cout << "ERROR: not logged in\n";
        return;
    }
    if (library_token == NULL) {
        cout << "ERROR: no token\n";
        return;
    }

    char *message, *response;
    int sockfd;
    sockfd = open_connection(server_addr, server_port, AF_INET, SOCK_STREAM, 0);

    message = compute_get_request(server_addr, get_books_route, NULL, library_token, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    int code_number = code(response);
    if (code_number == OK) {
        cout << "SUCCESS: books received\n";
        json response_json = json::parse(strstr(response, "["));

        cout << response_json.dump(2) << '\n';
    }
    else if (code_number == Unauthorized)
        cout << "ERROR: bad credentials\n";
    else if (code_number == Internal_Server_Error)
        cout << "ERROR: internal server error\n";
    else
        cout << "ERROR: Unknown code number\n";
    
    delete[] message;
    delete[] response;
}

void make_get_book() {
    if (cookies_len == 0) {
        cout << "ERROR: not logged in\n";
        return;
    }
    if (library_token == NULL) {
        cout << "ERROR: no token\n";
        return;
    }

    cout << "id=";
    string id_string = string_returned(true);
    int id = number(id_string);

    if (id == -1) {
        cout << "ERROR: bad id\n";
        return;
    }

    char *message, *response;
    int sockfd;
    sockfd = open_connection(server_addr, server_port, AF_INET, SOCK_STREAM, 0);

    char *way = new char[strlen(get_books_route) + id_string.size() + 1];
    strcpy(way, get_books_route);
    strcat(way, id_string.c_str());

    message = compute_get_request(server_addr, way, NULL, library_token, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    int code_number = code(response);
    if (code_number == OK) {
        cout << "SUCCESS: books found\n";
        json response_json = json::parse(strstr(response, "{"));

        cout << response_json.dump(2) << '\n';
    }
    else if (code_number == Resource_Not_Found)
        cout << "ERROR: non existent book\n";
    else if (code_number == Unauthorized)
        cout << "ERROR: bad credentials\n";
    else if (code_number == Internal_Server_Error)
        cout << "ERROR: internal server error\n";
    else
        cout << "ERROR: Unknown code number\n";
    
    delete[] message;
    delete[] response;
    delete[] way;
}

void make_add_book() {
    if (cookies_len == 0) {
        cout << "ERROR: not logged in\n";
        return;
    }
    if (library_token == NULL) {
        cout << "ERROR: no token\n";
        return;
    }

    cout << "title=";
    string title = string_returned(false);
    cout << "author=";
    string author = string_returned(false);
    cout << "genre=";
    string genre = string_returned(false);
    cout << "publisher=";
    string publisher = string_returned(false);
    cout << "page_count=";
    string page_count_string = string_returned(true);
    int page_count = number(page_count_string);
    
    if (title == "" || author == "" || genre == "" || publisher == "" || page_count == -1) {
        cout << "ERROR: bad book data\n";
        return;
    }

    char *message, *response;
    int sockfd;
    sockfd = open_connection(server_addr, server_port, AF_INET, SOCK_STREAM, 0);

    json user_object;
    user_object["title"] = title;
    user_object["author"] = author;
    user_object["genre"] = genre;
    user_object["publisher"] = publisher;
    user_object["page_count"] = page_count_string;
    string json_string = user_object.dump();

    char **body_data = new char *[1];
    body_data[0] = new char[json_string.size() + 1];
    strcpy(body_data[0], json_string.c_str());

    message = compute_post_request(server_addr, get_books_route, payload_type, library_token, body_data, 1, NULL, 0);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    int code_number = code(response);
    if (code_number == OK)
        cout << "SUCCESS: book added\n";
    else if (code_number == Bad_Request)
        cout << "ERROR: bad book data\n";
    else if (code_number == Unauthorized)
        cout << "ERROR: bad credentials\n";
    else if (code_number == Internal_Server_Error)
        cout << "ERROR: internal server error\n";
    else
        cout << "ERROR: Unknown code number\n";

    delete[] message;
    delete[] response;
    delete[] body_data[0];
    delete[] body_data;
}

void make_delete_book() {   
    if (cookies_len == 0) {
        cout << "ERROR: not logged in\n";
        return;
    }
    if (library_token == NULL) {
        cout << "ERROR: no token\n";
        return;
    }

    cout << "id=";
    string id_string = string_returned(true);
    int id = number(id_string);

    if (id == -1) {
        cout << "ERROR: bad id\n";
        return;
    }

    char *message;
    char *response;
    int sockfd;
    sockfd = open_connection(server_addr, server_port, AF_INET, SOCK_STREAM, 0);

    char *way = new char[strlen(get_books_route) + id_string.size() + 1];
    strcpy(way, get_books_route);
    strcat(way, id_string.c_str());

    message = compute_delete_request(server_addr, way, payload_type, library_token, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    int code_number = code(response);
    if (code_number == OK)
        cout << "SUCCESS: book deleted\n";
    else if (code_number == Resource_Not_Found)
        cout << "ERROR: non existent book\n";
    else if (code_number == Unauthorized)
        cout << "ERROR: bad credentials\n";
    else if (code_number == Internal_Server_Error)
        cout << "ERROR: internal server error\n";
    else
        cout << "ERROR: Unknown code number\n";

    delete[] message;
    delete[] response;
    delete[] way;    
}

void make_logout() {
    if (cookies_len == 0) {
        cout << "ERROR: not logged in\n";
        return;
    }

    char *message, *response;
    int sockfd;
    sockfd = open_connection(server_addr, server_port, AF_INET, SOCK_STREAM, 0);

    message = compute_get_request(server_addr, logout_route, NULL, NULL, cookies, cookies_len);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    int code_number = code(response);
    if (code_number == OK) {
        cout << "SUCCESS: user deleted\n";

        if (cookies_len) {
            for (int i=0; i<cookies_len; i++) {
                free(cookies[i]);
                cookies[i] = NULL;
            }
            free(cookies);
            cookies = NULL;
            cookies_len = 0;
        }
        if (library_token != NULL)
            free(library_token);
        library_token = NULL;
    }
    else if (code_number == Bad_Request)
        cout << "ERROR: not logged in\n";
    else if (code_number == Internal_Server_Error)
        cout << "ERROR: internal server error\n";
    else
        cout << "ERROR: Unknown code number\n";

    delete[] message;
    delete[] response;
}

int main(int argc, char *argv[])
{
    string command;
    while (true) {
        command = string_returned(true);
        if (command == "register")
            make_register();
        else if (command == "login")
            make_login();
        else if (command == "enter_library")
            make_enter_library();
        else if (command == "get_books")
            make_get_books();
        else if (command == "get_book")
            make_get_book();
        else if (command == "add_book")
            make_add_book();
        else if (command == "delete_book")
            make_delete_book();
        else if (command == "logout")
            make_logout();
        else if (command == "exit")
            break;
        else
            cout << "ERROR: unkown command type\n";
    }

    return 0;
}
