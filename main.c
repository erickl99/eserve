#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

const char *headers = "HTTP/1.1 200 OK\r\n"
                      "Server: eserve\r\n"
                      "Connection: close\r\n";

const char *default_response = "Content-type: text/plain\r\n"
                               "Content-length: 14\r\n"
                               "\r\n"
                               "Hello world!\r\n";

const char *response413 =
    "HTTP/1.1 413 Content Too Large\r\n"
    "Server: eserve\r\n"
    "Connection: close\r\n"
    "Content-type: text/html\r\n"
    "Content-length: 202\r\n\r\n"
    "<html>\r\n"
    "<body>\r\n"
    "<head><title>413 Content Too Large</title></head>\r\n"
    "<center><h1>413 Content Too Large</h1></center>\r\n"
    "<hr><center>Server eserve accepts at most 4KB of headers.</center>\r\n"
    "</body>\r\n"
    "</html>\r\n";

const char *response501 = "HTTP/1.1 501 Not Implemented\r\n"
                          "Server: eserve\r\n"
                          "Connection: close\r\n"
                          "Content-type: text/html\r\n"
                          "Content-length: 159\r\n\r\n"
                          "<html>\r\n"
                          "<body>\r\n"
                          "<head><title>501 Not Implemented</title></head>\r\n"
                          "<center><h1>501 Not Implemented</h1></center>\r\n"
                          "<hr><center>eserve</center>\r\n"
                          "</body>\r\n"
                          "</html>\r\n";

const char *response404 = "HTTP/1.1 404 Not Found\r\n"
                          "Server: eserve\r\n"
                          "Connection: close\r\n"
                          "Content-type: text/html\r\n"
                          "Content-length: 147\r\n\r\n"
                          "<html>\r\n"
                          "<body>\r\n"
                          "<head><title>404 Not Found</title></head>\r\n"
                          "<center><h1>404 Not Found</h1></center>\r\n"
                          "<hr><center>eserve</center>\r\n"
                          "</body>\r\n"
                          "</html>\r\n";

int create_socket() {
  struct addrinfo *info;
  struct addrinfo hints = {0};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if (getaddrinfo(NULL, "8000", &hints, &info) != 0) {
    fprintf(stderr, "Failed at getaddrinfo...\n");
    return -1;
  }

  int sfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
  if (sfd == -1) {
    fprintf(stderr, "Failed at creating the socket...\n");
    freeaddrinfo(info);
    return -1;
  }
  int optval = 1;
  setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

  if (bind(sfd, info->ai_addr, info->ai_addrlen) == -1) {
    freeaddrinfo(info);
    close(sfd);
    return -1;
  }
  freeaddrinfo(info);

  if (listen(sfd, 10) == -1) {
    fprintf(stderr, "Failed at listen...\n");
    close(sfd);
    return -1;
  }
  return sfd;
}

typedef enum {
  GET,
  HEAD,
  OPTIONS,
  TRACE,
  PUT,
  DELETE,
  POST,
  PATCH,
  CONNECT
} method;

typedef struct {
  char *target;
  method mthd;
  uint8_t size;
  char **headers;
} request;

int parse_headers(char *bytes, int size, request *req) {
  if (size < 7) {
    return -1;
  }
  int bytes_idx = 0;
  if (strncmp(bytes, "GET", 3) == 0) {
    req->mthd = GET;
    bytes_idx += 3;
  } else if (strncmp(bytes, "HEAD", 4) == 0) {
    req->mthd = HEAD;
    bytes_idx += 4;
  } else if (strncmp(bytes, "OPTIONS", 7) == 0) {
    req->mthd = OPTIONS;
    bytes_idx += 7;
  } else if (strncmp(bytes, "TRACE", 5) == 0) {
    req->mthd = TRACE;
    bytes_idx += 5;
  } else if (strncmp(bytes, "PUT", 3) == 0) {
    req->mthd = PUT;
    bytes_idx += 3;
  } else if (strncmp(bytes, "DELETE", 6) == 0) {
    req->mthd = DELETE;
    bytes_idx += 6;
  } else if (strncmp(bytes, "POST", 4) == 0) {
    req->mthd = POST;
    bytes_idx += 4;
  } else if (strncmp(bytes, "PATCH", 5) == 0) {
    req->mthd = PATCH;
    bytes_idx += 5;
  } else if (strncmp(bytes, "CONNECT", 7) == 0) {
    req->mthd = CONNECT;
    bytes_idx += 7;
  } else {
    return -1;
  }
  bytes_idx++;
  int target_start = bytes_idx;
  if (bytes_idx >= size || bytes[bytes_idx] != '/') {
    return -1;
  }
  while (bytes_idx < size && bytes[bytes_idx] != ' ') {
    bytes_idx++;
  }
  if (bytes_idx == size) {
    return -1;
  }
  bytes[bytes_idx] = '\0';
  req->target = bytes + target_start;
  bytes_idx++;
  if (bytes_idx + 10 >= size ||
      strncmp(bytes + bytes_idx, "HTTP/1.1\r\n", 10)) {
    return -1;
  }
  bytes_idx += 10;
  while (bytes_idx < size) {
    bytes_idx++;
  }
  return 0;
}

int web_server(int cfd) {
  char buffer[4096];
  int bytes_received = recv(cfd, buffer, 4096, 0);
  if (bytes_received < 1) {
    printf("Connection closed by client (or an error occurred)\n");
    close(cfd);
    return 1;
  }
  if (bytes_received >= 4 && strncmp(buffer, "quit", 4) == 0) {
    printf("Received shut down signal, shutting down server...\n");
    close(cfd);
    return 1;
  }
  if (bytes_received == 4096 && strncmp(buffer + 4092, "\r\n\r\n", 4)) {
    int response_len = strlen(response413);
    int bytes_sent = send(cfd, response413, response_len, 0);
    printf("Sent %d of %d bytes of error\n", bytes_sent, response_len);
    close(cfd);
    return 0;
  }
  request req;
  int result = parse_headers(buffer, bytes_received, &req);
  if (result == -1) {
    printf("An error occurred parsing headers...\n");
  } else {
    printf("Method: %d\nTarget: %s\n", req.mthd, req.target);
  }
  int headers_len = strlen(headers);
  int bytes_sent = send(cfd, headers, headers_len, 0);
  printf("Sent %d of %d bytes of headers\n", bytes_sent, headers_len);
  FILE *index_file = fopen("index.html", "rb");
  if (index_file == NULL) {
    int response_len = strlen(response404);
    bytes_sent = send(cfd, response404, response_len, 0);
    printf("Sent %d of %d bytes of error\n", bytes_sent, response_len);
  } else {
    struct stat fp_stat;
    int file_fd = fileno(index_file);
    fstat(file_fd, &fp_stat);
    int file_size = fp_stat.st_size;
    int total_bytes =
        sprintf(buffer, "Content-type: text/html\r\nContent-length: %d\r\n\r\n",
                file_size);
    int result = send(cfd, buffer, total_bytes, 0);
    result = sendfile(cfd, file_fd, 0, file_size);
    fclose(index_file);
    printf("Sent message\n");
  }
  close(cfd);
  return 0;
}

int main(int argc, char *argv[]) {
  int sfd = create_socket();
  if (sfd == -1) {
    return 1;
  }
  printf("Listening on localhost:8000...\n");
  char client_host_buffer[32];
  int stop_server = 0;
  while (!stop_server) {
    struct sockaddr_storage client_info;
    socklen_t len = sizeof(client_info);
    int cfd = accept(sfd, (struct sockaddr *)&client_info, &len);
    if (cfd == -1) {
      fprintf(stderr, "Failed to connect to client...\n");
      close(sfd);
      return 1;
    }
    getnameinfo((struct sockaddr *)&client_info, len, client_host_buffer, 32, 0,
                0, NI_NUMERICHOST);
    printf("Connected to %s\n", client_host_buffer);
    stop_server = web_server(cfd);
  }
  int success = close(sfd);
  printf("Closed server: %d\n", success);
  return 0;
}
