/*
 * g++ -DHAVE_MMAP -g stardict2txt.cpp
 */
#include "mapfile.hpp"
#include <sys/stat.h>
#include <stdio.h>
#include <string>
#include <fstream>
using namespace std;

struct Index {
    char* m_sWord;
    unsigned int m_nOffset;
    unsigned int m_nSize;
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
int toDict(const char *txtFile,  MapFile& map_file, string& bookName) {
    char *idxdatabuffer=map_file.begin();
    char *p = idxdatabuffer;
    string fileName = txtFile;
    fileName.replace(fileName.length()-4,5,".dict");
    ofstream fDictData(fileName.c_str());
    fileName.replace(fileName.length()-5,4,".ifo");
    fileName.erase(fileName.length()-1);
    ofstream fIfo(fileName.c_str());
    fileName.replace(fileName.length()-4,4,".idx");
    ofstream fIdx(fileName.c_str());
    char *keyStart = ++p, *valueStart = 0;
    int keyLen = 0, offset = 0, wordCount = 0;
    for(;*p;p++) {
        if(*p == '#') {
            if(keyLen && *(p-1) == '\n') {
                if(valueStart) {
                    int expLen = p-valueStart-1;
                    int iOff = offset;
                    offset += expLen;

                    fDictData.write(valueStart, expLen);
                    fIdx.write(keyStart, keyLen);
                    fIdx.write("\0",1);
                    switchEndianness(&expLen);
                    switchEndianness(&iOff);
                    fIdx.write((char*)&iOff, 4);
                    fIdx.write((char*)&expLen, 4);

                    wordCount++;
                    keyLen = 0;
                    valueStart = 0;
                }
                keyStart = p+1;
            }
        } else if(*p == '\n') {
            if(keyStart && !valueStart) {
                keyLen = p-keyStart;
            }
        } else if(*p == ';') {
            if(keyLen && !valueStart) {
                valueStart = p+1;
            }
        }
    }
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
    fIfo << endl;
    fIfo.close();
}
int fromDict(const char *idxFile,  MapFile& map_file) {
    char *idxdatabuffer=map_file.begin();
    char *p = idxdatabuffer;
    Index aIdx;
    string sDictFile = idxFile;
    sDictFile.replace(sDictFile.length()-4,5,".dict");
    ifstream fDictData(sDictFile.c_str());
    sDictFile.replace(sDictFile.length()-5,4,".txt");
    sDictFile.erase(sDictFile.length()-1);
    ofstream fOut(sDictFile.c_str());
    char * pExplanation;
    unsigned int maxWordLen = 0;
    unsigned int maxExpLen = 0;
    string maxLenString;
    size_t wordLen = 0;
    while(p) {
        aIdx.m_sWord = p;
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
        fOut.write(aIdx.m_sWord,wordLen);
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
