#include "server.h"

/** Flag to enable debugging */
#define DEBUG_SERVER 1


/** Array with games */
tGame games[MAX_GAMES];


void initServerStructures (){

	int i;

		if (DEBUG_SERVER)
			printf ("\nServer is on!!!Initializing...\n");

		// Init seed
		srand (time(NULL));

		// Init each game
		for (i=0; i<MAX_GAMES; i++){

			initBoard (games[i].board);

			if ((rand()%2)==0)
				games[i].currentPlayer = player1;
			else
				games[i].currentPlayer = player2;

			memset (games[i].player1Name, 0, STRING_LENGTH);
			memset (games[i].player2Name, 0, STRING_LENGTH);
			games[i].endOfGame = FALSE;
			games[i].status = gameEmpty;
		}
}


tPlayer switchPlayer (tPlayer currentPlayer){

	tPlayer nextPlayer;

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


int locateGameForPlayer (tString name){
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


void initGameByIndex (int index){

	initBoard (games[index].board);

	if ((rand()%2)==0)
		games[index].currentPlayer = player1;
	else
		games[index].currentPlayer = player2;

	memset (games[index].player1Name, 0, STRING_LENGTH);
	memset (games[index].player2Name, 0, STRING_LENGTH);
	games[index].endOfGame = FALSE;
	games[index].status = gameEmpty;

}




int *registerplayer_1_svc(tMessage *argMsg, struct svc_req *rqstp){

	static int  result;
	int gameIndex;

		// Init...
		gameIndex = -1;

		// Search fon an empty game
		//
		gameIndex = searchEmptyGame ();
		if (gameIndex == ERROR_SERVER_FULL){
			result = ERROR_SERVER_FULL;
		}
		else{	
			if (locateGameForPlayer (argMsg->msg) == ERROR_PLAYER_NOT_FOUND){
				if (games[gameIndex].status == gameEmpty){
					strcpy (games[gameIndex].player1Name, argMsg->msg);
					games[gameIndex].status = gameWaitingPlayer;
				}
				else if (games[gameIndex].status == gameWaitingPlayer){
					strcpy (games[gameIndex].player2Name, argMsg->msg);
					games[gameIndex].status = gameReady;
				}
				result = OK_NAME_REGISTERED;
				
			}
			else{
				result = ERROR_NAME_REPEATED;
			}
		}

	return &result;
}


tBlock *getgamestatus_1_svc(tMessage *argMsg, struct svc_req *rqstp){

	tBlock *response;
	int gameIndex;

		// Init...
		response = (tBlock*) malloc (sizeof (tBlock));

		// Locate the game
		//
		gameIndex = locateGameForPlayer (argMsg->msg);
		if(gameIndex == ERROR_PLAYER_NOT_FOUND){
			response->code = ERROR_PLAYER_NOT_FOUND;
			strcpy (response->msg, "ERROR_PLAYER_NOT_FOUND");
			printf("ERROR_PLAYER_NOT_FOUND\n");

		}
		else{
			if (isBoardFull(games[gameIndex].board) == TRUE){
					response->code = GAMEOVER_DRAW;
					strcpy (response->msg, "DRAW!!!!");
					initGameByIndex (gameIndex);
			}
			else{
				if (checkWinner (games[gameIndex].board, games[gameIndex].currentPlayer) == TRUE){
					if (games[gameIndex].currentPlayer == player1){
						if ((strcmp(games[gameIndex].player1Name,argMsg->msg) == 0)){
							response->code = GAMEOVER_WIN;
							strcpy (response->msg, "YOU WON!!!!");
						}
						else{
							response->code = GAMEOVER_LOSE;
							strcpy (response->msg, "YOU LOSE!!!!");
						}
					}
					else{
						if ((strcmp(games[gameIndex].player2Name,argMsg->msg) == 0)){
							response->code = GAMEOVER_WIN;
							strcpy (response->msg, "YOU WON!!!!");
						}
						else{
							response->code = GAMEOVER_LOSE;
							strcpy (response->msg, "YOU LOSE!!!!");
						}
					}
					strcpy (response->board, games[gameIndex].board);	
					initGameByIndex (gameIndex);
				}
				else if(games[gameIndex].status == gameWaitingPlayer){
					response->code = TURN_WAIT;
					strcpy (response->msg, "Waiting for other player...");
					strcpy (response->board, games[gameIndex].board);
				}
				else{
					if (games[gameIndex].currentPlayer == player1){
						if ((strcmp(games[gameIndex].player1Name,argMsg->msg) == 0)){
							response->code = TURN_MOVE;
							strcpy (response->msg, "Its your turn. You play with:o");
						}
						else{
							response->code = TURN_WAIT;
							strcpy (response->msg, "Your rival is thinking... please, wait! You play with:x");
						}
					}
					else{
						if ((strcmp(games[gameIndex].player2Name,argMsg->msg) == 0)){
							response->code = TURN_MOVE;
							strcpy (response->msg, "Its your turn. You play with:x");
						}
						else{
							response->code = TURN_WAIT;
							strcpy (response->msg, "Your rival is thinking... please, wait! You play with:o");
						}
					}
					strcpy (response->board, games[gameIndex].board);
				}	
			}		
		}
	return response;
}


tBlock * insertchipinboard_1_svc(tColumn *argCol, struct svc_req *rqstp){

	tMove moveResult;
	tBlock *response;
	int gameIndex;

		// Init...
		response = (tBlock*) malloc (sizeof (tBlock));

		// Locate the game
		gameIndex = locateGameForPlayer (argCol->player);
		
		moveResult = checkMove (games[gameIndex].board, argCol->column);
		if (moveResult == OK_move){
			insertChip (games[gameIndex].board, games[gameIndex].currentPlayer, argCol->column);
		}
		if (isBoardFull(games[gameIndex].board) == TRUE){
			response->code = GAMEOVER_DRAW;
			strcpy (response->msg, "DRAW!!!!");
		}
		else{
			if (checkWinner (games[gameIndex].board, games[gameIndex].currentPlayer) == TRUE){
				response->code = GAMEOVER_WIN;
				strcpy (response->msg, "YOU WON!!!!");
			}
			else{
				games[gameIndex].currentPlayer = switchPlayer (games[gameIndex].currentPlayer);
			
				if (games[gameIndex].currentPlayer == player1){
					if ((strcmp(games[gameIndex].player1Name,argCol->player) == 0)){
						response->code = TURN_MOVE;
						strcpy (response->msg, "Its your turn.");
					}
					else{
						response->code = TURN_WAIT;
						strcpy (response->msg, "Your rival is thinking... please, wait!");
					}
				}
				else{
					if ((strcmp(games[gameIndex].player2Name,argCol->player) == 0)){
						response->code = TURN_MOVE;
						strcpy (response->msg, "Its your turn.");
					}
					else{
						response->code = TURN_WAIT;
						strcpy (response->msg, "Your rival is thinking... please, wait!");
					}
				}
			}
		}		
		strcpy (response->board, games[gameIndex].board);

	return response;
}
