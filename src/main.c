/*
	pithond -	gpio control daemon using DGRAM (UDP) sockets
				can be loaded as a service
				must be run as root for pin control
*/

#include <syslog.h>
#include <signal.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <wiringPi.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PINS 20		// Max gpio pins + (-1) (pi3 has 17)
#define BUFSIZE 50	// Max reply length
#define DEFPORT "3141" // default port
#define CONFDEF "/etc/pithond.conf"
#define PIDDEF "/var/run/pithond.pid"
#define LOGDEF "/var/log/pithond.log"

const int gpio_pins_all[] = { 0, 1, 2, 3, 4, 5, 6, 7, 21, 22, 23, 24, 25, 26, 27, 28, 29, -1 };

struct gpio_pins {
	int out[PINS];
	int in[PINS];
	int high[PINS];
	int low[PINS];
} pins;

// Globals

int running = 1;
int pid_fd = -1;
char *pid_file_name = NULL;
char *port = NULL;
FILE *log_stream = NULL;

// Func protos

char *read_all(int mode);
void server_run(int sfd);
int open_socket(char *port);
void daemonize(char *pid_file);
unsigned long hash(char *ptr);
void init_pi(void);
char *process_input(char *input);
int read_conf_file(char *conf_name);
void print_help(void);
void handle_signal(int sig);

// Begin main()

int main(int argc, char *argv[]) {
	int start_daemonized = 0;
	int sfd, opt_value, opt_index = 0;
	char *log_file_name = NULL,
		*conf_file_name = NULL;
	char buf[BUFSIZE];
	char *app_name = argv[0];

	// process argv

	static struct option long_options[] = {
		{ "conf_file", required_argument, 0, 'c'} ,
		{ "pid_file", required_argument, 0, 'i'} , 
		{ "log_file", required_argument, 0, 'l'} , 
		{ "daemonize", no_argument, 0, 'd'} , 
		{ "port", required_argument, 0, 'p'} ,
		{ "help", no_argument, 0, 'h'},
		{ NULL, 0, 0, 0 }
	};

	while ((opt_value = getopt_long(argc, argv, "c:l:i:p:dh", long_options, &opt_index)) != -1) {
		switch (opt_value) {
		case 'c':
			conf_file_name = strdup(optarg);
			break;
		case 'l':
			log_file_name = strdup(optarg);
			break;
		case 'i':
			pid_file_name = strdup(optarg);
			break;
		case 'p':
			port = strdup(optarg);
			break;
		case 'd':
			start_daemonized = 1;
			break;
		case 'h':
		case '?':
			print_help();
			return 0;
		default:
			break;
		}
	}
	// set defaults
	if (port == NULL) port = strdup(DEFPORT);
	if (pid_file_name == NULL) pid_file_name = strdup(PIDDEF);
	if (conf_file_name == NULL)	conf_file_name = strdup(CONFDEF);
	if (log_file_name == NULL) log_file_name = strdup(LOGDEF);
	
	log_stream = fopen(log_file_name, "a+");
	if (log_stream == NULL) {
		syslog(LOG_ERR, "Could not open log file %s", strerror(errno));
		log_stream = stdout;
	}
	if (start_daemonized == 1)
		daemonize(pid_file_name);
	
	openlog(app_name, LOG_PID | LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO, "Started %s", argv[0]);
	
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	//test log

	read_conf_file(conf_file_name);

	init_pi();
	//sfd = openSocket(port);

	server_run(open_socket(port)); // start server. Infinite loop until signal or running = 0

	// shut down
	
	if (log_stream != stdout)
		fclose(log_stream);
	syslog(LOG_INFO, "Stopped %s", app_name);
	closelog();

	if (conf_file_name != NULL) free(conf_file_name);
	if (pid_file_name != NULL) free(pid_file_name);
	if (log_file_name != NULL) free(log_file_name);
	if (port != NULL) free(port);

	return 0;
}

// Read and return UDP packet. Infinite loops until signal or running = 0

void server_run(int sfd) {
	char buf[BUFSIZE], reply[BUFSIZE];
	socklen_t peer_addr_len;
	struct sockaddr_storage peer_addr;
	ssize_t nread;
	peer_addr_len = sizeof(struct sockaddr_storage);

	while (running == 1) {
		nread = recvfrom(sfd, buf, BUFSIZE, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
		if (nread == -1)
			continue;
		char host[NI_MAXHOST], service[NI_MAXSERV];
		if (getnameinfo((struct sockaddr *) &peer_addr, peer_addr_len, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV) != 0)
			syslog(LOG_ERR, "getnameinfo() %s\n", gai_strerror);
		strcpy(reply, process_input(buf));
		nread = strlen(reply) + 1;
		if (sendto(sfd, reply, nread, 0, (struct sockaddr *)&peer_addr, peer_addr_len) != nread)
			syslog(LOG_ERR, "Error sending response...\n");
		sleep(1);
	}
}

// Print help

void print_help(void) {
	printf("pithond [OPTIONS]\n\n");
	printf("\t-h help\n");
	printf("\t-d daemonize\n");
	printf("\t-p port\n");
	printf("\t-i pid file\n");
	printf("\t-l log file\n");
	printf("\t-c conf file\n");
}

// Read pin state (0) / mode (1) and return as string

char *read_all(int mode) {
	static char readall[BUFSIZE];
	int i;
	if (mode)
		for (i = 0; gpio_pins_all[i] != -1; i++)
			readall[i] = getAlt(gpio_pins_all[i]) ? 'O' : 'I';
	else
		for (i = 0; i < 32; i++)
			readall[i] = digitalRead(i) ? '1' : '0';
	readall[i] = '\0';
	return readall;
}

// Open UDP socket on *port and return socket descriptor

int open_socket(char *port) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	s = getaddrinfo(NULL, port, &hints, &result);

	if (s != 0) {
		syslog(LOG_ERR, "getaddrinfo() %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;
		close(sfd);
	}

	if (rp == NULL) {
		syslog(LOG_ERR, "Could not bind to %s\n", port);
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);
	
	return sfd; //return socket descriptor
}

// initialize pin using pins from struct gpio pins

void init_pi(void) {
	int i;

	wiringPiSetup(); // needs root

	// make sure we set values before states to prevent momentary high output

	for (i = 0; pins.high[i] != -1; i++) digitalWrite(pins.high[i], HIGH);
	for (i = 0; pins.low[i] != -1; i++) digitalWrite(pins.low[i], LOW);
	for (i = 0; pins.out[i] != -1; i++) pinMode(pins.out[i], OUTPUT);
	for (i = 0; pins.in[i] != -1; i++) pinMode(pins.in[i], INPUT);
}

// proccess udp packet received

char *process_input(char *input) {
	int state, wipin;
	int error = 0;
	const int MAX = 7;		// GX{0,1,2}{00-99}
							// 0 low
							// 1 high
							// 2 toggle
							// 00-31 wpi number
	char command[MAX + 1];
	static char newreply[BUFSIZE];
	strncpy(command, input, MAX);
	strncpy(newreply, "\0", 1);
	// 01234
	// GXppx
	if (command[1] != 'X') {
		error = 1;
	}
	else {
		switch (command[0]) {
		case 'G':		// set/toggle pin
			if (command[4] == '\0') {
				error = 1;
			}
			else {
				wipin = (command[2] - '0') * 10 + (command[3] - '0');
				state = command[4] - '0';
				int inarray = 0;
				for (int i = 0; pins.out[i] != -1; i++)
					if (pins.out[i] == wipin)
						inarray = 1;
				if (inarray == 0)
					error = 1;
				else {
					switch (state) {
					case 0:
						digitalWrite(wipin, LOW);
						sprintf(newreply, "%i 0", wipin);
						break;
					case 1:
						digitalWrite(wipin, HIGH);
						sprintf(newreply, "%i 1", wipin);
						break;
					case 2:
						if (digitalRead(wipin))
							digitalWrite(wipin, LOW);
						else
							digitalWrite(wipin, HIGH);
						sprintf(newreply, "%i %i", wipin, digitalRead(wipin));
						break;
					default:
						error = 1;
						break;
					}
				}
			}
			break;

		case 'I':		// read pin
			if (command[3] == '\0') {
				error = 1;
			}
			else {
				wipin = (command[2] - '0') * 10 + (command[3] - '0');
				if (wipin < 0 || wipin > 32) {
					error = 1;
				}
				else {
					if (wipin == 32) {
						strcpy(newreply, read_all(0));
					}
					else
						sprintf(newreply, "%i %i", wipin, digitalRead(wipin));
				}
			}
			break;

		case 'S':		// read state
			if (command[3] == '\0') {
				error = 1;
			}
			else {
				wipin = (command[2] - '0') * 10 + (command[3] - '0');
				if (wipin < 0 || wipin > 32) {
					error = 1;
				}
				else {
					if (wipin == 32) {
						// read all
						strcpy(newreply, read_all(1));
					}
					else {
						sprintf(newreply, "%i %c", wipin, getAlt(wipin) ? 'O' : 'I');
					}
				}
			}
			break;

		case 'X':		// ping
			if (command[2] == '5' && command[3] == '0')
				system("sudo reboot");
			if (command[2] == '7' && command[3] == '0')
				system("sudo halt");
			strcpy(newreply, "OK");
			break;

		default:
			error = 1;
			break;
		}
	}
	if (error == 1)
		strcpy(newreply, "-1");

	return newreply;
}

// open conf file and process it to struct gpio pins

int read_conf_file(char *conf_name) {
	int i, buf_p = 0;
	char buffer[1024], cbuf[255];
	int valbuf[PINS];
	int sstate = 0;
	FILE *conf_file;
	const int NWORDS = 4;
	char *words[] = {  // keywords in conf file
		"gpio_pins_out",		// sstate 1
		"gpio_pins_in",			// sstate 2, etc
		"gpio_pins_high",
		"gpio_pins_low"
	};

	if (conf_name == NULL)
		return 0;

	conf_file = fopen(conf_name, "r");

	if (conf_file == NULL) {
		syslog(LOG_ERR, "Can not open config file: %s, error: %s", conf_name, strerror(errno));
		return -1;
	}
	while (fgets(buffer, 1024, conf_file)) {
		buf_p = 0;
		for (int count = 0; count < PINS; count++) valbuf[count] = -1;  // reset temp pin table
		sstate = 0;
		for (i = 0; buffer[i] != '\0' && buffer[i] != '\n'; i++) {
			if (buffer[i] == '#')  // skip comment line
				break;
			if (buffer[i] == ' ' || buffer[i] == '\t')  // skip over whitespace
				continue;
			if (buffer[i] == '=') {  // check variable in word table
				cbuf[buf_p] = '\0';
				buf_p = 0;  //reset buffer
				for (int j = 0; j < NWORDS; j++)
					if (hash(cbuf) == hash(words[j])) // hash the words
						sstate = j + 1;  // found variable, assign sstate
				continue;
			}
			if (buffer[i] == ',') {
				buf_p++;  // next 
				continue;
			}
			if (sstate) {
				if (buf_p > PINS) break;
				if (valbuf[buf_p] != -1)  // change char to int...
					valbuf[buf_p] = (valbuf[buf_p] * 10) - 1;  // *10 since number was there
				valbuf[buf_p] += buffer[i] - '0' + 1;  // now add pin after char -> int
			}
			else {
				cbuf[buf_p] = buffer[i];  // add pre = to buffer to check for value
				buf_p++;
			}
		}
		if (sstate) {
			switch (sstate) {
			case 1:		// sstate 1
				for (int j = 0; j < PINS; j++) pins.out[j] = valbuf[j];
				//for (int j = 0; j < PINS; j++) printf ("%i ", pins.out[j]);
				//printf("OUTPUT\n");
				break;
			case 2:
				for (int j = 0; j < PINS; j++) pins.in[j] = valbuf[j];
				//for (int j = 0; j < PINS; j++) printf ("%i ", pins.in[j]);
				//printf("INPUT\n");
				break;
			case 3:
				for (int j = 0; j < PINS; j++) pins.high[j] = valbuf[j];
				//for (int j = 0; j < PINS; j++) printf ("%i ", pins.high[j]);
				//printf("HIGH\n");
				break;
			case 4:
				for (int j = 0; j < PINS; j++) pins.low[j] = valbuf[j];
				//for (int j = 0; j < PINS; j++) printf ("%i ", pins.low[j]);
				//printf("LOW\n");
				break;
			}
		}
	}
	fclose(conf_file);

	return 0;
}

// hash a string

unsigned long hash(char *ptr) {
	unsigned long h = 5381;
	int c;
	while (c = *ptr++)
		h = ((h << 5) + h) + c;
	return h;
}

// signal handling

void handle_signal(int sig) {
	if (sig == SIGINT) {
		//fprintf(log_stream, "Debug: stopping daemon...\n");
		// unlock and close lock file
		if (pid_fd != -1) {
			lockf(pid_fd, F_ULOCK, 0);
			close(pid_fd);
		}

		if (pid_file_name != NULL)
			unlink(pid_file_name);

		running = 0;
		signal(SIGINT, SIG_DFL);
	}
	else if (sig == SIGHUP) {
		//fprintf(log_stream, "Debug: reloading config file...\n");
		read_conf_file("/etc/pithond.cond");
	}
	else if (sig == SIGCHLD) {
		;	//fprintf(log_stream, "Debug: received SIGCHLD signal\n");
	}
}

//  daemonize

void daemonize(char *pid_file) {
	pid_t pid = 0;
	int fd;
	pid = fork();

	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);
	if (setsid() < 0)
		exit(EXIT_FAILURE);

	signal(SIGCHLD, SIG_IGN);

	pid = fork(); // 2nd fork

	if (pid < 0)
		exit(EXIT_FAILURE);
	if (pid > 0)
		exit(EXIT_SUCCESS);

	umask(0);
	chdir("/");

	for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--)
		close(fd);

	stdin = fopen("/dev/null", "r");
	stdout = fopen("/dev/null", "w+");
	stderr = fopen("/dev/null", "w+");

	if (pid_file != NULL) {
		char str[256];
		pid_fd = open(pid_file, O_RDWR | O_CREAT, 0640);
		if (pid_fd < 0)
			exit(EXIT_FAILURE);
		if (lockf(pid_fd, F_TLOCK, 0) < 0)
			exit(EXIT_FAILURE);

		sprintf(str, "%d\n", getpid());
		write(pid_fd, str, strlen(str));
	}
}
