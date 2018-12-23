/*
 * libsmLogger.cpp
 *
 *  Created on: Dec 22, 2018
 *      Author: Sleepy
 */

#include "smLoggerWrapper.h"
#include "smlogger.h"

InterProcessDebugBuffer* g_ipdb = NULL;

extern "C"
{

void initLogger(const char* path, int log_len, int swap_len)
{
    if (g_ipdb != NULL)
    {
        delete g_ipdb;
        g_ipdb = NULL;
    }
    g_ipdb = new InterProcessDebugBuffer(false, log_len, swap_len, path);
}

bool logInited()
{
    return ((g_ipdb != NULL) && g_ipdb->checkInit());
}

int logRead(char* content, int len)
{
    return ((g_ipdb != NULL) && g_ipdb->checkInit()) ? (int)g_ipdb->read((uint8_t*)content, (uint32_t)len) : 0;
}
int logWrite(const char* content, int len)
{
    return ((g_ipdb != NULL) && (g_ipdb->checkInit())) ? (int)(g_ipdb->write((const uint8_t*)content, (uint32_t)len, "\n")) : 0;
}

}









