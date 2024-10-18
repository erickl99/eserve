#ifndef parser_h
#define parser_h

typedef unsigned char uint8;

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
  uint8 size;
  char **headers;
} request;

int parse_headers(char *bytes, int size, request *req);
#endif // !parser_h
