
/*
A user (client) program that allows chatting through a server program.
When started, it prompts you to enter a name, and the entered name is registere>
If there is a user already logged in, standard input the name of the user.
If you type a message, all users will see the message with their name.
If a message is received, it will be printed to standard output.
If you want to log out, send "LOGOUT" to the server, the socket will be closed >
*/
#include<stdio.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<stdlib.h>
#include<poll.h>


/*
*Function to forcibly terminate when an error occurs
*Argument: message indicating where the error occurred
*Return value: none (exit the program when this function is called)
*/
void myerror(char *message){
        perror(message);
        exit(1);
}

int main(){
        int sockfd;                             //communication socket
        struct pollfd fds[2];   //Read socket for viewing with poll()
        int chkerr;             //chkerr:A variable that holds the return value>
        char buff[128];                 //Data for sending and receiving
        struct sockaddr_in serv_addr;   //server data

        printf("Enter IP addess:");
        scanf("%s",buff);

        //Create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0) myerror("socket_error");         //Error check, call mye>

        //Create address
        memset(&serv_addr, 0, sizeof(struct sockaddr_in));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(buff);
        serv_addr.sin_port = htons(10000);

        printf("Enter user name：");
        scanf("%s",buff);

        printf("Connecting...\n");
        //send a request to establish a connection to the server
        chkerr = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct s>
        if(chkerr < 0) myerror("connect_error");

        fds[0].fd = sockfd;                     //Register sockfd as a read soc>
        fds[0].events = POLLIN;         //Set so that POLLIN is returned in rev>

        fds[1].fd = 0;                          //Register standard input as a >
        fds[1].events = POLLIN;         //Set so that POLLIN is returned in rev>

        chkerr = write(fds[0].fd, buff, 128);   //Send data name
        if(chkerr < 0) myerror("write_name_error");

        while(1){
                poll(fds, 2, 0);        //Monitor sockfd and stdin

                if(fds[1].revents == POLLIN){   //Enter if there is input from the keyboard
                        scanf("%s", buff);
                        chkerr = write(fds[0].fd, buff, 128);   //Send data
                        if(chkerr < 0) myerror("write_string_error");
                }

                else if(fds[0].revents == POLLIN){      //Enter if there is a communication request from the server
                        chkerr = read(fds[0].fd, buff, 128);    //Receive data
                        if(chkerr < 0){
                                myerror("read_error");
                        }else if(chkerr == 0){
                                //If the value of chkerr is 0, it means that the server ended other than
        //"SERVER_END", so terminate the program.
                                printf("＊＊unexpected termination of server＊＊\n");
                                break;
                        }

                        if(strcmp(buff, "LOGOUT") == 0){
                                //Terminate the program if logout processing is performed
                                printf("logout\n");
                                break;
                        }else if(strcmp(buff, "SERVER_END") == 0){
                                //"SERVER_END" is signals the end of the server, so exit the program
                                printf("server terminated\n");
                                break;
                        }else{
                                printf("%s",buff);
                        }
                }
        }

  //Terminate socket
        chkerr = close(sockfd);
        if(chkerr < 0) myerror("close_sockfd_error");
}
