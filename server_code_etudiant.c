#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "server_code_etudiant.h"


#define LINKED_LIST

#ifdef LINKED_LIST

struct list{
  client_t* client;
  struct list* next;
};

struct list* client_list = NULL;

#else

client_t *clients[MAX_CLIENTS];

#endif

/**int cli_count(){
  struct list* p = client_list;
  int n=0;
  while(p){
    n=n+1;
    p=p->next;
  }
  return n;
}*/

/* Strip CRLF */
void strip_newline(char *s) {
  while(*s != '\0'){
    if(*s == '\r' || *s == '\n'){
      *s = '\0';
    }
    s++;
  }
}

client_t *get_client_from_fd(int fd) {

  #ifdef LINKED_LIST

  struct list* p = client_list;
  while(p){
    if ((p->client->fd) == fd){
      return(p->client);
    }
    p=p->next;
  }
  return NULL;

  #else

  int i;
  for(i=0; i<MAX_CLIENTS; i++) {
    if(clients[i] && clients[i]->fd == fd)
      return clients[i];
  }
  return NULL;

  #endif
}

client_t *get_client_from_name(char* name) {

  #ifdef LINKED_LIST

  struct list* p = client_list;
  while(p){
    if (strcmp(p->client->name,name)==0){
      return(p->client);
    }
    p=p->next;
  }
  return NULL;


  #else

  int i;
  for(i=0; i<MAX_CLIENTS; i++) {
    if(clients[i]) {
      if(strcmp(clients[i]->name, name) == 0) {
	return clients[i];
      }
    }
  }
  return NULL;

  #endif
}


void server_init() {
  #ifdef LINKED_LIST


  #else
  memset(clients, 0, sizeof(client_t*)*MAX_CLIENTS);
  #endif
  struct sigaction s;
  s.sa_handler = server_finalize;
  s.sa_flags = 0;
  //s.sa_mask = 0;
  sigaction(SIGINT, &s, NULL);
  sigaction(SIGTERM, &s, NULL);
  sigaction(SIGKILL, &s, NULL);
}

void server_finalize(int signo) {
  char buff_out[1024];
  sprintf(buff_out, "* The server is about to stop.\n");
  send_message_all(buff_out);

  exit(EXIT_SUCCESS);
}
/* Add client to queue */
void queue_add(client_t *cl){
  #ifdef LINKED_LIST

  struct list* new_list = malloc(sizeof(struct list));
  new_list->client = cl;
  new_list->next = client_list;
  client_list=new_list;

  #else

  int i;
  for(i=0;i<MAX_CLIENTS;i++){
    if(!clients[i]){
      clients[i] = cl;
      return;
    }
  }
  #endif
}

/* Delete client from queue */
void queue_delete(client_t *client){

  #ifdef LINKED_LIST

  if(client_list && client_list->client->uid == client->uid){
    struct list* c = client_list;
    client_list = client_list->next;
    free(c);
    return;
  }

  struct list* p = client_list;
  while(p->next){
    struct list* c = p->next;
    if(c->client->uid == client->uid){
      p->next=c->next;
      free(c);
      return;
    }

    p=p->next;
  }
  #else

  int i;
  for(i=0;i<MAX_CLIENTS;i++){
    if(clients[i]){
      if(clients[i]->uid == client->uid){
	clients[i] = NULL;
	return;
      }
    }
  }

  #endif
}

/* Send a message to a client */
void send_message(char *s, client_t *client){
  fwrite(s, sizeof(char), strlen(s), client->client_conn);
}

/* Send message to all clients */
void send_message_all(char *s){
  #ifdef LINKED_LIST

  struct list* p = client_list;
  while(p){
    send_message(s,p->client);
    p=p->next;
  }

  #else

  int i;
  for(i=0;i<MAX_CLIENTS;i++){
    if(clients[i]){
      send_message(s, clients[i]);
    }
  }

  #endif
}

void assign_default_name(client_t* cli) {
  FILE* s=fopen("default_names.txt","r");
  fseek(s,0,SEEK_END);
  int taille = ftell(s);
  int nb = taille/NAME_MAX_LENGTH;
  int b=0;
  while(b==0){
    int r = (rand()%nb);
    char * str=malloc(NAME_MAX_LENGTH);
    fseek(s,r*NAME_MAX_LENGTH,SEEK_SET);
    fread((void *)str,NAME_MAX_LENGTH,1,s);
    if (!get_client_from_name(str)){
      strcpy(cli->name,str);
      b=1;
    }
    free(str);
  }
  //sprintf(cli->name, "Anonymous_%d", cli->uid);
}

/* this function is called whzezeen a client connects to the server */
void say_hello(client_t *cli) {
  char buff_out[1024];
  /* choose a default name */
  assign_default_name(cli);
  sprintf(buff_out, "* %s joins the chatroom\n", cli->name);
  send_message_all(buff_out);
}

void process_cmd_msg(client_t*client,
		     char*param) {
  char*dest = strsep(&param, " ");
  if(!dest){
    send_message("* to who ?\n", client);
    return;
  }

  char buffer[1024];
  sprintf(buffer, "[PM][%s --> %s] %s\n", client->name, dest, param);
  client_t* to = get_client_from_name(dest);
  if(!to ){
    send_message("* %s does not exist!\n", client);
  } else {
    send_message(buffer, to);
    send_message(buffer, client);
  }
}

void process_cmd_help(client_t* client) {
  char buff_out[1024];
  sprintf(buff_out, "/help     Show help\n");
  strcat(buff_out, "/msg      <name> <message> Send private message\n");
  strcat(buff_out, "/ping     Server test\n");
  strcat(buff_out, "/quit     Quit chatroom\n");
  send_message(buff_out, client);
}

void process_cmd_ping(client_t* client,
		      char* param) {
  send_message("* pong\n", client);
}

void process_cmd_me(client_t *client, char* param){
  char buff_out[1024];
  sprintf(buff_out, "* %s %s\n", client->name, param);
  send_message(buff_out, client);
}

void send_active_clients(client_t * client){
  struct list * p = client_list;
  while(p){
    char s[1024];
    sprintf(s, "  * client %d | %s\n", p->client->fd,p->client->name);
    send_message(s,client);
    p=p->next;
  }
}
void process_cmd_names( client_t * client, char* param){
  if(!fork()){
    char buff_out[1024];
    sprintf(buff_out, "* There are %d clients\n",cli_count);
    send_message(buff_out,client);
    send_active_clients(client);
    exit(EXIT_SUCCESS);
  }
}

void handle_incoming_cmd(client_t *cli) {
  char buff_out[1024];
  char buff_in[1024];

  if(fgets(buff_in, 1024*sizeof(char), cli->client_conn) == 0) {
    if(!feof(cli->client_conn)) {
      perror("read failed");
      abort();
    } else {
      printf("Client %s disconnected\n", cli->name);
      queue_delete(cli);
      return;
    }
  }

  strip_newline(buff_in);

  /* Ignore empty buffer */
  if(!strlen(buff_in)){
    printf("Empty message\n");
  }

  /* Special options */
  char *cmd_line = buff_in;
  if(buff_in[0] == '/'){
    char *command;
    command = strsep(&cmd_line," ");
    if(!strcmp(command, "/quit")){
      exit(EXIT_SUCCESS);
    } else if(!strcmp(command, "/ping")) {
      process_cmd_ping(cli, cmd_line);
    } else if(!strcmp(command, "/msg")) {
      process_cmd_msg(cli, cmd_line);
    } else if(!strcmp(command, "/me")) {
      process_cmd_me(cli, cmd_line);
    } else if(!strcmp(command, "/names")) {
      process_cmd_names(cli, cmd_line);
    } else {
      /* /help or unknown command */
      process_cmd_help(cli);
    }
  }else{
    /* Send message */
    sprintf(buff_out, "[%s] %s\n", cli->name, cmd_line);
    send_message_all(buff_out);
  }
}
