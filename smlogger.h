#ifndef _INTER_PROCESS_RING_BUFFER_SLEEPY_H_
#define _INTER_PROCESS_RING_BUFFER_SLEEPY_H_

#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <algorithm>
#include <sys/mman.h>
#include <sys/stat.h>


#ifndef _POSIX_SHARED_MEMORY_OBJECTS
    #error Inter process lock is not support
#endif // _POSIX_SHARED_MEMORY_OBJECTS


/*****************************************************************************/
/*                                                                           */
/*                          class InterProcessMemory                         */
/*    Only supports C++ 11, Linux                                            */
/*        Buffer layout: |       Required Buffer       |   Header   |        */
/*                                                                           */
/*****************************************************************************/

class InterProcessMemory
{
protected:
    struct header
    {
        char zero[8];
        pthread_mutex_t mutex;
    };

    int m_fileNum;
    char* m_filePtr;
    FILE* m_ipmFile;
    header* m_ipmHeader;
    bool m_isMaster;
    uint32_t m_ipmLength;
    pthread_mutexattr_t m_mutexAttr;

public:
    InterProcessMemory()
    {
        m_fileNum = 0;
        m_filePtr = NULL;
        m_ipmFile = NULL;
        m_ipmHeader = NULL;

        m_isMaster = false;
        m_ipmLength = 0;

        pthread_mutexattr_init(&m_mutexAttr);
        pthread_mutexattr_setrobust(&m_mutexAttr, PTHREAD_MUTEX_ROBUST);
        pthread_mutexattr_setpshared(&m_mutexAttr, PTHREAD_PROCESS_SHARED);
    }
    ~InterProcessMemory()
    {
        releaseMappingFile();
    }

    bool init(const char* path, bool master = false, uint32_t length = 0)
    {
        if ((path == NULL) || (strlen(path) == 0) ||
            (master && (length == 0)))
        {
            return false;
        }

        m_isMaster = master;
        m_ipmLength = length + sizeof(header) + 1;

        if (!prepareMemoryMapping(path))
        {
            releaseMappingFile();
            return false;
        }

        m_ipmHeader = (header*)(m_filePtr + length);
        if (m_isMaster)
        {
            memset(m_ipmHeader->zero, 0, sizeof(m_ipmHeader->zero));
            pthread_mutex_init(&m_ipmHeader->mutex, &m_mutexAttr);
        }

        return true;
    }

    bool valid() const { return m_filePtr != NULL; }

    uint32_t size() const { return m_ipmLength - sizeof(header) - 1; };
    uint8_t* buffer() { return (uint8_t*)m_filePtr; };
    const uint8_t* buffer() const { return (const uint8_t*)m_filePtr; };

    void access()
    {
        int ret = pthread_mutex_lock(&m_ipmHeader->mutex);
        if (ret == EOWNERDEAD)
        {
            pthread_mutex_consistent(&m_ipmHeader->mutex);
        }
    };
    void release() { pthread_mutex_unlock(&m_ipmHeader->mutex); };

protected:
    bool prepareMemoryMapping(const char* path)
    {
        releaseMappingFile();

        if ((m_ipmFile = fopen(path, m_isMaster ? "wb+" : "rb+")) == NULL)
        {
            return false;
        }
        if ((m_fileNum = fileno(m_ipmFile)) < 0)
        {
            return false;
        }
        if (m_isMaster)
        {
            if (ftruncate(m_fileNum, (__off_t)m_ipmLength) < 0)
            {
                return false;
            }
        }
        else
        {
            struct stat sb;
            if ((fstat(m_fileNum, &sb) < 0) || (sb.st_size <= 0))
            {
                m_ipmLength = 0;
                return false;
            }
            m_ipmLength = sb.st_size;
        }
        return ((m_filePtr = (char*)mmap(NULL, m_ipmLength, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, m_fileNum, 0)) != NULL);
    }
    bool releaseMappingFile()
    {
        if (m_filePtr != NULL)
        {
            if (m_isMaster && (m_ipmHeader != NULL))
            {
                pthread_mutex_destroy(&m_ipmHeader->mutex);
            }
            munmap(m_filePtr, m_ipmLength);
            m_ipmHeader = NULL;
            m_ipmLength = 0;
            m_filePtr = NULL;
        }
        if (m_ipmFile != NULL)
        {
            fclose(m_ipmFile);
            m_ipmFile = NULL;
            m_fileNum = 0;
        }
        return true;
    }
};


/*****************************************************************************/
/*                                                                           */
/*                           class RingBufferShell                           */
/*                                                                           */
/*****************************************************************************/

class RingBufferShell
{
protected:
    uint8_t* m_buffer;
    uint32_t m_length;
    uint64_t* m_readPos;
    uint64_t* m_writePos;

public:
    RingBufferShell()
    {
        m_buffer = NULL;
        m_length = 0;
        m_readPos = NULL;
        m_writePos = NULL;
    }
    ~RingBufferShell() { };

    void init(uint8_t* buffer, uint32_t length, uint64_t* readPos, uint64_t* writePos)
    {
        m_buffer = buffer;
        m_length = length;
        m_readPos = readPos;
        m_writePos = writePos;
    }
    bool inited() const { return (m_buffer != NULL) && (m_length > 0) && (m_readPos != NULL) && (m_writePos != NULL); };

    bool end() const { return (*m_readPos) >= (*m_writePos); }
    void put(uint8_t val) { m_buffer[((*m_writePos)++) % m_length] = val; }
    void poke(int32_t offset, uint8_t val) { access((*m_readPos) + offset) = val; }
    uint8_t get() { return readable(*m_readPos) ? access((*m_readPos)++) : 0; }
    uint8_t peek(uint32_t offset) { uint64_t pos((*m_readPos) + offset); return readable(pos) ? access(pos) : 0; };

protected:
    uint8_t& access(uint64_t pos) { return m_buffer[pos % m_length]; };
    bool readable(uint64_t pos) { return (pos < (*m_writePos)); };
};


class IDebugBuffer
{
public:
    IDebugBuffer() { };
    virtual ~IDebugBuffer() { };

    virtual uint32_t read(uint8_t* buffer, uint32_t buffer_length) = 0;
    virtual uint32_t write(const uint8_t* content, uint32_t content_length, const char* tail) = 0;
    virtual uint32_t readSwap(uint32_t offset) = 0;
    virtual bool writeSwap(uint32_t offset, uint32_t val) = 0;
    virtual bool checkInit() = 0;
};


/*****************************************************************************/
/*                                                                           */
/*                       class InterProcessDebugBuffer                       */
/*      Buffer layout: |       Log Buffer       |   Header   |  SWAP  |      */
/*        Command to view the log : watch -d -n 1 tail /tmp/iplog.txt        */
/*                                                                           */
/*****************************************************************************/

class InterProcessDebugBuffer : public IDebugBuffer
{
protected:
    const bool MASTER;
    const uint32_t LOG_SIZE;
    const uint32_t SWAP_SIZE;
    const std::string LOG_FILE;

protected:
    struct header
    {
        char zero[8];
        uint64_t writePos;
        uint64_t latestPos;
    };

    header* m_header;
    uint64_t m_readPos;
    uint32_t* m_swapBuffer;
    RingBufferShell m_ringBuffer;
    InterProcessMemory m_ipMemory;

public:
    InterProcessDebugBuffer(bool master, uint32_t log_size, uint32_t swap_size, const char* log_file)
        : MASTER(master),
          LOG_SIZE(log_size > 0 ? log_size : 1),
          SWAP_SIZE(swap_size > 0 ? swap_size : 1),
          LOG_FILE(log_file)
    {
        m_header = NULL;
        m_readPos = 0;
        m_swapBuffer = NULL;
    };
    ~InterProcessDebugBuffer() { };

    uint32_t read(uint8_t* buffer, uint32_t buffer_length)
    {
        if (!m_ipMemory.valid())
        {
            return 0;
        }

        m_ipMemory.access();
        if (m_readPos > m_header->writePos)
        {
            m_readPos = m_header->writePos;
        }
        else if (m_header->writePos - m_readPos >= LOG_SIZE)
        {
            m_readPos = m_header->writePos - LOG_SIZE + std::min(LOG_SIZE / 10, (uint32_t)500);
        }
        uint32_t readed = 0;
        for ( ; ((readed < buffer_length) && !m_ringBuffer.end()); ++readed)
        {
            buffer[readed] = m_ringBuffer.get();
        }
        m_ipMemory.release();

        return readed;
    }
    uint32_t write(const uint8_t* content, uint32_t content_length, const char* tail)
    {
        if (!m_ipMemory.valid())
        {
            return 0;
        }

        m_ipMemory.access();
        m_header->latestPos = m_header->writePos;
        for (uint32_t i = 0; i < content_length; ++i)
        {
            m_ringBuffer.put(content[i]);
        }
        if (tail != NULL)
        {
            for (uint32_t i = 0; tail[i] != '\0'; ++i)
            {
                m_ringBuffer.put(tail[i]);
            }
        }
        m_ringBuffer.poke(0, 0);
        m_ipMemory.release();

        return content_length;
    }

    uint32_t readSwap(uint32_t offset)
    {
        if ((offset >= SWAP_SIZE) || m_swapBuffer == NULL || !m_ipMemory.valid())
        {
            return 0;
        }

        m_ipMemory.access();
        uint32_t val = m_swapBuffer[offset];
        m_ipMemory.release();

        return val;
    }
    bool writeSwap(uint32_t offset, uint32_t val)
    {
        if ((offset >= SWAP_SIZE) || m_swapBuffer == NULL || !m_ipMemory.valid())
        {
            return false;
        }

        m_ipMemory.access();
        m_swapBuffer[offset] = val;
        m_ipMemory.release();

        return true;
    }

    bool checkInit()
    {
        bool ret = true;
        if (!m_ipMemory.valid())
        {
            if (m_ipMemory.init(LOG_FILE.c_str(), MASTER, sizeof(header) + LOG_SIZE))
            {
                m_header = (header*)(m_ipMemory.buffer() + LOG_SIZE);
                memset(m_header->zero, 0, sizeof(m_header->zero));
                m_swapBuffer = (uint32_t*)(m_ipMemory.buffer() + LOG_SIZE + sizeof(header));
                m_readPos = m_header->latestPos;
                m_ringBuffer.init(m_ipMemory.buffer(), LOG_SIZE, &m_readPos, &m_header->writePos);
            }
            else
            {
                ret = false;
                m_header = NULL;
                m_ringBuffer.init(NULL, 0, NULL, NULL);
            }
        }
        return ret;
    }
};


/*******************************************************************************************/
/*                                                                                         */
/*                              class smLogger - printf Style                              */
/*                                                                                         */
/*******************************************************************************************/

class smLogger
{
protected:
    char* m_fmtBuffer;
    uint32_t m_bufferLen;
    pthread_mutex_t m_mutex;
    IDebugBuffer* m_dbgBuffer;
    pthread_mutexattr_t m_mutexAttr;

protected:
    smLogger()
    {
        m_bufferLen = 0;
        m_fmtBuffer = NULL;
        m_dbgBuffer = NULL;
        pthread_mutexattr_init(&m_mutexAttr);
        pthread_mutexattr_settype(&m_mutexAttr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_mutex, &m_mutexAttr);
    };
    ~smLogger()
    {
        delete[] m_fmtBuffer;
        m_fmtBuffer = NULL;
        m_bufferLen = 0;
        pthread_mutex_destroy(&m_mutex);
    };

public:
    static     smLogger& instance()
    {
        static smLogger _instance;
        return          _instance;
    }
    void init(IDebugBuffer* buffer)
    {
        m_dbgBuffer = buffer;
    }
    uint32_t log(const char* fmt, ...)
    {
        uint32_t logLen = 0;
        {
            va_list ap;
            va_start(ap, fmt);
                size_t needed = vsnprintf(NULL, 0, fmt, ap);
                if (needed > m_bufferLen)
                {
                    lock();
                    delete[] m_fmtBuffer;
                    m_bufferLen = needed + 32;
                    m_fmtBuffer = new char[m_bufferLen];
                    release();
                }
            va_end(ap);
        }
        {
            va_list ap;
                va_start(ap, fmt);
                lock();
                int len = vsprintf(m_fmtBuffer, fmt, ap);
                if ((m_dbgBuffer != NULL) && (len > 0))
                {
                    logLen = m_dbgBuffer->write((uint8_t*)m_fmtBuffer, (uint32_t)len, "\n");
                }
                release();
            va_end(ap);
        }
        return logLen;
    }
protected:
    void lock() { pthread_mutex_lock(&m_mutex); };
    void release() { pthread_mutex_unlock(&m_mutex); };
};


#define LOG_DEBUG(fmt, ...) smLogger::instance().log(fmt, ##__VA_ARGS__)
#define LOG_TRACE(fmt, ...) smLogger::instance().log("TRACE - " __FILE__ " (%d) : " fmt, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) smLogger::instance().log("ERROR - " __FILE__ " (%d) : " fmt, __LINE__, ##__VA_ARGS__)



#endif // _INTER_PROCESS_RING_BUFFER_SLEEPY_H_
