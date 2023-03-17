/*
A server program that handles chats between multiple clients.
If there is a communication request, create a new socket. When the number of clients reaches the limit, print a warning message to the server and clients and prevent new sockets from being created.
Send the name of the logged-in client to a newly logged-in client.
When a client is newly connected, when a client logs out, it sends login/logout along with its name to other clients. When a "LOGOUT" message is received from a client, it is assumed to be in a logout state and another client can enter.
If a normal message is sent, send the message to all clients.
Typing "SERVER_END" prompts all clients to close their sockets, confirms that all clients' sockets have been closed, then closes the sockets and terminates the program.
*/

#include<stdio.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<stdlib.h>
#include<poll.h>

#define MAX_USER 10 		//Set the maximum number of connected users
#define MAX_ROOM 10 //Set the Maximum number of rooms
#define MAX_ROOM_USER 5 //Set the maximum number of users per room
#define BUFFER_SIZE 128
/*
//Structure with user information
struct user{
	char name[128];		//user name
	int login;			//1 for login status, 0 for logout status
	struct pollfd new_sockfd[1];	//Read socket for viewing with poll()
};
*/
//Updated struct
struct user{
    char name[128]; //user name
    int login; //1 for login status, 0 for logout status
    int room; //room number
    int socket;
    struct pollfd new_sockfd[1]; //Read socket for viewing with poll()
};
struct user user[MAX_USER]; //Array that manages clients

//Structure with room information
struct room {
    int num_users; //number of users
    struct user users[MAX_USER];
    int user_list[MAX_ROOM_USER]; //list of users in the room
    char name[128];
};
struct room room[MAX_ROOM]; //Array that manages rooms
void *chat_thread(void *arg) {
    struct user *u = (struct user *)arg;
    char buff[128];
    int chkerr, i;

    while (1) {
        // Receive a message from the client
        chkerr = read(u->new_sockfd[0].fd, buff, 128);
        if (chkerr == 0) {
            // If the client has disconnected, log out and notify other clients
            printf("%s has logged out.\n", u->name);
            u->login = 0;
            for (i = 0; i < MAX_USER; i++) {
                if (user[i].login && i != u - user) {
                    sprintf(buff, "%s has logged out.\n", u->name);
                    write(user[i].new_sockfd[0].fd, buff, strlen(buff));
                }
            }
            break;
        } else if (chkerr < 0) {
            myerror("read_error");
        }

        // Send the message to all clients
        for (i = 0; i < MAX_USER; i++) {
            if (user[i].login && i != u - user) {
                write(user[i].new_sockfd[0].fd, buff, strlen(buff));
            }
        }
    }

    // Close the socket
    close(u->new_sockfd[0].fd);
}

void create_room()
{
    char room_name[128];
    int i, j;

    printf("Enter room name: ");
    fgets(room_name, sizeof(room_name), stdin);
    strtok(room_name, "\n");

    // Find an available room slot
    for(i = 0; i < MAX_ROOM; i++) {
        if(room[i].num_users == 0) {
            break;
        }
    }

    if(i == MAX_ROOM) {
        printf("Failed to create room. Maximum number of rooms reached.\n");
        return;
    }

    // Initialize the room
    room[i].num_users = 0;
    strcpy(room[i].name, room_name);

    printf("Room '%s' created.\n", room_name);
}
int num_rooms =0;
void process_command(char* command,char* room_name)
{
	if (strcmp(command, "CREATE") == 0)
	 {
            printf("Creating room '%s'\n", room_name);
            // Add room to list of rooms
            strcpy(room[num_rooms].name, room_name);
            room[num_rooms].num_users = 0;
            num_rooms++;
        }
}

/**
*Function to forcibly terminate when an error occurs
*Argument: message indicating where the error occurred
*Return value: none (exit the program when this function is called)
*/
void myerror(char *message){
	perror(message);
	exit(1);
}

/**
*Server operation: Terminate the server by typing "SERVER_END" on the keyboard
*/
int main(){
	int sockfd;			//communication socket
	struct user user[MAX_USER];		//Array that manages clients
	char buff[128], new_buff[128];	//Data for sending and receiving
	char command[BUFFER_SIZE], room_name[BUFFER_SIZE];
	struct pollfd fds[2];			//Read socket for viewing with poll()
	struct sockaddr_in serv_addr;	//server data
	int chkerr, i, j, k, nuser = 0;	//chkerr:A variable that holds the return value of the system call
				 //nuser：Number of users logged in

	//Initialize login state for all clients
	for(i=0;i<MAX_USER;i++){
		user[i].login = 0;
	}

	//Create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) myerror("socet_error");	//Error check, call myerror on error

	//Create address
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(10000);


	//Assign an address to a socket
	chkerr = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in));
	if(chkerr < 0) myerror("bind_error");

	fds[0].fd = sockfd;			//Register sockfd as a read socket to see with poll()
	fds[0].events = POLLIN;		//Set so that POLLIN is returned in revents

	fds[1].fd = 0;				//Register standard input as a read socket to see with poll()
	fds[1].events = POLLIN;		//Set so that POLLIN is returned in revents

	chkerr = listen(fds[0].fd, 5); //Instruct to start waiting for connection request
	if(chkerr < 0) myerror("listen_error");

	printf("The maximum number of connections for this server is %d\n",MAX_USER);
	while(1){
		poll(fds,2,0);	//Monitor sockfd and stdin
		if(fds[0].revents == POLLIN  &&  nuser < MAX_USER){
		//Enter if sockfd has a communication request. If the number of clients reaches the upper limit, it will not enter
			for(j=0; j< MAX_USER; j++){
				if(user[j].login != 1){	//Register a client to user[j] that is occupied
					user[j].new_sockfd[0].fd = accept(fds[0].fd, NULL, NULL);	//save fd to array
					if(user[j].new_sockfd[0].fd == -1) myerror("accept_error");

					user[j].new_sockfd[0].events = POLLIN;		//Set so that POLLIN is returned in revents

					chkerr = read(user[j].new_sockfd[0].fd, buff, 128);	//Receive user name
					if(chkerr < 0) myerror("read_name_error");

					strcpy(user[j].name, buff);	//Save name
					user[j].login = 1;			//Be logged in
					nuser++;				//increase by one
					sprintf(new_buff,"->%s log in ＊Number of remaining connections:%d\n",buff, MAX_USER-nuser);
					if(chkerr < 0) myerror("sprintf_error");

					if(nuser == MAX_USER){	//Alart if the number of users reached maximum
						sprintf(buff, "%s＊＊Maximum number of users reached＊＊\n", new_buff);
						if(chkerr < 0) myerror("sprintf_error");
						strcpy(new_buff, buff);
					}
					for(i=0;i<MAX_USER;i++){	//Send to all users
						if(user[i].login){
							chkerr = write(user[i].new_sockfd[0].fd, new_buff, 128);
							if(chkerr < 0) myerror("write_comment_error");
							if(i != j){
								chkerr = sprintf(buff,"->%s logged in\n",user[i].name);
								if(chkerr < 0) myerror("sprintf_error");
								chkerr = write(user[j].new_sockfd[0].fd, buff, 128);
								if(chkerr < 0) myerror("write_comment_error");
							}
						}
					}
					printf("%s",new_buff);
					break;
				}
			}
		}

		for(j=0;j<MAX_USER;j++){
			poll(user[j].new_sockfd,1,0);

			if(user[j].new_sockfd[0].revents == POLLIN  &&  user[j].login == 1){
			//Enter if there is a communication request from the client. You cannot enter if you are not logged in
				chkerr = read(user[j].new_sockfd[0].fd, buff, 128);	//Receipt of data
				if(chkerr < 0){
					myerror("read_comment_error");
				}else if(chkerr == 0){
				//enter if the socket with the client is closed by any method other than "LOGOUT" input
					strcpy(buff, "LOGOUT");		
				}

				if(strcmp(buff, "LOGOUT") == 0){	//If "LOGOUT" is entered
					chkerr = write(user[j].new_sockfd[0].fd, buff, 128);	//Send "LOGOUT" to the client to close the socket
					if(chkerr < 0) myerror("write_error");

					nuser--;	//Decrease by one
					user[j].login = 0;	//Be log out status

					chkerr = sprintf(new_buff, "<-%s logeged out ＊Number of remaining connections:%d\n", user[j].name, MAX_USER-nuser);
					if(chkerr < 0) myerror("sprintf_error");
				}
				else if (strcmp(command, "CREATE") == 0)
	 			{
				    printf("Creating room '%s'\n", room_name);
				    // Add room to list of rooms
				    strcpy(room[num_rooms].name, room_name);
				    room[num_rooms].num_users = 0;
				    num_rooms++;
        			}

				else{	//If any other comment is put
					chkerr = sprintf(new_buff, "%s: %s\n", user[j].name, buff);
					if(chkerr < 0) myerror("sprintf_error");
				}
 				printf("%s",new_buff);

				for(i=0;i<MAX_USER;i++){	//Send to all clients
					if(user[i].login){
						chkerr = write(user[i].new_sockfd[0].fd, new_buff, 128);
        				if(chkerr < 0) myerror("write_comment_error");
					}
				}
			}
		}
		if(fds[1].revents == POLLIN){	//Enter if there is input from the keyboard
			scanf("%s",buff);
			if(strcmp(buff, "SERVER_END") == 0){	//Exit from while statement when input from "SERVER_END"
				break;
			}
			else{
				printf("Invalid command\n");
			}
		}
	}
	for(i=0;i<MAX_USER;i++){	//Send all clients to close the socket
		if(user[i].login){
			chkerr = write(user[i].new_sockfd[0].fd, "SERVER_END", 128);
			if(chkerr < 0) myerror("write_serverend_error");
		}
	}


	//Wait until all users sockets are closed
	for(i=0;i<MAX_USER;i++){
		if(user[i].login){
			while(1){
				chkerr = read(user[i].new_sockfd[0].fd, buff, 128);
				if(chkerr < 0){
					myerror("close_read_error");
				}else if(chkerr == 0){
					//If the chkerr value is 0, the socket is closed
					break;
				}
			}
		}
	}

	printf("Terminate the server\n");

	//Terminate the server
	chkerr = close(sockfd);
	if(chkerr < 0) myerror("close_sockfd");
}
