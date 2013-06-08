#ifndef _INDEXFILE_HPP_
#define _INDEXFILE_HPP_
#include <string>
#include <vector>
#include <list>
#include <fstream>
#include <sys/stat.h>
#include "mapfile.hpp"
using namespace std;

const int INVALID_INDEX=-100;

void switchEndianness( void * lpMem );
int i_strcmp(const char *s1, const char *s2);
int file_exist(const char *filename);

class IndexFile {
    public:
        unsigned int wordentry_offset;
        unsigned int wordentry_size;
        IndexFile() : idxfile(NULL) {}
        ~IndexFile();
        bool load(const string& url, unsigned long wc, unsigned long fsize);
        const char *get_key(long idx);
        const char *get_entry(long idx, string &value);
        void get_data(long idx);
        const char *get_key_and_data(long idx);
        bool lookup(const char *str, long &idx);
        bool lookup2(const char *str, long &idx);

    private:
        static const int ENTR_PER_PAGE=32;
        static const char *CACHE_MAGIC;

        string fullfilename;
        vector<unsigned int> wordoffset;
        FILE *idxfile;
        unsigned long wordcount;

        char wordentry_buf[256+sizeof(unsigned int)*2]; // The length of "word_str" should be less than 256. See src/tools/DICTFILE_FORMAT.
        struct index_entry {
            long idx;
            string keystr;
            void assign(long i, const string& str) {
                idx=i;
                keystr.assign(str);
            }
        };
        index_entry first, last, middle, real_last;

        struct page_entry {
            char *keystr;
            unsigned int off, size;
        };
        vector<char> page_data;
        struct page_t {
            long idx;
            page_entry entries[ENTR_PER_PAGE];

            page_t(): idx(-1) {}
            void fill(char *data, int nent, long idx_);
        } page;
        unsigned long load_page(long page_idx);
        const char *read_first_on_page_key(long page_idx);
        const char *get_first_on_page_key(long page_idx);
        bool load_cache(const string& url);
        bool save_cache(const string& url);
};
#endif
