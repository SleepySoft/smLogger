#include <time.h>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


void slave(char selection);
void master(char selection);


std::string generateRandomString()
{
    char msgChar = '\0';
    uint32_t selection = rand() % 2;
    if (selection == 0)
    {
        msgChar = (rand() % ('z' - 'a' + 1))+ 'a';
    }
    else
    {
        msgChar = (rand() % ('Z' - 'A' + 1))+ 'A';
    }
    uint32_t msgLen = (rand() % (500 - 1 + 1))+ 1;
    return std::string(msgLen, msgChar);
}

void testPrintfTimeSpending()
{
    std::string text;
    uint32_t times_counter = 0;
    uint32_t length_counter = 0;
    struct timespec startTime, endTime;

    while (1)
    {
        if (times_counter == 0)
        {
            clock_gettime(CLOCK_MONOTONIC, &startTime);
        }
        ++times_counter;

        text = generateRandomString();
        length_counter += text.length();

        printf("%s\n", text.c_str());

        if (times_counter >= 100)
        {
            clock_gettime(CLOCK_MONOTONIC, &endTime);

            int64_t sub = (endTime.tv_sec - startTime.tv_sec)*1000000000ULL;
            int64_t deltaTime = sub + (endTime.tv_nsec - startTime.tv_nsec);

            printf("Sent %d times with %d bytes in %lld nano sec, avg %f ns pre byte\n",
                    times_counter, length_counter, (long long)deltaTime, (double)deltaTime / length_counter);

            times_counter = 0;
            length_counter = 0;
        }
    }
}


int main(int argc, char **argv)
{
    srand((unsigned)time(NULL));
    if ((argc >= 2) && ((argv[1][0] == 'p') || (argv[1][0] == 'P')))
    {
        testPrintfTimeSpending();
    }
    else if ((argc >= 2) && ((argv[1][0] == 'm') || (argv[1][0] == 'M')))
    {
        char selection = 'm';
        if  (argc >= 3)
        {
            selection = argv[2][0];
        }
        printf("Run as Master. Test = %c.\n", selection);
        master(selection);
    }
    else
    {
        char selection = 'a';
        if  (argc >= 3)
        {
            selection = argv[2][0];
        }
        printf("Run as Slave. Test = %c.\n", selection);
        slave(selection);
    }
	return 0;
}
