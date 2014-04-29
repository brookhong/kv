#Command line dictionary tool -- `kv`

The `kv` can

* extract plain text from a dictionary of StarDict, for example,

    kv extract oxford.idx

* build a dictionary of StarDict from a plain text file, like,

    kv build oxford.txt

* query by keyword from a specified dictionary,

    kv query oxford.idx key

* httpd service under Windows

    kv.exe server oxford.idx

    Then you can access http://localhost:8080/ to query word, the feature can work with [a chrome extension](https://github.com/brookhong/kv.crx) to query word on web page.

The plain text file should be formated like this:

    #key1
    ;explanation of key1
    #key2
    ;explanation of key2
    more explanation of key2
    #key3
    ;explanation of key3
    #key4
    #key5
    ;explanation of key4 and key5
    more and more

If you have `#` in your values, you can use another key marker such as `_KEY_STARTER_`, then tell `kv` about it with option `-k`.
Explanations must be started with semicolon(;).

## Build Instructions
* LINUX/MAC : g++ -DHAVE_MMAP kv.cpp md5.cpp indexfile.cpp levenshtein.cpp
* WINDOWS   : cl -D_WIN32 kv.cpp md5.cpp indexfile.cpp levenshtein.cpp
