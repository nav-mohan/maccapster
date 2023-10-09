#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <vector>
#include <iostream>
#include <thread>
#include <atomic>

#if !defined(MOCKSERVER_H)
#define MOCKSERVER_H

#define READ_BUFFER_SIZE 1024
void open_and_send(const char *filename, int clientfd);
int make_server_socket(const char *portNumber);
void sigint_handler(int signum);
void sigpipe_handler(int signum);
void start_server(const char *portNumber);
void clear_files();
void add_file(const char *filepath);
void stop_server();

#endif // MOCKSERVER_H
