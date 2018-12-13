#include "client_functions.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include "rdwrn.h"
#include <sys/utsname.h>

/* get the option 1 send_hello funcion message from the server. User as option 1 
 * function in the client menu.
 */
void get_hello(int socket)
{
  char hello_string[256];
  size_t k;

  readn(socket, (unsigned char *)&k, sizeof(size_t));
  readn(socket, (unsigned char *)hello_string, k);

  printf("Hello String: %s\n", hello_string);
  printf("Received: %zu bytes\n\n", k);
} // end get_hello()

/* gets an array of random numbers from the server and prints them to the console.*/
void send_and_get_ints(int socket)
{
  int array[5];
  size_t payload_length;
  size_t n = readn(socket, (unsigned char *)&payload_length, sizeof(size_t));
  printf("payload_length is: %zu (%zu bytes)\n", payload_length, n);
  n = readn(socket, (unsigned char *)array, payload_length);

  // print the contents of the array
  int i;
  for (i = 0; i < 5; i++)
  {
    printf("the array of ints: %i\n", array[i]);
  }
}

void get_random_numbers()
{
  char hello_string[32];
  size_t k;

  readn(socket, (unsigned char *)&k, sizeof(size_t));
  readn(socket, (unsigned char *)hello_string, k);
}

/* gets system about the server from the server then prints it with the appropriate
 * labels
 */
void get_uts(int socket)
{
  struct utsname uts;
  size_t payload_length;
  size_t n = readn(socket, (unsigned char *)&payload_length, sizeof(size_t));
  printf("payload_length is: %zu (%zu bytes)\n", payload_length, n);
  n = readn(socket, (unsigned char *)&uts, payload_length);

  printf("Node name:    %s\n", uts.nodename);
  printf("System name:  %s\n", uts.sysname);
  printf("Release:      %s\n", uts.release);
  printf("Version:      %s\n", uts.version);
  printf("Machine:      %s\n\n", uts.machine);
}

/* Add an extra send/recieve to this function to get the size of the data being sent
 * then make the hello_string that size.
 */
void get_filenames(int socket)
{
  char hello_string[512];
  size_t k;

  readn(socket, (unsigned char *)&k, sizeof(size_t));
  readn(socket, (unsigned char *)hello_string, k);

  printf("filenames in upload directory:\n%s", hello_string);
  //printf("Received: %zu bytes\n\n", k);
}

/* gets the current time from the server */
void get_time(int socket)
{
  char hello_string[32];
  size_t k;

  readn(socket, (unsigned char *)&k, sizeof(size_t));
  readn(socket, (unsigned char *)hello_string, k);

  printf("%s\n", hello_string);
}

/* ============================= recieve file functions ============================== */
/* function to print and format file/server information */
void gotoxy(int x, int y)
{
  printf("%c[%d;%df", 0x1B, y, x);
}

/* get a file from the server. recieves data in chunks of 1024 bytes. The server sends
 * the file size over and the loop appends the data from the file in these chunks until
 * and subtracts the file size until it is empty.
 */
int get_file2(int sockfd, char *fname)
{
  //system("clear"); // clear the user window
  int bytesReceived = 0;
  char recvBuff[1024] = "";
  memset(recvBuff, '0', sizeof(recvBuff));

  size_t payload_length_fname = sizeof(fname);
  writen(sockfd, (unsigned char *)&payload_length_fname, sizeof(size_t));
  writen(sockfd, (unsigned char *)fname, payload_length_fname);

  /* Create the file on the client side */
  FILE *fp;
  printf("File Name: %s\n", fname);
  //printf("Receiving file...\n");
  fp = fopen(fname, "ab"); // open file
  if (NULL == fp)
  {
    printf("Error opening file");
    return 1;
  }
  long double sz = 1;

  // get the size of the file about to be sent, read funcions are same as prev functions
  int file_size;
  size_t payload_length;
  size_t n = readn(sockfd, (unsigned char *)&payload_length, sizeof(size_t));
  n = readn(sockfd, (unsigned char *)recvBuff, payload_length);
  file_size = atoi(recvBuff); // convert buffer data to int
  printf("file size is %i\n", file_size);
  int remain_data = file_size; // set this to the full size of the file
  //printf("%i\n", remain_data);

  /* Receive data in chunks of 1024 bytes */
  while (remain_data > 0)
  {
    bytesReceived = read(sockfd, recvBuff, 1024);
    sz++;
    //gotoxy(0, 4);
    //printf("Received: %llf Mb\n", (sz / 1024));
    //fflush(stdout);
    fwrite(recvBuff, 1, bytesReceived, fp);
    //printf("%s \n", recvBuff);
    printf("bytes recieved %i\n", bytesReceived);
    remain_data -= bytesReceived;
    bzero(recvBuff, 1024); // Clear the leftover stuff in the buffer
  }
  // check for function errors
  if (bytesReceived < 0)
  {
    printf("\n Read Error \n");
  }
  printf("\nFile OK....Completed\n");
  //close(sockfd);
}

/* checks that the file is on the server by sending a char array of user input
 * for the server funciton to seach for. Outputs a message to conosle if the file
 * is not there, otherwise it will return 0;
 */
int file_check(int socket, char fname[])
{
  size_t payload_length_fname = sizeof(fname);
  writen(socket, (unsigned char *)&payload_length_fname, sizeof(size_t));
  writen(socket, (unsigned char *)fname, payload_length_fname);

  char is_file[256] = "";
  size_t k;
  readn(socket, (unsigned char *)&k, sizeof(size_t));
  readn(socket, (unsigned char *)is_file, k);

  //printf(is_file);

  if (strcmp(is_file, "File present\n") == 0)
  {
    printf("%s", is_file);
    return 0;
  }
  printf(is_file);
  return 1;
}
