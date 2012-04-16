#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "text.h"
#include "stringpool.h"

textcolumn_t* textcolumn_from_column(column_t*column, int num_rows)
{
    assert(column->type == TEXT);
    int y;

    textcolumn_t* textcolumn = calloc(1, sizeof(textcolumn_t));
    textcolumn->num_rows = num_rows;
    textcolumn->entries = calloc(num_rows, sizeof(sentence_t));

    dict_t*words = dict_new(&charptr_type);

    for(y=0;y<num_rows;y++)
    {
        sentence_t*sentence = &textcolumn->entries[y];

        dict_t*occurences = dict_new(&charptr_type);
        const char*text = column->entries[y].text;
        const char*p = text;
        while(*p) {
            while(*p && strchr(" \t\r\n\f", *p)) {
                p++;
            }
            if(!*p)
                break;
            const char*word_start = p;
            while(*p && !strchr(" \t\r\n\f", *p)) {
                p++;
            }
            const char*word_end = p;

            const char*str = register_string_n(word_start, word_end-word_start);
            word_t*word = dict_lookup(words, str);
            if(!word) {
                word = calloc(1, sizeof(word_t));
                word->str = str;
                dict_put(words, str, word);
            }

            word_occurence_t* occ = dict_lookup(occurences, str);
            if(!occ) {
                occ = calloc(1, sizeof(word_occurence_t));
                occ->word = word;
            }
            occ->count++;
            if(occ->count == 1) {
                word->occurences++;
            }
        }

        sentence->num_words = occurences->num;
        sentence->words = calloc(sentence->num_words, sizeof(word_occurence_t*));
        int i = 0;
        DICT_ITERATE_ITEMS(occurences, char*, str, word_occurence_t*, occ) {
            sentence->words[i++] = occ;
        }

        dict_destroy(occurences);
    }

    textcolumn->num_words = words->num;
    textcolumn->words = calloc(textcolumn->num_words, sizeof(word_t*));
    int i = 0;
    DICT_ITERATE_ITEMS(words, char*, str, word_t*, word) {
        textcolumn->words[i++] = word;
    }
    return textcolumn;
}

void textcolumn_print(textcolumn_t*t)
{
    printf("words: ");
    int i;
    for(i=0;i<t->num_words;i++) {
        if(i)
            printf(" ");
        printf("%s(%d)", t->words[i]->str, t->words[i]->occurences);
    }
    printf("\n");

    int y;
    for(y=0;y<t->num_rows;y++)
    {
        sentence_t*sentence = &t->entries[y];
        printf("[%d] ", y);
        int i;
        for(i=0;i<sentence->num_words;i++) {
            word_occurence_t*occ = sentence->words[i];
            if(i)
                printf(" ");
            printf("%s(%d)", occ->word->str, occ->count);
        }
        printf("\n");
    }
}
