#ifndef BOOLEAN_INDEX_H
#define BOOLEAN_INDEX_H
#include "document_parser.h"

typedef struct {
    int* positions;
    int count;
    int capacity;
} PositionList;


typedef struct {
    char* term;
    int* doc_ids;
    PositionList* positions; 
    int doc_count;
    int capacity;
} IndexEntry;


typedef struct {
    IndexEntry* entries;
    int count;
    int capacity;
} BooleanIndex;



void init_index(BooleanIndex* index, int initial_capacity);

void add_to_index(BooleanIndex* index, const char* term, int doc_id, int position);

IndexEntry* find_term(BooleanIndex* index, const char* term);

int* intersect_sorted_arrays(int* arr1, int count1, int* arr2, int count2, int* result_count);

int* boolean_and(BooleanIndex* index, const char* term1, const char* term2, int* result_count);

int* boolean_or(BooleanIndex* index, const char* term1, const char* term2, int* result_count);

int* boolean_not(BooleanIndex* index, const char* term, DocumentCollection* docs, int* result_count);

int* phrase_search(BooleanIndex* index, const char* phrase, int* result_count);

void save_index(BooleanIndex* index, const char* filename);

BooleanIndex* load_index(const char* filename);

void free_index(BooleanIndex* index);

#endif