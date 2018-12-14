#include "server_functions.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include "rdwrn.h"
#include <sys/utsname.h>
#include <dirent.h>
#include <sys/stat.h>
#include <netdb.h>
// includes for getIp
#include <net/if.h>
#include <time.h>
// includes for stat etc
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>

#include <fcntl.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>

/* function to send a char array to the client. Also gets the ip
 * address of the server computer and sends that to the client
 */
void send_hello(int socket)
{
  char hostbuffer[256];
  char *IPbuffer;
  struct hostent *host_entry;
  int hostname;

  // To retrieve hostname
  hostname = gethostname(hostbuffer, sizeof(hostbuffer));
  if (hostname == -1)
  {
    perror("gethostname");
    exit(1);
  }

  // To retrieve host information
  host_entry = gethostbyname(hostbuffer);
  if (host_entry == NULL)
  {
    perror("gethostbyname");
    exit(1);
  }

  // To convert an Internet network
  // address into ASCII string
  IPbuffer = inet_ntoa(*((struct in_addr *)
                             host_entry->h_addr_list[0]));

  char hello_string_full[250];
  char hello_string[] = "hello SP Gavin Ross S1821951 from IP: ";

  //cat IPbuffer to the full string
  strcat(hello_string, IPbuffer);

  size_t n = strlen(hello_string) + 1;
  writen(socket, (unsigned char *)&n, sizeof(size_t));
  writen(socket, (unsigned char *)hello_string, n);
} // end send_hello()

/* generates random numbers for the get_and_send_ints funciton */
int generateRandNum()
{
  int randNum = rand();
  int inRangeNum = (randNum % 1000);
  printf("random number is: %d\n", inRangeNum);
  return inRangeNum;
}

/* sends an array of random numbers to the client */
void get_and_send_ints(int socket)
{
  // make random number array
  int array[5];
  int i;
  for (i = 0; i < 5; i++)
  {
    array[i] = generateRandNum(); //append each result to the array
  }
  size_t payload_length = sizeof(array);
  writen(socket, (unsigned char *)&payload_length, sizeof(size_t));
  writen(socket, (unsigned char *)array, payload_length);
}

/* send the system info to the client via system calls */
void send_uts(int socket)
{
  struct utsname uts;
  // check if the functon was successful
  if (uname(&uts) == -1)
  {
    perror("uname error");
    exit(EXIT_FAILURE);
  }

  printf("Node name:    %s\n", uts.nodename);
  printf("System name:  %s\n", uts.sysname);
  printf("Release:      %s\n", uts.release);
  printf("Version:      %s\n", uts.version);
  printf("Machine:      %s\n", uts.machine);

  size_t payload_length = sizeof(struct utsname);
  writen(socket, (unsigned char *)&payload_length, sizeof(size_t));
  writen(socket, (unsigned char *)&uts, payload_length);
}

/* a function to get information on each file in the uploads directory 
 * and print on server
 */
void stat_file(char *file)
{
  char file_path[50];
  file_path[0] = '\0';
  strcat(file_path, "./upload/");
  strcat(file_path, file);

  struct stat sb;
  // check if the function was successfull
  if (stat(file_path, &sb) == -1)
  {
    perror("stat");
    printf("error: stat failed\n");
  }

  printf("File type:                ");

  switch (sb.st_mode & S_IFMT)
  {
  case S_IFBLK:
    printf("block device\n");
    break;
  case S_IFCHR:
    printf("character device\n");
    break;
  case S_IFDIR:
    printf("directory\n");
    break;
  case S_IFIFO:
    printf("FIFO/pipe\n");
    break;
  case S_IFLNK:
    printf("symlink\n");
    break;
  case S_IFREG:
    printf("regular file\n");
    break;
  case S_IFSOCK:
    printf("socket\n");
    break;
  default:
    printf("unknown?\n");
    break;
  }

  printf("File size:                %lld bytes\n",
         (long long)sb.st_size);
  printf("\n");
}

/* a function to print all the file names from the uploads directory 
 * in a string and display them on the server console.
 */
int print_file_sizes()
{
  printf("\nThis is all the files with info in the uploads folder\n");
  struct dirent **namelist; // holds the data structure of file names
  int n;

  if ((n = scandir("./upload/", &namelist, NULL, alphasort)) == -1)
    perror("scandir");
  else
  {
    while (n--)
    {
      printf("\nFile name:		  %s\n", namelist[n]->d_name);
      stat_file(namelist[n]->d_name);
      free(namelist[n]); //NB
    }
    free(namelist); //NB
  }
  return 0;
}

int file_filter(const struct dirent *dir) 
{
	if (dir->d_name[0] != '.') {
		return 1;
	}
	else
		return 0;
}
/* a function to send all the file names appending in one char array with a 
 * \n as a seperator.
 */
void send_file_names(int socket)
{
  /* check if the upload dir is there, if not then create it */
  struct stat st = {0};
  if (stat("./upload", &st) == -1) {
    mkdir("./upload", 0700);
  }

  struct dirent **namelist;
  int status;
  int n;

  /** get the length of each filename in the directory to make an array */
  int char_count = 0; // this is to hold the length of how long the filelist[] will be
  int current_file = scandir("upload", &namelist, file_filter, alphasort);
  // counts the length of the filelist string
  while (current_file--)
  {
    // get the lenght of the string
    int str_length = strlen(namelist[current_file]->d_name);
    char_count += (str_length + 1); // plus 1 for each \n ??????
  }

  /* // This holds the total length of the char array of file names */
  char filelist[500] = "";

  // scan all the files in the uplaod directory
  n = scandir("upload", &namelist, file_filter, alphasort);
  if (n < 0)
    perror("scandir");
  else
  {
    while (n--)
    {
      strcat(filelist, namelist[n]->d_name);
      strcat(filelist, "\n");
      free(namelist[n]);
    }
    free(namelist);
  }

  strcat(filelist, "\0"); // add this to end the stirng

  // send the string
  size_t string = strlen(filelist) + 1;
  writen(socket, (unsigned char *)&string, sizeof(size_t));
  writen(socket, (unsigned char *)filelist, string);
} // end of send_file_names()

/* a functon to send the current time on the server to the client via a system call */
void send_time(int socket)
{
  time_t t; // always look up the manual to see the error conditions
  //  here "man 2 time"
  if ((t = time(NULL)) == -1)
  {
    perror("time error");
    exit(EXIT_FAILURE);
  }

  // localtime() is in standard library so error conditions are
  //  here "man 3 localtime"
  struct tm *tm;
  if ((tm = localtime(&t)) == NULL)
  {
    perror("localtime error");
    exit(EXIT_FAILURE);
  }

  printf("%s\n", asctime(tm));

  //char hello_string[100];
  size_t n = strlen(asctime(tm)) + 1;
  writen(socket, (unsigned char *)&n, sizeof(size_t));
  writen(socket, (unsigned char *)asctime(tm), n);
}

/* ================================== send files functions =============================*/

/* function to send a file from the server upload directory to the client current 
 * working directory.
 */
int send_file3(int arg)
{
  int connfd = arg;
  printf("Connection accepted and id: %d\n", connfd);

  /* Get file name and Create file where data will be stored */
  char fname[256] = "";
  size_t k;
  readn(connfd, (unsigned char *)&k, sizeof(size_t));
  readn(connfd, (unsigned char *)fname, k);

  // add the file name to the fname_with_dir to search for the file
  char fname_with_dir[256] = "./upload/";
  strcat(fname_with_dir, fname);

  // open a stream with the file
  FILE *fp = fopen(fname_with_dir, "rb");
  if (fp == NULL)
  {
    printf("File opern error");
    return 1;
  }

  /* // code to send filesize */
  struct stat *buf;
  buf = malloc(sizeof(struct stat)); // create space on the heap

  stat(fname, buf);
  int size = buf->st_size;
  printf("file size is %d\n", size);

  /* Get file stats */
  char convert_int[64];
  sprintf(convert_int, "%d", size);
  size_t payload_length = sizeof(convert_int);

  // write to client
  writen(connfd, (unsigned char *)&payload_length, sizeof(size_t));
  writen(connfd, (unsigned char *)convert_int, payload_length);

  /* Read data from file and send it*/
  while (1)
  {
    /* First read file in chunks of 256 bytes */
    unsigned char buff[1024] = {0};
    int nread = fread(buff, 1, 1024, fp);
    //printf("Bytes read %d \n", nread);

    /* If read was success, send data. */
    if (nread > 0)
    {
      printf("Sending \n");
      write(connfd, buff, nread);
    }
    if (nread < 1024)
    {
      if (feof(fp))
      {
        printf("End of file\n");
        printf("File transfer completed for id: %d\n", connfd);
      }
      if (ferror(fp))
        printf("Error reading\n");
      break;
    }
  }
  printf("Closing Connection for id: %d\n", connfd);
  close(fp); // close file stream
  free(buf); // free buf memory
  return 0;
}

/* function to check if a file is on the server upload directory. It takes a string
 * sent over by the client and searches through the directory for a match. 
 * Returns 0 for a match, 1 is it's not there.
 */
int file_check(int socket)
{
  struct dirent **namelist;
  int status;
  int n;

  char filelist[500] = "";
  char is_file = 'n';
  char not_file = 'n';

  /* Get file name and Create file where data will be stored */
  char fname[256] = "";
  size_t k;
  readn(socket, (unsigned char *)&k, sizeof(size_t));
  readn(socket, (unsigned char *)fname, k);

  n = scandir("upload", &namelist, NULL, alphasort);
  if (n < 0)
    perror("scandir");
  else
  {
    while (n--)
    {
      printf("%s\n", namelist[n]->d_name); // print the name of the file
      if (strcmp(namelist[n]->d_name, fname) == 0)
      {
        is_file = 'y';
        break;
      }
      else
        is_file = 'n';
      free(namelist[n]);
    }
    free(namelist);
  }

  //printf("char count = %d\n", char_count);
  printf("%c\n", is_file);

  // send the string
  char is_file_string[] = "File present\n";
  char not_file_string[] = "File not found\n";

  size_t is_file_ln = strlen(is_file_string) + 1;
  size_t not_file_ln = strlen(not_file_string) + 1;

  if (is_file == 'y')
  {
    writen(socket, (unsigned char *)&is_file_ln, sizeof(size_t));
    writen(socket, (unsigned char *)is_file_string, is_file_ln);
    return 0;
  }
  writen(socket, (unsigned char *)&not_file_ln, sizeof(size_t));
  writen(socket, (unsigned char *)not_file_string, not_file_ln);
  return 1;
}

/* a function to get the ip address of the current device acting as a server */
int getIp()
{

  char hostbuffer[256];
  char *IPbuffer;
  struct hostent *host_entry;
  int hostname;

  // To retrieve hostname
  hostname = gethostname(hostbuffer, sizeof(hostbuffer));
  if (hostname == -1)
  {
    perror("gethostname");
    exit(1);
  }

  // To retrieve host information
  host_entry = gethostbyname(hostbuffer);
  if (host_entry == NULL)
  {
    perror("gethostbyname");
    exit(1);
  }

  // To convert an Internet network
  // address into ASCII string
  IPbuffer = inet_ntoa(*((struct in_addr *)
                             host_entry->h_addr_list[0]));

  //printf("Hostname: %s\n", hostbuffer);
  printf("Host IP: %s\n", IPbuffer);

  return 0;
}
