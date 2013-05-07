#ifndef _MAPFILE_HPP_
#define _MAPFILE_HPP_

#ifdef HAVE_MMAP
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

class MapFile {
    public:
        MapFile(void) {
            data = 0;
#ifdef HAVE_MMAP
            mmap_fd = -1;
#elif defined(_WIN32)
            hFile = 0;
            hFileMap = 0;
#endif
            }
        ~MapFile();
        bool open(const char *file_name, unsigned long file_size);
        inline char *begin(void) { return data; }
    private:
        char *data;
        unsigned long size;
#ifdef HAVE_MMAP
        int mmap_fd;
#elif defined(_WIN32)
        HANDLE hFile;
        HANDLE hFileMap;
#endif
};

inline bool MapFile::open(const char *file_name, unsigned long file_size)
{
    size=file_size;
#ifdef HAVE_MMAP
    if ((mmap_fd = ::open(file_name, O_RDONLY)) < 0) {
        return false;
    }
    data = (char *)mmap( 0, file_size, PROT_READ, MAP_SHARED, mmap_fd, 0);
    if ((void *)data == (void *)(-1)) {
        data=0;
        return false;
    }
#elif defined( _WIN32)
    hFile = CreateFile(file_name, GENERIC_READ, 0, 0, OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, 0);
    hFileMap = CreateFileMapping(hFile, 0, PAGE_READONLY, 0,
            file_size, 0);
    data = (char *)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, file_size);
#else
    return false;
#endif

    return true;
}

inline MapFile::~MapFile()
{
    if (!data)
        return;
#ifdef HAVE_MMAP
    munmap(data, size);
    ::close(mmap_fd);
#else
#  ifdef _WIN32
    UnmapViewOfFile(data);
    CloseHandle(hFileMap);
    CloseHandle(hFile);
#  endif
#endif
}

#endif//!_MAPFILE_HPP_
