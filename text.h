#ifndef __text_h__
#define  __text_h__

#include "dataset.h"

typedef struct _word {
    char*chars;
    int occurences;
} word_t;

typedef struct _sentence {
    int num_words;
    int**words;
} sentence_t;

typedef struct _textcolumn {
    const char*name;
    bool is_text;

    word_t* words;
    sentence_t* entries;
} textcolumn_t;

#endif
