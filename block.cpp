/*
 * block.cpp
 *
 *  Created on: Feb 26, 2019
 *      Author: Sleepy
 */

#include "blockLogger.h"
#include "smlogger.h"

#include <string>
#include <stdio.h>

#define LOOPS(x) for (uint32_t i = 0; i < x; ++i)

void doBlockLoggerTest(uint8_t* buffer, uint32_t size)
{
    BlockLogger::instance().final();
    BlockLogger::instance().init(buffer, size);

    BlockLogger::instance().registerBlock("Registered Named Log: ", "%0.2f", 150);
    BlockLogger::instance().registerBlock("", "Registered Anonymous Log: %08X", 300);

    for (uint32_t i = 0; i < 100; ++i)
    {
        BlockLogger::instance().named_log("Registered Named Log: ", i * 11.0f);
    }

    for (uint32_t i = 0; i < 100; ++i)
    {
        BlockLogger::instance().named_log("Unregistered Named Log: ", i * 22.0f);
    }

    for (uint32_t i = 0; i < 100; ++i)
    {
        BlockLogger::instance().anonymous_log("Registered Anonymous Log: %08X", i * 333);
    }

    for (uint32_t i = 0; i < 100; ++i)
    {
        BlockLogger::instance().anonymous_log("Unregistered Anonymous Log - %d", i * 444);
    }

    char var_str[] = "[char* type]";

    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater bool",       true);
    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater char*",      var_str);
    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater const char*","[const char* type]");

    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater char",       'c');
    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater float",      233.333f);
    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater double",     666.666);

    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater int8_t",     (int8_t)-11);
    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater int16_t",    (int16_t)-33333);
    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater int32_t",    (int32_t)-5555555555);
    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater int64_t",    (int64_t)-7777777777777777);

    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater uint8_t",    (uint8_t)11);
    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater uint16_t",   (uint16_t)3333);
    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater uint32_t",   (uint32_t)5555555555);
    LOOPS(10) BlockLogger::instance().named_log("Test named log default formater uint64_t",   (uint64_t)7777777777777777);

    char formater[1024] = { 0 };
    for (uint32_t i = 0; i < 100; ++i)
    {
        snprintf(formater, sizeof(formater), "Test Log Buffer Over Flow [%d] - %%d", i);
        BlockLogger::instance().anonymous_log(formater, i);
    }
}

static const uint32_t TEST_BUFFER_SIZE = 5000;

void testBlockLoggerWithLocalRam()
{
    uint8_t* buffer = new uint8_t[TEST_BUFFER_SIZE];
    memset(buffer, 0, TEST_BUFFER_SIZE);

    doBlockLoggerTest(buffer, TEST_BUFFER_SIZE);

    FILE* file = fopen("/tmp/localblock.txt", "wb");
    fwrite(buffer, 1, TEST_BUFFER_SIZE, file);
    fclose(file);

    delete[] buffer;
    buffer = NULL;
}


void testBlockLoggerWithSharedRam()
{
    InterProcessMemory ipm;
    ipm.init("/tmp/ipblock.txt", true, TEST_BUFFER_SIZE);
    doBlockLoggerTest(ipm.buffer(), TEST_BUFFER_SIZE);
}

void block()
{
    testBlockLoggerWithLocalRam();
    testBlockLoggerWithSharedRam();
}




