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
inline void ConvertEndian32( void * lpMem )
{
    unsigned char * p = (unsigned char*)lpMem;
    p[0] = p[0] ^ p[3];
    p[3] = p[0] ^ p[3];
    p[0] = p[0] ^ p[3];
    p[1] = p[1] ^ p[2];
    p[2] = p[1] ^ p[2];
    p[1] = p[1] ^ p[2];
}
int main(int argc,char** argv)
{
    if(argc == 1) {
        printf("usage:\t%s <dict_name>.idx\n"
                "\t<dict_name>.idx and <dict_name>.dict must be put under the same path.\n"
                "\t<dict_name>.dict can be extracted from <dict_name>.dict.dz by gzip.\n"
                ,
                argv[0]);
        return 1;
    }
    struct stat buf;
    if(stat(argv[1],&buf) == 0) {
        MapFile map_file;
        if(!map_file.open(argv[1], buf.st_size)) {
            return 1;
        }
        char *idxdatabuffer=map_file.begin();
        char *p = idxdatabuffer;
        Index aIdx;
        string sDictFile = argv[1];
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
            ConvertEndian32(&(aIdx.m_nOffset));
            p += 4;
            aIdx.m_nSize = *(unsigned int*)p;
            ConvertEndian32(&(aIdx.m_nSize));
            if(aIdx.m_nSize == 0)
                break;
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
        printf("%u %u %s\n",maxWordLen,maxExpLen,maxLenString.c_str());
        fOut.close();
    }
    return 0;
}
