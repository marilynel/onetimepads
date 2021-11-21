/**
 * Create a key file of specified length
 * characters in file will be any of the 27 allowed chars, generated using standard Unix randomization methods
 *      -> generate random number between 0 and 26 and add A (up to 25)
 *      -> if randNum == 26 convert to space
 * do not create spaces every fice chars
 * does not need to be "cryptographically secure" --> use rand()
 * last char keygen outputs should be a newline
 * error text must be output to stderr
 * 
 * 
 * 
 * To call keygen:
 *      keygen <intNumKeyLength>
 * keygen outputs to stdout
 * for example:
 *      keygen 256 > mykey
 *          creates a key of 256 chars (257 if you include the newline) and redirects to a file called mykey
 *      keygen 5
 *          GSLIJ\n
 **/


// gcc --std=c99 -o keygen keygen.c

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char* argv[]) {
    srand(time(NULL));
    if (argc != 2) {
        printf("please enter a number\n");
        exit(0);
    }

    int numChars = atoi(argv[1]);

    for (int i = 0; i < numChars; i++) {
        int n = rand() % ((26 + 1) - 0) + 0;
        char key;
        if (n == 26) {
            key = ' ';
        }
        else {
            key = n + 'A';
        }
        printf("%c", key);
    }
    printf("\n");
    return 0;
}
