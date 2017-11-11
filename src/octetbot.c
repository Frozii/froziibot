#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

char* read_config(char *name)
{
    // create a char pointer and allocate 512 bytes to it
    char *value = malloc(512);

    // add the null-terminator to the first element of the array, if the file pointer is NULL we will return a null-terminated string
    value[0] = '\0';

    // create a file pointer and point it to the start of the config file, open the file with read permissions
    FILE *config_file = fopen("config", "r");

    // if the file pointer is pointing to a valid location
    if (config_file) {
        while (1) {
            // char array to hold the configuration key string
            char config_key[512];

            // char array to hold the configuration value
            char config_value[512];

            // parse the Key = Value pairs
            // 
            // fscanf will on success return the amount of arguments it assigned
            // in our case it returns 2 because we assigned config_key and config_value
            // 
            // " %511[^= ] = %s "
            //  ^================ optionally skip whitespace
            //   ^^^^^=========== read a string of at most 511 letters
            //        ^^^^^====== [] is a set, [^= ] means that the string we read cannot contain the characters inside of the set, which means our string cannot contain a '=' or a ' '
            //             ^===== optionally skip whitespace
            //              ^==== read a literal '='
            //               ^=== optionally skip whitespace
            //                ^^= reads a string of non-whitespace characters
            //                  ^ optionally skip whitespace
            //                
            // the first read is placed into config_key and the second read is placed into config_value
            int status = fscanf(config_file, " %511[^= ] = %s ", config_key, config_value);

            // if we hit End-Of-File
            if (status == EOF) {

                // break out of the while loop
                break;
            }

            // strcmp() compares the two given strings
            // it will return 0 if the compared strings are equal
            if (strcmp(config_key, name) == 0) {
                // copy the width of "config_value" + 1 from "config_value" to "value", the + 1 is so that we get the null-terminator as well
                strncpy(value, config_value, strlen(config_value) + 1);
                break;
            }
        }
        // close the open file
        fclose(config_file);
    }
    return value;
}

void send_nickname_packet(int socket, char *nickname)
{
    // char array which we will send to the server
    char nickname_packet[512];

    // NICK command is used to give/change the existing one
    // after it we send a carriage return and a newline
    sprintf(nickname_packet, "NICK %s\r\n", nickname);

    // transmit our char array through the socket to the server
    send(socket, nickname_packet, strlen(nickname_packet), 0);
}

void send_username_packet(int socket, char *nickname)
{
    // char array which we will send to the server
    char username_packet[512];

    // USER command is used to specify the username and realname of a new user
    // USER <user> <mode> <unused> <realname>
    // after it we send a carriage return and a newline
    sprintf(username_packet, "USER %s 0 * :%s\r\n", nickname, nickname);

    // transmit our char array through the socket to the server
    send(socket, username_packet, strlen(username_packet), 0);
}

void join_channels(int socket, char *channels)
{
    // char array which we will send to the server
    char join_packet[512];

    // JOIN command is used by a user to request to start listening to the specific channel(s)
    sprintf(join_packet, "JOIN %s\r\n", channels);

    // transmit our char array through the socket to the server
    send(socket, join_packet, strlen(join_packet), 0);
}

void read_line(int socket, char *line)
{
    // holds the length of the line
    int length = 0;
  
    while (1) {
        // holds the current character in the line
        char data;

        // recv() will return length of the message in bytes
        int result = recv(socket, &data, 1, 0); 

        // error handling
        if (result <= 0 || data == EOF) {
            perror("Connection was closed or an error was encountered");
            exit(1);
        }

        // save the current character into the array
        line[length] = data;

        // increment by one
        length++;

        // checks if the last two characters of the line were a carriage return and a newline
        if (length >= 2 && line[length - 2] == '\r' && line[length - 1] == '\n') {

            // add null-terminator to the end of the string
            line[length - 2] = '\0';

            //return length;

            // break out of the while loop
            break;
        }
    }
}

char *read_operation(char *line)
{
    // char array to hold the operation
    char *operation = malloc(512);

    // char array to be used as a duplicate
    char copy[512];

    // copy contents of line into copy
    strncpy(copy, line, strlen(line) + 1);

    // a pointer which will hold the start of a substring found by strstr()
    char *start;

    if (start = strstr(copy, "PING")) {
        sprintf(operation, "PING");
    }

    else if (start = strstr(copy, "JOIN")) {
        sprintf(operation, "JOIN");
    }

    else if (start = strstr(copy, "MODE")) {
        sprintf(operation, "MODE");
    }

    else if (start = strstr(copy, "KICK")) {
        sprintf(operation, "KICK");
    }

    else if ((start = strstr(copy, "PART")) || (start = strstr(copy, "CLOSE"))) {
        sprintf(operation, "PART");
    }

    else if (start = strstr(copy, "QUIT")) {
        sprintf(operation, "QUIT");
    }

    else if (start = strstr(copy, "PRIVMSG")) {
        sprintf(operation, "PRIVMSG");
    }

    else if (start = strstr(copy, "NOTICE")) {
        sprintf(operation, "NOTICE");
    }

    else {
        // add the null-terminator to the empty string
        operation[0] = '\0';
    }

    return operation;
}

char *read_arguments(char *line)
{
    // char array to hold the arguments
    char *arguments = malloc(512);

    // char array to be used as a duplicate, we need a duplicate because strtok() breaks the string
    char copy[512];

    // copy contents of line into copy
    strncpy(copy, line, strlen(line) + 1);

    // searches for the first appearance of ' ' or ':' and returns a pointer to it
    char *token = strstr(copy, " :");

    if (token) {
        // copy the characters starting from the right side of ' ', ':' arguments
        strncpy(arguments, token + 2, strlen(token) + 1);
    }

    else {
        // add the null-terminator to the empty string
        arguments[0] = '\0';
    }

    return arguments;
}

char *get_text_argument(char *line)
{
    // char array to hold the text arguments
    char *arguments = malloc(512);

    // char array to be used as a duplicate, we need a duplicate because strtok() breaks the string
    char copy[512];

    // copy contents of line into copy
    strncpy(copy, line, strlen(line) + 1);

    char *token = strstr(copy, ":");

    int iteration = 0;

    while (token) {
        if (iteration >= 2) {
            strncpy(arguments, token, strlen(token) + 1);

            break;
        }

        token = strtok(NULL, ":");

        // increment the iteration counter
        iteration++;
    }

    return arguments;
}

char *read_user(char *line)
{
    // char array to hold the user
    char *user = malloc(512);

    // char array to be used as a duplicate, we need a duplicate because strtok() breaks the string
    char copy[512];

    // copy contents of line into copy
    strncpy(copy, line, strlen(line) + 1);

    char *token = strtok(copy, "!");

    if (token) {
        // copy the characters starting from the start of the string with an offset of 1 byte
        strncpy(user, token + 1, strlen(token) + 1);
    }

    else {
        // add the null-terminator to the empty string
        user[0] = '\0';
    }

    return user;
}

void send_pong(int socket, char *arguments)
{
    // char array which we will send to the server
    char pong_packet[512];

    // PONG message is a reply to the PING message from the server
    // Command: PONG
    // Parameters: <server> [ <server2> ]
    sprintf(pong_packet, "PONG %s\r\n", arguments);

    // transmit our char array through the socket to the server
    send(socket, pong_packet, strlen(pong_packet), 0);
}

void send_greeting(int socket, char *user, char *channel)
{
    // char array which will hold the greeting message
    char greeting_packet[512];

    sprintf(greeting_packet, "PRIVMSG %s :%s has joined.\r\n", channel, user);

    // transmit our char array through the socket to the server
    send(socket, greeting_packet, strlen(greeting_packet), 0);
}

void send_goodbye(int socket, char *user, char *channel)
{
    // char array which will hold the goodbye message
    char goodbye_packet[512];

    sprintf(goodbye_packet, "PRIVMSG %s :%s has left.\r\n", channel, user);

    // transmit our char array through the socket to the server
    send(socket, goodbye_packet, strlen(goodbye_packet), 0);
}

void reply_invoked(int socket, char *user, char *channel)
{
    // char array which will hold the respond message
    char invoked_reply[512];

    sprintf(invoked_reply, "PRIVMSG %s :%s\r\n", channel, user);

    // transmit our char array through the socket to the server
    send(socket, invoked_reply, strlen(invoked_reply), 0);
}

void log_it(char *channel, char *user, char *message_text, FILE *log_file)
{
    // array to store our date string into
    char current_date[50];
    
    // pointer to a structure which is suitable for holding a time and date
    struct tm *current_time;

    // stores the current calendar time which was given to us by time()
    // it will return to us the amount of seconds that have passed since January 1, 1970
    time_t raw_time = time(0);

    // gmtime() takes our calendar time and fills the tm structure with that information
    current_time = gmtime(&raw_time);

    // formats the structure current_time according to the formatting rules and stores it into current_date
    strftime(current_date, sizeof(current_date), "%Y-%m-%d %H:%M:%S", current_time);
    
    // prints our time and message into the log file
    fprintf(log_file, "[%s %s] %s %s\n", current_date, channel, user, message_text);

    // close the log file
    fclose(log_file);
}

int main()
{
    // creates a file descriptor that refers to the communication endpoint, the socket being the endpoint of two-way communication.
    // 
    // AF_INET designates that we can communicate with IPv4 addresses from our socket
    // SOCK_STREAM provides reliable two-way communication
    // 0 specifies that we are using the Transmission Control Protocol
    int socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);

    // error handling
    if (socket_descriptor == -1) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    char *ip = read_config("server");
    char *port = read_config("port");

    // structure for handling internet addresses which we will refer to as server
    struct sockaddr_in server;

    // set the address family
    server.sin_family = AF_INET;

    // set the port
    // 
    // htons() makes sure our port value is stored in memory in network byte order
    // atoi() turns our port from a string to an integer
    server.sin_port = htons(atoi(port));

    // set the internet address
    // 
    // inet_addr() will turn our string *ip* to an integer value for use as an internet address
    server.sin_addr.s_addr = inet_addr(ip);

    printf("The IP is: %s\n", ip);
    printf("The port is: %s\n", port);

    // free malloc'd pointer
    free(ip);

    // free malloc'd pointer
    free(port);

    // connect our socket to a remote server
    // 
    // in order to connect we need to give it our socket descriptor, the location of our server which has been typecasted and the size of it.
    if (connect(socket_descriptor, (struct sockaddr*)&server, sizeof(server)) == -1) {
        perror("Failed to connect");
        exit(EXIT_FAILURE);
    }

    char *nick = read_config("nick");
    char *channels = read_config("channels");

    printf("The nick is: %s\n", nick);
    printf("Our channels are: %s\n", channels);

    send_nickname_packet(socket_descriptor, nick);
    send_username_packet(socket_descriptor, nick);
    join_channels(socket_descriptor, channels);

    // free malloc'd pointer
    free(nick);

    // free malloc'd pointer
    free(channels);
    
    while (1) {
        // create a pointer to the log file
        FILE *log_file = fopen("octetbot.log", "a+");
        
        // char array to hold the current text line
        char line[512];

        // read the current text line into the array
        read_line(socket_descriptor, line);

        char *user = read_user(line);
        char *operation = read_operation(line);
        char *arguments = read_arguments(line);

        // print the server output
        printf("%s\n", line);

        if (strcmp(operation, "PING") == 0) {
            char *channel = read_config("channels");
            char *message_text = "Got PING, Sending PONG.";
            
            send_pong(socket_descriptor, arguments);
            log_it(channel, "!", message_text, log_file);
        }

        else if (strcmp(operation, "JOIN") == 0) {
            char *channel = read_config("channels");
            char *message_text = "has joined.";
            
            send_greeting(socket_descriptor, user, channel);
            log_it(channel, user, message_text, log_file);
        }

        else if (strcmp(operation, "PART") == 0) {
            char *channel = read_config("channels");
            char *message_text = "has left.";

            send_goodbye(socket_descriptor, user, channel);
            log_it(channel, user, message_text, log_file);
        }

        else if (strcmp(operation, "PRIVMSG") == 0) {
            char *channel = read_config("channels");
            char *message_text = get_text_argument(line);

            // add : to the start of messages, makes it a little easier to read the log file
            char *new_message_text = malloc(512);
            sprintf(new_message_text, ":%s", message_text);
            
            if (strcmp(message_text, "!octetbot") == 0) {
                reply_invoked(socket_descriptor, user, channel); 
           }
            
            log_it(channel, user, new_message_text, log_file);

            // free malloc'd pointer
            free(new_message_text);
        }

        // free malloc'd pointer
        free(user);

        // free malloc'd pointer
        free(operation);

        // free malloc'd pointer
        free(arguments);
    }
}
