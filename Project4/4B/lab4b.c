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

int period = 1;
char scale = 'F';
int log_flag = 0;
int log_fd;
int stop = 0; //flag for input "stop"
char curtime[9];
int begin = 1; //just to check if it's beginning
mraa_aio_context temp;
mraa_gpio_context gpio_btn;
struct timespec start, end;
int untilstoptime = 0;

void close_all() { //close and free everything
  	mraa_aio_close(temp);
    mraa_gpio_close(gpio_btn);
 	return;
}

int get_time() { //store time info into "curtime" and return current second
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
	if(lt == NULL){
		fprintf(stderr, "ERROR in localtime\n");
		exit(1);
	}
	if(strftime(curtime, sizeof(curtime), "%H:%M:%S", lt) == 0) {
		fprintf(stderr, "ERROR in strftime\n");
		exit(1);
	}
	return lt->tm_sec;
}

void shutdown() {
	int trash = get_time(); //return value will not be used
	if(log_flag){
		dprintf(log_fd, "%s SHUTDOWN\n", curtime);
	}
	close_all();
	exit(0);
	return;
}

void get_temp() {
	int period_on = 0;
	if(begin){
		period_on = 1;
	} else{
		if(clock_gettime(CLOCK_MONOTONIC, &end)){
			fprintf(stderr, "ERROR in clock_gettime()\n");
   			exit(1);
  		}
  		int runtime = (end.tv_sec - start.tv_sec) + untilstoptime;
  		if(runtime > 0 && (runtime % period) == 0){ //check if it is in period
  			period_on = 1;
  		}
	}

	if(period_on){
		const int B = 4275;               // B value of the thermistor
		const int R0 = 100000;            // R0 = 100k
		int a = mraa_aio_read(temp);
		if(a < 0){
			fprintf(stderr, "ERROR in mraa_aio_read()\n");
			exit(1);
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
		} else{ //when scale is C
		   	printf("%s %04.1f\n", curtime, temperature);
		   	if (log_flag){
		  		dprintf(log_fd, "%s %04.1f\n", curtime, temperature);
		  	}
		}
		//start counting based on period
		if(clock_gettime(CLOCK_MONOTONIC, &start)){
			fprintf(stderr, "ERROR in clock_gettime()\n");
	   		exit(1);
	  	}
	  	untilstoptime = 0;
	  	begin = 0;
	}
	return;
}

void input_action(char str_in[10]) { //this function execute each input stdin
	if(strcmp(str_in, "OFF\n") == 0){
		if(log_flag){ //print input in log file
		   	dprintf(log_fd, "%s", str_in);
		}
		shutdown();
	} else if(strcmp(str_in, "STOP\n") == 0){
		if(log_flag){ //print input in log file
		    dprintf(log_fd, "%s", str_in);
		}
		stop = 1; //set the stop flag
		if(clock_gettime(CLOCK_MONOTONIC, &end)){
			fprintf(stderr, "ERROR in clock_gettime()\n");
   			exit(1);
  		}
  		untilstoptime = (end.tv_sec - start.tv_sec);
	} else if(strcmp(str_in, "START\n") == 0){
		if(log_flag){ //print input in log file
		    dprintf(log_fd, "%s", str_in);
		}
		stop = 0; //unset the stop flag
		if(clock_gettime(CLOCK_MONOTONIC, &start)){
			fprintf(stderr, "ERROR in clock_gettime()\n");
   			exit(1);
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
    		exit(1);
	    }
		if(log_flag){ //print input in log file
		    dprintf(log_fd, "%s", str_in);
		}
	} else{
		dprintf(log_fd, "ERROR in invalid argument\n");
		close_all();
		exit(1);
	}
	return;
}

int main(int argc, char *argv[])
{
  	//takes parameters for period, scale and log
  	static const struct option long_options[] =
		{
	 		{"period", required_argument, NULL, 'p'},
	 		{"scale", required_argument, NULL, 's'},
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
	  	case 'p' :
	    	period = atoi(optarg);
	    	if(period <= 0){
	    		fprintf(stderr, "ERROR in setting period\n");
    			exit(1);
	    	}
	    	break;
	  	case 's' :
	    	if(strcmp(optarg, "C") == 0){
				scale = 'C';
	      	} else if(strcmp(optarg, "F") == 0){
				scale = 'F';
	      	} else{
    			fprintf(stderr, "ERROR in setting scale\n");
	      		exit(1);
	      	}
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

	//initialize mraa
    temp = mraa_aio_init(0);
    if (temp == NULL){
        fprintf(stderr, "ERROR in mraa_aio_init()\n");
        mraa_aio_close(temp);
		exit(1);
    }

    //initialize mraa_gpio
    gpio_btn = mraa_gpio_init(3);
    if(gpio_btn == NULL){
        fprintf(stderr, "ERROR in mraa_gpio_init()\n");
        close_all();
		exit(1);
    }
    mraa_gpio_dir(gpio_btn, MRAA_GPIO_IN);
    
    struct pollfd fd[1];
	fd[0].fd = STDIN_FILENO;
   	fd[0].events = POLLIN | POLLHUP | POLLERR;

   	int btn_check, ret, len, i, j;
   	char str_input[50], buf[10]; //str_input is whole stdin and buf is for single instruction
	while(1){
	    int value = poll(fd, 1, 0);
	    if(value == -1){
	    	fprintf(stderr, "ERROR in poll()\n");
	    	close_all();
	    	exit(1);
	    } else if(value > 0){
	    	//button
	    	btn_check = mraa_gpio_read(gpio_btn);
	    	if(btn_check > 0){ //button is on
	    		shutdown();
	    	} else if(btn_check < 0){
	    		fprintf(stderr, "ERROR in button: mraa_gpio_read()\n");
	    		close_all();
	    		exit(1);
	    	}
	    	//temperature
	    	if(stop == 0){
		    	get_temp();
    		}

			//check input from stdin
			if(fd[0].revents & POLLIN) {
		    	bzero(str_input, 50); //empty the string for storing input
		    	bzero(buf, 10); //empty the string
		    	ret = read(fd[0].fd, str_input, 50); //read input stdin
		    	if(ret == 0){ //EOF
		    		break;
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
			}
			if(fd[0].revents & POLLERR) {
				fprintf(stderr, "ERROR in reading from STDIN\n");
				close_all();
				exit(1);
			}
			if(fd[0].revents & POLLHUP) {
				break;
			}
		} else{ //the call timed out and no file descriptors were ready: MAIN LOOP
			//button
	    	btn_check = mraa_gpio_read(gpio_btn);
	    	if(btn_check > 0){ //button is on
	    		shutdown();
	    	} else if(btn_check < 0){
	    		fprintf(stderr, "ERROR in button: mraa_gpio_read()\n");
	    		close_all();
	    		exit(1);
	    	}
	    	//temperature
	    	if(stop == 0){
		    	get_temp();
    		}
		}
	} 
    close_all();
    return MRAA_SUCCESS;
}
