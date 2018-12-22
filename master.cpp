#include <string>
#include <stdlib.h>
#include <iostream>
#include "smlogger.h"


std::string generateRandomString();

void test_ipbuffer_random(InterProcessDebugBuffer& ipdb)
{
    uint32_t times_counter = 0;
    uint32_t length_counter = 0;
    std::string text;
    struct timespec startTime, endTime;

    while(1)
    {
        if (ipdb.checkInit())
        {
            if (times_counter == 0)
            {
                clock_gettime(CLOCK_MONOTONIC, &startTime);
            }
            ++times_counter;

            text = generateRandomString();
            length_counter += text.length();

            ipdb.write((uint8_t*)text.c_str(), text.size(), "\n");

            if (times_counter >= 1000)
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
        else
        {
            usleep(100000);
        }
    }
}

void test_ipbuffer_manual(InterProcessDebugBuffer& ipdb)
{
    uint32_t writed = 0;
    std::string text;

    while(1)
    {
        if (ipdb.checkInit())
        {
            std::getline(std::cin, text);
            writed = ipdb.write((uint8_t*)text.c_str(), text.size(), "\n");
            if (writed > 0)
            {
                printf("--------------------> Write len = %d\n", writed);
            }
        }
        else
        {
            usleep(100000);
        }
    }
}

void test_logger_random(InterProcessDebugBuffer& ipdb)
{
    uint32_t writed = 0;
    uint32_t times_counter = 0;
    uint32_t length_counter = 0;
    std::string text;
    struct timespec startTime, endTime;

    smLogger::instance().init(&ipdb);

    while(1)
    {
        if (ipdb.checkInit())
        {
            if (times_counter == 0)
            {
                clock_gettime(CLOCK_MONOTONIC, &startTime);
            }
            ++times_counter;

            text = generateRandomString();
            writed = LOG_TRACE("float: %f, int: %d, string: %s",
                    (rand() % (65535 - 0 + 1)) + 0,
                    (float)rand() / (float)rand(),
                    text.c_str());
            length_counter += writed;

            if (times_counter >= 1000)
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
        else
        {
            usleep(100000);
        }
    }
}

void master(char selection)
{
    InterProcessDebugBuffer ipdb(true, 10 * 1024, 10, "/tmp/iplog.txt");

    switch(selection)
    {
    case 'm': { test_ipbuffer_manual(ipdb); } break;
    case 'M': { test_ipbuffer_manual(ipdb); } break;
    case 'r': { test_ipbuffer_random(ipdb); } break;
    case 'R': { test_ipbuffer_random(ipdb); } break;
    case 'l': { test_logger_random(ipdb); } break;
    case 'L': { test_logger_random(ipdb); } break;
    default : { ; } break;
    }
}

