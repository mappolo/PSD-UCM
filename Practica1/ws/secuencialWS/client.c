#include "client.h"

#define DEBUG_CLIENT 1


void setTimer (){

	struct itimerval timer;

		// Set the timer
		timer.it_value.tv_sec = TIMER_SEC;
		timer.it_value.tv_usec = 0;
		timer.it_interval = timer.it_value;


		if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
			perror("Error while setting the timer\n");
			exit(1);
		}
}

void stopTimer (){

	struct itimerval timer;

		// Set the timer
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 0;
		timer.it_interval = timer.it_value;


		if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
			perror("Error while stopping the timer\n");
			exit(1);
		}

}

void *alarmHandler (void){

	//if (DEBUG_CLIENT)
		//printf ("[Client] Timer()! -> Waking up...\n");
}

unsigned int readMove (){

	xsd__string enteredMove;
	unsigned int move;
	unsigned int isRightMove;

		// Init...
		enteredMove = (xsd__string) malloc (STRING_LENGTH);
		memset (enteredMove, 0, STRING_LENGTH);
		isRightMove = FALSE;
		move = STRING_LENGTH;

		while (!isRightMove){

			printf ("Enter a move [0-6]:");

			// Read move
			fgets (enteredMove, STRING_LENGTH-1, stdin);

			// Remove new-line char
			enteredMove[strlen(enteredMove)-1] = 0;

			// Length of entered move is not correct
			if (strlen(enteredMove) != 1){
				printf ("Entered move is not correct. It must be a number between [0-6]\n");
			}

			// Check if entered move is a number
			else if (isdigit(enteredMove[0])){

				// Convert move to an int
				move =  enteredMove[0] - '0';

				if (move > 6)
					printf ("Entered move is not correct. It must be a number between [0-6]\n");
				else
					isRightMove = TRUE;
			}

			// Entered move is not a number
			else
				printf ("Entered move is not correct. It must be a number between [0-6]\n");
		}

	return move;
}


int main(int argc, char **argv){

	struct soap soap;			/** Soap struct */
	char *serverURL;			/** Server URL */
	unsigned int endOfGame;			/** Flag to control the end of the game */
	conecta4ns__tMessage playerName;	/** Player name */
	conecta4ns__tBlock gameStatus;		/** Game status */
	unsigned int playerMove;		/** Player move */
	int resCode;				/** Return code from server */


		// Init gSOAP environment
		//
		soap_init(&soap);

		// Obtain server address
		serverURL = argv[1];

		// Allocate memory for player name and init
		playerName.msg = (xsd__string) malloc (STRING_LENGTH);
		memset(playerName.msg, 0, STRING_LENGTH);

		// Allocate memory for game status and init
		gameStatus.msgStruct.msg = (xsd__string) malloc (STRING_LENGTH);
		memset(gameStatus.msgStruct.msg, 0, STRING_LENGTH);
		gameStatus.board = (xsd__string) malloc (BOARD_WIDTH*BOARD_HEIGHT);
		memset (gameStatus.board, 0, BOARD_WIDTH*BOARD_HEIGHT);
		gameStatus.__size = BOARD_WIDTH*BOARD_HEIGHT;

		// Init
		resCode = 0;
		endOfGame = FALSE;

		// Check arguments
		if (argc !=2) {
			printf("Usage: %s http://server:port\n",argv[0]);
			exit(0);
		}

		// While player is not registered in the server side
		while (resCode != OK_NAME_REGISTERED){

			// Init player's name
			do{
				memset(playerName.msg, 0, STRING_LENGTH);
				printf ("Enter player name:");
				fgets(playerName.msg, STRING_LENGTH-1, stdin);

				// Remove '\n'
				playerName.msg[strlen(playerName.msg)-1] = 0;

			}while (strlen(playerName.msg) <= 2);
				
				playerName.__size = strlen (playerName.msg);

				// Try to register the player
				soap_call_conecta4ns__register(&soap,serverURL, "", playerName, &resCode);

				// Check for errors...
				if (soap.error) {
					soap_print_fault(&soap, stderr);
					exit(1);
				}
				if (resCode == ERROR_SERVER_FULL){
				printf("Server full\n");
				}
				else if (resCode == ERROR_NAME_REPEATED){
					printf("Name repeated\n");
				}
		}

		// Player is successfully registered
		if (DEBUG_CLIENT)
			printf ("Player registered as %s ... setting up the alarm!\n", playerName.msg);


		// Set alarm...
		if (signal(SIGALRM, (void (*)(int)) alarmHandler) == SIG_ERR) {
			perror("Error while setting a handler for SIGALRM");
			exit(1);
		}

		// While game continues...
		while (!endOfGame){
			
			soap_call_conecta4ns__getStatus(&soap,serverURL, "", playerName, &gameStatus);
			gameStatus.msgStruct.msg[gameStatus.msgStruct.__size] = 0;
	
			// Other player is thinking... waiting!
			if (gameStatus.code==TURN_WAIT){
				printBoard (gameStatus.board, gameStatus.msgStruct.msg);
				// Set timer to wake up from pause...
				setTimer ();

				// Player waits...
				pause();

				// Stop timer!
				stopTimer();
			}
			else if(gameStatus.code==TURN_MOVE){
				printBoard (gameStatus.board, gameStatus.msgStruct.msg);
				playerMove = readMove ();

				soap_call_conecta4ns__insertChip(&soap,serverURL, "", playerName, playerMove, &gameStatus);
				gameStatus.msgStruct.msg[gameStatus.msgStruct.__size] = 0;
			}

			if(gameStatus.code==GAMEOVER_DRAW){
				printBoard (gameStatus.board, gameStatus.msgStruct.msg);
				endOfGame = TRUE;	
			}
			else if (gameStatus.code == GAMEOVER_WIN){
				printBoard (gameStatus.board, gameStatus.msgStruct.msg);
				endOfGame = TRUE;
			}
			else if (gameStatus.code == GAMEOVER_LOSE){
				printBoard (gameStatus.board, gameStatus.msgStruct.msg);
				endOfGame = TRUE;
			}

		}


		// Clean the environment
		//
		soap_destroy(&soap);
  		soap_end(&soap);
  		soap_done(&soap);

  return 0;
}
