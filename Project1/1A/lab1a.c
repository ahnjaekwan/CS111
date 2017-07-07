#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <poll.h>

pid_t child_pid; 
pid_t parent_pid;
int to_child_pipe[2];
int from_child_pipe[2];
int shellFlag = 0; //check --shell
struct termios old_terminal; //save for restore
struct termios new_terminal;

void close_pipes(){
  close(to_child_pipe[0]);
  close(from_child_pipe[0]);
  close(to_child_pipe[1]);
  close(from_child_pipe[1]);
}

void restore_old_terminal() {
  if(shellFlag) {
    close_pipes();
    tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
    
    int status = 0;
    waitpid(child_pid, &status, 0);
    if(WIFEXITED(status))
      fprintf(stderr, "SHELL EXIT STATUS=%d \n", WEXITSTATUS(status));
    else if(WIFSIGNALED(status))
      fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d \n", WTERMSIG(status), WEXITSTATUS(status));
    else
      fprintf(stderr, "SHELL EXIT \n");
  }
  else
    tcsetattr(0, TCSANOW, &old_terminal);
  
  exit(0);
}

void sig_handler(int signum) {
  if(shellFlag == 1 && signum == SIGINT)
    kill(child_pid, SIGINT);
  if(shellFlag == 1 && signum == SIGPIPE)
    kill(child_pid, SIGPIPE);
  if(signum == SIGPIPE)
    exit(0);
}


void read_and_write(int readinput, int writeoutput) {  
  char buf[10];
  int nchar;
  int i = 0;
  int continueshell = 1;
  while(!shellFlag || continueshell) //no need to worry about exceeding buffer size because it gets character-at-a-time.
    {
      read(readinput, buf, 1);
      if((buf[i] == '\n' || buf[i] == '\r') && readinput == STDIN_FILENO  && writeoutput == STDOUT_FILENO) //buffer size should be at least 2; if received <cr> or <lf>, map into <cr><lf>
	{
	  buf[i] = '\r';
	  buf[i+1] = '\n';
	  write(writeoutput, buf, 2); //in this case, size of buf is 2 because we are writing <cr><lf>
	  continueshell = 0;
	}
      else if((buf[i] == '\n' || buf[i] == '\r') && readinput == STDIN_FILENO  && writeoutput != STDOUT_FILENO) //buffer size should be at least 2; if received <cr> or <lf>, map into <cr><lf>
	{
	  buf[i] = '\r';
	  buf[i+1] = '\n';
	  write(STDOUT_FILENO, buf, 2);
	  buf[i] = '\n';
	  write(writeoutput, buf, 1); //in this case, size of buf is 2 because we are writing <cr><lf>
	  continueshell = 0;
	}
      else if(buf[i] == '\n' && readinput != STDIN_FILENO  && writeoutput == STDOUT_FILENO)
	{
	  buf[i] = '\r';
	  buf[i+1] = '\n';
	  write(writeoutput, buf, 2);
	  continueshell = 0;
	}
      else if(buf[i] == 4) //ASCII of ^D is 4; when received character is ^D, restore back to old terminal and exit with code 0
	{
	  if(shellFlag == 1){
	    //send signal
	    //CLOSE PIPE TO SHELL
	    write(writeoutput, buf, 1);
	    close(readinput);
	    close(writeoutput);
	    kill(child_pid, SIGHUP);
	  }
	  break;
	}
      else if(buf[i] == 3 && shellFlag == 1)
	{
	  kill(child_pid, SIGINT);
	}
      else
	{
	  if(readinput == STDIN_FILENO && writeoutput != STDOUT_FILENO){
	    write(STDOUT_FILENO, buf, 1);
	  }
	  write(writeoutput, buf, 1); //write the received characters back out to the display, one character at a time
	}
    }
  return;
}


int
main (int argc, char** argv)
{
  tcgetattr(STDIN_FILENO, &old_terminal); //save old terminal in order to restore later
  atexit(restore_old_terminal);
  new_terminal = old_terminal;

  //set new terminal as non-canonical input mode with no echo
  new_terminal.c_iflag = ISTRIP;
  new_terminal.c_oflag = 0;
  new_terminal.c_lflag = 0;
  
  if (argc ==1){
    tcsetattr(0, TCSANOW, &new_terminal); //set new terminal I/O
    read_and_write(STDIN_FILENO, STDOUT_FILENO);
  }
    
  else if (argc>1)
    {
       static const struct option long_options[] =
	 {
	   {"shell", no_argument, NULL, 's'},
	   {0,0,0,0}
	 };

       while(1)
	{
	  int option_index = 0;
	  //store option and argument
	  int ch;
	  ch = getopt_long(argc, argv, "",long_options, &option_index);
	  
	  //if no argument
	  if(ch == -1) {
	    break;
	  }
	  
	  switch(ch) {
	  case 's' :
	    {
	      signal(SIGINT, sig_handler);
	      signal(SIGPIPE, sig_handler);
	      shellFlag = 1;
	      break;
	    }
	  default:
	    {
	      fprintf(stderr, "unrecognized argument!\n");
	      exit(1);
	    }
	  }
	}
       if (shellFlag == 1){
	 tcsetattr(0, TCSANOW, &new_terminal); //set new terminal I/O	 
	 child_pid = -1;

	 if (pipe(to_child_pipe) == -1) {
	   fprintf(stderr, "pipe() failed!\n");
	   exit(1);
	 }
	 if (pipe(from_child_pipe) == -1) {
	   fprintf(stderr, "pipe() failed!\n");
	   exit(1);
	 }
	 child_pid = fork();
	 parent_pid = getpid();

	 if (child_pid > 0) {
	   close(to_child_pipe[0]);
	   close(from_child_pipe[1]);

	   struct pollfd fds[2];
	   fds[0].fd = STDIN_FILENO;
	   fds[0].events = POLLIN | POLLHUP | POLLERR;
	   fds[1].fd = from_child_pipe[0];
	   fds[1].events = POLLIN | POLLHUP | POLLERR;

	   while(1) {
	     int value = poll(fds, 2, 0);
	     if(fds[0].revents & POLLIN) {
	       read_and_write(STDIN_FILENO, to_child_pipe[1]);//
	     }
	     if(fds[1].revents & POLLIN) {
	       read_and_write(from_child_pipe[0], STDOUT_FILENO);//
	     }
	     if(fds[0].revents & (POLLHUP + POLLERR)) {
	       fprintf(stderr, "error in shell!(1)");
	       break;
	     }	     
	     if(fds[1].revents & (POLLHUP + POLLERR)) {
	       fprintf(stderr, "error in shell!(2)");
	       break;
	     }
	   }
	 } else if (child_pid == 0) {
	   close(to_child_pipe[1]);
	   close(from_child_pipe[0]);
	   dup2(to_child_pipe[0], STDIN_FILENO);
	   dup2(from_child_pipe[1], STDOUT_FILENO);
	   dup2(from_child_pipe[2], STDERR_FILENO);
	   close(to_child_pipe[0]);
	   close(from_child_pipe[1]);

	   char *execvp_argv[2];
	   char execvp_filename[] = "/bin/bash";
	   execvp_argv[0] =execvp_filename;
	   execvp_argv[1] = NULL;
	   if (execvp(execvp_filename, execvp_argv) == -1) {
	     fprintf(stderr, "execvp() failed!\n");
	     kill(parent_pid, SIGPIPE);
	     exit(1);
	   }
	 } else {
	   fprintf(stderr, "fork() failed!\n");
	   exit(1);
	 }
       }
    }
  
  return 0;
}
