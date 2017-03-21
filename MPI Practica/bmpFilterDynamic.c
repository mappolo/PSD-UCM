#include "bmpBlackWhite.h"
#include <stdio.h>
#include "mpi.h"


/** Enable output for filtering information */
#define DEBUG_FILTERING 0

/** Show information of input and output bitmap headers */
#define SHOW_BMP_HEADERS 1


int main(int argc, char** argv){

	tBitmapFileHeader imgFileHeaderInput;			/** BMP file header for input image */
	tBitmapInfoHeader imgInfoHeaderInput;			/** BMP info header for input image */
	tBitmapFileHeader imgFileHeaderOutput;			/** BMP file header for output image */
	tBitmapInfoHeader imgInfoHeaderOutput;			/** BMP info header for output image */
	char* sourceFileName;							/** Name of input image file */
	char* destinationFileName;						/** Name of output image file */
	int inputFile, outputFile;						/** File descriptors */
	unsigned char *outputBuffer;					/** Output buffer for filtered pixels */
	unsigned char *inputBuffer;						/** Input buffer to allocate original pixels */
	unsigned int rowSize;							/** Number of pixels per row */
	unsigned int threshold;							/** Threshold */
	unsigned int currentRow;						/** Current row being processed */
	unsigned int currentPixel;						/** Current pixel being processed */
	unsigned int readBytes;							/** Number of bytes read from input file */
	unsigned int writeBytes;						/** Number of bytes written to output file */
	unsigned int numPixels;							/** Number of neighbour pixels (including current pixel) */
	tPixelVector vector;							/** Vector of neighbour pixels */
	struct timeval tvBegin, tvEnd;					/** Structs to calculate the total processing time */

	int restotam;

	int filas, aux, ultimo, grano, tocan;
	filas = 0;
	aux = 0;

	// Total procesos y rank
	int SIZE, rank, tag, tam;
	
	MPI_Status Stat;

	// Init
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &SIZE);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	tag = 1;
	
	if (SIZE >= 3) {
		// Master process
		if (rank == 0) {
			// Check arguments
			if (argc != 5){
				printf ("Usage: ./bmpFilter sourceFile destinationFile threshold\n");
				exit (0);
			}

			// Get input arguments...
			sourceFileName = argv[1];
			destinationFileName = argv[2];
			threshold = atoi(argv[3]);
			grano = atoi(argv[4]);

			// Init seed
			srand(time(NULL));

			// Show info before processing
			printf ("Applying filter to image %s with threshold %d. Generating image %s\n", sourceFileName, threshold, destinationFileName);

			// Filter process begin
			gettimeofday(&tvBegin, NULL);

			// Read headers from input file
			readHeaders (sourceFileName, &imgFileHeaderInput, &imgInfoHeaderInput);
			readHeaders (sourceFileName, &imgFileHeaderOutput, &imgInfoHeaderOutput);

			// Write header to the output file
			writeHeaders (destinationFileName, &imgFileHeaderOutput, &imgInfoHeaderOutput);

			// Calculate row size for input and output images
			rowSize = (((imgInfoHeaderInput.biBitCount * imgInfoHeaderInput.biWidth) + 31) / 32 ) * 4;

			// Show headers...
			if (SHOW_BMP_HEADERS){
				printf ("Source BMP headers:\n");
				printBitmapHeaders (&imgFileHeaderInput, &imgInfoHeaderInput);
				printf ("Destination BMP headers:\n");
				printBitmapHeaders (&imgFileHeaderOutput, &imgInfoHeaderOutput);
			}

			// Open source image
			if((inputFile = open(sourceFileName, O_RDONLY)) < 0){
				printf("ERROR: Source file cannot be opened: %s\n", sourceFileName);
				exit(1);
			}

			// Open target image
			if((outputFile = open(destinationFileName, O_WRONLY | O_APPEND, 0777)) < 0){
				printf("ERROR: Target file cannot be open to append data: %s\n", destinationFileName);
				exit(1);
			}
		
			// Allocate memory to copy the bytes between the header and the image data
			outputBuffer = (unsigned char*) malloc ((imgFileHeaderInput.bfOffBits-BIMAP_HEADERS_SIZE) * sizeof(unsigned char));

			// Copy bytes between headers and pixels
			lseek (inputFile, BIMAP_HEADERS_SIZE, SEEK_SET);
			read (inputFile, outputBuffer, imgFileHeaderInput.bfOffBits-BIMAP_HEADERS_SIZE);
			write (outputFile, outputBuffer, imgFileHeaderInput.bfOffBits-BIMAP_HEADERS_SIZE);

			// Allocate memory for input and output buffers
			inputBuffer = (unsigned char *) malloc (rowSize * sizeof (unsigned char)*imgInfoHeaderInput.biHeight);
			outputBuffer = (unsigned char*) malloc (rowSize * sizeof (unsigned char)*imgInfoHeaderInput.biHeight);
	
			// Read current row data to input buffer
			if ((readBytes = read (inputFile, inputBuffer, rowSize *imgInfoHeaderInput.biHeight)) != rowSize *imgInfoHeaderInput.biHeight){
				showError ("Error while reading from source file");
			}	
			
		
			tam = imgInfoHeaderInput.biHeight / grano;

			restotam = ((imgInfoHeaderInput.biHeight % grano));
			

			printf("MASTER grano %i\n", grano);
			printf("MASTER tamaño de la division %i\n", tam);
			printf("MASTER resto %i\n", restotam);
			printf("MASTER rowSize %i\n", rowSize);
			printf("MASTER imgInfoHeaderInput.biHeight %i\n", imgInfoHeaderInput.biHeight);
			printf("\n");
			
			
		}

		// Bcast de tamaño
		MPI_Bcast(&rowSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&tam, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(&threshold, 1, MPI_INT, 0, MPI_COMM_WORLD);
		
		// Master process
		if (rank == 0) {
		
			int enviados[grano];
			int recibidos[grano];
			int k,l, terminado = 0, terminadoEnviar;
			for (k = 0; k < grano; k++){
				enviados[k] = 0;
				recibidos[k] = 0;
			}

			//sends y recv
			int sprocesos, offset1, trozo,  parte, reparto = 0;
			offset1 = 0;
			for (sprocesos = 1; sprocesos < SIZE; sprocesos++){
				if(reparto >= (imgInfoHeaderInput.biHeight - restotam)){
					sprocesos = SIZE + 1;
				}
				else{
					enviados[sprocesos - 1] = 1;
					if (enviados[(grano-1)] == 1){
						ultimo = restotam;
						trozo = offset1;
						parte = sprocesos;
						MPI_Send(&terminado, 1, MPI_INT, sprocesos, tag, MPI_COMM_WORLD);
						MPI_Send(&ultimo, 1, MPI_INT, sprocesos, tag, MPI_COMM_WORLD);
						MPI_Send(&trozo, 1, MPI_INT, sprocesos, tag, MPI_COMM_WORLD);
						MPI_Send(&parte, 1, MPI_INT, sprocesos, tag, MPI_COMM_WORLD);
						MPI_Send(&inputBuffer[offset1], rowSize * (tam + restotam), MPI_UNSIGNED_CHAR, sprocesos, tag, MPI_COMM_WORLD);
						//printf ("Enviado master a %i:\n",sprocesos);
						offset1 = offset1 + (rowSize * tam);

					}
					else{
						ultimo = 0;
						trozo = offset1;
						parte = sprocesos;
						MPI_Send(&terminado, 1, MPI_INT, sprocesos, tag, MPI_COMM_WORLD);
						MPI_Send(&ultimo, 1, MPI_INT, sprocesos, tag, MPI_COMM_WORLD);
						MPI_Send(&trozo, 1, MPI_INT, sprocesos, tag, MPI_COMM_WORLD);
						MPI_Send(&parte, 1, MPI_INT, sprocesos, tag, MPI_COMM_WORLD);
						MPI_Send(&inputBuffer[offset1], rowSize * tam, MPI_UNSIGNED_CHAR, sprocesos, tag, MPI_COMM_WORLD);
						offset1 = offset1 + (rowSize * tam);
					}
				}
				reparto = reparto + tam;				
			}

			int rprocesos, offset2;
			offset2 = 0;
			while (terminado == 0){

				MPI_Recv(&aux, 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &Stat);
				filas = filas + aux;
	
				MPI_Recv(&trozo, 1, MPI_INT, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD, &Stat);
				offset2 = trozo;
	
				MPI_Recv(&parte, 1, MPI_INT, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD, &Stat);
				recibidos[(parte - 1)] = 1;

				if (parte == grano){
					MPI_Recv(&outputBuffer[offset2], rowSize * (tam + restotam), MPI_UNSIGNED_CHAR, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD, &Stat);
				}
				else{
					MPI_Recv(&outputBuffer[offset2], rowSize * tam, MPI_UNSIGNED_CHAR, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD, &Stat);
					for (l = 0; l < grano; l++){
						if (enviados[l] == 0){
							terminadoEnviar = 0;
							parte = l + 1;
							offset2 = (rowSize * tam);
							offset2 = offset2 * l;
							l = grano + 1;
						}
						else{
							terminadoEnviar = 1;
						}
					}
					if(terminadoEnviar == 0){
						//vuelvo a enviar
						enviados[parte - 1] = 1;
						if (parte == grano){
							ultimo = restotam;
							trozo = offset2;
							MPI_Send(&terminado, 1, MPI_INT, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD);
							MPI_Send(&ultimo, 1, MPI_INT, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD);
							MPI_Send(&trozo, 1, MPI_INT, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD);
							MPI_Send(&parte, 1, MPI_INT, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD);
							MPI_Send(&inputBuffer[offset2], rowSize * (tam + restotam), MPI_UNSIGNED_CHAR, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD);

						}
						else{
							ultimo = 0;
							trozo = offset2;
							MPI_Send(&terminado, 1, MPI_INT, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD);
							MPI_Send(&ultimo, 1, MPI_INT, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD);
							MPI_Send(&trozo, 1, MPI_INT, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD);
							MPI_Send(&parte, 1, MPI_INT, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD);
							MPI_Send(&inputBuffer[offset2], rowSize * tam, MPI_UNSIGNED_CHAR, Stat.MPI_SOURCE, tag, MPI_COMM_WORLD);
						}
					}
					
				}				
				//compruebo que he recibido todo
				for (l = 0; l < grano; l++){
					if (recibidos[l] == 0){
						terminado = 0;
						l = grano + 1;
					}
					else{
						terminado = 1;
					}
				}

				if (terminado == 1){
					for (sprocesos = 1; sprocesos < SIZE; sprocesos++){
						MPI_Send(&terminado, 1, MPI_INT, sprocesos, tag, MPI_COMM_WORLD);
					}
				}
			}
			printf("Numero total de filas procesadas: %i\n", filas);
			printf("\n");
		
		}

		// Worker process
		else if (rank > 0){

			int count = 0, parte, offset, terminado = 0;

			MPI_Recv(&terminado, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &Stat);
			if (terminado == 0){		
				MPI_Recv(&ultimo, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &Stat);			

				// Allocate memory for input and output buffers
				inputBuffer = (unsigned char *) malloc (rowSize * sizeof (unsigned char)*(tam + ultimo));
				outputBuffer = (unsigned char*) malloc (rowSize * sizeof (unsigned char)*(tam + ultimo));
				while (terminado == 0){
					count = 0;
					if (ultimo != 0){
						free(inputBuffer);
						free(outputBuffer);

						inputBuffer = (unsigned char *) malloc (rowSize * sizeof (unsigned char)*(tam + ultimo));
						outputBuffer = (unsigned char*) malloc (rowSize * sizeof (unsigned char)*(tam + ultimo));

					}
			
					MPI_Recv(&offset, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &Stat);
					MPI_Recv(&parte, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &Stat);	
	

					if (ultimo  == 0){
						MPI_Recv(inputBuffer, rowSize * tam, MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD, &Stat);
					}
					else{
						MPI_Recv(inputBuffer, rowSize * (tam + ultimo), MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD, &Stat);	
					}	
						

					// For each row...
					for (currentRow=0; currentRow<(tam + ultimo); currentRow++){
						count++;
						// For each pixel in the current row...
						for (currentPixel=0; currentPixel<rowSize; currentPixel++){
							// Current pixel
							numPixels = 0;
							vector[numPixels] = inputBuffer[currentPixel + (currentRow * rowSize)];
							numPixels++;

							// Not the first pixel
							if (currentPixel > 0){
								vector[numPixels] = inputBuffer[(currentPixel-1)+ (currentRow * rowSize)];
								numPixels++;
							}

							// Not the last pixel
							if (currentPixel < (imgInfoHeaderInput.biWidth-1)){
								vector[numPixels] = inputBuffer[(currentPixel+1)+ (currentRow * rowSize)];
								numPixels++;
							}

							// Store current pixel value
							outputBuffer[currentPixel + (currentRow * rowSize)]= calculatePixelValue(vector, numPixels, threshold, DEBUG_FILTERING);
						}
					}
					//printf("worker %i Filas %i procesada \n", rank, count);
					MPI_Send(&count, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
					MPI_Send(&offset, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
					MPI_Send(&parte, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);

					if (ultimo  == 0){
						MPI_Send(outputBuffer, rowSize * tam, MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD);
					}
					else{
						MPI_Send(outputBuffer, rowSize * (tam + ultimo), MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD);
					}
	
					MPI_Recv(&terminado, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &Stat);	

					if (ultimo == 0 && terminado == 0){

						MPI_Recv(&ultimo, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &Stat);
					}
				}
				free(inputBuffer);
				free(outputBuffer);
			}
			//printf("Worker %i terminado \n", rank);
			
		}
	
		// Master process
		if (rank == 0) {
			// Write to output file
			if ((writeBytes = write (outputFile, outputBuffer, rowSize *imgInfoHeaderInput.biHeight)) != rowSize *imgInfoHeaderInput.biHeight){
				showError ("Error while writing to destination file");
			}
		
			// Close files
			close (inputFile);
			close (outputFile);

			// End of processing
			gettimeofday(&tvEnd, NULL);

			printf("Filtering time: %ld.%06ld\n", ((tvEnd.tv_usec + 1000000 * tvEnd.tv_sec) - (tvBegin.tv_usec + 1000000 * tvBegin.tv_sec)) / 1000000,
								  	  	  	  	  ((tvEnd.tv_usec + 1000000 * tvEnd.tv_sec) - (tvBegin.tv_usec + 1000000 * tvBegin.tv_sec)) % 1000000);

			free(inputBuffer);
			free(outputBuffer);

		}
	}
	else{
		if (rank == 0) {
			printf("Esta prueba funciona para minimo 3 procesos.\n");
		}

	}

	MPI_Finalize();
}
