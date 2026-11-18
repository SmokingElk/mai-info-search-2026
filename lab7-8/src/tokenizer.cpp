#include "../include/tokenizer.h"
#include "../include/utils.h"
#include <cstdlib>
#include <cstring>


static const char* STOP_WORDS[] = {
    "the", "and", "to", "of", "in", "for", "on", "with", "at", "by",
    "это", "и", "в", "на", "с", "по", "о", "у", "за", "из",
    "a", "an", "or", "but", "is", "are", "was", "were",
    "я", "ты", "он", "она", "оно", "мы", "вы", "они"
};

static const int STOP_WORDS_COUNT = sizeof(STOP_WORDS) / sizeof(STOP_WORDS[0]);

bool is_stop_word(const char* word) {
    for (int i = 0; i < STOP_WORDS_COUNT; i++) {
        if (strcmp(word, STOP_WORDS[i]) == 0) {
            return true;
        }
    }
    return false;
}

TokenArray tokenize_text(const char* text) {
    TokenArray result = {nullptr, 0};
    
    if (!text) return result;
    
    
    char* text_copy = strdup(text);
    if (!text_copy) return result;
    
    
    clean_string(text_copy);
    to_lowercase(text_copy);
    
    
    const char* delimiters = " \t\n\r";
    int token_count = 0;
    char** tokens = split_string(text_copy, delimiters, &token_count);
    
    free(text_copy);
    
    if (!tokens || token_count == 0) {
        return result;
    }
    
    
    char** filtered_tokens = (char**)malloc(token_count * sizeof(char*));
    if (!filtered_tokens) {
        free_string_array(tokens, token_count);
        return result;
    }
    
    int filtered_count = 0;
    for (int i = 0; i < token_count; i++) {
        if (tokens[i] && strlen(tokens[i]) > 1) { 
            filtered_tokens[filtered_count] = strdup(tokens[i]);
            filtered_count++;
        }
    }
    
    
    free_string_array(tokens, token_count);
    
    if (filtered_count == 0) {
        free(filtered_tokens);
        return result;
    }
    
    
    char** final_tokens = (char**)realloc(filtered_tokens, filtered_count * sizeof(char*));
    if (!final_tokens) {
        free_string_array(filtered_tokens, filtered_count);
        return result;
    }
    
    result.tokens = final_tokens;
    result.count = filtered_count;
    
    return result;
}

void free_tokens(TokenArray* tokens) {
    if (!tokens || !tokens->tokens) return;
    
    free_string_array(tokens->tokens, tokens->count);
    tokens->tokens = nullptr;
    tokens->count = 0;
}

TokenArray remove_stop_words(TokenArray* tokens) {
    TokenArray result = {nullptr, 0};
    
    if (!tokens || !tokens->tokens || tokens->count == 0) {
        return result;
    }
    
    
    char** filtered_tokens = (char**)malloc(tokens->count * sizeof(char*));
    if (!filtered_tokens) return result;
    
    int filtered_count = 0;
    for (int i = 0; i < tokens->count; i++) {
        if (!is_stop_word(tokens->tokens[i])) {
            filtered_tokens[filtered_count] = strdup(tokens->tokens[i]);
            filtered_count++;
        }
    }
    
    if (filtered_count == 0) {
        free(filtered_tokens);
        return result;
    }
    
    
    char** final_tokens = (char**)realloc(filtered_tokens, filtered_count * sizeof(char*));
    if (!final_tokens) {
        free_string_array(filtered_tokens, filtered_count);
        return result;
    }
    
    result.tokens = final_tokens;
    result.count = filtered_count;
    
    return result;
}