/*
 * Copyright (c) 2013 Brook Hong
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
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
