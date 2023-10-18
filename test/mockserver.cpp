#include "mockserver.h"

int sockfd;
std::vector<int> clients;
std::atomic_bool isRunning;

std::vector<std::string>filenames;

void clear_files()
{
    printf("CLEARIGN FILES\n");
    filenames.clear();
}

void add_file(const char *filepath)
{
    printf("APPENDING FILE %s\n",filepath);
    filenames.emplace_back(filepath);
}

int make_server_socket(const char *portNumber)
{
    // Create a socket
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
    int port = atoi(portNumber);
    // Bind the socket to a local address
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    return sockfd;    
}


void open_and_send(const char *filename, int clientfd)
{
    int bytes_read,bytes_sent, total_bytes_sent;
    char *buffer = (char*)malloc(READ_BUFFER_SIZE);
    FILE *xmlFile = fopen(filename,"r");
    while ((bytes_read = fread(buffer,sizeof(char),READ_BUFFER_SIZE,xmlFile)))
    {
        // std::cout << "READ " << bytes_read << " BYTES" << std::endl;
        bytes_sent = send(clientfd,buffer,bytes_read,0);
        // std::cout << "SENT " << bytes_sent << " BYTES" << std::endl;
        if(bytes_read != bytes_sent) return;
        memset(buffer,0,READ_BUFFER_SIZE);
        total_bytes_sent += bytes_sent;
    }
    printf("Done sending %s %d bytes\n",filename,total_bytes_sent);
    fclose(xmlFile);
}


void sigint_handler(int signum) 
{
    printf("Caught SIGINT signal!\n");
   for (auto c : clients)
    {
        close(c);
        shutdown(c,SHUT_RDWR);
    }    
    close(sockfd);
    exit(1);
}

void sigpipe_handler(int signum) 
{
    printf("Caught SIGPIPE signal!\n");

    for (auto c : clients)
    {
        close(c);
        shutdown(c,SHUT_RDWR);
    }
}

void start_server(const char *portNumber, const uint32_t HEARTBEAT_FREQUENCY)
{
    if(isRunning.load()) return;
    isRunning.store(1);
    if(!filenames.size()) return;
    sockfd = make_server_socket(portNumber);
    signal(SIGINT, sigint_handler);
    signal(SIGPIPE, sigpipe_handler);

    while (isRunning.load())
    {
        printf("Listening...\n");
        // Start listening for incoming connections
        listen(sockfd, 5);
        int clientfd;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
    
        printf("Accepting...\n");
        clients.push_back(clientfd);

        while(1)
        {
            for(auto filename : filenames){
                open_and_send(filename.c_str(),clientfd);
                sleep(HEARTBEAT_FREQUENCY);
            }
            sleep(2*HEARTBEAT_FREQUENCY);
        }

        // Close the client socket
        shutdown(clientfd,SHUT_RDWR);
        close(clientfd);
    }
    printf("EXITING NOW...\n");
}

void stop_server()
{
    isRunning.store(0);
    close(sockfd);
}

int main(int argc, char *argv[])
{
    if(argc < 4)
    {
        printf("USAGE:\t %s <PORT> <HEARTBEAT_FREQUENCY> <FILE1> <FILE2> ...\n",argv[0]);
        exit(1);
    }
    clear_files();
    for(int i = 3; i < argc; i++)
        add_file(argv[i]);
    
    std::thread serverThread([&](){
        start_server(argv[1],atof(argv[2]));
    });
    serverThread.join();
}