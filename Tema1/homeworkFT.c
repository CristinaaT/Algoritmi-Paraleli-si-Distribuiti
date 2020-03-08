#include <complex.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/* Put in a constant variable a part of the expression from the FT forumla. */
const complex cexp_term = -2.0 * acos(-1) * I;

unsigned numThreads;

/* Avoid to use global variables, so use a structure for thread's arguments. */
typedef struct arguments {
	unsigned start;
	unsigned end;
	unsigned n;

	complex *buffer;
	complex *output;
} arguments;

void *ft(void *input)
{
	/*
	 * Split the sets of points given in the input file into equal intervals.
	 * Apply the forumla presented in homework's text.
	 * Use the same variables for notation.
	 */
	arguments args = *(arguments *) input;

	unsigned k, n;
	complex aux_sum;

	for (k = args.start; k < args.end; k++) {
		aux_sum = 0;

		for (n = 0; n < args.n; n++) {
			aux_sum += args.buffer[n] * cexp(cexp_term / args.n * k * n);
		}

		args.output[k] = aux_sum;
	}

	return NULL;
}

int readInput(int argc, char **argv, unsigned *n, complex **buffer,
              complex **output)
{
	/*
	 * Parse the input data and get:
	 * - the number of threads from the command line arguments.
	 * - the input points for the transformation from the input file.
	 *
	 * Check the validity of the input data and also of the every return code
	 * of called functions.
	 *
	 * This function also allocates memory for buffer and output arrays, so
	 * pay attention at freeing it at the end of the program.
	 */
	if (argc != 4) {
		printf("Usage: %s <inputFile> <outputFile> <numThreads>\n", argv[0]);
		return EXIT_FAILURE;
	}

	numThreads = atoi(argv[3]);
	if (!numThreads) {
		fprintf(stderr, "The <numThreads> variable is invalid!\n");
		return EXIT_FAILURE;
	}

	FILE *filePointer = fopen(argv[1], "r");
	if (!filePointer) {
		fprintf(stderr, "Failed to open the input file.\n");
		return EXIT_FAILURE;
	}

	if (!fscanf(filePointer, "%u", n)) {
		fclose(filePointer);
		fprintf(stderr, "Failed to read from the input file.\n");
		return EXIT_FAILURE;
	}

	*buffer = malloc((*n) * sizeof(complex));
	*output = malloc((*n) * sizeof(complex));

	if (!buffer || !output) {
		fclose(filePointer);
		fprintf(stderr, "Failed to allocate memory for the arrays!\n");
		return EXIT_FAILURE;
	}

	double aux;
	unsigned index;

	for (index = 0; index < (*n); index++) {
		if (!fscanf(filePointer, "%lf", &aux)) {
			free(*buffer);
			free(*output);
			fclose(filePointer);

			fprintf(stderr, "Failed to read from the input file.\n");
			return EXIT_FAILURE;
		}

		(*buffer)[index] = aux;
	}

	fclose(filePointer);
	return EXIT_SUCCESS;
}

int writeResults(char *filename, unsigned n, complex *output)
{
	/*
	 * Write the result of the Fourrier Transformation to the output file.
	 */
	FILE *filePointer = fopen(filename, "w");
	if (!filePointer) {
		fprintf(stderr, "Failed to open the output file.\n");
		return EXIT_FAILURE;
	}

	if (!fprintf(filePointer, "%u\n", n)) {
		fprintf(stderr, "Failed to write to the output file.\n");
		return EXIT_FAILURE;
	}

	unsigned index;
	for (index = 0; index < n; index++) {
		if (!fprintf(filePointer, "%lf %lf\n", creal(output[index]),
		             cimag(output[index]))) {
			fclose(filePointer);
			fprintf(stderr, "Failed to write to the output file.\n");
			return EXIT_FAILURE;
		}
	}

	fclose(filePointer);
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	int returnCode;
	unsigned index, n = 0;

	complex *buffer = NULL;
	complex *output = NULL;

	returnCode = readInput(argc, argv, &n, &buffer, &output);
	if (returnCode == EXIT_FAILURE) {
		exit(EXIT_FAILURE);
	}

	/*
	 * Build the request arguments for each thread -> start and wait them.
	 *
	 * If n is not multiple of numThreads, what is extra (n % numThreads) will
	 * be assigned to the first (n % numThreads) threads using the following
	 * formulas.
	 *
	 * Every thread will receive his interval (start, end), the values needed
	 * for calculus and the output array.
	 */
	arguments args[numThreads];
	pthread_t threads[numThreads];

	unsigned chunkSize = n / numThreads;
	unsigned extraSize = n % numThreads;

	for (index = 0; index < numThreads; index++) {
		args[index].start = index * chunkSize + MIN(index, extraSize);
		args[index].end   = (index + 1) * chunkSize + MIN(index + 1, extraSize);

		args[index].n      = n;
		args[index].buffer = buffer;
		args[index].output = output;

		pthread_create(&(threads[index]), NULL, ft, (void *) &args[index]);
	}

	for (index = 0; index < numThreads; index++) {
		pthread_join(threads[index], NULL);
	}

	returnCode = writeResults(argv[2], n, output);

	/*
	 * Free the allocated memory.
	 */
	free(buffer);
	free(output);

	exit(returnCode);
}
