#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <unistd.h>
#pragma comment(lib, "ws2_32.lib")

#define FRAME_SIZE 256

void receive_data(int sockfd);

int main()
{
  WSADATA wsa;
  SOCKET s;
  struct sockaddr_in server;

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
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_family = AF_INET;
  server.sin_port = htons(8888);

  if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
  {
    puts("connect error");
    return 1;
  }

  puts("Connected");

  printf("Receiving file...\n");
  receive_data(s);

  printf("Data segments received and written to file successfully.\n");

  return 0;
}

void receive_data(int sockfd)
{
  int seq_number = 0;
  FILE *fp = fopen("ReceivedFile.txt", "wb");
  if (fp == NULL)
  {
    perror("Error in opening file. Exiting...");
    exit(1);
  }

  while (1)
  {
    char buffer[FRAME_SIZE];
    int recv_size = recv(sockfd, buffer, FRAME_SIZE, 0);
    if (recv_size <= 0)
    {
      break;
    }

    if (strncmp(buffer, "EndOfFile", 9) == 0)
    {
      break;
    }

    if (seq_number == 0)
    {
      send(sockfd, "ACK0", 4, 0);
    }
    else
    {
      sleep(4);
      send(sockfd, "ACK1", 4, 0);
    }

    fwrite(buffer, 1, recv_size, fp);
    seq_number = (seq_number + 1) % 2;
  }

  fclose(fp);
}