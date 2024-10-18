#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"

typedef struct {
  int size;
  int idx;
  char *bytes;
  char *start;
  request *req;
} parser;

parser prsr;

void skip_whitespace() {
  while (prsr.idx < prsr.size) {
    switch (prsr.bytes[prsr.idx]) {
    case ' ':
      prsr.idx++;
      break;
    default:
      return;
    }
  }
}

int is_letter(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

char *next_key() {
  char *header_key = prsr.bytes + prsr.idx;
  while (prsr.idx < prsr.size && prsr.bytes[prsr.idx] != ':' &&
         (is_letter(prsr.bytes[prsr.idx]) || prsr.bytes[prsr.idx] == '-')) {
    prsr.idx++;
  }
  if (prsr.idx == prsr.size) {
    return NULL;
  }
  if (prsr.bytes[prsr.idx] != ':') {
    return NULL;
  }
  prsr.bytes[prsr.idx] = '\0';
  printf("%s: ", header_key);
  prsr.idx++;
  return header_key;
}

char *next_value() {
  char *header_value = prsr.bytes + prsr.idx;
  while (prsr.idx < prsr.size && prsr.bytes[prsr.idx] != '\r') {
    prsr.idx++;
  }
  prsr.idx++;
  if (prsr.idx >= prsr.size || prsr.bytes[prsr.idx] != '\n') {
    return NULL;
  }
  prsr.bytes[prsr.idx - 1] = '\0';
  printf("%s\n", header_value);
  prsr.idx++;
  return header_value;
}

int finished() {
  if (strncmp(prsr.bytes + prsr.idx, "\r\n", 2) == 0) {
    return 1;
  }
  if (prsr.idx + 2 >= prsr.size) {
    return 2;
  }
  return 0;
}

int parse_headers(char *bytes, int size, request *req) {
  prsr.bytes = bytes;
  prsr.size = size;
  prsr.req = req;
  prsr.idx = 0;
  if (!is_letter(prsr.bytes[prsr.idx])) {
    return -1;
  }
  int finish = finished();
  while (!finish) {
    char *header_key = next_key();
    if (header_key == NULL) {
      return -1;
    }
    skip_whitespace();
    char *header_value = next_value();
    if (header_value == NULL) {
      return -1;
    }
    finish = finished();
  }
  if (finish == 2) {
    return -1;
  }
  return 0;
}
