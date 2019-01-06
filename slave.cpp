#include "smlogger.h"
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <inttypes.h>


void slave_auto(InterProcessDebugBuffer& ipdb)
{
    uint8_t buffer[512] = { 0 };
    uint32_t readed = 0;
    uint32_t read_count = 0;
    smLogger::instance().init(&ipdb);

    while(1)
    {
        if (ipdb.checkInit())
        {
            readed = ipdb.read(buffer, sizeof(buffer));
            if (readed > 0)
            {
                ++read_count;
                printf("--------------------> Read len = %d, Read pos = %" PRIu64 "\n", readed, ipdb.rpos());
                for (uint32_t i = 0; i < readed; ++i)
                {
                    putchar(buffer[i]);
                }
                printf("\n");

                if (read_count % 10 == 0)
                {
                    LOG_DEBUG("-------> Trigger %d", read_count);
                }
            }
            else
            {
                usleep(1);
            }
        }
        else
        {
            usleep(100000);
        }
    }
}

void slave_manual(InterProcessDebugBuffer& ipdb)
{
    uint8_t buffer[512] = { 0 };
    int32_t offset = 0;
    uint32_t readed = 0;
    smLogger::instance().init(&ipdb);

    static struct termios oldTermios, newTermios;
    tcgetattr(STDIN_FILENO, &oldTermios);
    newTermios = oldTermios;
    cfmakeraw(&newTermios);
//    newTermios.c_lflag &= ~(ICANON);

    while(1)
    {
        if (ipdb.checkInit())
        {
            tcsetattr(STDIN_FILENO, TCSANOW, &newTermios);

            char c1 = getchar();
//            printf("%d\n", (int)c1);
            if (c1 == 'q') { break; }
            if (c1 == 27)
            {
                char c2 = getchar();
//                printf("%d\n", (int)c2);
                if (c1 == 'q') { break; }
                if (c2 == 91)
                {
                    char c3 = getchar();

                    if (c3 == 65)
                    {
                        offset--;
                    }
                    else if (c3 == 66)
                    {
                        offset++;
                    }
                    else if (c3 == 68)
                    {
                        offset -= 10;
                    }
                    else if (c3 == 67)
                    {
                        offset += 10;
                    }
                }
            }

            tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);

            printf("offset = %d\n", offset);

            ipdb.seek(offset);
            readed = ipdb.read(buffer, sizeof(buffer));
            if (readed > 0)
            {
                for (uint32_t i = 0; i < readed; ++i)
                {
                    putchar(buffer[i]);
                }
            }

            printf("Read pos = %d\n", (int)ipdb.rpos());
        }
        else
        {
            usleep(100000);
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldTermios);
}


void slave(char selection)
{
    InterProcessDebugBuffer ipdb(false, 1 * 1024 * 1024, 10, "/tmp/iplog.txt");

    switch(selection)
    {
    case 'a': { slave_auto(ipdb); } break;
    case 'A': { slave_auto(ipdb); } break;
    case 'm': { slave_manual(ipdb); } break;
    case 'M': { slave_manual(ipdb); } break;
    default : { ; } break;
    }
}

