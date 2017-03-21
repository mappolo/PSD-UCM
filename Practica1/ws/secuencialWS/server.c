#include "server.h"

/** Flag to enable debugging */
#define DEBUG_SERVER 1


/** Array with games */
tGame games[MAX_GAMES];


void initServerStructures (){

	int i;

	if (DEBUG_SERVER)
		printf ("Initializing...\n");

	// Init seed
	srand (time(NULL));

	// Init each game
	for (i=0; i<MAX_GAMES; i++){

		// Allocate and init board
		games[i].board = (xsd__string) malloc (BOARD_WIDTH*BOARD_HEIGHT);
		initBoard (games[i].board);

		// Calculate the first player to play
		if ((rand()%2)==0)
			games[i].currentPlayer = player1;
		else
			games[i].currentPlayer = player2;

		// Allocate and init player names
		games[i].player1Name = (xsd__string) malloc (STRING_LENGTH);
		games[i].player2Name = (xsd__string) malloc (STRING_LENGTH);
		memset (games[i].player1Name, 0, STRING_LENGTH);
		memset (games[i].player2Name, 0, STRING_LENGTH);

		// Game status
		games[i].endOfGame = FALSE;
		games[i].status = gameEmpty;
	}
}

conecta4ns__tPlayer switchPlayer (conecta4ns__tPlayer currentPlayer){

	conecta4ns__tPlayer nextPlayer;

		if (currentPlayer == player1)
			nextPlayer = player2;
		else
			nextPlayer = player1;

	return nextPlayer;
}

int searchEmptyGame (){

	int i, found, result;

		// Init...
		found = FALSE;
		i = 0;
		result = -1;

		// Search for a non-active game
		while ((!found) && (i<MAX_GAMES)){

			if ((games[i].status == gameEmpty) ||
				(games[i].status == gameWaitingPlayer)){
				found = TRUE;
			}
			else
				i++;
		}

		if (found){
			result = i;
		}
		else{
			result = ERROR_SERVER_FULL;
		}

	return result;
}

int locateGameForPlayer (xsd__string name){
	
	int i, found, result;

		// Init...
		found = FALSE;
		i = 0;
		result = -1;

		// Search for a non-active game
		while ((!found) && (i<MAX_GAMES)){

			if ((strcmp(games[i].player1Name,name) == 0) || (strcmp(games[i].player2Name,name) == 0)){
				found = TRUE;
			}
			else
				i++;
		}

		if (found){
			result = i;
		}
		else{
			result = ERROR_PLAYER_NOT_FOUND;
		}

	return result;
}

void freeGameByIndex (int index){

	// Allocate and init board
	games[index].board = (xsd__string) malloc (BOARD_WIDTH*BOARD_HEIGHT);
	initBoard (games[index].board);

	// Calculate the first player to play
	if ((rand()%2)==0)
		games[index].currentPlayer = player1;
	else
		games[index].currentPlayer = player2;

	// Allocate and init player names
	games[index].player1Name = (xsd__string) malloc (STRING_LENGTH);
	games[index].player2Name = (xsd__string) malloc (STRING_LENGTH);
	memset (games[index].player1Name, 0, STRING_LENGTH);
	memset (games[index].player2Name, 0, STRING_LENGTH);

	// Game status
	games[index].endOfGame = FALSE;
	games[index].status = gameEmpty;
}


int conecta4ns__register (struct soap *soap, conecta4ns__tMessage playerName, int *code){
	static int  result;
	int gameIndex;

	playerName.msg[playerName.__size] = 0;
	// Init...
	gameIndex = -1;
	
	// Search fon an empty game
	//
	gameIndex = searchEmptyGame ();
	if (gameIndex == ERROR_SERVER_FULL){
		*code = ERROR_SERVER_FULL;
	}
	else{	
		if (locateGameForPlayer (playerName.msg) == ERROR_PLAYER_NOT_FOUND){
			if (games[gameIndex].status == gameEmpty){
				strcpy (games[gameIndex].player1Name, playerName.msg);
				games[gameIndex].status = gameWaitingPlayer;
			}
			else if (games[gameIndex].status == gameWaitingPlayer){
				strcpy (games[gameIndex].player2Name, playerName.msg);
				games[gameIndex].status = gameReady;
			}
			*code = OK_NAME_REGISTERED;
			
		}
		else{
			*code = ERROR_NAME_REPEATED;
		}
	}
	

  return SOAP_OK;
}

int conecta4ns__getStatus (struct soap *soap, conecta4ns__tMessage playerName, conecta4ns__tBlock* status){

	int gameIndex;
	char messageToPlayer [STRING_LENGTH];
		
		// Set \0 at the end of the string
		playerName.msg[playerName.__size] = 0;

		// Allocate memory for the status structure
		(status->msgStruct).msg = (xsd__string) malloc (STRING_LENGTH);
		memset ((status->msgStruct).msg, 0, STRING_LENGTH);
		status->board = (xsd__string) malloc (BOARD_WIDTH*BOARD_HEIGHT);
		memset (status->board, 0, BOARD_WIDTH*BOARD_HEIGHT);
		status->__size = BOARD_WIDTH*BOARD_HEIGHT;


		if (DEBUG_SERVER)
			printf ("Receiving getStatus() request from -> %s [%d]\n", playerName.msg, playerName.__size);
		
		gameIndex = locateGameForPlayer (playerName.msg);
		if(gameIndex == ERROR_PLAYER_NOT_FOUND){
			status->code = ERROR_PLAYER_NOT_FOUND;
			strcpy (status->msgStruct.msg, "ERROR_PLAYER_NOT_FOUND");
			status->msgStruct.__size = strlen (status->msgStruct.msg);
			printf("ERROR_PLAYER_NOT_FOUND\n");

		}
		else{
			if (isBoardFull(games[gameIndex].board) == TRUE){
					status->code = GAMEOVER_DRAW;
					strcpy (status->msgStruct.msg, "DRAW!!!!");
					status->msgStruct.__size = strlen (status->msgStruct.msg);
					freeGameByIndex(gameIndex);
			}
			else{
				if (checkWinner (games[gameIndex].board, games[gameIndex].currentPlayer) == TRUE){
					if (games[gameIndex].currentPlayer == player1){
						if ((strcmp(games[gameIndex].player1Name,playerName.msg) == 0)){
							status->code = GAMEOVER_WIN;
							strcpy (status->msgStruct.msg, "YOU WON!!!!");
							status->msgStruct.__size = strlen (status->msgStruct.msg);
						}
						else{
							status->code = GAMEOVER_LOSE;
							strcpy (status->msgStruct.msg, "YOU LOSE!!!!");
							status->msgStruct.__size = strlen (status->msgStruct.msg);
						}
					}
					else{
						if ((strcmp(games[gameIndex].player2Name,playerName.msg) == 0)){
							status->code = GAMEOVER_WIN;
							strcpy (status->msgStruct.msg, "YOU WON!!!!");
							status->msgStruct.__size = strlen (status->msgStruct.msg);
						}
						else{
							status->code = GAMEOVER_LOSE;
							strcpy (status->msgStruct.msg, "YOU LOSE!!!!");
							status->msgStruct.__size = strlen (status->msgStruct.msg);
						}
					}
					strcpy (status->board, games[gameIndex].board);	
					status->__size = BOARD_WIDTH*BOARD_HEIGHT;
					freeGameByIndex (gameIndex);
				}
				else if(games[gameIndex].status == gameWaitingPlayer){
					status->code = TURN_WAIT;
					strcpy (status->msgStruct.msg, "Waiting for other player...");
					status->msgStruct.__size = strlen (status->msgStruct.msg);
					strcpy (status->board, games[gameIndex].board);	
					status->__size = BOARD_WIDTH*BOARD_HEIGHT;
				}
				else{
					if (games[gameIndex].currentPlayer == player1){
						if ((strcmp(games[gameIndex].player1Name,playerName.msg) == 0)){
							status->code = TURN_MOVE;
							strcpy (status->msgStruct.msg, "Its your turn. You play with:o");
							status->msgStruct.__size = strlen (status->msgStruct.msg);
						}
						else{
							status->code = TURN_WAIT;
							strcpy (status->msgStruct.msg, "Your rival is thinking... please, wait! You play with:x");
							status->msgStruct.__size = strlen (status->msgStruct.msg);
						}
					}
					else{
						if ((strcmp(games[gameIndex].player2Name,playerName.msg) == 0)){
							status->code = TURN_MOVE;
							strcpy (status->msgStruct.msg, "Its your turn. You play with:x");
							status->msgStruct.__size = strlen (status->msgStruct.msg);
						}
						else{
							status->code = TURN_WAIT;
							strcpy (status->msgStruct.msg, "Your rival is thinking... please, wait! You play with:o");
							status->msgStruct.__size = strlen (status->msgStruct.msg);
						}
					}
					strcpy (status->board, games[gameIndex].board);	
					status->__size = BOARD_WIDTH*BOARD_HEIGHT;
				}	
			}		
		}
		

	return SOAP_OK;
}

int conecta4ns__insertChip (struct soap *soap, conecta4ns__tMessage playerName, int playerMove, conecta4ns__tBlock* status){

	int gameIndex;
	conecta4ns__tMove moveResult;


		// Set \0 at the end of the string
		playerName.msg[playerName.__size] = 0;

		// Allocate memory for the status structure
		(status->msgStruct).msg = (xsd__string) malloc (STRING_LENGTH);
		memset ((status->msgStruct).msg, 0, STRING_LENGTH);
		status->board = (xsd__string) malloc (BOARD_WIDTH*BOARD_HEIGHT);
		memset (status->board, 0, BOARD_WIDTH*BOARD_HEIGHT);


		gameIndex = locateGameForPlayer (playerName.msg);
		
		moveResult = checkMove (games[gameIndex].board, playerMove);
		if (moveResult == OK_move){
			insertChip (games[gameIndex].board, games[gameIndex].currentPlayer, playerMove);
		}
		if (isBoardFull(games[gameIndex].board) == TRUE){
			status->code = GAMEOVER_DRAW;
			strcpy (status->msgStruct.msg, "DRAW!!!!");
			status->msgStruct.__size = strlen (status->msgStruct.msg);
		}
		else{
			if (checkWinner (games[gameIndex].board, games[gameIndex].currentPlayer) == TRUE){
				status->code = GAMEOVER_WIN;
				strcpy (status->msgStruct.msg, "YOU WON!!!!");
				status->msgStruct.__size = strlen (status->msgStruct.msg);
			}
			else{
				games[gameIndex].currentPlayer = switchPlayer (games[gameIndex].currentPlayer);
			
				if (games[gameIndex].currentPlayer == player1){
					if ((strcmp(games[gameIndex].player1Name,playerName.msg) == 0)){
						status->code = TURN_MOVE;
						strcpy (status->msgStruct.msg, "Its your turn.");
						status->msgStruct.__size = strlen (status->msgStruct.msg);
					}
					else{
						status->code = TURN_WAIT;
						strcpy (status->msgStruct.msg, "Your rival is thinking... please, wait!");
						status->msgStruct.__size = strlen (status->msgStruct.msg);
					}
				}
				else{
					if ((strcmp(games[gameIndex].player2Name,playerName.msg) == 0)){
						status->code = TURN_MOVE;
						strcpy (status->msgStruct.msg, "Its your turn.");
						status->msgStruct.__size = strlen (status->msgStruct.msg);
					}
					else{
						status->code = TURN_WAIT;
						strcpy (status->msgStruct.msg, "Your rival is thinking... please, wait!");
						status->msgStruct.__size = strlen (status->msgStruct.msg);
					}
				}
			}
		}		
		strcpy (status->board, games[gameIndex].board);	
		status->__size = BOARD_WIDTH*BOARD_HEIGHT;
		

	return SOAP_OK;
}




int main(int argc, char **argv){ 

  int primarySocket, secondarySocket;
  struct soap soap;

		if (argc < 2) {
			printf("Usage: %s <port>\n",argv[0]);
			exit(-1);
		}

		// Init structures
		initServerStructures();

		// Init environment
		//
		soap_init(&soap);

		// Bind to the specified port
		//
		primarySocket = soap_bind(&soap, NULL, atoi(argv[1]), 100);

		// Check result of binding
		//
		if (primarySocket < 0) {
  			soap_print_fault(&soap, stderr); 
			exit(-1); 
		}

		printf ("Server is ON! waiting for requests...\n");

		// Listen to next connection
		while (TRUE) {
			// accept
		  	secondarySocket = soap_accept(&soap);    

		  	if (secondarySocket < 0) {
				soap_print_fault(&soap, stderr); exit(-1);
		  	}

			// Execute invoked operation
		  	soap_serve(&soap);

			// Clean up!
		  	soap_end(&soap);
			
		}

  return 0;
}
