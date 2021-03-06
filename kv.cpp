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
 * LINUX    : g++ -DHAVE_MMAP kv.cpp md5.cpp indexfile.cpp levenshtein.cpp mongoose.c
 * WINDOWS  : cl -D_WIN32 kv.cpp md5.cpp indexfile.cpp levenshtein.cpp mongoose.c
 */
#include "mapfile.hpp"
#include "indexfile.h"
#include "md5.h"
#include <map>
#include <stdarg.h>

string gKeyMarker = "#";
string gPort = "8080";
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

bool createMapFile(const char *fileName, MapFile &map_file) {
    struct stat buf;
    return (stat(fileName, &buf) == 0 && map_file.open(fileName, buf.st_size));
}

void buildDict(const char *txtFile, string& bookName) {
    MapFile map_file;
    if(!createMapFile(txtFile, map_file)) {
        return;
    }
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
    size_t lenKeyMarker = gKeyMarker.length();
    p += lenKeyMarker;
    char *keyStart = p, *valueStart = 0;
    int keyLen = 0, offset = 0, wordCount = 0;
    map<char*, Index> idxMap;
    map<char*, int> synonyms;
    map<char*, int>::iterator itci;
    map<string, Index> digestMap;
    for(;;p++) {
        if(*p == '\n') {
            if(keyStart && !valueStart) {
                keyLen = p-keyStart;
                if(*(p+1) == '\r') {
                    keyLen -= 1;
                }
                string key(keyStart, keyLen);
                if(key.find("\r") != string::npos) {
                    printf("Invalid input file.\n");
                    return;
                }
            }
        } else if(*p == ';') {
            if(keyLen && !valueStart) {
                valueStart = p+1;
            }
        } else if(*p == '\0' || strncmp(p, gKeyMarker.c_str(), lenKeyMarker) == 0) {
            if(keyLen && *(p-1) == '\n') {
                if(valueStart) {
                    int expLen = p-valueStart-1;
                    int iOff = offset;

                    string value(valueStart, expLen);
                    string digest = md5(value);
                    if(digestMap.count(digest)) {
                        Index & idx = digestMap[digest];
                        expLen = idx.m_nSize;
                        iOff = idx.m_nOffset;
                    } else {
                        fDictData.write(valueStart, expLen);
                        offset += expLen;
                        digestMap[digest] = Index(0, expLen, iOff);
                    }
                    idxMap[keyStart] = Index(keyLen, expLen, iOff);
                    for(itci = synonyms.begin(); itci != synonyms.end(); itci++) {
                        idxMap[itci->first] = Index(itci->second, expLen, iOff);
                        wordCount++;
                    }

                    wordCount++;
                    keyLen = 0;
                    valueStart = 0;
                    synonyms.clear();
                } else {
                    synonyms[keyStart] = keyLen;
                }
                keyStart = p+lenKeyMarker;
                p += lenKeyMarker-1;
            }
            if(*p == '\0') {
                break;
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
            int cmp = i_strcmp(str.c_str(), s.c_str());
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
void extractDict(const char *idxFileName) {
    MapFile map_file;
    if(!createMapFile(idxFileName, map_file)) {
        return;
    }
    char *p = map_file.begin();
    Index aIdx;
    string sDictFile = idxFileName;
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
        fOut.write(gKeyMarker.c_str(),gKeyMarker.length());
        fOut.write(sWord,wordLen);
        fOut.write("\n",1);
        fOut.write(";",1);
        fOut.write(pExplanation,aIdx.m_nSize);
        fOut.write("\n",1);
        free(pExplanation);
    }
    fOut.write(gKeyMarker.c_str(),gKeyMarker.length());
    printf("Max Word Length\t\t: %u\n"
            "Max Explanation Length\t: %u\n"
            "Longest Word\t\t: %s\n",
            maxWordLen,maxExpLen,maxLenString.c_str());
    fOut.close();
}
std::string string_format(const std::string fmt, ...) {
    int size = 100;
    std::string str;
    va_list ap;
    while (1) {
        str.resize(size);
        va_start(ap, fmt);
        int n = vsnprintf((char *)str.c_str(), size, fmt.c_str(), ap);
        va_end(ap);
        if (n > -1 && n < size) {
            str.resize(n);
            return str;
        }
        if (n > -1)
            size = n + 1;
        else
            size *= 2;
    }
    return str;
}
string queryDict(const char *idxFileName, const char *keyword) {
    string result;
    string fileName = idxFileName;
    if(!file_exist(idxFileName) || strlen(idxFileName) < 4) {
        result = string(idxFileName) + " is not a valid idx file.\n";
    } else {
        fileName.replace(fileName.length()-4,4,".ifo");
        if(!file_exist(fileName.c_str())) {
            result = fileName + " not exists\n";
        } else {
            ifstream fIfo(fileName.c_str());
            char line[256];
            unsigned int wc = 0, idxFileSize = 0;
            while((wc == 0 || idxFileSize == 0) && !fIfo.eof()){
                fIfo.getline(line, 256);
                if(strncmp(line, "wordcount=", 10) == 0) {
                    sscanf(line, "wordcount=%u\n", &wc);
                } else if(strncmp(line, "idxfilesize=", 12) == 0) {
                    sscanf(line, "idxfilesize=%u\n", &idxFileSize);
                }
            }

            IndexFile idxFile;
            long cur = INVALID_INDEX;
            list<FuzzyResult> results;
            if( idxFile.load(idxFileName, wc, idxFileSize)
                    && (idxFile.lookup(keyword, cur)
                        || idxFile.lookupWithGrammar(keyword, cur)
                        || idxFile.lookupFuzzy(keyword, results)
                        || idxFile.lookupPartial(keyword, results)
                       ) ) {
                if(results.size() > 0) {
                    list<FuzzyResult>::iterator it;
                    for(it = results.begin(); it != results.end(); it++) {
                        string value, key;
                        key = idxFile.get_entry(it->idx, value);
                        result += string_format("\n%s(%d)\n%s\n", key.c_str(), it->distance, value.c_str());
                    }
                } else {
                    string value, key;
                    key = idxFile.get_entry(cur, value);
                    result = value;
                }
            }
        }
    }
    return result;
}
int showUsage() {
    printf( "kv -- a simple dict tool to build dict(StarDict), extract dict and query\n\n"
            "Build\n\tkv build [-t <title>] [-k <key marker>] <plain txt file>\n\n"
            "\tThe plain text file should be formated as below:\n"
            "--------------------------------------------------------------------------------\n"
            "#key1\n"
            ";explainations must be started with semicolon, explanation of key1\n"
            "#key2\n"
            ";explanation of key2\n"
            "more explanation of key2\n"
            "#key3\n"
            ";explanation of key3\n"
            "#key4\n"
            "#key5\n"
            ";explanation of key4 and key5\n"
            "more and more\n"
            "--------------------------------------------------------------------------------\n"
            "\tIf you have `#` in your values, you can use another key marker such as `_KEY_STARTER_`, then tell `kv` about it with option `-k`.\n"
            "\tExplanations must be started with semicolon(;).\n"
            "Extract\n\tkv extract <.idx file>\n\n"
            "Query\n\tkv query <.idx file> <keyword>\n\n"
            "Server\n\tkv server [-p <port>] <.idx file>\n\n"
            "Example\n\tkv build -k '_KEY_STARTER_' test.txt\n"
            "\tkv query ./test.idx key\n"
            "\tkv server -p 127.0.0.1:9078 ./test.idx\n\n"
            );
    return 1;
}

int httpServer(const char *idxFileName, const char *port);
int main(int argc,char** argv)
{
    if(argc < 2) {
        return showUsage();
    } else {
        int i = 2;
        string sBookName;
        while(i<argc) {
            if(argv[i][0] == '-') {
                switch (argv[i][1]) {
                    case 't':
                        if(++i < argc) {
                            sBookName = argv[i];
                        } else {
                            return showUsage();
                        }
                        break;
                    case 'k':
                        if(++i < argc) {
                            gKeyMarker = argv[i];
                        } else {
                            return showUsage();
                        }
                        break;
                    case 'p':
                        if(++i < argc) {
                            gPort = argv[i];
                        } else {
                            return showUsage();
                        }
                    default:
                        break;
                }
                i++;
            } else {
                break;
            }
        }
        if(i == argc) {
            return showUsage();
        }

        if(0 == strcmp(argv[1],"build")) {
            buildDict(argv[i], sBookName);
        } else if(0 == strcmp(argv[1],"extract")) {
            extractDict(argv[i]);
        } else if(0 == strcmp(argv[1],"query")) {
            if(i+1 < argc) {
                string ret = queryDict(argv[i], argv[i+1]);
                printf("%s\n", ret.c_str());
            } else {
                return showUsage();
            }
        } else if(0 == strcmp(argv[1],"server")) {
            return httpServer(argv[i], gPort.c_str());
        } else {
            return showUsage();
        }
    }
    return 0;
}
