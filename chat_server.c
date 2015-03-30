#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>
#include <syslog.h>
#include "rb_tree.c"
#include "message.c"

#define MSG_LEN 1024
#define FIELD_LEN 6
#define NAME_LIST_LEN 1000000
#define SERVER "server"
#define LOGOUT "logout"
#define LOGIN "login"

int sockfd; // server's socket for listen the port
static struct sockaddr_in remote_addr;  // remote address
static struct sockaddr_in local_addr;   // local address
pthread_rwlock_t data_lock = PTHREAD_RWLOCK_INITIALIZER; //thread lock for read and write
pthread_rwlock_t name_lock = PTHREAD_RWLOCK_INITIALIZER; //thread lock for read and write
char *name_list; //list of username

void *deal_request(void *); //deal request method
void split(char *msg_fields[], char *str, const char *cut);
void notify_all(struct Node *root, int sockfd, char *from, char *content, char *except_name); //send to everyone except 'except_name'
void send_message(int sockfd, char *from, char *to, char* content); //send message to one
void my_strcpy(char *dest, const char *src);

//init the server, the argument port specify which port to listen
int server_init(int port) {
	initialize();
	name_list = (char*)malloc(NAME_LIST_LEN);
	my_strcpy(name_list, "list");
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(port);
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		syslog(LOG_ERR, "create socket error.");
		return -1;
	}
	if (bind(sockfd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr)) < 0) {
		syslog(LOG_ERR, "bind error.");
		return -1;
	}
	return 0;
}

//let the server to lisen to the port
void server_listen() {
	listen(sockfd, 10);
	int addr_size;
	pthread_t tid;
	int i;
	syslog(LOG_INFO, "Server listenning the port");
	while (1) {
		int new_sockfd = accept(sockfd, (struct sockaddr*)&remote_addr, &addr_size);
		syslog(LOG_INFO, "remote ip: %s port: %d, Established", inet_ntoa(remote_addr.sin_addr), remote_addr.sin_port);
		if (pthread_create(&tid, NULL, deal_request, (void*)&new_sockfd) != 0) {
			syslog(LOG_ERR, "create thread error.");
		}
		if (pthread_detach(tid) != 0) {
			syslog(LOG_ERR, "detach thread error.");
		}
	}
	free(name_list);
}


void *deal_request(void *arg) {
	char *msg_fields[FIELD_LEN];
	int i;
	for (i = 0; i < FIELD_LEN; ++i) {
		msg_fields[i] = (char *)malloc(MSG_LEN);
	}
	int sockfd = *(int *)arg;
	char buf[MSG_LEN];
	char come[5 + MAX_NAME_LEN];
	char tmp[MSG_LEN];
	ssize_t nbytes;
	struct Node *node;
	while (1) {
		memset(buf, 0, sizeof(buf));
		nbytes = recv(sockfd, buf, (size_t)sizeof(buf), 0);
		if (nbytes > 0) {
			//putchar('\n');
			fflush(NULL);
			zero_del(buf, nbytes);
			buf[nbytes >> 1] = 0;
			split(msg_fields, buf, ",:");

			if (strcmp(msg_fields[3], SERVER) == 0) { //if the message to server
				if (strcmp(msg_fields[5], LOGIN) == 0) { //if the client login
					pthread_rwlock_rdlock(&data_lock);
					node = search(msg_fields[1]);          //find whether the username has already login
					pthread_rwlock_unlock(&data_lock);
					if (node != NIL) {  //if the username was found
						send_message(sockfd, SERVER, msg_fields[1], "exist"); // tell the client that the username exist
						continue;
					}
					pthread_rwlock_wrlock(&name_lock);
					strcat(name_list, "&");               //put the username in name_list
					strcat(name_list, msg_fields[1]);
					pthread_rwlock_unlock(&name_lock);

					send_message(sockfd, SERVER, msg_fields[1], name_list);  //send the name_list to the user

					pthread_rwlock_wrlock(&data_lock);
					insert(create_node(msg_fields[1], strlen(msg_fields[1]) + 1, sockfd)); //insert into <user_name, sockfd> pair to the rb-tree
					pthread_rwlock_unlock(&data_lock);
					// notifyall that a new user has login
					memset(come, 0, sizeof(come));
					strcat(come, "come&");
					notify_all(head, sockfd, SERVER, strcat(come, msg_fields[1]), msg_fields[1]);
					syslog(LOG_INFO, "User %s Login", msg_fields[1]);
				} else if (strcmp(msg_fields[5], LOGOUT) == 0) {
					// find the <user_name, sockfd> pair node in the rb-tree
					pthread_rwlock_rdlock(&data_lock); node = search(msg_fields[1]);
					pthread_rwlock_unlock(&data_lock);
					if (node == NIL) {
						continue;
					}
					pthread_rwlock_wrlock(&data_lock);
					delete(node); //if find then delete it
					pthread_rwlock_unlock(&data_lock);
					//notify that a user has logout
					memset(come, 0, sizeof(come));
					strcat(come, "leave&");
					notify_all(head, sockfd, SERVER, strcat(come, msg_fields[1]), msg_fields[1]);
					//delte the username from the name_list
					pthread_rwlock_wrlock(&name_lock);
					char *pos = strstr(name_list + 4, msg_fields[1]);
					my_strcpy(tmp, pos + strlen(msg_fields[1]));
					my_strcpy(pos - 1, tmp);
					pthread_rwlock_unlock(&name_lock);
					syslog(LOG_INFO, "User %s Logout", msg_fields[1]);
				}
			} else if (strcmp(msg_fields[3], "@") == 0) {
				//send message to everyone
				my_strcpy(tmp, msg_fields[5]);
				my_strcpy(msg_fields[5], "public&");
				memcpy(msg_fields[5] + 7, tmp, strlen(tmp) + 1);
				notify_all(head, sockfd, msg_fields[1], msg_fields[5], msg_fields[1]);
			} else { 
				//send message to one
				pthread_rwlock_rdlock(&data_lock);
				node = search(msg_fields[3]);          //find whether the username has already login
				pthread_rwlock_unlock(&data_lock);
				if (node == NIL) {  //if the username was found
					continue;
				}
				my_strcpy(tmp, msg_fields[5]);
				my_strcpy(msg_fields[5], "private&");
				memcpy(msg_fields[5] + 8, tmp, strlen(tmp) + 1);
				send_message(node->socket_fd, msg_fields[1], msg_fields[3], msg_fields[5]);
			}
		} else {
			break;
		}
	}
	for (i = 0; i < FIELD_LEN; ++i) {
		free(msg_fields[i]);
	}
	close(sockfd);
}

void my_strcpy(char *dst, const char *src) {
	int i = 0;
	while (src[i] != 0) {
		dst[i] = src[i];
		++i;
	}
	dst[i] = 0;
}

//split the message str, the argument cut determine how to split, the result was store in msg_fields
void split(char *msg_fields[], char *str, const char *cut) {
	char *ptr = strtok(str, cut);
	int i;
	while (i < FIELD_LEN && ptr != NULL) {
		my_strcpy(msg_fields[i++], ptr);
		ptr = strtok(NULL, cut);
	}
	fflush(NULL);
}

//send message to one
void send_message(int sockfd, char *from, char *to, char* content) {
	char buf[MSG_LEN >> 1];
	char buf2[MSG_LEN];
	memset(buf, 0, sizeof(buf));
	memset(buf2, 0, sizeof(buf2));
	strcat(buf, "from:");
	strcat(buf, from);
	strcat(buf, ",to:");
	strcat(buf, to);
	strcat(buf, ",content:");
	strcat(buf, content);
	zero_add(buf, strlen(buf), buf2);
	buf2[strlen(buf) << 1] = 0;
	send(sockfd, buf2, strlen(buf) << 1, 0);
	//send(sockfd, buf, sizeof(buf), 0);
}

//send to everyone except 'except_name'
void notify_all(struct Node *root, int sockfd, char *from, char *content, char *except_name) {
	if (root == NIL) return;
	if (strcmp(except_name, root->user_name) != 0) {
		send_message(root->socket_fd, from, root->user_name, content);
	}
	notify_all(root->left, sockfd, from, content, except_name);
	notify_all(root->right, sockfd, from, content, except_name);
}
