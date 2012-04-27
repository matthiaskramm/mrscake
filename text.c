#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "text.h"
#include "stringpool.h"

#define CUTOFF_FRACTION 0.1

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

        sentence->string_size = strlen(text);
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
        word->idf = logf((float)num_rows / (float)word->occurences);
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

int compare_abs_word_score(const void*_score1, const void*_score2)
{
    const word_score_t*score1 = _score1;
    const word_score_t*score2 = _score2;
    if(fabs(score1->score) > fabs(score2->score))
        return -1;
    if(fabs(score1->score) < fabs(score2->score))
        return 1;
    return 0;
}

relevant_words_t* textcolumn_get_relevant_words(textcolumn_t*t, column_t*desired_response, category_t category, int max_words)
{
    relevant_words_t*r = calloc(1, sizeof(relevant_words_t));
    r->textcolumn = t;

    r->word_score = calloc(t->num_words, sizeof(word_score_t));
    int i;
    for(i=0;i<t->num_words;i++) {
        r->word_score[i].index = i;
    }

    int y;
    for(y=0;y<t->num_rows;y++)
    {
        int sign = (desired_response->entries[y].c == category)? 1 : -1;
        int i;
        sentence_t*sentence = &t->entries[y];
        for(i=0;i<sentence->num_word_counts;i++) {
            word_count_t*cnt = sentence->word_counts[i];
            float tf = cnt->count / (float)sentence->string_size;
            r->word_score[cnt->word->index].score += sign * tf * cnt->word->idf;
        }
    }
    if(max_words > t->num_words)
        max_words = t->num_words;

    qsort(r->word_score, t->num_words, sizeof(word_score_t), compare_abs_word_score);
    r->word_score_dict = dict_new(&ptr_type);
    for(i=0;i<max_words;i++) {
        int j = r->word_score[i].index;
        if(fabs(r->word_score[i].score) < fabs(r->word_score[0].score * CUTOFF_FRACTION))
            break;
        dict_put(r->word_score_dict, t->words[j], &r->word_score[i]);
    }
    r->num = i;
    return r;
}

column_t*textcolumn_expand(relevant_words_t*r, column_t*desired_response, category_t category)
{
    textcolumn_t*t = r->textcolumn;
    column_t*column = column_new(r->textcolumn->num_rows, CONTINUOUS);
    int y;
    for(y=0;y<t->num_rows;y++)
    {
        sentence_t*sentence = &t->entries[y];
        float value = 0.0f;
        int i;
        for(i=0;i<sentence->num_word_counts;i++) {
            word_count_t*cnt = sentence->word_counts[i];
            word_score_t*score = dict_lookup(r->word_score_dict, cnt->word);
            if(score) {
                value += cnt->count * cnt->word->idf * score->score / sentence->string_size;
            }

        }
        column->entries[y].f = value;
    }
    return column;
}
