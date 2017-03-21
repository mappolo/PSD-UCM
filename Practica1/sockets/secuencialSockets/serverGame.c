#include "serverGame.h"

void sendMessageToPlayer (int socketClient, char* message){
	int msgLength;
	unsigned int tam;
	tam = strlen(message);
	msgLength = send(socketClient, &tam, sizeof(tam), 0);
	if (msgLength < 0)
		showError("ERROR sendMessageToPlayer tam");


	msgLength = send(socketClient, message, tam, 0);
	if (msgLength < 0)
		showError("ERROR sendMessageToPlayer message");
}

void receiveMessageFromPlayer (int socketClient, char* message){
	int msgLength;
	unsigned int tam;
	msgLength = recv(socketClient, &tam, sizeof(tam), 0);
	if (msgLength < 0)
		showError("receiveMessageFromPlayer tam");
	
	msgLength = recv(socketClient, message, tam, 0);
	if (msgLength < 0)
		showError("ERROR receiveMessageFromPlayer message");
}

void sendCodeToClient (int socketClient, unsigned int code){
	int msgLength;
	unsigned int codigo;
	codigo = code;
	msgLength = send(socketClient, &codigo, sizeof(code), 0);
	if (msgLength < 0)
		showError("ERROR sendCodeToClient");

}

void sendBoardToClient (int socketClient, tBoard board){
	int msgLength;
	msgLength = send(socketClient, board, sizeof(tBoard), 0);
	if (msgLength < 0)
		showError("ERROR sendBoardToClient");

}

unsigned int receiveMoveFromPlayer (int socketClient){
	int msgLength;
	unsigned int msg;
	msgLength = recv(socketClient, &msg, sizeof(unsigned int), 0);
	if (msgLength < 0)
		showError("ERROR receiveCode");

	return msg;

}

int getSocketPlayer (tPlayer player, int player1socket, int player2socket){

	int socket;

		if (player == player1)
			socket = player1socket;
		else
			socket = player2socket;

	return socket;
}

tPlayer switchPlayer (tPlayer currentPlayer){

	tPlayer nextPlayer;

		if (currentPlayer == player1)
			nextPlayer = player2;
		else
			nextPlayer = player1;

	return nextPlayer;
}



int main(int argc, char *argv[]){

	int socketfd;						/** Socket descriptor */
	struct sockaddr_in serverAddress;	/** Server address structure */
	unsigned int port;					/** Listening port */
	struct sockaddr_in player1Address;	/** Client address structure for player 1 */
	struct sockaddr_in player2Address;	/** Client address structure for player 2 */
	int socketPlayer1, socketPlayer2;	/** Socket descriptor for each player */
	unsigned int clientLength;			/** Length of client structure */

	tBoard board;						/** Board of the game */
	tPlayer currentPlayer;				/** Current player */
	tMove moveResult;					/** Result of player's move */
	tString player1Name;				/** Name of player 1 */
	tString player2Name;				/** Name of player 2 */
	int endOfGame;						/** Flag to control the end of the game*/
	unsigned int column;				/** Selected column to insert the chip */
	tString message;					/** Message sent to the players */



		// Check arguments
		if (argc != 2) {
			fprintf(stderr,"ERROR wrong number of arguments\n");
			fprintf(stderr,"Usage:\n$>%s port\n", argv[0]);
			exit(1);
		}

		// Init seed
		srand(time(NULL));

		// Create the socket
		 socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		// Check
		if (socketfd < 0)
			showError("ERROR while opening socket");

		// Init server structure
		memset(&serverAddress, 0, sizeof(serverAddress));

		// Get listening port
		port = atoi(argv[1]);

		// Fill server structure
		 serverAddress.sin_family = AF_INET;
		 serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
		 serverAddress.sin_port = htons(port);

		// Bind
		 if (bind(socketfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
			 showError("ERROR while binding");

		// Listen
		listen(socketfd, 2);

		// Get length of client structure
		clientLength = sizeof(player1Address);

		// Accept player 1 connection!!!
		socketPlayer1 = accept(socketfd, (struct sockaddr *) &player1Address, &clientLength);

		// Check accept result
		if (socketPlayer1 < 0)
			showError("ERROR while accepting connection for player 1");

		printf ("Player 1 is connected!\n");

		// Accept player 2 connection!!!
		socketPlayer2 = accept(socketfd, (struct sockaddr *) &player2Address, &clientLength);

		// Check accept result
		if (socketPlayer2 < 0)
			showError("ERROR while accepting connection for player 1");

		printf ("Player 2 is connected!\n");

		// Receive player 1 info
		memset(player1Name, 0, STRING_LENGTH);
		receiveMessageFromPlayer (socketPlayer1, player1Name);	
		
		printf ("Name of player 1 received: %s\n", player1Name);

		// Receive player 2 info
		memset(player2Name, 0, STRING_LENGTH);
		receiveMessageFromPlayer (socketPlayer2, player2Name);

		printf ("Name of player 2 received: %s\n", player2Name);

		// Send rival name to player 1
		sendMessageToPlayer (socketPlayer1, player2Name);


		// Send rival name to player 2
		sendMessageToPlayer (socketPlayer2, player1Name);
		
		printf ("Init board\n");
		// Init board
		initBoard (board);
		endOfGame = FALSE;


		// Calculates who is moving first and send the corresponding code
	    if ((rand() % 2) == 0){
	    	currentPlayer = player1;
		printf("%s empieza\n", player1Name);	
	
		memset (message, 0, STRING_LENGTH);
		strcpy (message, "Its your turn. You play with:o");
		sendCodeToClient (socketPlayer1, TURN_MOVE);
		sendMessageToPlayer (socketPlayer1, message);
		sendBoardToClient (socketPlayer1, board);
		
		memset (message, 0, STRING_LENGTH);
		strcpy (message, "Your rival is thinking... please, wait! You play with:x");
		sendCodeToClient (socketPlayer2, TURN_WAIT);
		sendMessageToPlayer (socketPlayer2, message);
		sendBoardToClient (socketPlayer2, board);	

		column = receiveMoveFromPlayer (socketPlayer1);
	    }
	    else{
	    	currentPlayer = player2;
		printf("%s empieza\n", player2Name);	

		memset (message, 0, STRING_LENGTH);
		strcpy (message, "Its your turn. You play with:o");
		sendCodeToClient (socketPlayer2, TURN_MOVE);
		sendMessageToPlayer (socketPlayer2, message);
		sendBoardToClient (socketPlayer2, board);

		memset (message, 0, STRING_LENGTH);
		strcpy (message, "Your rival is thinking... please, wait! You play with:x");
		sendCodeToClient (socketPlayer1, TURN_WAIT);
		sendMessageToPlayer (socketPlayer1, message);
		sendBoardToClient (socketPlayer1, board);

		column = receiveMoveFromPlayer (socketPlayer2);
	    }

	// While game continues...
	while (endOfGame == FALSE){
		if (isBoardFull (board) == TRUE){
			endOfGame = TRUE;

			memset (message, 0, STRING_LENGTH);
			strcpy (message, "DRAW!!!!");
			sendCodeToClient (socketPlayer1, GAMEOVER_DRAW);
			sendMessageToPlayer (socketPlayer1, message);
			sendBoardToClient (socketPlayer1, board);

			memset (message, 0, STRING_LENGTH);
			strcpy (message, "DRAW!!!!");
			sendCodeToClient (socketPlayer2, GAMEOVER_DRAW);
			sendMessageToPlayer (socketPlayer2, message);
			sendBoardToClient (socketPlayer2, board);

		}
		else{
			moveResult = checkMove (board, column);
			if (moveResult == OK_move){
				insertChip (board, currentPlayer, column);
				if (checkWinner (board, currentPlayer) == TRUE){
					if (currentPlayer == player1){
						endOfGame = TRUE;						

						memset (message, 0, STRING_LENGTH);
						strcpy (message, "YOU WON!!!!");
						sendCodeToClient (socketPlayer1, GAMEOVER_WIN);
						sendMessageToPlayer (socketPlayer1, message);
						sendBoardToClient (socketPlayer1, board);
		
						memset (message, 0, STRING_LENGTH);
						strcpy (message, "YOU LOSE!!!!");
						sendCodeToClient (socketPlayer2, GAMEOVER_LOSE);
						sendMessageToPlayer (socketPlayer2, message);
						sendBoardToClient (socketPlayer2, board);

					}	
					else{
						endOfGame = TRUE;		
						
						memset (message, 0, STRING_LENGTH);
						strcpy (message, "YOU WON!!!!");
						sendCodeToClient (socketPlayer2, GAMEOVER_WIN);
						sendMessageToPlayer (socketPlayer2, message);
						sendBoardToClient (socketPlayer2, board);

						memset (message, 0, STRING_LENGTH);
						strcpy (message, "YOU LOSE!!!!");
						sendCodeToClient (socketPlayer1, GAMEOVER_LOSE);
						sendMessageToPlayer (socketPlayer1, message);
						sendBoardToClient (socketPlayer1, board);

					}

				}
				else{
					currentPlayer = switchPlayer (currentPlayer);
					if (currentPlayer == player1){
						memset (message, 0, STRING_LENGTH);
						strcpy (message, "Its your turn.");
						sendCodeToClient (socketPlayer1, TURN_MOVE);
						sendMessageToPlayer (socketPlayer1, message);
						sendBoardToClient (socketPlayer1, board);
		
						memset (message, 0, STRING_LENGTH);
						strcpy (message, "Your rival is thinking... please, wait!!");
						sendCodeToClient (socketPlayer2, TURN_WAIT);
						sendMessageToPlayer (socketPlayer2, message);
						sendBoardToClient (socketPlayer2, board);
					}	
					else{
						memset (message, 0, STRING_LENGTH);
						strcpy (message, "Its your turn.");
						sendCodeToClient (socketPlayer2, TURN_MOVE);
						sendMessageToPlayer (socketPlayer2, message);
						sendBoardToClient (socketPlayer2, board);

						memset (message, 0, STRING_LENGTH);
						strcpy (message, "Your rival is thinking... please, wait!!");
						sendCodeToClient (socketPlayer1, TURN_WAIT);
						sendMessageToPlayer (socketPlayer1, message);
						sendBoardToClient (socketPlayer1, board);
					}
				}
		
			}
			else{
				if (currentPlayer == player1){
					memset (message, 0, STRING_LENGTH);
					strcpy (message, "Full column...try again");
					sendCodeToClient (socketPlayer1, TURN_MOVE);
					sendMessageToPlayer (socketPlayer1, message);
					sendBoardToClient (socketPlayer1, board);
		
					memset (message, 0, STRING_LENGTH);
					strcpy (message, "Rival movement invalid...please wait");
					sendCodeToClient (socketPlayer2, TURN_WAIT);
					sendMessageToPlayer (socketPlayer2, message);
					sendBoardToClient (socketPlayer2, board);
				}	
				else{
					memset (message, 0, STRING_LENGTH);
					strcpy (message, "Full column...try again");
					sendCodeToClient (socketPlayer2, TURN_MOVE);
					sendMessageToPlayer (socketPlayer2, message);
					sendBoardToClient (socketPlayer2, board);

					memset (message, 0, STRING_LENGTH);
					strcpy (message, "Rival movement invalid...please wait");
					sendCodeToClient (socketPlayer1, TURN_WAIT);
					sendMessageToPlayer (socketPlayer1, message);
					sendBoardToClient (socketPlayer1, board);
				}
			}
		}
		if (endOfGame == FALSE){
			if (currentPlayer == player1){
				column = receiveMoveFromPlayer (socketPlayer1);
			}
			else{
				column = receiveMoveFromPlayer (socketPlayer2);

			}
		}
	}

	
	printf("Partida finalizada!!!\n");	

	close(socketfd);
	close(socketPlayer1);
	close(socketPlayer2);
	
	return 0;
}
