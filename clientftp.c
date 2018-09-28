/* 
 * Client FTP program
 * Client Ftp program allows client to manipulate operating system using the following commands:
 * user, pass, ls, pwd, cd, mkdir, rmdir, help, rm, stat, and quit
 * The client can create directories, remove directories, remove files, check the current working directory,
 * check the contents of the current working directory, change the working directory, ask for help, check
 * the status of the ftp and quit the ftp. 
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#define SERVER_FTP_PORT 3470
#define DATA_CONNECTION_PORT 3471

/* Error and OK codes */
#define OK 0
#define ER_INVALID_HOST_NAME -1
#define ER_CREATE_SOCKET_FAILED -2
#define ER_BIND_FAILED -3
#define ER_CONNECT_FAILED -4
#define ER_SEND_FAILED -5
#define ER_RECEIVE_FAILED -6


/* Function prototypes */

int clntConnect(char	*serverName, int *s);
int sendMessage (int s, char *msg, int  msgSize);
int receiveMessage(int s, char *buffer, int  bufferSize, int *msgSize);


/* List of all global variables */

char userCmd[1024];	/* user typed ftp command line read from keyboard */
char cmd[1024];		/* ftp command extracted from userCmd */
char argument[1024];	/* argument extracted from userCmd */
char replyMsg[1024];    /* buffer to receive reply message from server */


/*
 * main
 *
 * Function connects to the ftp server using clntConnect function.
 * Reads one ftp command in one line from the keyboard into userCmd array.
 * Sends the user command to the server.
 * Receive reply message from the server.
 * On receiving reply to QUIT ftp command from the server,
 * close the control connection socket and exit from main
 *
 * Parameters
 * argc		- Count of number of arguments passed to main (input)
 * argv  	- Array of pointer to input parameters to main (input)
 *		   It is not required to pass any parameter to main
 *		   Can use it if needed.
 *
 * Return status
 *	OK	- Successful execution until QUIT command from client 
 *	N	- Failed status, value of N depends on the function called or cmd processed
 */

int main(int argc,char *argv[])
{
	/* List of local varibles */

	int ccSocket;	/* Control connection socket - to be used in all client communication */
	int dcSocket;
        int dcListenSocket;
	int msgSize;	/* size of the reply message received from the server */
	int status = OK;

	/*
	 * NOTE: without \n at the end of format string in printf,
         * UNIX will buffer (not flush)
	 * output to display and you will not see it on monitor.
 	 */

	printf("Started execution of client ftp\n");


	/* Connect to client ftp*/

	printf("Calling clntConnect to connect to the server\n");	/* changed text */

	status=clntConnect("192.168.200.230", &ccSocket);  /* Off-campus IP: "134.241.37.230" */
	if(status != 0)
	{
		printf("Connection to server failed, exiting main. \n");
		return (status);
	}

        status = svcInitServer(&dcListenSocket);

	/* 
	 * Read an ftp command with argument, if any, in one line from user into userCmd.
	 * Copy ftp command part into ftpCmd and the argument into arg array.
 	 * Send the line read (both ftp cmd part and the argument part) in userCmd to server.
	 * Receive reply message from the server.
	 * until quit command is typed by the user.
	 */

	do
	{
	        char tempCmd[1024];
	        strcpy(tempCmd, userCmd);
		
	        char* cmdpointer = strtok(tempCmd," ");
	        char* argpointer = strtok(NULL," ");
		
	        strcpy(cmd, cmdpointer);
	        strcpy(argument, argpointer);

		printf("my ftp> ");
		gets(userCmd);
		status = sendMessage(ccSocket, userCmd, strlen(userCmd)+1);
		if(status != OK)
                {
		    break;
		}
		if(strcmp(cmd, "send") == 0)
                {
		    dcSocket = accept(dcListenSocket, NULL, NULL);
		    FILE* fptr = fopen("argument", "r");
		    int sizeOfBuf;
		    do
                    {
			char buf[100];
		        sizeOfBuf = fread(buf, 1, sizeof(buf), fptr);
	                sendMessage(dcSocket, buf, sizeOfBuf);
	   	    }while(!(feof(fptr)));
		    fclose(fptr);
	            close(dcSocket);
		}

		else if(strcmp(cmd, "get") == 0)
                {
	            dcSocket = accept(dcListenSocket, NULL, NULL);
		    FILE* fptr = fopen("argument", "w");
		    int bytesReceived;
		    do
                    {
			char buf[100];
			
			if(fptr == NULL)
                        {
			    close(dcSocket);
			}
			status = receiveMessage(dcSocket, buf, sizeof(buf), &bytesReceived);
			status = fwrite(buf, 1, bytesReceived, fptr);
		    }while(bytesReceived != 0 && status == 0);
		    fclose(fptr);
		    close(dcSocket);
		}
	  
		
		else
                {
		 
		    /* Receive reply message from the the server */
		    status = receiveMessage(ccSocket, replyMsg, sizeof(replyMsg), &msgSize);
		    if(status < 0)
		    {
		        break;
		    }
		}
	}
	while (strcmp(cmd, "quit") != 0);

	printf("Closing control connection \n");
	close(ccSocket);  /* close control connection socket */
        close(dcListenSocket);

	printf("Exiting client main \n");

	return (status);

}  /* end main() */


/*
 * clntConnect
 *
 * Function to create a socket, bind local client IP address and port to the socket
 * and connect to the server
 *
 * Parameters
 * serverName	- IP address of server in dot notation (input)
 * s		- Control connection socket number (output)
 *
 * Return status
 *	OK			- Successfully connected to the server
 *	ER_INVALID_HOST_NAME	- Invalid server name
 *	ER_CREATE_SOCKET_FAILED	- Cannot create socket
 *	ER_BIND_FAILED		- bind failed
 *	ER_CONNECT_FAILED	- connect failed
 */


int clntConnect (
	char *serverName, /* server IP address in dot notation (input) */
	int *s 		  /* control connection socket number (output) */
	)
{
	int sock;	/* local variable to keep socket number */

	struct sockaddr_in clientAddress;  	/* local client IP address */
	struct sockaddr_in serverAddress;	/* server IP address */
	struct hostent	   *serverIPstructure;	/* host entry having server IP address in binary */


	/* Get IP address os server in binary from server name (IP in dot natation) */
	if((serverIPstructure = gethostbyname(serverName)) == NULL)
	{
		printf("%s is unknown server. \n", serverName);
		return (ER_INVALID_HOST_NAME);  /* error return */
	}

	/* Create control connection socket */
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("cannot create socket ");
		return (ER_CREATE_SOCKET_FAILED);	/* error return */
	}

	/* initialize client address structure memory to zero */
	memset((char *) &clientAddress, 0, sizeof(clientAddress));

	/* Set local client IP address, and port in the address structure */
	clientAddress.sin_family = AF_INET;	/* Internet protocol family */
	clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);  /* INADDR_ANY is 0, which means */
						 /* let the system fill client IP address */
	clientAddress.sin_port = 0;  /* With port set to 0, system will allocate a free port */
			  /* from 1024 to (64K -1) */

	/* Associate the socket with local client IP address and port */
	if(bind(sock,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
	{
		perror("cannot bind");
		close(sock);
		return(ER_BIND_FAILED);	/* bind failed */
	}


	/* Initialize serverAddress memory to 0 */
	memset((char *) &serverAddress, 0, sizeof(serverAddress));

	/* Set ftp server ftp address in serverAddress */
	serverAddress.sin_family = AF_INET;
	memcpy((char *) &serverAddress.sin_addr, serverIPstructure->h_addr, 
			serverIPstructure->h_length);
	serverAddress.sin_port = htons(SERVER_FTP_PORT);

	/* Connect to the server */
	if (connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
	{
		perror("Cannot connect to server ");
		close (sock); 	/* close the control connection socket */
		return(ER_CONNECT_FAILED);  	/* error return */
	}


	/* Store listen socket number to be returned in output parameter 's' */
	*s=sock;

	return(OK); /* successful return */
}  // end of clntConnect() */



/*
 * sendMessage
 *
 * Function to send specified number of octet (bytes) to client ftp
 *
 * Parameters
 * s		- Socket to be used to send msg to client (input)
 * msg  	- Pointer to character arrary containing msg to be sent (input)
 * msgSize	- Number of bytes, including NULL, in the msg to be sent to client (input)
 *
 * Return status
 *	OK		- Msg successfully sent
 *	ER_SEND_FAILED	- Sending msg failed
 */

int sendMessage(
	int s, 		/* socket to be used to send msg to client */
	char *msg, 	/*buffer having the message data */
	int msgSize 	/*size of the message/data in bytes */
	)
{
	int i;


	/* Print the message to be sent byte by byte as character */
	for(i=0;i<msgSize;i++)
	{
		printf("%c",msg[i]);
	}
	printf("\n");
	if((send(s,msg,msgSize,0)) < 0) /* socket interface call to transmit */
	{
		perror("unable to send ");
		return(ER_SEND_FAILED);
	}
	

	return(OK); /* successful send */
}


/*
 * receiveMessage
 *
 * Function to receive message from client ftp
 *
 * Parameters
 * s		- Socket to be used to receive msg from client (input)
 * buffer  	- Pointer to character arrary to store received msg (input/output)
 * bufferSize	- Maximum size of the array, "buffer" in octent/byte (input)
 *		    This is the maximum number of bytes that will be stored in buffer
 * msgSize	- Actual # of bytes received and stored in buffer in octet/byes (output)
 *
 * Return status
 *	OK			- Msg successfully received
 *	ER_RECEIVE_FAILED	- Receiving msg failed
 */

int receiveMessage (
	int s, 		/* socket */
	char *buffer, 	/* buffer to store received msg */
	int bufferSize, /* how large the buffer is in octet */
	int *msgSize 	/* size of the received msg in octet */
	)
{
	int i;

	*msgSize=recv(s,buffer,bufferSize,0); /* socket interface call to receive msg */

	if(*msgSize<0)
	{
		perror("unable to receive");
		return(ER_RECEIVE_FAILED);
	}

	/* Print the received msg byte by byte */
	for(i=0;i<*msgSize;i++)
	{
		printf("%c", buffer[i]);
	}
	printf("\n");

	return (OK);
}


/*
 * clntExtractReplyCode
 *
 * Function to extract the three digit reply code 
 * from the server reply message received.
 * It is assumed that the reply message string is of the following format
 *      ddd  text
 * where ddd is the three digit reply code followed by or or more space.
 *
 * Parameters
 *	buffer	  - Pointer to an array containing the reply message (input)
 *	replyCode - reply code number (output)
 *
 * Return status
 *	OK	- Successful (returns always success code
 */

int clntExtractReplyCode (
	char	*buffer,    /* Pointer to an array containing the reply message (input) */
	int	*replyCode  /* reply code (output) */
	)
{
	/* extract the codefrom the server reply message */
   sscanf(buffer, "%d", replyCode);

   return (OK);
}  // end of clntExtractReplyCode()

int svcInitServer (
	int *s 		/*Listen socket number returned from this function */
	)
{
	int sock;
	struct sockaddr_in svcAddr;
	int qlen;

	/*create a socket endpoint */
	if( (sock=socket(AF_INET, SOCK_STREAM,0)) <0)
	{
		perror("cannot create socket");
		return(ER_CREATE_SOCKET_FAILED);
	}

	/*initialize memory of svcAddr structure to zero. */
	memset((char *)&svcAddr,0, sizeof(svcAddr));

	/* initialize svcAddr to have server IP address and server listen port#. */
	svcAddr.sin_family = AF_INET;
	svcAddr.sin_addr.s_addr=htonl(INADDR_ANY);  /* server IP address */
	svcAddr.sin_port=htons(DATA_CONNECTION_PORT);    /* server listen port # */

	/* bind (associate) the listen socket number with server IP and port#.
	 * bind is a socket interface function 
	 */
	if(bind(sock,(struct sockaddr *)&svcAddr,sizeof(svcAddr))<0)
	{
		perror("cannot bind");
		close(sock);
		return(ER_BIND_FAILED);	/* bind failed */
	}

	/* 
	 * Set listen queue length to 1 outstanding connection request.
	 * This allows 1 outstanding connect request from client to wait
	 * while processing current connection request, which takes time.
	 * It prevents connection request to fail and client to think server is down
	 * when in fact server is running and busy processing connection request.
	 */
	qlen=1; 


	/* 
	 * Listen for connection request to come from client ftp.
	 * This is a non-blocking socket interface function call, 
	 * meaning, server ftp execution does not block by the 'listen' funcgtion call.
	 * Call returns right away so that server can do whatever it wants.
	 * The TCP transport layer will continuously listen for request and
	 * accept it on behalf of server ftp when the connection requests comes.
	 */

	listen(sock,qlen);  /* socket interface function call */

	/* Store listen socket number to be returned in output parameter 's' */
	*s=sock;

	return(OK); /*successful return */
}
