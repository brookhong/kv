#Command line dictionary tool -- `kv`

The `kv` can

* extract plain text from a dictionary of StarDict, for example,

    kv extract oxford.idx

* build a dictionary of StarDict from a plain text file, like,

    kv build oxford.txt

* query by keyword from a specified dictionary,

    kv query oxford.idx key


The plain text file should be formated like this:

    #key1
    ;explaination of key1
    #key2
    ;explaination of key2
    more explaination of key2
    #key3
    ;explaination of key3
    #key4
    #key5
    ;explaination of key4 and key5
    more and more
    #

If you have `#` in your values, you can use other key markers such as `&*#$#*`, then tell `kv` about it with option `-k`.

## Build Instructions
* LINUX/MAC : g++ -DHAVE_MMAP kv.cpp md5.cpp indexfile.cpp levenshtein.cpp
* WINDOWS   : cl -D_WIN32 kv.cpp md5.cpp indexfile.cpp levenshtein.cpp
