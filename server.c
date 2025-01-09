/*Chat Server written in C by San Ramzi*/
/* gcc server.c -lpthread -o server */
/* ./server <port_number> <users_file> */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define SERVER_ADDRESS "127.0.0.1"
#define DEFAULT_BUFLEN 1024

//User struct
typedef struct {
    int id;
    char username[50];
    char password[50];
    int chatroom;
}user;

//Message struct
typedef struct{
    user m_user;
    char m_str[DEFAULT_BUFLEN];
    struct tm m_time;
}message;

//Message linked list struct
typedef struct message_list{
    message m;
    struct message_list* next;
}message_list;

//Mutex lock
pthread_mutex_t lock;

//Users array
user users[100];
int user_count = 0;

//Logged in users
int logged_in_users[100];
int logged_in_users_count = 0;

char last_message[DEFAULT_BUFLEN];

//Menu string
char* menu = "Type the number of the chatroom to join.\n1-General\n2-Gaming\n3-Politics\n4-Fitness\n/quit or /exit to terminate connection.\n";
//Usage error message
void usage_message(char* name){
    printf("Usage: %s <port_number> <users_file>\n",name);
}
//Server help
char* server_help = "**Server Commands**\n/create <chatroom_name>\n/list\nhelp\nexit\n";

//Function declarations

message new_message(user u, char* str, struct tm time);
void output_message(char output[DEFAULT_BUFLEN],message m);
int load_users(char* password_file);
user* authenticate_user(char* username, char* password);
void* chatroom_output(void* arg);
void* client_function(void* arg);
void server_init(int port_no);

//Main function
int main(int argc, char* argv[])
{
    //Check CLI Arguments ./server <port_number> <users_file>
    if(argc != 3)
    {
        usage_message(argv[0]);
        return -1;
    }

    int port = atoi(argv[1]);
    char* users_file = argv[2];

    //Loading file error
    if(load_users(users_file) == -1)
    {
        printf("Error. Couldn't load users file.\n");
        usage_message(argv[0]);
        return -1;
    }

    //Invalid port number error
    if(!port)
    {
        printf("Error. Invalid port number.\n");
        usage_message(argv[0]);
        return -1;
    }
    //Server startup
    memset(last_message,0,DEFAULT_BUFLEN);
    server_init(port);
    return 0;
}

//New message function
message new_message(user u, char* str, struct tm time)
{
    message m;
    m.m_user = u;
    strcpy(m.m_str,str);
    m.m_time = time;
    return m;
}

//Read users from file
int load_users(char* password_file)
{
    //Open file
    FILE* file = fopen(password_file,"r");
    //Failed to open error
    if(!file)
    {
        printf("Failed to open password file.\n");
        return -1;
    }
    //Read line from file pid:username:pwd
    char line[DEFAULT_BUFLEN];
    while(fgets(line, sizeof(line),file)){
        char* userid = strtok(line,":");
        char* username = strtok(NULL,":");
        char *password = strtok(NULL, "\n");
        if (username && password) {
            users[user_count].id = atoi(userid);
            strncpy(users[user_count].username, username, sizeof(users[user_count].username));
            strncpy(users[user_count].password, password, sizeof(users[user_count].password));
            user_count++;
        }
    }
    //Close file
    fclose(file);
    return 0;
}

//Authenticate user function
user* authenticate_user(char* username, char* password)
{
    //Loop in users array
    for(int i = 0;i < user_count;i++)
    {
        //If the usernames and the passwords match return the user
        if(strcmp(username,users[i].username) == 0)
        {
            if(strcmp(users[i].password,password) == 0)
            {
                user* result = malloc(sizeof(user));
                strncpy(result->username,users[i].username,sizeof(result->username));
                result->chatroom = 0;
                return result;
            }
            return NULL;
        }
    }
    return NULL;
}

//Message to char output function
void output_message(char output[DEFAULT_BUFLEN],message m){
    char temp_output[DEFAULT_BUFLEN],time_str[100];
    snprintf(time_str,100,"%02d:%02d:%02d",m.m_time.tm_hour,m.m_time.tm_min,m.m_time.tm_sec);
    snprintf(temp_output,DEFAULT_BUFLEN,"%s %s: %s\n",time_str,m.m_user.username,m.m_str);
    strcpy(output,temp_output);
}

//Chatroom output thread
void* chatroom_output(void* arg){
    int client_socket = *(int*)arg;
    char output[DEFAULT_BUFLEN];
    while(1){
        memset(output,0,DEFAULT_BUFLEN);
        pthread_mutex_lock(&lock);
        if(last_message != NULL)
        {
            if(strncmp(output,last_message,DEFAULT_BUFLEN) != 0)
            {
                pthread_mutex_unlock(&lock);
                sleep(1);
                continue;
            }
            strncpy(output,last_message,DEFAULT_BUFLEN);
        }
        pthread_mutex_unlock(&lock);
        write(client_socket,output,strlen(output));
    }
}

//Client function
void* client_function(void* arg)
{
    int client_socket = *(int*)arg;
    free(arg);
    char username[50],
    output[DEFAULT_BUFLEN],
    read_buffer[DEFAULT_BUFLEN],
    arg_a[DEFAULT_BUFLEN],
    arg_b[DEFAULT_BUFLEN],
    arg_c[DEFAULT_BUFLEN];
    user* current_user = NULL;
    pthread_t output_thread;
    while(1)
    {
        //Reset char buffers
        memset(read_buffer,0,DEFAULT_BUFLEN);
        memset(output,0,DEFAULT_BUFLEN);
        memset(arg_a,0,DEFAULT_BUFLEN);
        memset(arg_b,0,DEFAULT_BUFLEN);
        memset(arg_c,0,DEFAULT_BUFLEN);
        //If user is logged in but not in a chatroom show menu
        if(current_user && current_user->chatroom == 0){
            snprintf(output,DEFAULT_BUFLEN,"%s",menu);
            write(client_socket,output,strlen(output));
            //Reset output buffer
            memset(output,0,DEFAULT_BUFLEN);
        }
        //Read user input
        read(client_socket,read_buffer,DEFAULT_BUFLEN);
        //If user is not logged in
        if(current_user == NULL)
        {
            //Parse user input to 3 char arguments
            char *token = strtok(read_buffer, " ");
            if (token != NULL) {
                strncpy(arg_a, token,DEFAULT_BUFLEN);
                token = strtok(NULL, " ");
                if (token != NULL) strncpy(arg_b, token,DEFAULT_BUFLEN);
                token = strtok(NULL, " ");
                if (token != NULL) strncpy(arg_c, token,DEFAULT_BUFLEN);
            }
            arg_a[strcspn(arg_a,"\n")] = 0;
            arg_b[strcspn(arg_b,"\n")] = 0;
            arg_c[strcspn(arg_c,"\n")] = 0;
            if(strcmp(arg_a,"/quit") == 0 || strcmp(arg_a,"/exit") == 0)
            {
                break;
            }

            if(strcmp(arg_a,"login"))
            {
                write(client_socket,"User not logged in.\nlogin <user_name> <password>\n",50);
                continue;
            }
            else if(strcmp(arg_b,"") && strcmp(arg_c,""))
            {
                pthread_mutex_lock(&lock);
                current_user = authenticate_user(arg_b,arg_c);
                pthread_mutex_unlock(&lock);
                if(current_user != NULL){
                    snprintf(output,DEFAULT_BUFLEN,"User %s successfully authenticated.\n",arg_b);
                    write(client_socket,output,strlen(output));
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
            char *token = strtok(read_buffer, "\n");
            if(token != NULL){
                strncpy(read_buffer,token,DEFAULT_BUFLEN);
            }
            else{
                continue;
            }
            if(strcmp(read_buffer,"/quit") == 0 || !strcmp(read_buffer,"/exit"))
            {
                break;
            }
            if(current_user->chatroom == 0){
                if(strcmp(read_buffer,"1") == 0){
                    //pthread_create(&output_thread,NULL,chatroom_output,client_socket);
                    current_user->chatroom = 1;
                    continue;
                }
                else if(strcmp(read_buffer,"2") == 0){
                    current_user->chatroom = 2;
                    continue;
                }
                else if(strcmp(read_buffer,"3") == 0){
                    current_user->chatroom = 3;
                    continue;
                }
                else if(strcmp(read_buffer,"4") == 0){
                    current_user->chatroom = 4;
                    continue;
                }
            }
            else{
                if(current_user->chatroom == 1){
                    //Get current time in string format
                    time_t t = time(NULL);
                    struct tm tm = *localtime(&t);
                    //If user input is not empty
                    if(read_buffer != NULL)
                    {
                        snprintf(output,DEFAULT_BUFLEN,"%s",read_buffer);
                        message message_to_send = new_message(*current_user,output,tm);
                        char last_message_temp[DEFAULT_BUFLEN];
                        output_message(last_message_temp,message_to_send);
                        pthread_mutex_lock(&lock);
                        strcpy(last_message,last_message_temp);
                        pthread_mutex_unlock(&lock);
                        continue;
                    }
                    else{
                        continue;
                    }
                    //Leave chatroom
                    if(strcmp(read_buffer,"/leave") == 0){
                        //pthread_cancel(output_thread);
                        snprintf(output,DEFAULT_BUFLEN,"%s left",current_user->username);
                        message leave_message;
                        leave_message.m_user = *current_user;
                        strcpy(leave_message.m_str,output);
                        leave_message.m_time = tm;

                        pthread_mutex_lock(&lock);
                        output_message(last_message,leave_message);
                        pthread_mutex_unlock(&lock);
                        current_user->chatroom = 0;
                        continue;
                    }
                }
            }
        }
    }
    close(client_socket);
}

//Server commands
void* server_input(void* arg) {
    int server_socket = *(int*)arg;
    char input[DEFAULT_BUFLEN],
    output[DEFAULT_BUFLEN],
    arg_a[DEFAULT_BUFLEN],
    arg_b[DEFAULT_BUFLEN];
    while (1) {
        memset(input,0,DEFAULT_BUFLEN);
        memset(arg_a,0,DEFAULT_BUFLEN);
        memset(arg_b,0,DEFAULT_BUFLEN);
        //Get user input
        fgets(input,DEFAULT_BUFLEN,stdin);
        char *token = strtok(input, " ");
        if (token != NULL) {
            strncpy(arg_a, token,DEFAULT_BUFLEN);
            token = strtok(NULL, " ");
            if (token != NULL) strncpy(arg_b, token,DEFAULT_BUFLEN);
        }
        arg_a[strcspn(arg_a,"\n")] = 0;
        arg_b[strcspn(arg_b,"\n")] = 0;
        //If input is "exit" or "quit"
        if (strcmp(arg_a, "exit") == 0 || strcmp(arg_a, "quit") == 0) {
            //Close server socket and exit program
            close(server_socket);
            printf("Server shutdown. Press Enter to exit...\n");
            getchar();
            exit(0);
        }
        else{
            if(strcmp(arg_a,"help") == 0 || strcmp(arg_a,"--h") == 0 || strcmp(arg_a,"-h") == 0){
                printf("%s",server_help);
            }
            else if(strcmp(arg_a,"/create") == 0){
                if(!arg_b){

                }
            }
            else{
                printf("Type help for server commands.\n");
            }
        }
    }
    return NULL;
}

//Server startup
void server_init(int port_no)
{
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

    pthread_t server_monitor;
    if(pthread_create(&server_monitor,NULL,server_input,&server_socket)){
        printf("Error creating server input thread.\n");
    }
    else{
        pthread_detach(server_monitor);
    }
    printf("Chat server started on %s:%d\n",inet_ntoa(server.sin_addr),port_no);
    
    //Main loop for receiving connections
    while(1)
    {
        //Set client socket, address size and new child thread
        struct sockaddr_in client;
        int address_size = sizeof(server);
        pthread_t child;
        int* client_socket_ptr = malloc(sizeof(int));

        //Accept incoming connection
        client_socket = accept(server_socket,(struct sockaddr*)&client,&address_size);
        if(client_socket < 0)
        {
            continue;
        }
        printf("Incoming connection from %s\n",inet_ntoa(client.sin_addr));
        
        *client_socket_ptr = client_socket;
        
        //Create a thread and run client_function
        if(pthread_create(&child,NULL,client_function,client_socket_ptr))
        {
            printf("Error creating client thread.\n");
            free(client_socket_ptr);
            continue;
        }
        else
        {
            pthread_detach(child);
        }
    }
}