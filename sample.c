/*
  Sample solution for NOS 2014 assignment: implement a simple multi-threaded 
  IRC-like chat service.

  (C) Paul Gardner-Stephen 2014.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/filio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

pthread_rwlock_t message_log_lock;

struct client_thread {
  pthread_t thread;
  int thread_id;
  int fd;

  int state;
  time_t timeout;
};

int create_listen_socket(int port)
{
  int sock = socket(AF_INET,SOCK_STREAM,0);
  if (sock==-1) return -1;

  int on=1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) == -1) {
    close(sock); return -1;
  }
  if (ioctl(sock, FIONBIO, (char *)&on) == -1) {
    close(sock); return -1;
  }
  
  /* Bind it to the next port we want to try. */
  struct sockaddr_in address;
  bzero((char *) &address, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  if (bind(sock, (struct sockaddr *) &address, sizeof(address)) == -1) {
    close(sock); return -1;
  } 

  if (listen(sock, 20) != -1) return sock;

  close(sock);
  return -1;
}

int accept_incoming(int sock)
{
  struct sockaddr addr;
  unsigned int addr_len = sizeof addr;
  int asock;
  if ((asock = accept(sock, &addr, &addr_len)) != -1) {
    return asock;
  }

  return -1;
}

int read_from_socket(int sock,unsigned char *buffer,int *count,int buffer_size)
{
  int t=time(0);
  if (*count>=buffer_size) return 0;
  int r=read(sock,&buffer[*count],buffer_size-*count);
  while(r!=0) {
    if (r>0) {
      (*count)+=r;
      t=time(0);
    }
    r=read(sock,&buffer[*count],buffer_size-*count);
    if (r==-1&&errno!=EAGAIN) {
      perror("read() returned error. Stopping reading from socket.");
      return -1;
    }
    // timeout after a few seconds of nothing
    if (time(0)-t>3) break;
  }
  buffer[*count]=0;
  return 0;
}

int parse_byte(struct client_thread *t, char c)
{
  // Parse next byte read on socket.
  // If a complete line, then parse the line
  return 0;
}

#define SERVER_GREETING ":irc.nos2014.net 020 * :Please register.\n"
void *client_connection(void *data)
{
  int i;
  int bytes=0;
  char buffer[8192];

  struct client_thread *t=data;
  t->state=0;
  t->timeout=time(0)+5;

  int r=write(t->fd,SERVER_GREETING,strlen(SERVER_GREETING));
  if (r<1) perror("write");
  
  // Main socket reading loop
  while(1) {
    bytes=read(t->fd,(unsigned char *)buffer,sizeof(buffer));
    for(i=0;i<bytes;i++) parse_byte(t,buffer[i]);
  }

  close(t->fd);
  pthread_exit(0);
}

int main(int argc,char **argv)
{
  if (argc!=2) {
    fprintf(stderr,"usage: sample <tcp port>\n");
    exit(-1);
  }

  int master_socket = create_listen_socket(atoi(argv[1]));

  if (pthread_rwlock_init(&message_log_lock,NULL))
    {
      fprintf(stderr,"Failed to create rwlock for message log.\n");
      exit(-1);
    }

  while(1) {
    int client_sock = accept_incoming(master_socket);
    if (client_sock!=-1) {
      // Got connection -- do something with it.
      struct client_thread *t=calloc(sizeof(struct client_thread),1);
      if (t!=NULL) {
	t->fd = client_sock;
	if (pthread_create(&t->thread, NULL, client_connection, 
			   (void*)t))
	  {
	    // Thread creation failed
	    close(client_sock);
	  }	
      }
    }
  }

}