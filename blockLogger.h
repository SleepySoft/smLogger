#ifndef _BLOCK_LOGGER_SLEEPY_H_
#define _BLOCK_LOGGER_SLEEPY_H_

/*****************************************************************************************/
/*                                                                                       */
/*                                     Block Logger                                      */
/*  Block Logger is such a kind of log:                                                  */
/*    1.The total log size is constant. It will never increase after allocated.          */
/*    2.A class of log only shows in its zone, which we called "block".                  */
/*    3.A log block is identified by its name, or format string, if name is not present. */
/*    4.A log block can keep history according to its reserved size.                     */
/*    5.The latest log is pointed by ITEM_MARKER. The following log may be corrupted.    */
/*    6.It's better to declare a block before using it (recommended), if not, it's fine. */
/*                                                                                       */
/*****************************************************************************************/

#include <map>
#include <string>
#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <memory.h>


#define ITEM_MARKER " < "
#define ITEM_SEPERATOR ", "
#define LINE_SEPERATOR "\r\n"
#define BLOCK_SEPERATOR "\n------------\n"

typedef const char* CONSTSTR;

class BlockLogger
{
protected:
    struct BlockData
    {
        uint8_t* buffer;
        uint32_t capacity;

        uint32_t itemStart;
        uint32_t itemOffset;
        std::string formater;

        BlockData() : buffer(NULL), capacity(0), itemStart(0), itemOffset(0) { }

        void resetItemAddr() { itemOffset = 0; }
        void shiftItemAddr(uint32_t used) { itemOffset += used; }
        uint8_t* getItemAddr() const { return buffer + itemStart + itemOffset; };
        uint32_t getFreeSpace() const { return capacity > itemStart + itemOffset + sizeof(BLOCK_SEPERATOR) - 1 ? 
                                               capacity - itemStart - itemOffset - sizeof(BLOCK_SEPERATOR) + 1 : 0; }
    };
protected:
    uint8_t* m_buffer;
    uint32_t m_capacity;
    uint32_t m_allocated;
    std::map< std::string, BlockData* > m_blocks;
protected:
    BlockLogger() : m_buffer(NULL), m_capacity(0), m_allocated(0) { };
    ~BlockLogger() { final(); };
public:
    static BlockLogger& instance() { static BlockLogger instance; return instance;  }

    void init(uint8_t* buffer, uint32_t size) { m_buffer = buffer; m_capacity = size; }
    void final() {
        for (auto iter = m_blocks.begin(); iter != m_blocks.end(); ++iter) { delete (*iter).second; };
        m_buffer = NULL; m_capacity = 0; m_allocated = 0; m_blocks.clear(); };

    BlockData* registerBlock(const char* name, const char* formater, uint32_t reserved)
    {
        bool nameValid = (name != NULL && name[0] != '\0');
        bool formaterValid = (formater != NULL && formater[0] != '\0');

        if (!nameValid && !formaterValid) { return NULL; }

        reserved += nameValid ? strlen(name) + 3 : 0;
        reserved += sizeof(BLOCK_SEPERATOR) - 1;

        BlockData* block = findBlock(name);
        if (block == NULL) 
        {
            block = newBlock(nameValid ? name : formater, reserved);

            if (block != NULL)
            {
                if (nameValid)
                {
                    int needed = snprintf((char*)block->buffer, block->capacity, "%s: ", name);
                    block->itemStart = needed;
                }
                if (formaterValid)
                {
                    block->formater = formater;
                }
                memcpy(block->buffer + block->capacity - (sizeof(BLOCK_SEPERATOR) - 1), BLOCK_SEPERATOR, sizeof(BLOCK_SEPERATOR) - 1);
            }
        }
        return block;
    }

    template< class T >
    void named_log(const char* name, const T& val)
    {
        BlockData* block = findBlock(name);
        if (block == NULL)
        {
            int needed = snprintf(NULL, 0, defaultFormater(val), val);
            block = registerBlock(name, defaultFormater(val), needed * 10);
        }
        if ((block != NULL) && (block->formater != ""))
        {
            if (block->getFreeSpace() == 0)
            {
                block->resetItemAddr();
            }
            if (block->itemOffset != 0)
            {
                int used = snprintf((char*)block->getItemAddr(), block->getFreeSpace(), "%s", ITEM_SEPERATOR);
                block->shiftItemAddr(used);
            }
            int needed = snprintf((char*)block->getItemAddr(), block->getFreeSpace(), block->formater.c_str(), val);
            if (needed > (int)block->getFreeSpace())
            {
                block->resetItemAddr();
                needed = snprintf((char*)block->getItemAddr(), block->getFreeSpace(), block->formater.c_str(), val);
            }
            block->shiftItemAddr(needed);
            if (block->getFreeSpace() > sizeof(ITEM_MARKER))
            {
                memcpy((char*)block->getItemAddr(), ITEM_MARKER, sizeof(ITEM_MARKER) - 1);
            }
        }
    }

    void anonymous_log(const char* format, ...)
    {
        BlockData* block = findBlock(format);

        va_list args;
        va_start(args, format);

        if (block == NULL)
        {
            int needed = vasnrintf(NULL, 0, format, args);
            block = registerBlock("", format, needed * 5);
        }

        if (block != NULL)
        {
            if (block->getFreeSpace() == 0)
            {
                block->resetItemAddr();
            }
            if (block->itemOffset != 0)
            {
                int used = snprintf((char*)block->getItemAddr(), block->getFreeSpace(), "%s", LINE_SEPERATOR);
                block->shiftItemAddr(used);
            }
            int needed = vsnprintf((char*)block->getItemAddr(), block->getFreeSpace(), format, args);
            if (needed > (int)block->getFreeSpace())
            {
                block->resetItemAddr();
                needed = vsnprintf((char*)block->getItemAddr(), block->getFreeSpace(), format, args);
            }
            block->shiftItemAddr(needed);
            if (block->getFreeSpace() > sizeof(ITEM_MARKER LINE_SEPERATOR))
            {
                memcpy((char*)block->getItemAddr(), ITEM_MARKER LINE_SEPERATOR, sizeof(ITEM_MARKER LINE_SEPERATOR) - 1);
            }
        }

        va_end(args);
    }

protected:
    BlockData* newBlock(const char* key, uint32_t size)
    {
        BlockData* block = new BlockData;
        block->capacity = size;
        block->buffer = allocateBlock(size);

        if (block->buffer != NULL && block->capacity >= size)
        {
            m_blocks[key] = block;
        }
        else
        {
            delete block;
            block = NULL;
        }
        return block;
    }

    BlockData* findBlock(const char* key)
    {
        auto iter = m_blocks.find(std::string(key));
        return iter != m_blocks.end() ? (*iter).second : NULL;
    }

    uint8_t* allocateBlock(uint32_t size)
    {
        uint8_t* buffer = NULL;
        if (m_allocated + size < m_capacity)
        {
            buffer = m_buffer + m_allocated;
            m_allocated += size;
        }
        return buffer;
    }

protected:
    const char* defaultFormater(const bool&) { return "%d"; }
    const char* defaultFormater(const char*&) { return "%s"; }
    const char* defaultFormater(const CONSTSTR&) { return "%s"; }

    const char* defaultFormater(const char&) { return "%c"; }
    const char* defaultFormater(const float&) { return "%f"; }
    const char* defaultFormater(const double&) { return "%f"; }

    const char* defaultFormater(const int8_t&) { return "%d"; }
    const char* defaultFormater(const int16_t&) { return "%d"; }
    const char* defaultFormater(const int32_t&) { return "%d"; }
#ifdef _WIN32
    const char* defaultFormater(const int64_t&) { return "%I64d"; }
#else
    const char* defaultFormater(const int64_t&) { return "%lld"; }
#endif // _WIN32

    const char* defaultFormater(const uint8_t&) { return "%u"; }
    const char* defaultFormater(const uint16_t&) { return "%u"; }
    const char* defaultFormater(const uint32_t&) { return "%u"; }
#ifdef _WIN32
    const char* defaultFormater(const uint64_t&) { return "%I64u"; }
#else
    const char* defaultFormater(const uint64_t&) { return "%llu"; }
#endif // _WIN32

    static int vasnrintf(char* buffer, size_t length, const char* format, va_list args)
    {
        va_list dupArgs;       // localize args copy

#ifdef __GNUC__
        va_copy(dupArgs, args); // Clone arguments for reuse by different call to vsprintf.
#else 
        dupArgs = args;        // Simply ptr copy for GCC compatibility
#endif

        int needed = vsnprintf(buffer, length, format, args);

        // NOTE: dupArgs on GCC platform is mangled by usage in v*printf() and cannot be reused.
#ifdef __GNUC__
        va_end(dupArgs); // cleanup 
#endif

        return needed;
    }

    static bool isFormater(const char* text)
    {
        bool findFormater = false;
        while (*text)
        {
            if ((*text) == '%')
            {
                findFormater = !findFormater;
            }
            else
            {
                if (findFormater)
                {
                    break;
                }
                findFormater = false;
            }
            ++text;
        }
        return findFormater;
    }
};

#endif // _BLOCK_LOGGER_SLEEPY_H_
