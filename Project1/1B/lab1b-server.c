#include <mcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <poll.h>

pid_t child_pid; 
int to_child_pipe[2];
int from_child_pipe[2];
MCRYPT M1, M2;

void error1(char *msg) {
    perror(msg);
    exit(1);
}
void harvest_status(){
	int status;
	if (waitpid(child_pid, &status, 0) == -1)
		error1("ERROR on waitpid()");

	mcrypt_generic_deinit(M1);
  	mcrypt_generic_deinit(M2);
  	mcrypt_module_close(M1);
  	mcrypt_module_close(M2); 
  	/*
    if(WIFEXITED(status))
      	fprintf(stderr, "SHELL EXIT STATUS=%d\n", WEXITSTATUS(status));
    else if(WIFSIGNALED(status))
      	fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
    else
      	fprintf(stderr, "SHELL EXIT\n");
    */
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
}
void error(char *msg) {
    perror(msg);
    harvest_status();
    exit(1);
}
/*
void pipe_handler(int signo) {
  harvest_status();
  exit(0);
}
*/
int main(int argc, char *argv[])
{
    int encryptFlag = 0;
    int portno, keyfd;
    char buffer[256];
    while(1)
    {
      static const struct option long_options[] =
      {
        {"port", required_argument, NULL, 'p'},
        {"encrypt", required_argument, NULL, 'e'},
        {0,0,0,0}
      };
      int option_index = 0;
      //store option and argument
      int ch;
      ch = getopt_long(argc, argv, "p:e",long_options, &option_index);
      
      //if no argument
      if(ch == -1) {
        break;
      }
      
      switch(ch) {
        case 'p' :
          portno = atoi(optarg);
          break;
        case 'e' :
          encryptFlag = 1;
          keyfd = open(optarg, O_RDONLY);
          break;
        default:
          error1("ERROR, no such option");
      }
    }

    int sockfd, newsockfd, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
	
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error1("ERROR oon socket()");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error1("ERROR on bind()");
    
    listen(sockfd,5);

   	clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) 
        error1("ERROR on accept()");

	if (pipe(to_child_pipe) == -1 || pipe(from_child_pipe) == -1) 
		error1("ERROR on pipe()");
	child_pid = fork();

	if (child_pid < 0) {
		error1("ERROR on fork");
	} 
	else if (child_pid ==0){
	   	close(to_child_pipe[1]);
	   	close(from_child_pipe[0]);
	   	dup2(to_child_pipe[0], STDIN_FILENO);
	   	dup2(from_child_pipe[1], STDOUT_FILENO);
	   	dup2(from_child_pipe[1], STDERR_FILENO);
	   	close(to_child_pipe[0]);
	   	close(from_child_pipe[1]);

	   	char *execvp_argv[2];
	   	char execvp_filename[] = "/bin/bash";
	   	execvp_argv[0] =execvp_filename;
	   	execvp_argv[1] = NULL;
	   	if (execvp(execvp_filename, execvp_argv) == -1)
	   		error("ERROR on execvp()");
	}
	else {
		/*
		struct sigaction sa;
		sa.sa_handler = pipe_handler;
      	sigemptyset(&sa.sa_mask);
      	sa.sa_flags = 0;

	    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
	        error("ERROR on sigaction()");
	        exit(1);
	    }
		*/

      	char keystring[20];
      	bzero(keystring, 20);
      	int keylength = 0;
		char *initialized = "initialized";
		if(encryptFlag){
			M1 = mcrypt_module_open("twofish", NULL, "cfb", NULL);
        	M2 = mcrypt_module_open("twofish", NULL, "cfb", NULL);
        	read(keyfd, keystring, 16);
        	keylength = strlen(keystring);
        	mcrypt_generic_init(M1, keystring, keylength, initialized);
        	mcrypt_generic_init(M2, keystring, keylength, initialized);
		}

	   	close(to_child_pipe[0]);
	   	close(from_child_pipe[1]);

	   	struct pollfd fds[2];
	   	fds[0].fd = newsockfd;
	   	fds[0].events = POLLIN | POLLHUP | POLLERR;
	   	fds[1].fd = from_child_pipe[0];
	   	fds[1].events = POLLIN | POLLHUP | POLLERR;

	   	int i =0;
	   	int j =0;
	   	int readnum = 0;
	   	int value, length;

	   	while(1) {
        	value = poll(fds, 2, 0);
        	if(value == -1){
        		error("ERROR on poll()");
        	}
        	else if (value > 0){
        		for(i = 0; i<2;i++){
			        if(fds[i].revents & POLLIN) {
			            bzero(buffer,256);
			            readnum =read(fds[i].fd, buffer, 256);
				    	length = strlen(buffer);	
			           	if(readnum < 0){
			    			error("ERROR on read()");                
			    		}
			    		else{
			    			if(i == 0){
			    				if(encryptFlag)
			    					mdecrypt_generic(M2, buffer, length);
			    				if(readnum == 0)		
				    				kill(child_pid, SIGTERM);
			    				for(j=0;j<length;j++){
									if(buffer[j] == 3){
			    						kill(child_pid, SIGINT);
			    						break;
			    					}
			    					else if (buffer[j] == 4){
			    						close(to_child_pipe[1]);
			    						break;
			    					}
			    					else
				        				write(to_child_pipe[1], buffer +j, 1);
			    				}
			    			}
			    			else if(i == 1){
			    				if(encryptFlag)
			    					mcrypt_generic(M1, buffer, length);
			    				write(fds[0].fd, buffer, length);
			    			}
			            }
			        }
			        if(fds[i].revents & POLLERR) {
			            harvest_status();
			            exit(0);
		        	}
		        	if(fds[i].revents & POLLHUP && (i == 1)) {
				        close(from_child_pipe[0]);
			            harvest_status();
			            exit(0);
		        	}
		        }
        	}
	   	}
	} 

    return 0; 
}
