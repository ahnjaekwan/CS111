/*
NAME: Jaekwan Ahn
EMAIL: ahnjk0513@gmail.com
ID: 604057669
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> //long option
#include <time.h> //get_time
#include <string.h> //strcmp
#include <strings.h> //bzero
#include "mraa.h"
#include "mraa/aio.h"
#include <poll.h>
#include <unistd.h> //read
#include <math.h> //log

#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

SSL_CTX *SSL_CTX_new(const SSL_METHOD *method);
int SSL_CTX_up_ref(SSL_CTX *ctx);

const SSL_METHOD *TLS_method(void);
const SSL_METHOD *TLS_server_method(void);
const SSL_METHOD *TLS_client_method(void);

SSL *ssl;
SSL_CTX *ctx;
int tls = 0; //flag
char* id = "604057669";
char* host = "lever.cs.ucla.edu";
int portnum;
int sock_fd;

int period = 1;
char scale = 'F';
int log_flag = 0;
int log_fd;
int stop = 0; //flag for input "stop"
char curtime[9];
int begin = 1; //just to check if it's beginning
mraa_aio_context temp;
struct timespec start, end;
int untilstoptime = 0;

void initialize_ctx() {
    SSL_load_error_strings();
	OpenSSL_add_all_algorithms();
    SSL_library_init();

    SSL_METHOD const *client_method = SSLv23_client_method();
    ctx = SSL_CTX_new(client_method);
    if(ctx == NULL){
        ERR_print_errors_fp(stderr);
        abort();
    }
}

void open_TCP(char* host, int port) {
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd < 0){
	    fprintf(stderr, "ERROR in socket()\n");
	    exit(2);
  	}

	struct hostent *server = malloc(sizeof(struct hostent));
	server = gethostbyname(host);
	if(server == NULL){
	    fprintf(stderr,"ERROR in gethostbyname()\n");
	    exit(1);
	}

	struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);

	int ret = connect(sock_fd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
	if(ret < 0){
	    fprintf(stderr, "ERROR in connect()\n");
	    exit(2);
	}

	char buffer[30];
	if(tls == 1){
		initialize_ctx();
	    ssl = SSL_new(ctx);
	    SSL_set_fd(ssl, sock_fd);
	    if (SSL_connect(ssl) == -1) {
	      fprintf(stderr, "SSL_connect() failed: %s\n", strerror(errno));
	      exit(2);
	    }
	    bzero(buffer, 30);
	    sprintf(buffer, "ID=%s\n", id);
	    SSL_write(ssl, buffer, strlen(buffer));
	} else{
		dprintf(sock_fd, "ID=%s\n", id);
	}
}

void close_all() { //close and free everything
  	mraa_aio_close(temp);
}

int get_time() { //store time info into "curtime" and return current second
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
	if(lt == NULL){
		fprintf(stderr, "ERROR in localtime\n");
		exit(2);
	}
	if(strftime(curtime, sizeof(curtime), "%H:%M:%S", lt) == 0) {
		fprintf(stderr, "ERROR in strftime\n");
		exit(2);
	}
	return lt->tm_sec;
}

void Shutdown() {
	char buffer[30];
	int trash = get_time(); //return value will not be used
	if(log_flag){
		dprintf(log_fd, "%s SHUTDOWN\n", curtime);
	}
	if(tls == 1){
		bzero(buffer, 30);
	    sprintf(buffer, "%s SHUTDOWN\n", curtime);
	    SSL_write(ssl, buffer, strlen(buffer));
	} else{
	    dprintf(sock_fd, "%s SHUTDOWN\n", curtime);	
	}
	close_all();
	exit(0);
}

void get_temp() {
	int period_on = 0;
	if(begin){
		period_on = 1;
	} else{
		if(clock_gettime(CLOCK_MONOTONIC, &end)){
			fprintf(stderr, "ERROR in clock_gettime()\n");
   			exit(2);
  		}
  		int runtime = (end.tv_sec - start.tv_sec) + untilstoptime;
  		if(runtime > 0 && (runtime % period) == 0){ //check if it is in period
  			period_on = 1;
  		}
	}
	char buffer[30];
	if(period_on){
		const int B = 4275;               // B value of the thermistor
		const int R0 = 100000;            // R0 = 100k
		int a = mraa_aio_read(temp);
		if(a < 0){
			fprintf(stderr, "ERROR in mraa_aio_read()\n");
			exit(2);
		}
		double R = 1023.0/a-1.0;
		R = R0*R;
	    double temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet
		//temperature here is now in Celcius
		int trash = get_time(); //return value will not be used
		if(scale == 'F'){
		   	printf("%s %04.1f\n", curtime, temperature * 1.8 + 32);
		  	if (log_flag){
		  		dprintf(log_fd, "%s %04.1f\n", curtime, temperature * 1.8 + 32);
		  	}
		  	if (tls == 1){
		        bzero(buffer, 30);
		        sprintf(buffer, "%s %04.1f\n", curtime, temperature * 1.8 + 32);
		        SSL_write(ssl, buffer, strlen(buffer));
		  	} else{
		  		dprintf(sock_fd, "%s %04.1f\n", curtime, temperature * 1.8 + 32);
		  	}
		} else{ //when scale is C
		   	printf("%s %04.1f\n", curtime, temperature);
		   	if (log_flag){
		  		dprintf(log_fd, "%s %04.1f\n", curtime, temperature);
		  	}
		  	if (tls == 1){
		        bzero(buffer, 30);
		        sprintf(buffer, "%s %04.1f\n", curtime, temperature);
		        SSL_write(ssl, buffer, strlen(buffer));
		  	} else{
		  		dprintf(sock_fd, "%s %04.1f\n", curtime, temperature);
		  	}
		}
		//start counting based on period
		if(clock_gettime(CLOCK_MONOTONIC, &start)){
			fprintf(stderr, "ERROR in clock_gettime()\n");
	   		exit(2);
	  	}
	  	untilstoptime = 0;
	  	begin = 0;
	}
}

void input_action(char str_in[10]) { //this function execute each input stdin
	if(strcmp(str_in, "OFF\n") == 0){
		if(log_flag){ //print input in log file
		   	dprintf(log_fd, "%s", str_in);
		}
		Shutdown();
	} else if(strcmp(str_in, "STOP\n") == 0){
		if(log_flag){ //print input in log file
		    dprintf(log_fd, "%s", str_in);
		}
		stop = 1; //set the stop flag
		if(clock_gettime(CLOCK_MONOTONIC, &end)){
			fprintf(stderr, "ERROR in clock_gettime()\n");
   			exit(2);
  		}
  		untilstoptime = (end.tv_sec - start.tv_sec);
	} else if(strcmp(str_in, "START\n") == 0){
		if(log_flag){ //print input in log file
		    dprintf(log_fd, "%s", str_in);
		}
		stop = 0; //unset the stop flag
		if(clock_gettime(CLOCK_MONOTONIC, &start)){
			fprintf(stderr, "ERROR in clock_gettime()\n");
   			exit(2);
  		}
	} else if(strcmp(str_in, "SCALE=F\n") == 0){
		if(log_flag){ //print input in log file
		    dprintf(log_fd, "%s", str_in);
		}
		scale = 'F';
	} else if(strcmp(str_in, "SCALE=C\n") == 0){
		if(log_flag){ //print input in log file
		    dprintf(log_fd, "%s", str_in);
		}
		scale = 'C';
	} else if(strncmp(str_in, "PERIOD=", 7) == 0){
		period = atoi(str_in + 7);
		if(period <= 0){
	    	fprintf(stderr, "ERROR in setting period\n");
			close_all();
    		exit(2);
	    }
		if(log_flag){ //print input in log file
		    dprintf(log_fd, "%s", str_in);
		}
	} else{
		dprintf(log_fd, "ERROR in invalid argument\n");
		close_all();
		exit(2);
	}
}

int main(int argc, char *argv[])
{
  	//takes parameters for id, host and log with "--" option
  	static const struct option long_options[] =
		{
	 		{"id", required_argument, NULL, 'i'},
	 		{"host", required_argument, NULL, 'h'},
	 		{"log", required_argument, NULL, 'l'},
	 		{0,0,0,0}
		};

	int ch;
  	while(1){
	  	ch = getopt_long(argc, argv, "",long_options, NULL);
	  	if(ch == -1) {
	    	break;
	  	}
	  	switch(ch) {
	  	case 'i' :
	  		if(strlen(optarg) != 9){
	    		fprintf(stderr, "ERROR: ID should be 9 digit number\n");
    			exit(1);
	  		}
	    	id = optarg;
	    	break;
	  	case 'h' :
	    	host = optarg;
	    	break;
	  	case 'l' :
	  		log_flag = 1;
	  		log_fd = creat(optarg,00666);
	    	break;
	  	default:
	    	fprintf(stderr, "ERROR, no such option\n");
	    	exit(1);
	  	}
    }
    //takes parameter for tls and port number
    if(strstr(argv[0], "tls") != NULL){
    	tls = 1;
    }
    portnum = atoi(argv[argc - 1]);

    //open TCP connection to the server at the specified address and port
    open_TCP(host, portnum);

	//initialize mraa
    temp = mraa_aio_init(0);
    if (temp == NULL){
        fprintf(stderr, "ERROR in mraa_aio_init()\n");
        mraa_aio_close(temp);
		exit(2);
    }
    
    struct pollfd fd[1];
	fd[0].fd = sock_fd;
   	fd[0].events = POLLIN | POLLHUP | POLLERR;

   	int ret, len, i, j;
   	char str_input[50], buf[10]; //str_input is whole stdin and buf is for single instruction
	while(1){
	    int value = poll(fd, 1, 0);
	    if(value == -1){
	    	fprintf(stderr, "ERROR in poll()\n");
	    	close_all();
	    	exit(2);
	    } else if(value > 0){
			//check input from stdin
			if(fd[0].revents & POLLIN) {
		    	bzero(str_input, 50); //empty the string for storing input
		    	bzero(buf, 10); //empty the string
		    	if(tls == 1){
		    		ret = SSL_read(ssl, str_input, 50);
		    		if(ret <= 0){
						fprintf(stderr, "ERROR in SSL_read()\n");
						close_all();
						exit(2);
		    		}
		    	} else{
			    	ret = read(fd[0].fd, str_input, 50); //read input stdin
			    	if(ret == 0){ //EOF
			    		break;
		    		} else if(ret < 0){
						fprintf(stderr, "ERROR in read()\n");
						close_all();
						exit(2);
		    		}
		    	}
		    	len = strlen(str_input);
		    	j = 0;
		    	for(i = 0; i < len; i++){
		    		if(str_input[i] == '\n'){
		    			buf[j] = str_input[i];
		    			input_action(buf);
		    			bzero(buf, 10);
		    			j = 0;
		    		} else if(str_input[i] == '\0'){ //EOF
		    			break;
		    		} else{
		    			buf[j] = str_input[i];
		    			j++;
		    		}
		    	}
		    	//temperature
		    	if(stop == 0){
			    	get_temp();
	    		}
			}
			if(fd[0].revents & POLLERR) {
				fprintf(stderr, "ERROR in read()\n");
				close_all();
				exit(2);
			}
			if(fd[0].revents & POLLHUP) {
				break;
			}
		} else{ //the call timed out and no file descriptors were ready: MAIN LOOP
	    	//temperature
	    	if(stop == 0){
		    	get_temp();
    		}
		}
	}
	
    close_all();
    return 0;
}
