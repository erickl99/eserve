#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define WEB_SERVER 0
#define ECHO_SERVER 1

const char *headers = "HTTP/1.1 200 OK\r\n"
                      "Server: eserve\r\n"
                      "Connection: close\r\n";

const char *default_response = "Content-type: text/plain\r\n"
                               "Content-length: 14\r\n"
                               "\r\n"
                               "Hello world!\r\n";

const char *error_response = "HTTP/1.1 501 Not Implemented\r\n"
                             "Server: eserve\r\n"
                             "Connection: close\r\n"
                             "Content-type: text/html\r\n"
                             "Content-length: 151\r\n\r\n"
                             "<html>\r\n"
                             "<body>\r\n"
                             "<head><title>405 Not Allowed</title></head>\r\n"
                             "<center><h1>405 Not Allowed</h1></center>\r\n"
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

void web_server(int cfd) {
  char buffer[1024];
  int bytes_received = recv(cfd, buffer, 1024, 0);
  if (bytes_received < 1) {
    printf("Connection closed by client (or an error occurred)\n");
    close(cfd);
    return;
  }
  printf("Received http message:\n%.*s\n", bytes_received, buffer);
  int headers_len = strlen(headers);
  int bytes_sent = send(cfd, headers, headers_len, 0);
  printf("Sent %d of %d bytes of headers\n", bytes_sent, headers_len);
  FILE *fp = fopen("index.html", "rb");
  if (fp == NULL) {
    int response_len = strlen(default_response);
    bytes_sent = send(cfd, default_response, response_len, 0);
    printf("Sent %d of %d bytes of default\n", bytes_sent, headers_len);
  } else {
    struct stat fp_stat;
    fstat(fileno(fp), &fp_stat);
    int file_size = fp_stat.st_size;
    int total_bytes =
        sprintf(buffer, "Content-type: text/html\r\nContent-length: %d\r\n\r\n",
                file_size);
    send(cfd, buffer, total_bytes, 0);
    fread(buffer, 1, file_size, fp);
    send(cfd, buffer, file_size, 0);
    printf("Sent message\n");
  }
  close(cfd);
}

void echo_server(int cfd) {
  char message[1024];
  char *greeting =
      "Welcome to the echo server! Type \"quit\" to close the connection\n";
  send(cfd, greeting, strlen(greeting), 0);

  for (;;) {
    int bytes_received = recv(cfd, message, 1024, 0);
    if (bytes_received < 1) {
      printf("Connection closed by client (or an error occurred)\n");
      close(cfd);
      break;
    }
    printf("Received %d byte message: %.*s\n", bytes_received, bytes_received,
           message);
    if (bytes_received >= 4 && strncmp(message, "quit", 4) == 0) {
      close(cfd);
      break;
    }
    int bytes_sent = send(cfd, message, bytes_received, 0);
    printf("Sent %d bytes of %d bytes\n", bytes_sent, bytes_received);
  }
}

int main(int argc, char *argv[]) {
  int server_mode = WEB_SERVER;
  if (argc > 1) {
    if (strcmp(argv[1], "echo") == 0) {
      server_mode = ECHO_SERVER;
    } else if (strcmp(argv[1], "web") != 0) {
      fprintf(stderr, "Usage: server [mode]\n");
      return 1;
    }
  }
  int sfd = create_socket();
  if (sfd == -1) {
    return 1;
  }
  printf("Listening on localhost:8000...\n");

  struct sockaddr_storage client_info;
  socklen_t len = sizeof(client_info);
  int cfd = accept(sfd, (struct sockaddr *)&client_info, &len);
  if (cfd == -1) {
    fprintf(stderr, "Failed to connect to client...\n");
    close(sfd);
    return 1;
  }

  char buffer[32];
  getnameinfo((struct sockaddr *)&client_info, len, buffer, 32, 0, 0,
              NI_NUMERICHOST);
  printf("Connected to %s\n", buffer);

  if (server_mode == WEB_SERVER) {
    web_server(cfd);
  } else {
    echo_server(cfd);
  }
  int success = close(sfd);
  printf("Looks good to me: %d\n", success);
  return 0;
}
