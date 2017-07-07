#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

void my_handler(int signum)
{  
  char my_message[35] = "signal received, now quitting...\n";
  if(signum == SIGSEGV)
    {
      write(2, &my_message, 34);
      exit(4);
    }
}

int main(int argc, char** argv)
{
  int ch;
  int fd0, fd1;
  int s_flag = 0;
  static const struct option long_options[] =
    {
      {"input", required_argument, NULL, 'i'},
      {"output", required_argument, NULL, 'o'},
      {"segfault", no_argument, NULL, 's'},
      {"catch", no_argument, NULL, 'c'},
      {0, 0, 0, 0}
    };
  
  while(1)
    {
      int option_index = 0;
      //store option and argument
      ch = getopt_long(argc, argv, "i:o:sc",long_options, &option_index);
      
      //if no argument
      if(ch == -1)
	{
	  break;
	}
      
      switch (ch) {
      case 'i':
	{
	  //open file descriptor
	  fd0 = open(optarg,O_RDONLY);

	  //report failure when unable to open
	  if(fd0==-1)
	    {
	      fprintf(stderr, "Cannot open the input file.\n");
	      //printf("Error: %s\n", strerror(errno));
	      exit(2);
	    }

	  //making it as new fd0
	  dup2(fd0,0);
	  close(fd0);
	  break;
	}
      case 'o':
	{
	  //open file descriptor
	  fd1 = creat(optarg,00644);

	  //report failure if unable to write
	  if(fd1==-1)
	    {
	      fprintf(stderr, "The file is not writable.\n");
	      //printf("%s\n", strerror(errno));
	      exit(3);
	    }
	  
	  //making it as new fd1
	  dup2(fd1,1);
	  close(fd1);
	  break;
	}
      case 's':
	{
	  //set the flag in order to treat --catch and not to copy from stdin to stdout
	  s_flag = 1;
      	  break;
	}
      case 'c':
	{
	  //if there is a segfault, then exit with return code of 4
	  signal(SIGSEGV, my_handler);
	  break;
	}
      default:
	{
	  //if there is unrecognized argument
	  fprintf(stderr, "Unrecognized argument.\n");
	  exit(1);
	}
      }
    };
  if(s_flag != 1)
    {
      //if there is no segfault, copy stdin to stdout
      char* buffer;
      buffer = (char*) malloc(sizeof(char));
    
      ssize_t status = read(0,buffer,1);
      while(status > 0)
	{
	  write(1,buffer,1);
	  status = read(0,buffer,1);
	}
      free(buffer);
    }
  else
    {
      //segfault
      char *ptr = NULL;
      *ptr = '0';
    }
  
  //if program succeeds, exit with code 0
  exit(0);
}
