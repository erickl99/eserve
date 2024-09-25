#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  struct addrinfo *info;
  struct addrinfo hints = {0};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if (getaddrinfo(NULL, "8000", &hints, &info) != 0) {
    fprintf(stderr, "Failed at getaddrinfo...\n");
    return 1;
  }

  int sfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
  if (sfd == -1) {
    fprintf(stderr, "Failed at creating the socket...\n");
    freeaddrinfo(info);
    return 1;
  }

  if (bind(sfd, info->ai_addr, info->ai_addrlen) == -1) {
    fprintf(stderr, "Failed at bind...\n");
    freeaddrinfo(info);
    close(sfd);
    return 1;
  }
  freeaddrinfo(info);

  if (listen(sfd, 10) == -1) {
    fprintf(stderr, "Failed at listen...\n");
    close(sfd);
    return 1;
  }

  printf("Listening on port 8000...\n");
  struct sockaddr_storage client_info;
  socklen_t client_len = sizeof(client_info);
  int cfd = accept(sfd, (struct sockaddr *)&client_info, &client_len);
  if (cfd == -1) {
    fprintf(stderr, "Failed to connect to client...\n");
    close(sfd);
    return 1;
  }

  char buffer[100];
  getnameinfo((struct sockaddr *)&client_info, client_len, buffer, 100, 0, 0,
              NI_NUMERICHOST);
  printf("Connected to %s\n", buffer);
  close(cfd);
  int success = close(sfd);
  printf("Looks good to me: %d\n", success);
  return 0;
}
