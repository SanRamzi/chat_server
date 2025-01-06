/*Chat Server written in C by San Ramzi*/
/* gcc server.c -lpthread */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define SERVER_ADDRESS "127.0.0.1"
#define DEFAULT_BUFLEN 1024

//User struct
typedef struct {
    char username[50];
    char password[50];
}user;
//Users array
user users[100];
int user_count = 0;
//Function declarations
void menu();
int authenticate_user(char* username, char* password);
void* client_function(void* arg);
void server_init(int port_no);
void usage_message(char* name);
int load_users(char* password_file);
//Main function
int main(int argc, char* argv[]){
    if(argc != 3){
        usage_message(argv[0]);
        return -1;
    }

    int port = atoi(argv[1]);
    char* users_file = argv[2];

    if(load_users(users_file) == -1){
        printf("Error. Couldn't load users file.\n");
        usage_message(argv[0]);
        return -1;
    }

    if(!port){
        printf("Error. Invalid port number.\n");
        usage_message(argv[0]);
        return -1;
    }

    server_init(port);
    return 0;
}
//Show menu function
void menu(int client_socket,char* username){
    char output[DEFAULT_BUFLEN];
    snprintf(output,DEFAULT_BUFLEN,"Welcome %s\nType the number of the chatroom to join.\n1-General\n2-Gaming\n3-Politics\n4-Fitness\nType quit or exit to terminate connection.\n",username);
    write(client_socket,output,strlen(output));
}
//Authenticate user function
int authenticate_user(char* username, char* password)
{
    for(int i = 0;i < sizeof(users) / sizeof(users[0]);i++)
    {
        if(strcmp(username,users[i].username) == 0)
        {
            if(strcmp(users[i].password,password) == 0){
                return 1;
            }
            return 0;
        }
    }
    return 0;
}
//Client function
void* client_function(void* arg)
{
    int client_socket = *(int*)arg;
    free(arg);
    char output[DEFAULT_BUFLEN], read_buffer[DEFAULT_BUFLEN], arg_a[DEFAULT_BUFLEN],arg_b[DEFAULT_BUFLEN],arg_c[DEFAULT_BUFLEN];
    int authenticated = 0;
    while(1)
    {
        memset(read_buffer,0,DEFAULT_BUFLEN);
        memset(output,0,DEFAULT_BUFLEN);
        memset(arg_a,0,DEFAULT_BUFLEN);
        memset(arg_b,0,DEFAULT_BUFLEN);
        memset(arg_c,0,DEFAULT_BUFLEN);
        read(client_socket,read_buffer,DEFAULT_BUFLEN);
        if(!read_buffer){
            break;
        }
        //Get user input
        char *token = strtok(read_buffer, " ");
        if (token != NULL) {
            strcpy(arg_a, token);
            token = strtok(NULL, " ");
            if (token != NULL) strcpy(arg_b, token);
            token = strtok(NULL, " ");
            if (token != NULL) strcpy(arg_c, token);
        }
        arg_a[strcspn(arg_a,"\n")] = 0;
        arg_b[strcspn(arg_b,"\n")] = 0;
        arg_c[strcspn(arg_c,"\n")] = 0;
        printf("Command: %s, Arg1: %s, Arg2: %s\n", arg_a, arg_b, arg_c);
        if(!authenticated){
            if(strcmp(arg_a,"login") != 0)
            {
                write(client_socket,"User not logged in.\nlogin <user_name> <password>\n",50);
                continue;
            }
            else if(strcmp(arg_b,"") != 0 && strcmp(arg_c,"") != 0)
            {
                if(authenticate_user(arg_b,arg_c)){
                    authenticated = 1;
                    snprintf(output,DEFAULT_BUFLEN,"User %s successfully authenticated.\n",arg_b);
                    write(client_socket,output,strlen(output));
                    menu(client_socket,arg_b);
                    continue;
                }
                else{
                    write(client_socket,"Wrong username/password.\n",26);
                    continue;
                }
            }
        }
        else
        {
            if(strcmp(arg_a,"quit") == 0 || strcmp(arg_a,"exit") == 0)
            {
                break;
            }
            else if(strcmp(arg_a,"1") == 0){
                
            }
            else if(strcmp(arg_a,"2") == 0){
                
            }
            else if(strcmp(arg_a,"3") == 0){
                
            }
            else if(strcmp(arg_a,"4") == 0){
                
            }
        }
    }
    close(client_socket);
}
//Server startup
void server_init(int port_no){
    int server_socket, client_socket;
    struct sockaddr_in server;
    server_socket = socket(AF_INET,SOCK_STREAM,0);
    //Socket creation error
    if(server_socket == -1){
        printf("Error creating socket.\n");
        return;
    }
    //Set server name, address, and port
    server.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server.sin_family = AF_INET;
    server.sin_port = htons(port_no);

    //Binding error
    if(bind(server_socket,(struct sockaddr *)&server,sizeof(server))){
        close(server_socket);
        printf("Binding error. Port might be blocked. Exiting...\n");
        return;
    }
    
    //Listening error
    if(listen(server_socket,3)){
        close(server_socket);
        printf("Listening error. Exiting...\n");
        return;
    }

    printf("Chat server started on %s:%d\n",inet_ntoa(server.sin_addr),port_no);
    //Main loop for receiving connections
    while(1){
        //Set client socket, address size and new child thread
        struct sockaddr_in client;
        int address_size = sizeof(server);
        pthread_t child;
        int* client_socket_ptr = malloc(sizeof(int));

        //Accept incoming connection
        client_socket = accept(server_socket,(struct sockaddr*)&client,&address_size);
        if(client_socket < 0){
            continue;
        }
        printf("Incoming connection from %s\n",inet_ntoa(client.sin_addr));
        *client_socket_ptr = client_socket;
        
        //Create a thread and run client_function
        if(pthread_create(&child,NULL,client_function,client_socket_ptr)){
            printf("Error creating thread.\n");
            free(client_socket_ptr);
            continue;
        }
        else{
            pthread_detach(child);
        }
    }
}
//Usage error message
void usage_message(char* name){
    printf("Usage: %s <port_number> <users_file>\n",name);
}
//Read users from file
int load_users(char* password_file){
    FILE* file = fopen(password_file,"r");
    if(!file){
        printf("Failed to open password file.\n");
        return -1;
    }
    char line[DEFAULT_BUFLEN];
    while(fgets(line, sizeof(line),file)){
        char* username = strtok(line,":");
        char *password = strtok(NULL, "\n");
        if (username && password) {
            strncpy(users[user_count].username, username, sizeof(users[user_count].username));
            strncpy(users[user_count].password, password, sizeof(users[user_count].password));
            user_count++;
        }
    }
    fclose(file);
    return 0;
}