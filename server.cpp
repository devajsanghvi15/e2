#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#define FRAME_SIZE 256
#define TIMEOUT 3
#define SIZE 1024
#define WINDOW_SIZE 7

void send_data(int new_sock, FILE *fp);

int main()
{
  WSADATA wsa;
  SOCKET s, new_sock;
  struct sockaddr_in server, client;
  int c;
  const char *filename = "text.txt";

  printf("\nInitialising Winsock...");
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
  {
    printf("Failed. Error Code : %d", WSAGetLastError());
    return 1;
  }

  printf("Initialised.\n");

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
  {
    printf("Could not create socket : %d", WSAGetLastError());
    return 1;
  }

  printf("Socket created.\n");
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(8888);

  if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
  {
    printf("Bind failed with error code : %d", WSAGetLastError());
    return 1;
  }

  puts("Bind done");

  listen(s, 3);

  puts("Waiting for incoming connections...");

  c = sizeof(struct sockaddr_in);
  new_sock = accept(s, (struct sockaddr *)&client, &c);
  if (new_sock == INVALID_SOCKET)
  {
    printf("accept failed with error code : %d", WSAGetLastError());
    return 1;
  }

  puts("Connection accepted");

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL)
  {
    perror("Error in reading file. Exiting...");
    exit(1);
  }

  send_data(new_sock, fp);
  fclose(fp);

  printf("File data sent successfully.\n");

  return 0;
}

void send_data(int new_sock, FILE *fp)
{
  int seq_number = 0;
  char buffer[FRAME_SIZE * WINDOW_SIZE];

  while (1)
  {
    int bytes_read = fread(buffer, 1, FRAME_SIZE * WINDOW_SIZE, fp);
    if (bytes_read <= 0)
    {
      send(new_sock, "End Of file", sizeof("End Of file"), 0);
      break;
    }

    send(new_sock, buffer, bytes_read, 0);

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(new_sock, &readfds);

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    if (select(0, &readfds, NULL, NULL, &timeout) <= 0)
    {
      printf("Timeout/Error: Resending frame with sequence number %d\n", seq_number);
      fseek(fp, -bytes_read, SEEK_CUR);
      continue;
    }

    // wait for ack
    char ackMsg[SIZE];
    int recv_size = recv(new_sock, ackMsg, SIZE, 0);
    if (recv_size > 0 && strncmp(ackMsg, "ACK", 3) == 0)
    {
      printf("Received ACK for frame with sequence number %d\n", seq_number);
      printf("\nSending 7 frames\n");
      seq_number = (seq_number + 1) % 2;
    }
    else
    {
      printf("Error in receiving ACK. Resending frame with sequence number %d\n", seq_number);
      fseek(fp, -bytes_read, SEEK_CUR);
    }
  }
}