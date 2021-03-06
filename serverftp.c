/* 
 * server FTP program
 *
 * The Server Ftp program allows the server to check the commands given by the client
 * and create a message based on the command given. The server ftp will either use the 
 * system command, direct output to a file and read from that file, or simply reply
 * with a status message.
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
#define ER_ACCEPT_FAILED -7


/* Function prototypes */

int svcInitServer(int *s);
int sendMessage (int s, char *msg, int  msgSize);
int receiveMessage(int s, char *buffer, int  bufferSize, int *msgSize);


/* List of all global variables */

char userCmd[1024];	/* user typed ftp command line received from client */
char cmd[1024];		/* ftp command (without argument) extracted from userCmd */
char argument[1024];	/* argument (without ftp command) extracted from userCmd */
char replyMsg[1024];       /* buffer to send reply message to client */


/*
 * main
 *
 * Function to listen for connection request from client
 * Receive ftp command one at a time from client
 * Process received command
 * Send a reply message to the client after processing the command with staus of
 * performing (completing) the command
 * On receiving QUIT ftp command, send reply to client and then close all sockets
 *
 * Parameters
 * argc		- Count of number of arguments passed to main (input)
 * argv  	- Array of pointer to input parameters to main (input)
 *		   It is not required to pass any parameter to main
 *		   Can use it if needed.
 *
 * Return status
 *	0			- Successful execution until QUIT command from client 
 *	ER_ACCEPT_FAILED	- Accepting client connection request failed
 *	N			- Failed stauts, value of N depends on the command processed
 */

int main(int argc, char *argv[])
{
	/* List of local varibale */

	int msgSize;        /* Size of msg received in octets (bytes) */
	int listenSocket;   /* listening server ftp socket for client connect request */
	int ccSocket;        /* Control connection socket - to be used in all client communication */
    int dcSocket;
	int status;
	
	char* userList[7] = {"Jim", "Joe", "Mary", "John", "Tom", "Tim"};
	char* passList[7] = {"1234", "abcd", "23mn", "4de2", "4rp3", "0987"};
	int signedIn = 0;
	int userMatchIndex = -1;

	/*
	 * NOTE: without \n at the end of format string in printf,
         * UNIX will buffer (not flush)
	 * output to display and you will not see it on monitor.
	*/
	printf("Started execution of server ftp\n");


	 /*initialize server ftp*/
	printf("Initialize ftp server\n");	/* changed text */

	status=svcInitServer(&listenSocket);
	if(status != 0)
	{
		printf("Exiting server ftp due to svcInitServer returned error\n");
		exit(status);
	}


	printf("ftp server is waiting to accept connection\n");

	/* wait until connection request comes from client ftp */
	ccSocket = accept(listenSocket, NULL, NULL);

	printf("Came out of accept() function \n");

	if(ccSocket < 0)
	{
		perror("cannot accept connection:");
		printf("Server ftp is terminating after closing listen socket.\n");
		close(listenSocket);  /* close listen socket */
		return (ER_ACCEPT_FAILED);  // error exist
	}

	printf("Connected to client, calling receiveMsg to get ftp cmd from client \n");


	/* Receive and process ftp commands from client until quit command.
 	 * On receiving quit command, send reply to client and 
         * then close the control connection socket "ccSocket". 
	 */
	do
	{
	    /* Receive client ftp commands until */
 	    status=receiveMessage(ccSocket, userCmd, sizeof(userCmd), &msgSize);
	    if(status < 0)
	    {
		printf("Receive message failed. Closing control connection \n");
		printf("Server ftp is terminating.\n");
		break;
	    }
        
	    char tempCmd[1024];
	    strcpy(tempCmd, userCmd);
		
	    char* cmdpointer = strtok(tempCmd," ");
	    char* argpointer = strtok(NULL," ");
		
	    strcpy(cmd, cmdpointer);
	    strcpy(argument, argpointer);

	    /*
 	     * ftp server sends only one reply message to the client for 
	     * each command received in this implementation.
	     */
	    
	    if(strcmp(cmd,"user") == 0)
        {
		
	    	for(int i = 0; i < 7; i++)
            {
	    	    if(strcmp(argument, userList[i]) == 0){
		        userMatchIndex = i;
			break;
		    }
		}
		if(signedIn == 0)
        {
		    strcpy(replyMsg, "331 User name okay, need password.");
		    signedIn = 1;
		}
		else
        {
		    strcpy(replyMsg, "530 Not logged in. \n");
		}
	    }

	    else if(strcmp(cmd,"pass") == 0)
        {
	        if(signedIn == 0)
            {
		        strcpy(replyMsg, "530 Not logged in. \n");
		        break;
		    }
		    else if(strcmp(passList[userMatchIndex], argument) == 0)
            {
		        strcpy(replyMsg, "User logged in, proceed.");
		        signedIn = 1;
		    }
	    }
		    
	    else if(strcmp(cmd,"mkdir") == 0)
        {
		    int status;
		    if(signedIn == 0)
            {
		        strcpy(replyMsg, "530 Not logged in. \n");
		        break;
		    }
            status = system(userCmd);
		    if(status == 0)
            {
	   	        strcpy(replyMsg, "200 command ok \n");
		    }
		    else
            {
		        strcpy(replyMsg, "500 command failed \n");
		    }
	    }
	    
	    else if(strcmp(cmd,"rmdir") == 0)
        {
		    int status;
		    if(signedIn == 0)
            {
		        strcpy(replyMsg, "530 Not logged in. \n");
		        break;
		    }
           	status = system(userCmd);
		
		    if(status == 0)
            {
	   	        strcpy(replyMsg, "200 command ok \n");
		    }
		    else
            {
		        strcpy(replyMsg, "500 command failed \n");
	        }
	    }

	    else if(strcmp(cmd, "cd") == 0)
        {
		    int status;
		    if(signedIn == 0)
            {
		        strcpy(replyMsg, "530 Not logged in. \n");
		        break;
		    }
		    status = chdir(argument);
		    printf("%d",status);
		    if(status == 0)
            {
	   	        strcpy(replyMsg, "200 command ok \n");
		    }
		    else
            {
		        strcpy(replyMsg, "500 command failed \n");
	        }
	    }   
	
	    else if(strcmp(cmd, "rm") == 0)
        {
		    int status;
		    if(signedIn == 0)
            {
		        strcpy(replyMsg, "530 Not logged in. \n");
		        break;
		    }
		    status = system(userCmd);
		    if(status == 0)
            {
	   	        strcpy(replyMsg, "200 command ok \n");
		    }
		    else
            {
		        strcpy(replyMsg, "500 command failed \n");
	        }
	    }
		
	    else if(strcmp(cmd, "pwd") == 0)
        {
		    int status;
		    if(signedIn == 0)
            {
		        strcpy(replyMsg, "530 Not logged in. \n");
		        break;
	        }
		    status = system("pwd > output");
		    FILE* fptr = fopen("output", "r");
		    if(fptr != NULL)
            {
		        char temp[1024];
		        int status = fread(temp, 1, 1024, fptr);
		        if(status < 0)
                {
		            strcpy(replyMsg, "450 Requested file action not taken. \n");
		        }
		        else
                {
		            temp[status] = NULL;
		            strcpy(replyMsg, "255 Requester file action okay, completed. \n");
		            strcat(replyMsg, temp);
		        }
		    }
		    else
            {
		        strcpy(replyMsg, "450 Requested file action not taken. \n");
		    }
	    }

	    else if(strcmp(cmd, "ls") == 0)
        {
		    int status;
	        if(signedIn == 0)
            {
		        strcpy(replyMsg, "530 Not logged in. \n");
		        break;
	     	}
		    status = system("ls > output");
		    FILE* fptr = fopen("output", "r");
		    if(fptr != NULL)
            {
		        char temp[1024];
		        int status = fread(temp, 1, 1024, fptr);
		        if(status < 0)
                {
		            strcpy(replyMsg, "450 Requested file action not taken. \n");
		        }
		        else
                {
		            temp[status] = NULL;
		            strcpy(replyMsg, "255 Requested file action okay, completed. \n");
		            strcat(replyMsg, temp);
		        }
		    }
		    else
            {
		        strcpy(replyMsg, "450 Requested file action not taken. \n");
		    }
	    }
		
	    else if(strcmp(cmd, "help") == 0)
        {
		    if(signedIn == 0)
            {
		        strcpy(replyMsg, "530 Not logged in. \n");
		        break;
		    }
		    strcpy(replyMsg, "214 Help message. \n");
	    	strcpy(replyMsg, "Implemented ftp Commands: user, pass, mkdir, rmdir, cd, rm, pwd, ls, stat, help.\n");
	    }
        
        else if(strcmp(cmd, "send") == 0)
        {
	    	status = clntConnect("192.168.200.230", &dcSocket);
		    FILE* fptr = fopen("argument", "w");
		    int bytesReceived;
	        int sizeOfBuf;
		    char buf[100];
		    do
            {
		        status = receiveMessage(dcSocket, buf, sizeof(buf), &bytesReceived);
		        status = fwrite(buf, 1, bytesReceived, fptr);
		    }while(bytesReceived > 0);
		    fclose(fptr);
		    close(dcSocket);
		    strcpy(replyMsg, "200 File transfer successful.");
	    }
	
	    else if(strcmp(cmd, "get") == 0)
        {
	        status = clntConnect("192.168.200.230", &dcSocket);
		    FILE* fptr = fopen("argument", "r");
	   	    do
            {
		        char buf[100];
		        status = fread(buf, 1, sizeof(buf), fptr);
		        status = sendMessage(dcSocket, buf, sizeof(buf));
		    }while(!feof(fptr));
		    fclose(fptr);
		    close(dcSocket);
		    strcpy(replyMsg, "200 File transfer successful.");
	    }
		
	    
	    else
        {
		    if(signedIn == 0)
            {
		        strcpy(replyMsg, "530 Not logged in. \n");
		        break;
		    }
	    	strcpy(replyMsg, "200 command ok \n");
	    }
	    
	    
	    
	    status=sendMessage(ccSocket,replyMsg,strlen(replyMsg) + 1);	/* Added 1 to include NULL character in */
				/* the reply string strlen does not count NULL character */
	    if(status < 0)
	    {
		    break;  /* exit while loop */
	    }

	}while(strcmp(cmd, "quit") != 0);

	printf("Closing control connection socket.\n");
	close (ccSocket);  /* Close client control connection socket */

	printf("Closing listen socket.\n");
	close(listenSocket);  /*close listen socket */

	printf("Existing from server ftp main \n");

	return (status);
}


/*
 * svcInitServer
 *
 * Function to create a socket and to listen for connection request from client
 *    using the created listen socket.
 *
 * Parameters
 * s		- Socket to listen for connection request (output)
 *
 * Return status
 *	OK			- Successfully created listen socket and listening
 *	ER_CREATE_SOCKET_FAILED	- socket creation failed
 */

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
	svcAddr.sin_port=htons(SERVER_FTP_PORT);    /* server listen port # */

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


/*
 * sendMessage
 *
 * Function to send specified number of octet (bytes) to client ftp
 *
 * Parameters
 * s		- Socket to be used to send msg to client (input)
 * msg  	- Pointer to character array containing msg to be sent (input)
 * msgSize	- Number of bytes, including NULL, in the msg to be sent to client (input)
 *
 * Return status
 *	OK		- Msg successfully sent
 *	ER_SEND_FAILED	- Sending msg failed
 */

int sendMessage(
	int    s,	/* socket to be used to send msg to client */
	char   *msg, 	/* buffer having the message data */
	int    msgSize 	/* size of the message/data in bytes */
	)
{
	int i;


	/* Print the message to be sent byte by byte as character */
	for(i=0; i<msgSize; i++)
	{
		printf("%c",msg[i]);
	}
	printf("\n");

	if((send(s, msg, msgSize, 0)) < 0) /* socket interface call to transmit */
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
 *	R_RECEIVE_FAILED	- Receiving msg failed
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
	serverAddress.sin_port = htons(DATA_CONNECTION_PORT);

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


