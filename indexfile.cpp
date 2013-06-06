#include "indexfile.h"
const char *IndexFile::CACHE_MAGIC="StarDict's Cache, Version: 0.1";

void switchEndianness( void * lpMem )
{
    unsigned char * p = (unsigned char*)lpMem;
    p[0] = p[0] ^ p[3];
    p[3] = p[0] ^ p[3];
    p[0] = p[0] ^ p[3];
    p[1] = p[1] ^ p[2];
    p[2] = p[1] ^ p[2];
    p[1] = p[1] ^ p[2];
}

int i_strcmp(const char *s1, const char *s2)
{
#ifdef _WIN32
    int cmp = _stricmp(s1, s2);
#else
    int cmp = strcasecmp(s1, s2);
#endif
    return cmp;
}

int file_exist (char *filename) {
  struct stat buffer;
  return (stat (filename, &buffer) == 0);
}

void IndexFile::page_t::fill(char *data, int nent, long idx_)
{
    idx=idx_;
    char *p=data;
    long len;
    for (int i=0; i<nent; ++i) {
        entries[i].keystr=p;
        len=strlen(p);
        p+=len+1;
        entries[i].off = *(unsigned int*)p;
        switchEndianness(&(entries[i].off));
        p+=sizeof(unsigned int);
        entries[i].size = *(unsigned int*)p;
        switchEndianness(&(entries[i].size));
        p+=sizeof(unsigned int);
    }
}

IndexFile::~IndexFile()
{
    if (idxfile)
        fclose(idxfile);
}

inline const char *IndexFile::read_first_on_page_key(long page_idx)
{
    fseek(idxfile, wordoffset[page_idx], SEEK_SET);
    unsigned int page_size=wordoffset[page_idx+1]-wordoffset[page_idx];
    fread(wordentry_buf, min(sizeof(wordentry_buf), (size_t)page_size), 1, idxfile); //TODO: check returned values, deal with word entry that strlen>255.
    return wordentry_buf;
}

inline const char *IndexFile::get_first_on_page_key(long page_idx)
{
    if (page_idx<middle.idx) {
        if (page_idx==first.idx)
            return first.keystr.c_str();
        return read_first_on_page_key(page_idx);
    } else if (page_idx>middle.idx) {
        if (page_idx==last.idx)
            return last.keystr.c_str();
        return read_first_on_page_key(page_idx);
    } else
        return middle.keystr.c_str();
}

bool IndexFile::load_cache(const string& url)
{
    string cacheFile = url+".oft";

    struct stat idxstat, cachestat;
    if (stat(url.c_str(), &idxstat)!=0 || stat(cacheFile.c_str(), &cachestat)!=0)
        return false;
    if (cachestat.st_mtime<idxstat.st_mtime)
        return false;

    MapFile mf;
    if (!mf.open(cacheFile.c_str(), cachestat.st_size))
        return false;
    if (strncmp(mf.begin(), CACHE_MAGIC, strlen(CACHE_MAGIC))!=0)
        return false;
    memcpy(&wordoffset[0], mf.begin()+strlen(CACHE_MAGIC), wordoffset.size()*sizeof(wordoffset[0]));
    return true;
}

bool IndexFile::save_cache(const string& url)
{
    string cacheFile = url+".oft";
    FILE *out=fopen(cacheFile.c_str(), "wb");
    if (!out)
        return false;
    if (fwrite(CACHE_MAGIC, 1, strlen(CACHE_MAGIC), out)!=strlen(CACHE_MAGIC))
        return false;
    if (fwrite(&wordoffset[0], sizeof(wordoffset[0]), wordoffset.size(), out)!=wordoffset.size())
        return false;
    fclose(out);
    printf("save to cache %s\n", cacheFile.c_str());
    return true;
}

bool IndexFile::load(const string& url, unsigned long wc, unsigned long fsize)
{
    fullfilename = url;
    wordcount=wc;
    unsigned long npages=(wc-1)/ENTR_PER_PAGE+2;
    wordoffset.resize(npages);
    if (!load_cache(url)) {
        MapFile map_file;
        if (!map_file.open(url.c_str(), fsize))
            return false;
        const char *idxdatabuffer=map_file.begin();

        const char *p1 = idxdatabuffer;
        unsigned long index_size;
        unsigned int j=0;
        for (unsigned int i=0; i<wc; i++) {
            index_size=strlen(p1) +1 + 2*sizeof(unsigned int);
            if (i % ENTR_PER_PAGE==0) {
                wordoffset[j]=p1-idxdatabuffer;
                ++j;
            }
            p1 += index_size;
        }
        wordoffset[j]=p1-idxdatabuffer;
        if (!save_cache(url))
            fprintf(stderr, "cache update failed\n");
    }

    if (!(idxfile = fopen(url.c_str(), "rb"))) {
        wordoffset.resize(0);
        perror("fopen");
        return false;
    }

    first.assign(0, read_first_on_page_key(0));
    last.assign(wordoffset.size()-2, read_first_on_page_key(wordoffset.size()-2));
    middle.assign((wordoffset.size()-2)/2, read_first_on_page_key((wordoffset.size()-2)/2));
    real_last.assign(wc-1, get_key(wc-1));

    return true;
}

inline unsigned long IndexFile::load_page(long page_idx)
{
    unsigned long nentr=ENTR_PER_PAGE;
    if (page_idx==long(wordoffset.size()-2))
        if ((nentr=wordcount%ENTR_PER_PAGE)==0)
            nentr=ENTR_PER_PAGE;

    if (page_idx!=page.idx) {
        page_data.resize(wordoffset[page_idx+1]-wordoffset[page_idx]);
        fseek(idxfile, wordoffset[page_idx], SEEK_SET);
        fread(&page_data[0], 1, page_data.size(), idxfile);
        page.fill(&page_data[0], nentr, page_idx);
    }

    return nentr;
}

const char *IndexFile::get_key(long idx)
{
    load_page(idx/ENTR_PER_PAGE);
    long idx_in_page=idx%ENTR_PER_PAGE;
    wordentry_offset=page.entries[idx_in_page].off;
    wordentry_size=page.entries[idx_in_page].size;
    return page.entries[idx_in_page].keystr;
}

const char *IndexFile::get_entry(long idx, string &value)
{
    load_page(idx/ENTR_PER_PAGE);
    long idx_in_page=idx%ENTR_PER_PAGE;
    wordentry_offset=page.entries[idx_in_page].off;
    wordentry_size=page.entries[idx_in_page].size;

    size_t period = fullfilename.find_last_of(".");
    fullfilename.replace(period,fullfilename.length()-period,".dict");
    ifstream fDictData(fullfilename.c_str());
    char * pExplanation = (char*)malloc(wordentry_size);
    fDictData.seekg(wordentry_offset,ios::beg);
    fDictData.read(pExplanation,wordentry_size);
    value = string(pExplanation, wordentry_size);
    free(pExplanation);

    return page.entries[idx_in_page].keystr;
}

bool IndexFile::lookup(const char *str, long &idx)
{
    bool bFound=false;
    long iFrom;
    long iTo=wordoffset.size()-2;
    int cmpint;
    long iThisIndex;
    if (i_strcmp(str, first.keystr.c_str())<0) {
        idx = 0;
        return false;
    } else if (i_strcmp(str, real_last.keystr.c_str()) >0) {
        idx = INVALID_INDEX;
        return false;
    } else {
        iFrom=0;
        iThisIndex=0;
        while (iFrom<=iTo) {
            iThisIndex=(iFrom+iTo)/2;
            cmpint = i_strcmp(str, get_first_on_page_key(iThisIndex));
            if (cmpint>0)
                iFrom=iThisIndex+1;
            else if (cmpint<0)
                iTo=iThisIndex-1;
            else {
                bFound=true;
                break;
            }
        }
        if (!bFound)
            idx = iTo;    //prev
        else
            idx = iThisIndex;
    }
    if (!bFound) {
        unsigned long netr=load_page(idx);
        iFrom=1; // Needn't search the first word anymore.
        iTo=netr-1;
        iThisIndex=0;
        while (iFrom<=iTo) {
            iThisIndex=(iFrom+iTo)/2;
            cmpint = i_strcmp(str, page.entries[iThisIndex].keystr);
            if (cmpint>0)
                iFrom=iThisIndex+1;
            else if (cmpint<0)
                iTo=iThisIndex-1;
            else {
                bFound=true;
                break;
            }
        }
        idx*=ENTR_PER_PAGE;
        if (!bFound)
            idx += iFrom;    //next
        else
            idx += iThisIndex;
    } else {
        idx*=ENTR_PER_PAGE;
    }
    return bFound;
}
