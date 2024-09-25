#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define WEB_SERVER 0
#define ECHO_SERVER 1

const char *http_message = "HTTP/1.1 200 OK\r\n"
                           "Server: Eserve\r\n"
                           "Connection: close\r\n"
                           "Content-type: text/plain\r\n"
                           "Content-length: 14\r\n"
                           "\r\n"
                           "Hello world!\r\n";

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
    fprintf(stderr, "Failed at bind: %s\n", strerror(errno));
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
  char response[1024];
  int bytes_received = recv(cfd, response, 1024, 0);
  if (bytes_received < 1) {
    printf("Connection closed by client (or an error occurred)\n");
    close(cfd);
    return;
  }
  printf("Received http message: %.*s\n", bytes_received, response);
  int message_len = strlen(http_message);
  int bytes_sent = send(cfd, http_message, message_len, 0);
  printf("Sent %d of %d bytes of header\n", bytes_sent, message_len);
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
