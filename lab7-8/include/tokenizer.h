#ifndef TOKENIZER_H
#define TOKENIZER_H


typedef struct {
    char** tokens;
    int count;
} TokenArray;


TokenArray tokenize_text(const char* text);


void free_tokens(TokenArray* tokens);


TokenArray remove_stop_words(TokenArray* tokens);

#endif