/*
NAME: JAEKWAN AHN
EMAIL: ahnjk0513@gmail.com
ID: 604057669
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <strings.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <getopt.h>
#include <fcntl.h>
#include <poll.h>
#include <mcrypt.h>

struct termios old_terminal; //save for restore
struct termios new_terminal;
MCRYPT M1, M2;
int logfd;
int logFlag = 0;
int encryptFlag = 0;

void restore_old_terminal() {
  tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);

  mcrypt_generic_deinit(M1);
  mcrypt_generic_deinit(M2);
  mcrypt_module_close(M1);
  mcrypt_module_close(M2); 
}
void error(char *msg) {
  perror(msg);
  restore_old_terminal();
  exit(1);
}
/*void sig_handler(int signum) {
  //tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
  if(signum == SIGINT)
    exit(0);
  if(signum == SIGPIPE)
    exit(0);
}*/
void sent_log(char sent[], int log_fd){
    char sentbuffer1[300];
    sprintf(sentbuffer1, "SENT %lu bytes: %s\n", strlen(sent), sent);
    write(log_fd, sentbuffer1, strlen(sentbuffer1));
}
void received_log(char received[], int log_fd){
    char receivedbuffer1[300];
    sprintf(receivedbuffer1, "RECEIVED %lu bytes: %s\n", strlen(received), received);
    write(log_fd, receivedbuffer1, strlen(receivedbuffer1));
}
int read_and_write(int readinput, int writeoutput, char logbuffer[]) {  
  char buf;
  int k = 0;
  int readstatus;
  while(1) //no need to worry about exceeding buffer size because it gets character-at-a-time.
  {
    readstatus = read(readinput, &buf, 1);
    if(readstatus < 0)
      error("ERROR on read()");
    else if(readstatus == 0)
      return 0;
    if((buf == '\n' || buf == '\r') && readinput == STDIN_FILENO) {
	    if(encryptFlag){
        if(encryptFlag && readinput != STDIN_FILENO)
          mdecrypt_generic(M2, &buf, 1);
        else if(encryptFlag && readinput == STDIN_FILENO)
          mcrypt_generic(M1, &buf, 1);
      }
      buf = '\r';
      write(STDOUT_FILENO, &buf, 1);
      buf = '\n';
      write(writeoutput, &buf, 1);
      write(STDOUT_FILENO, &buf, 1);
      logbuffer[k++] = '\n';
      break;
    }
    else if(buf == '\n') {
      buf = '\r';
      write(writeoutput, &buf, 1);
      buf = '\n';
      write(writeoutput, &buf, 1);
      if(encryptFlag && readinput != STDIN_FILENO)
        mdecrypt_generic(M2, &buf, 1);
      else if(encryptFlag && readinput == STDIN_FILENO)
        mcrypt_generic(M1, &buf, 1);
      logbuffer[k++] = '\n';
      break;
    }
	  else if(buf == 3 || buf ==4){
	    write(writeoutput, &buf, 1);
      if(encryptFlag && readinput != STDIN_FILENO)
        mdecrypt_generic(M2, &buf, 1);
      else if(encryptFlag && readinput == STDIN_FILENO)
        mcrypt_generic(M1, &buf, 1);
	    break;
	  }
    else {
      if(readinput == STDIN_FILENO && writeoutput != STDOUT_FILENO)
        write(STDOUT_FILENO, &buf, 1);
      write(writeoutput, &buf, 1);
      if(encryptFlag && readinput != STDIN_FILENO)
        mdecrypt_generic(M2, &buf, 1);
      else if(encryptFlag && readinput == STDIN_FILENO)
        mcrypt_generic(M1, &buf, 1);
      logbuffer[k++] = buf;
    }

    if(strlen(logbuffer)==255){
      if(readinput == STDIN_FILENO){
         sent_log(logbuffer, logfd);
         bzero(logbuffer,256);
         k=0;
      }
      else{
        received_log(logbuffer, logfd);
        bzero(logbuffer,256);
        k=0;
      }
    }

  }
  return 1;
}

int main(int argc, char *argv[])
{
    int portno, keyfd;
    int encryptFlag = 0;

    while(1)
    {
      static const struct option long_options[] =
      {
        {"port", required_argument, NULL, 'p'},
        {"log", required_argument, NULL, 'l'},
        {"encrypt", required_argument, NULL, 'e'},
        {0,0,0,0}
      };
      int option_index = 0;
      //store option and argument
      int ch;
      ch = getopt_long(argc, argv, "p:l:e",long_options, &option_index);
      
      //if no argument
      if(ch == -1) {
        break;
      }
      
      switch(ch) {
        case 'p' :
          portno = atoi(optarg);
          break;
        case 'l' :
          logfd = creat(optarg, 00666);
          if(logfd < 0)
            error("ERROR creating log file");
          logFlag = 1;
          break;
        case 'e' :
          encryptFlag = 1;
          keyfd = open(optarg, O_RDONLY);
          break;
        default:
          error("ERROR, no such option");
      }
    }

    int sockfd, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    server = gethostbyname("localhost");
    if (server == NULL)
        error("ERROR, no such host");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    //Set the non-canonical mode
    if(!isatty(STDIN_FILENO))
        error("ERROR terminal");
    tcgetattr(STDIN_FILENO, &old_terminal); //save old terminal in order to restore later

    //set new terminal as non-canonical input mode with no echo
    tcgetattr(STDIN_FILENO, &new_terminal);
    new_terminal.c_iflag = ISTRIP;
    new_terminal.c_oflag = 0;
    new_terminal.c_lflag = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal);

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

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN | POLLHUP | POLLERR;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN | POLLHUP | POLLERR;

    char logbuffer[256];
    int i = 0;
    while(1) {
        int value = poll(fds, 2, 0);
	      if(value == -1){
	        error("ERROR poll()");
	      }
	      else if(value > 0){
	        for(i =0; i<2; i++){
             if(fds[i].revents & POLLIN) {
                 bzero(logbuffer,256);
	               if(i==0){
	                 if(read_and_write(fds[0].fd, fds[1].fd, logbuffer) == 0){
                      restore_old_terminal();
                      exit(0);
                    }
	                 if(logFlag){
		                  sent_log(logbuffer, logfd);
                       bzero(logbuffer,256);
                   }
	               }
	               else if(i==1){
	                  if(read_and_write(fds[1].fd, STDOUT_FILENO, logbuffer) == 0){
                      restore_old_terminal();
                      exit(0);
                    }
                    if(logFlag){
                      received_log(logbuffer, logfd);
                      bzero(logbuffer,256);
                    }                    
	               }
            }
            if(fds[i].revents & (POLLHUP + POLLERR)) {
              restore_old_terminal();
              exit(0);
            } 
	       }
      }
    }
  return 0;  
}
