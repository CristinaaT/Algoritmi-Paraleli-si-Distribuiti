#include <complex.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/* Put in a constant a part of the expression from the FFT forumla. */
const complex cexp_term = -acos(-1) * I;

unsigned numThreads;

/* Avoid to use global variables, so use a structure for thread's arguments. */
typedef struct arguments {
	unsigned n;
	unsigned step;

	complex *buffer;
	complex *output;
} arguments;

void *fftParalelized(void *input);

void *fft(complex *buffer, complex *output, unsigned n, unsigned step)
{
	/*
	 * Get the code suggested in the homework for FFT and parallelize it.
	 *
	 * In the forumla presented in Rosetta code for FFT, the tree built during
	 * recursive calls is a binary tree, because each node creates two more
	 * children.
	 *
	 * Thus, depending on the number of threads available, I chose to
	 * parallelize all the calculations when at a specific level in the tree of
	 * recursive calls will be a number of nodes equals to the number of
	 * threads.
	 *
	 * The observation is that this thing will be done in the previous level of
	 * the tree, where each of the two children of the node will be processed
	 * using a different thread.
	 *
	 * In other words, I took the scheme in which the calculations are presented
	 * on the levels using the butterfly scheme and I chose to parallelize the
	 * level where the number of butterflies equals the number of threads.
	 */
	if (step >= n) {
		return NULL;
	}

	if (step * 2 == numThreads) {
		arguments args[2];
		pthread_t threads[2];

		args[0].n      = n;
		args[0].step   = step * 2;
		args[0].buffer = output;
		args[0].output = buffer;

		args[1].n      = n;
		args[1].step   = step * 2;
		args[1].buffer = output + step;
		args[1].output = buffer + step;

		pthread_create(&(threads[0]), NULL, fftParalelized, (void *) &args[0]);
		pthread_create(&(threads[1]), NULL, fftParalelized, (void *) &args[1]);

		pthread_join(threads[0], NULL);
		pthread_join(threads[1], NULL);

	} else {
		fft(output, buffer, n, step * 2);
		fft(output + step, buffer + step, n, step * 2);
	}

	complex term;
	unsigned index;

	for (index = 0; index < n; index += step * 2) {
		term = cexp(cexp_term * index / n) * output[index + step];

		buffer[index / 2]       = output[index] + term;
		buffer[(index + n) / 2] = output[index] - term;
	}

	return NULL;
}

void *fftParalelized(void *input)
{
	arguments args = (*(arguments *) input);
	return fft(args.buffer, args.output, args.n, args.step);
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
		(*output)[index] = aux;
	}

	fclose(filePointer);
	return EXIT_SUCCESS;
}

int writeResults(char *filename, unsigned n, complex *output)
{
	/*
	 * Write the result of the Fast Fourrier Transformation to the output file.
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
	unsigned n = 0;

	complex *buffer = NULL;
	complex *output = NULL;

	returnCode = readInput(argc, argv, &n, &buffer, &output);
	if (returnCode == EXIT_FAILURE) {
		exit(EXIT_FAILURE);
	}

	/*
	 * Start the calculations.
	 * The parallelization will be started at a specific level in the tree.
	 */
	fft(buffer, output, n, 1);

	returnCode = writeResults(argv[2], n, buffer);

	/*
	 * Free the allocated memory.
	 */
	free(buffer);
	free(output);

	exit(returnCode);
}
