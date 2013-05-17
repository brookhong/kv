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
 *
 * BUILD INSTRUCTIONS
 * LINUX    : g++ -DHAVE_MMAP -g dc.cpp
 * WINDOWS  : cl -D_WIN32 dc.cpp
 */
#include "mapfile.hpp"
#include <sys/stat.h>
#include <stdio.h>
#include <string>
#include <map>
#include <fstream>
using namespace std;

struct Index {
    int m_nKeyLen;
    int m_nOffset;
    int m_nSize;
    Index() {
        m_nKeyLen = 0;
        m_nSize = 0;
        m_nOffset = 0;
    }
    Index(int kLen, int eLen, int eOff) {
        m_nKeyLen = kLen;
        m_nSize = eLen;
        m_nOffset = eOff;
    }
};
inline void switchEndianness( void * lpMem )
{
    unsigned char * p = (unsigned char*)lpMem;
    p[0] = p[0] ^ p[3];
    p[3] = p[0] ^ p[3];
    p[0] = p[0] ^ p[3];
    p[1] = p[1] ^ p[2];
    p[2] = p[1] ^ p[2];
    p[1] = p[1] ^ p[2];
}
void toDict(const char *txtFile,  MapFile& map_file, string& bookName) {
    char *p = map_file.begin();
    string fileName = txtFile;
    size_t period = fileName.find_last_of(".");
    fileName.replace(period,fileName.length()-period,".dict");
    ofstream fDictData(fileName.c_str(), ios_base::binary);
    fileName.replace(fileName.length()-5,4,".ifo");
    fileName.erase(fileName.length()-1);
    ofstream fIfo(fileName.c_str(), ios_base::binary);
    fileName.replace(fileName.length()-4,4,".idx");
    ofstream fIdx(fileName.c_str(), ios_base::binary);
    char *keyStart = ++p, *valueStart = 0;
    int keyLen = 0, offset = 0, wordCount = 0;
    map<char*, Index> idxMap;
    for(;*p;p++) {
        if(*p == '#') {
            if(keyLen && *(p-1) == '\n') {
                if(valueStart) {
                    int expLen = p-valueStart-1;
                    int iOff = offset;
                    offset += expLen;

                    fDictData.write(valueStart, expLen);
                    idxMap[keyStart] = Index(keyLen, expLen, iOff);

                    wordCount++;
                    keyLen = 0;
                    valueStart = 0;
                }
                keyStart = p+1;
            }
        } else if(*p == '\n') {
            if(keyStart && !valueStart) {
                keyLen = p-keyStart;
                if(*(p+1) == '\r') {
                    keyLen -= 1;
                }
            }
        } else if(*p == ';') {
            if(keyLen && !valueStart) {
                valueStart = p+1;
            }
        }
    }
    char** idxVector = (char**)malloc(wordCount*sizeof(char*));
    int* lenVector = (int*)malloc(wordCount*sizeof(int));
    map<char*, Index>::iterator it;
    offset = 0;
    for(it = idxMap.begin(); it != idxMap.end(); it++) {
        string str(it->first, it->second.m_nKeyLen);
        int start = 0, pos, end = offset-1;
        while(start<=end) {
            pos = (start+end)/2;
            string s(idxVector[pos], lenVector[pos]);
            int cmp = strcasecmp(str.c_str(), s.c_str());
            if(cmp < 0) {
                end = pos-1;
            } else {
                start = pos+1;
            }
        }
        for(int i = offset; i > start; i--) {
            idxVector[i] = idxVector[i-1];
            lenVector[i] = lenVector[i-1];
        }
        idxVector[start] = it->first;
        lenVector[start] = it->second.m_nKeyLen;
        offset++;
    }
    for(int i = 0; i < wordCount; i++) {
        keyStart = idxVector[i];
        Index aIdx = idxMap[keyStart];
        fIdx.write(keyStart, aIdx.m_nKeyLen);
        fIdx.write("\0",1);
        switchEndianness(&(aIdx.m_nSize));
        switchEndianness(&(aIdx.m_nOffset));
        fIdx.write((char*)&aIdx.m_nOffset, 4);
        fIdx.write((char*)&aIdx.m_nSize, 4);
    }
    free(idxVector);
    free(lenVector);
    fIdx.close();
    struct stat buf;
    stat(fileName.c_str(), &buf);
    fIfo << "StarDict's dict ifo file\nversion=2.4.2\n";
    fIfo << "wordcount=";
    fIfo << wordCount;
    fIfo << "\nidxfilesize=";
    fIfo << buf.st_size;
    fIfo << "\nbookname=";
    if(bookName == "") {
        bookName = txtFile;
    }
    fIfo << bookName;
    fIfo << "\nauthor=brook hong";
    fIfo << "\nwebsite=https://github.com/brookhong";

    time_t rawtime;
    struct tm * timeinfo;
    char sDate[80];
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    strftime (sDate,80,"%Y.%m.%d %H:%M:%S",timeinfo);

    fIfo << "\ndate=";
    fIfo << sDate;
    fIfo << "\nsametypesequence=m";
    fIfo << endl;
    fIfo.close();
}
void fromDict(const char *idxFile,  MapFile& map_file) {
    char *p = map_file.begin();
    Index aIdx;
    string sDictFile = idxFile;
    sDictFile.replace(sDictFile.length()-4,5,".dict");
    ifstream fDictData(sDictFile.c_str());
    sDictFile.replace(sDictFile.length()-5,4,".txt");
    sDictFile.erase(sDictFile.length()-1);
    ofstream fOut(sDictFile.c_str(), ios_base::binary);
    char * pExplanation;
    unsigned int maxWordLen = 0;
    unsigned int maxExpLen = 0;
    string maxLenString;
    size_t wordLen = 0;
    char * sWord = 0;
    while(p) {
        sWord = p;
        wordLen = strlen(p);
        if(wordLen>maxWordLen) {
            maxWordLen = wordLen;
            maxLenString = p;
        }
        p += wordLen+1;
        aIdx.m_nOffset = *(unsigned int*)p;
        switchEndianness(&(aIdx.m_nOffset));
        p += 4;
        aIdx.m_nSize = *(unsigned int*)p;
        switchEndianness(&(aIdx.m_nSize));
        if(aIdx.m_nSize == 0) {
            break;
        }
        p += 4;
        maxExpLen = (aIdx.m_nSize>maxExpLen)?aIdx.m_nSize:maxExpLen;
        pExplanation = (char*)malloc(aIdx.m_nSize);
        fDictData.seekg(aIdx.m_nOffset,ios::beg);
        fDictData.read(pExplanation,aIdx.m_nSize);
        fOut.write("#",1);
        fOut.write(sWord,wordLen);
        fOut.write("\n",1);
        fOut.write(";",1);
        fOut.write(pExplanation,aIdx.m_nSize);
        fOut.write("\n",1);
        free(pExplanation);
    }
    fOut.write("#",1);
    printf("Max Word Length\t\t: %u\n"
            "Max Explanation Length\t: %u\n"
            "Longest Word\t\t: %s\n",
            maxWordLen,maxExpLen,maxLenString.c_str());
    fOut.close();
}
int main(int argc,char** argv)
{
    if(argc < 2) {
        printf("usage:\t%s <build [-t <title>] dict.txt | extract dict.idx>\n"
                "\t<dict_name>.idx and <dict_name>.dict must be put under the same path.\n"
                "\t<dict_name>.dict can be extracted from <dict_name>.dict.dz by gzip.\n"
                ,
                argv[0]);
        return 1;
    } else {
        int i = 2;
        string sBookName;
        while(i<argc) {
            if(argv[i][0] == '-') {
                switch (argv[i][1]) {
                    case 't':
                        sBookName = argv[++i];
                        break;
                    default:
                        break;
                }
                i++;
            } else {
                break;
            }
        }
        struct stat buf;
        if(stat(argv[i],&buf) == 0) {
            MapFile map_file;
            if(!map_file.open(argv[i], buf.st_size)) {
                return 1;
            }
            if(0 == strcmp(argv[1],"build")) {
                toDict(argv[i], map_file, sBookName);
            } else if(0 == strcmp(argv[1],"extract")) {
                fromDict(argv[i], map_file);
            }
        }
    }
    return 0;
}
