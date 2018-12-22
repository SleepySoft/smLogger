#include "smlogger.h"

void slave()
{
    uint8_t buffer[512] = { 0 };
    uint32_t readed = 0;
    uint32_t read_count = 0;
    InterProcessDebugBuffer ipdb(false, 10 * 1024, 10, "/tmp/iplog.txt");
    smLogger::instance().init(&ipdb);

    while(1)
    {
        if (ipdb.checkInit())
        {
            readed = ipdb.read(buffer, sizeof(buffer));
            if (readed > 0)
            {
                ++read_count;
                printf("--------------------> Read len = %d\n", readed);
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

