#ifndef __text_h__
#define  __text_h__

#include "dataset.h"

typedef struct _word {
    const char*str;
    int occurences; // number of rows that have this word
} word_t;

typedef struct _word_occurence {
    word_t*word;
    int count; // number of times this word appears in this row&column cell
} word_occurence_t;

typedef struct _sentence {
    int num_words;
    word_occurence_t**words;
} sentence_t;

typedef struct _textcolumn {
    int num_words;
    word_t** words;
    int num_rows;
    sentence_t* entries;
} textcolumn_t;

textcolumn_t* textcolumn_from_column(column_t*column, int num_rows);

#endif
