#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/*
The best way to count the max occurrence in a ring buffer which seems to be quite complicated.
*/

template < class T, uint32_t THRESHOLD >
class selector
{
protected:
    T m_chosen;
    uint32_t m_waitAndSee;
    uint32_t m_hesitatation;
public:
    selector() { reset(T()); }
    ~selector() { }

    void reset(const T& val)
    {
        m_chosen = val;
        m_waitAndSee = 0;
        m_hesitatation = 0;
    }
    void update(const T& val)
    {
        if (!decided())
        {
            if (val == m_chosen)
            {
                ++m_waitAndSee;
            }
            else
            {
                m_chosen = val;
                m_waitAndSee = 1;
            }
            ++m_hesitatation;
        }
    }

    T& selection() { return m_chosen;  }
    bool decided() const { return m_waitAndSee >= THRESHOLD; }
    uint32_t hesitatation() const { return m_hesitatation; }
};

void test_selector()
{
    static uint32_t baseType = 15;
    static uint32_t noizeRate = 20;

    srand((unsigned)time(NULL));

    uint32_t maxHesitatation = 0;
    selector< uint32_t, 5 > sel;

    for (uint32_t i = 1; i <= 10000; ++i)
    {
        bool noize = (((uint32_t)abs(rand()) % 100) <= noizeRate);
        uint32_t samplingType = noize ? (baseType + rand() % 10) : baseType;

        samplingType = i % 100 ? samplingType : 0;

        if (samplingType == 0)
        {
            printf("Hesitatation Count £º %d\n", sel.hesitatation());
            sel.reset(0);
            printf("----------------------------------------------------------------------\n");
        }
        else
        {
            sel.update(samplingType);
        }
        maxHesitatation = sel.selection() > maxHesitatation ? sel.selection() : maxHesitatation;
        printf("Sampling Type: %d , Selected Type: %d\n", samplingType, sel.selection());
    }

    printf("\nThe maxHesitatation : %d\n\n", maxHesitatation);
}

#include "blockLogger.h"

#define LOOPS(x) for (uint32_t i = 0; i < x; ++i) 

void main()
{
    static const uint32_t TEST_BUFFER_SIZE = 5000;

    uint8_t* buffer = new uint8_t[TEST_BUFFER_SIZE];
    memset(buffer, ' ', TEST_BUFFER_SIZE);

    BlockLogger::instance().init(buffer, TEST_BUFFER_SIZE);
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

    FILE* file = fopen("log.txt", "wb");
    fwrite(buffer, 1, TEST_BUFFER_SIZE, file);
    fclose(file);

    delete[] buffer;
    buffer = NULL;
}