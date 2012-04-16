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
        int word_count = 0;
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
                word->index = words->num;
                dict_put(words, str, word);
            }

            word_count_t* occ = dict_lookup(occurences, str);
            if(!occ) {
                occ = calloc(1, sizeof(word_count_t));
                occ->word = word;
                dict_put(occurences, str, occ);
            }
            occ->count++;
            if(occ->count == 1) {
                word->occurences++;
            }
            word_count++;
        }

        sentence->num_words = word_count;
        sentence->num_word_counts = occurences->num;
        sentence->word_counts = calloc(sentence->num_word_counts, sizeof(word_count_t*));
        int i = 0;
        DICT_ITERATE_ITEMS(occurences, char*, str, word_count_t*, occ) {
            sentence->word_counts[i++] = occ;
        }

        dict_destroy(occurences);
    }

    textcolumn->num_words = words->num;
    textcolumn->words = calloc(textcolumn->num_words, sizeof(word_t*));
    int i = 0;
    DICT_ITERATE_ITEMS(words, char*, str, word_t*, word) {
        textcolumn->words[word->index] = word;
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
        for(i=0;i<sentence->num_word_counts;i++) {
            word_count_t*occ = sentence->word_counts[i];
            if(i)
                printf(" ");
            printf("%s(%d)", occ->word->str, occ->count);
        }
        printf("\n");
    }
}

typedef struct _word_score {
    int index;
    float score;
} word_score_t;

int compare_word_score(const void*_score1, const void*_score2)
{
    const word_score_t*score1 = _score1;
    const word_score_t*score2 = _score2;
    if(score1->score > score2->score)
        return -1;
    if(score1->score < score2->score)
        return 1;
    return 0;
}

column_t* textcolumn_baysiate(textcolumn_t*t, column_t*desired_response, category_t category)
{
    word_score_t*word_score = calloc(t->num_words, sizeof(word_score_t));
    int i;
    for(i=0;i<t->num_words;i++) {
        word_score[i].index = i;
    }

    int y;
    for(y=0;y<t->num_rows;y++)
    {
        int sign = (desired_response->entries[y].c == category)? 1 : -1;
        int i;
        sentence_t*sentence = &t->entries[y];
        for(i=0;i<sentence->num_word_counts;i++) {
            word_count_t*occ = sentence->word_counts[i];
            float tf = occ->count / (float)sentence->num_word_counts;
            float idf = occ->word->occurences / (float)t->num_rows;
            float tf_idf = tf / idf;
            word_score[occ->word->index].score += sign * tf_idf;
        }
    }

    qsort(word_score, t->num_words, sizeof(word_score_t), compare_word_score);
    for(i=0;i<t->num_words;i++) {
        int j = word_score[i].index;
        printf("%s %f\n", t->words[j]->str, word_score[i].score);
    }
}
