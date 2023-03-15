/*
A server program that handles chats between multiple clients.
If there is a communication request, create a new socket. When the number of clients reaches the limit, print a warning message to the server and clients and prevent new sockets from being created.
Send the name of the logged-in client to a newly logged-in client.
When a client is newly connected, when a client logs out, it sends login/logout along with its name to other clients. When a "LOGOUT" message is received from a client, it is assumed to be in a logout state and another client can enter.
If a normal message is sent, send the message to all clients.
Typing "SERVER_END" prompts all clients to close their sockets, confirms that all clients' sockets have been closed, then closes the sockets and terminates the program.
*/

#include<stdio.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<stdlib.h>
#include<poll.h>

#define MAX_USER 10 		//Set the maximum number of connected users

//Structure with user information
struct user{
	char name[128];		//user name
	int login;			//1 for login status, 0 for logout status
	struct pollfd new_sockfd[1];	//Read socket for viewing with poll()
};


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

	//アドレスを作る
	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(10000);

	//setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));		//デバッグ時有効

	//ソケットにアドレスを割り当てる
	chkerr = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in));
	if(chkerr < 0) myerror("bind_error");

	fds[0].fd = sockfd;			//poll()で見る読み込みソケットとしてsockfdを登録
	fds[0].events = POLLIN;		//reventsにPOLLINが返ってくるように設定する

	fds[1].fd = 0;				//poll()で見る読み込みソケットとして標準入力を登録
	fds[1].events = POLLIN;		//reventsにPOLLINが返ってくるように設定する

	chkerr = listen(fds[0].fd, 5); //コネクション要求を待ち始めるよう指示
	if(chkerr < 0) myerror("listen_error");

	printf("このサーバーの最大接続人数は%dです\n",MAX_USER);
	while(1){
		poll(fds,2,0);	//sockfdと標準入力を監視
		if(fds[0].revents == POLLIN  &&  nuser < MAX_USER){
		//sockfdに通信要求がある場合入る。クライアントの人数が上限に達している場合は入らない
			for(j=0; j< MAX_USER; j++){
				if(user[j].login != 1){	//ログインされていないuser[j]にクライアントを登録
					user[j].new_sockfd[0].fd = accept(fds[0].fd, NULL, NULL);	//fdを配列に保存
					if(user[j].new_sockfd[0].fd == -1) myerror("accept_error");

					user[j].new_sockfd[0].events = POLLIN;		//reventsにPOLLINが返ってくるように設定する

					chkerr = read(user[j].new_sockfd[0].fd, buff, 128);	//クライアントからデータ(クライアントの名前)を受け取る
					if(chkerr < 0) myerror("read_name_error");

					strcpy(user[j].name, buff);	//nameに名前を保存する
					user[j].login = 1;			//ログイン状態にする
					nuser++;				//クライアントの人数を1人増やす
					sprintf(new_buff,"->%sさんがloginしました ＊残り接続可能数:%d\n",buff, MAX_USER-nuser);
					if(chkerr < 0) myerror("sprintf_error");

					if(nuser == MAX_USER){	//最大人数に達した時警告文をいれる
						sprintf(buff, "%s＊＊ユーザ人数が上限になりました＊＊\n", new_buff);
						if(chkerr < 0) myerror("sprintf_error");
						strcpy(new_buff, buff);
					}
					for(i=0;i<MAX_USER;i++){	//全クライアントに送信する
						if(user[i].login){
							chkerr = write(user[i].new_sockfd[0].fd, new_buff, 128);
							if(chkerr < 0) myerror("write_comment_error");
							if(i != j){
								chkerr = sprintf(buff,"->%sさんがloginしています\n",user[i].name);
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
			//クライアントからの通信要求がある場合に入る。ログイン状態でない場合は入らない
				chkerr = read(user[j].new_sockfd[0].fd, buff, 128);	//データの受け取り
				if(chkerr < 0){
					myerror("read_comment_error");
				}else if(chkerr == 0){
				//chkerrが0の場合、"LOGOUT"入力以外の方法でクライアントとのソケットが切れた場合入る
					strcpy(buff, "LOGOUT");		//LOGOUT扱いにする
				}

				if(strcmp(buff, "LOGOUT") == 0){	//"LOGOUT"が入力された時
					chkerr = write(user[j].new_sockfd[0].fd, buff, 128);	//クライアントに"LOGOUT"を送りソケットをcloseさせる
					if(chkerr < 0) myerror("write_error");

					nuser--;	//クライアントを1人減らす
					user[j].login = 0;	//ログアウト状態にする

					chkerr = sprintf(new_buff, "<-%sさんがlogoutしました ＊残り接続可能数:%d\n", user[j].name, MAX_USER-nuser);
					if(chkerr < 0) myerror("sprintf_error");
				}else{	//それ以外のコメントが送られた場合
					chkerr = sprintf(new_buff, "%s: %s\n", user[j].name, buff);
					if(chkerr < 0) myerror("sprintf_error");
				}
 				printf("%s",new_buff);

				for(i=0;i<MAX_USER;i++){	//全クライアントに送信する
					if(user[i].login){
						chkerr = write(user[i].new_sockfd[0].fd, new_buff, 128);
        				if(chkerr < 0) myerror("write_comment_error");
					}
				}
			}
		}
		if(fds[1].revents == POLLIN){	//キーボードからの入力がある場合に入る
			scanf("%s",buff);
			if(strcmp(buff, "SERVER_END") == 0){	//"SERVER_END"から入力された場合while文から抜ける
				break;
			}
			else{
				printf("無効なコマンドです\n");
			}
		}
	}
	for(i=0;i<MAX_USER;i++){	//全クライアントにソケットをcloseするよう送信する
		if(user[i].login){
			chkerr = write(user[i].new_sockfd[0].fd, "SERVER_END", 128);
			if(chkerr < 0) myerror("write_serverend_error");
		}
	}


	//全クライアントのソケットcloseするまで待つ
	for(i=0;i<MAX_USER;i++){
		if(user[i].login){
			while(1){
				chkerr = read(user[i].new_sockfd[0].fd, buff, 128);
				if(chkerr < 0){
					myerror("close_read_error");
				}else if(chkerr == 0){
					//chkerrの値が0ならばソケットがcloseされている
					break;
				}
			}
		}
	}

	printf("サーバーを終了します\n");

	//ソケットを終了する
	chkerr = close(sockfd);
	if(chkerr < 0) myerror("close_sockfd");
}
