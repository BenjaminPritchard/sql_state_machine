//-----------------------------------------------------------------------------------------------
//
// sql_state_server.c
//
// Server routine for sql_state_engine
//
// Benajmin Pritchard
// 1-October-2020
//
// NOTE: this code doesn't appear to work. I am not sure why. I need more time to debug it.
//		For now, just use the bash script sql_state_server.sh instead (which uses socat)
// 
// This code is derived from here:
//	http://www6.uniovi.es/cscene/CS5/CS5-05.html
//
// This wraper listens on a TCP/IP port 4242 for incoming connections, and when one does, 
// it spawns sql_state_engine.
//
// Just connect using putty or telent to interact via keyboard. 
//
// Or, to interact programmatically, just open a connection to port 4242, 
// and then send one or more of the following commands to the engine in ASCII format:
//
//	1 - show state of database
//	2 - show readonly transaction log
//	2 - update the database
//	3 - roll database back to state n
//
// When you are done, just close the connection.
//
// see client_test.py for an example
//
//-----------------------------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

// just execute sql_state_engine for this connection,
// redirecting its stdin/stdout/std to the socket
int exec_comm_handler( int sck )
{
  close(0); /* close standard input  */
  close(1); /* close standard output */
  close(2); /* close standard error  */

  // this is a neat trick...
  // see here:
  //	http://www6.uniovi.es/cscene/CS5/CS5-05.html
  if( dup(sck) != 0 || dup(sck) != 1 || dup(sck) != 2 ) {
    perror("error duplicating socket for stdin/stdout/stderr");
    exit(1);
  }
  // we don't need to say anything here;
  // obviously whatever sql_state_engine is printing will go to whoever is connecting now!
  //execl( "/bin/sh", "/bin/sh", "-c", "./sql_state_engine" );
  execl( "./sql_state_engine", "./sql_state_engine", NULL);
  
  perror("the execl(3) call failed.");
  exit(1);
}

int main(int argc, char* argvp[])
{
  int sck, client, addrlen;
  struct sockaddr_in this_addr, peer_addr;
  pid_t child_pid;
  unsigned short port = 4243; 
  
  printf("sql_state_listener\n");		// identify ourself
  printf("listening on port %u...\n", port);		

  addrlen = sizeof( struct sockaddr_in );
  memset( &this_addr, 0, addrlen );
  memset( &peer_addr, 0, addrlen );

  this_addr.sin_port        = htons(port);
  this_addr.sin_family      = AF_INET;
  this_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  sck = socket( AF_INET, SOCK_STREAM, IPPROTO_IP);
  bind( sck, (struct sockaddr * restrict) &this_addr, addrlen );
  listen( sck, 5 );
  
  // just run forever until we get SIGINT       
  while( -1 != (client = accept( sck, (struct sockaddr * restrict) &peer_addr, &addrlen ) ) ) {
    child_pid = fork();
	
    if( child_pid < 0 ) 
	{		
      perror("Error forking");  
	  exit(1);   
	}

	// if this is the new thread, then execute
    if( child_pid == 0 ) 
	{
      exec_comm_handler(sck);
    } 
	
	// if this is the original (listening) thread, just let let someone know we processed a connection OK...
	if ( child_pid > 0)
	{
		printf("Incoming connection handled ok...\n");
	}
	
  }

  exit(0);
  return 0;
}