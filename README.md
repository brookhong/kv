Dictionary Compiler for StarDict

The `dc` can help to extract plain text from a dictionary of StarDict, for example,

    dc extract oxford.idx


Also can help to build a dictionary of StarDict from a plain text file, like,

    dc build oxford.txt


The plain text file should be formated like this:

    #key1
    ;explaination of key1
    #key2
    ;explaination of key2
    more explaination of key2
    #key3
    ;explaination of key1
    #key4
    ;explaination of key1
    #
