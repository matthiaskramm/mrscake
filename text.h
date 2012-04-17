#ifndef __text_h__
#define  __text_h__

#include "dataset.h"

typedef struct _word {
    int index;
    const char*str;
    int occurences; // number of rows that have this word
    float idf;
} word_t;

typedef struct _word_count {
    word_t*word;
    int count; // number of times this word appears in this row&column cell
} word_count_t;

typedef struct _sentence {
    int num_words;
    int num_word_counts;
    int string_size;
    word_count_t**word_counts;
} sentence_t;

typedef struct _textcolumn {
    int num_words;
    word_t** words;
    int num_rows;
    sentence_t* entries;
} textcolumn_t;

typedef struct _word_column {
    word_t*word;
} word_column_t;

typedef struct _word_score {
    int index;
    float score;
} word_score_t;

typedef struct _relevant_words {
    word_score_t*word_score;
    int num;
    dict_t*word_score_dict;
    textcolumn_t*textcolumn;
} relevant_words_t;

textcolumn_t* textcolumn_from_column(column_t*column, int num_rows);
void textcolumn_print(textcolumn_t*t);
relevant_words_t* textcolumn_get_relevant_words(textcolumn_t*t, column_t*desired_response, category_t category, int max_words);
column_t*textcolumn_expand(relevant_words_t*r, column_t*desired_response, category_t category);

#endif
