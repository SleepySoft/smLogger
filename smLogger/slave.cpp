#include "smlogger.h"

void slave()
{
    uint8_t buffer[512] = { 0 };
    uint32_t readed = 0;
    InterProcessDebugBuffer ipdb(false, 10 * 1024, 10, "/tmp/iplog.txt");

    while(1)
    {
        if (ipdb.checkInit())
        {
            readed = ipdb.read(buffer, sizeof(buffer));
            if (readed > 0)
            {
                printf("--------------------> Read len = %d\n", readed);
                for (uint32_t i = 0; i < readed; ++i)
                {
                    putchar(buffer[i]);
                }
                printf("\n");
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

