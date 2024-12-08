#define _GNU_SOURCE
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define PORT 2345
#include <sys/stat.h>
#include <fcntl.h>
#define HTTP_OK "HTP/1.0 200 OK\r\n"
#define HTTP_BAD_REQUEST "HTTP/1.0 400 Bad Request\r\n"
#define HTTP_FORBIDDEN "HTTP/1.0 403 Permission Denied\r\n"
#define HTTP_NOT_FOUND "HTTP/1.0 404 Not Found\r\n"
#define HTTP_INTERNAL_ERROR "HTTP/1.0 500 Internal Server Error\r\n"
#define HTTP_NOT_IMPLEMENTED "HTTP/1.0 501 Not Implemented\r\n"


// Common HTTP Headers
#define CONTENT_TYPE_HTML "Content-Type: text/html\r\n"
#define HEADER_END "\r\n"


// step 1: modify handle_request to parse http requests 



void handle_request(int nfd) {
    FILE *network = fdopen(nfd, "r+");
    char *line = NULL;
    size_t size = 0;
    char method[10], filepath[1024], http_version[10];
    struct stat file_stat;

    if (!network) {
        perror("fdopen");
        close(nfd);
        return;
    }

    if (getline(&line, &size, network) <= 0) {
        fprintf(network, "HTTP/1.0 400 Bad Request\r\n\r\n");
        fclose(network);
        free(line);
        return;
    }

    if (sscanf(line, "%s %s %s", method, filepath, http_version) != 3 ||
        (strcmp(http_version, "HTTP/1.0") != 0 && strcmp(http_version, "HTTP/1.1") != 0)) {
        fprintf(network, "HTTP/1.0 400 Bad Request\r\n\r\n");
        fclose(network);
        free(line);
        return;
    }

    if (filepath[0] == '/')
        memmove(filepath, filepath + 1, strlen(filepath));

    if (strstr(filepath, "..") != NULL) {
        fprintf(network, "HTTP/1.0 403 Permission Denied\r\n\r\n");
        fclose(network);
        free(line);
        return;
    }

    if (stat(filepath, &file_stat) != 0) {
        fprintf(network, "HTTP/1.0 404 Not Found\r\n\r\n");
        fclose(network);
        free(line);
        return;
    }

    fprintf(network, HTTP_OK);
    fprintf(network, "Content-Type: text/html\r\n");
    fprintf(network, "Content-Length: %ld\r\n\r\n", file_stat.st_size);

    if (strcmp(method, "GET") == 0) {
        int file_fd = open(filepath, O_RDONLY);
        if (file_fd == -1) {
            fprintf(network, "HTTP/1.0 403 Permission Denied\r\n\r\n");
        } else {
            char buffer[1024];
            ssize_t bytes_read;
            while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
                fwrite(buffer, 1, bytes_read, network);
            }
            close(file_fd);
        }
    }

    fclose(network);
    free(line);
}












void run_service(int fd)
{
   while (1)
   {
      int nfd = accept_connection(fd);
      if (nfd != -1)
      {
         printf("Connection established\n");
         handle_request(nfd);
         printf("Connection closed\n");
      }
   }
}

int main(void)
{
   int fd = create_service(PORT); // creates the server

   if (fd == -1) 
   {
      perror(0);
      exit(1);
   }

   printf("listening on port: %d\n", PORT);
   run_service(fd);
   close(fd);

   return 0;
}
