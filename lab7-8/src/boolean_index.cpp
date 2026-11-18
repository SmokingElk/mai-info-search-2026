#include "../include/boolean_index.h"
#include "../include/utils.h"
#include "../include/document_parser.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>


unsigned int hash_string(const char* str) {
    unsigned int hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    
    return hash;
}

void init_index(BooleanIndex* index, int initial_capacity) {
    index->entries = (IndexEntry*)malloc(initial_capacity * sizeof(IndexEntry));
    index->count = 0;
    index->capacity = initial_capacity;
    
    for (int i = 0; i < initial_capacity; i++) {
        index->entries[i].term = nullptr;
        index->entries[i].doc_ids = nullptr;
        index->entries[i].positions = nullptr;
        index->entries[i].doc_count = 0;
        index->entries[i].capacity = 0;
    }
}

void expand_entry_capacity(IndexEntry* entry) {
    if (!entry) return;
    
    int new_capacity = entry->capacity == 0 ? 4 : entry->capacity * 2;
    
    
    int* new_doc_ids = (int*)realloc(entry->doc_ids, new_capacity * sizeof(int));
    if (!new_doc_ids) return;
    entry->doc_ids = new_doc_ids;
    
    
    PositionList* new_positions = (PositionList*)realloc(entry->positions, new_capacity * sizeof(PositionList));
    if (!new_positions) {
        
        entry->doc_ids = (int*)realloc(entry->doc_ids, entry->capacity * sizeof(int));
        return;
    }
    entry->positions = new_positions;
    
    
    for (int i = entry->capacity; i < new_capacity; i++) {
        entry->positions[i].positions = nullptr;
        entry->positions[i].count = 0;
        entry->positions[i].capacity = 0;
    }
    
    entry->capacity = new_capacity;
}

void add_to_index(BooleanIndex* index, const char* term, int doc_id, int position) {
    if (!index || !term) return;
    
    
    IndexEntry* entry = nullptr;
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].term, term) == 0) {
            entry = &index->entries[i];
            break;
        }
    }
    
    
    if (!entry) {
        
        if (index->count >= index->capacity) {
            int new_capacity = index->capacity * 2;
            IndexEntry* new_entries = (IndexEntry*)realloc(index->entries, new_capacity * sizeof(IndexEntry));
            if (!new_entries) return;
            
            index->entries = new_entries;
            index->capacity = new_capacity;
            
            
            for (int i = index->count; i < new_capacity; i++) {
                index->entries[i].term = nullptr;
                index->entries[i].doc_ids = nullptr;
                index->entries[i].positions = nullptr;
                index->entries[i].doc_count = 0;
                index->entries[i].capacity = 0;
            }
        }
        
        entry = &index->entries[index->count];
        entry->term = strdup(term);
        entry->doc_ids = nullptr;
        entry->positions = nullptr;
        entry->doc_count = 0;
        entry->capacity = 0;
        index->count++;
    }
    
    
    int doc_index = -1;
    for (int i = 0; i < entry->doc_count; i++) {
        if (entry->doc_ids[i] == doc_id) {
            doc_index = i;
            break;
        }
    }
    
    
    if (doc_index == -1) {
        
        if (entry->doc_count >= entry->capacity) {
            expand_entry_capacity(entry);
        }
        
        doc_index = entry->doc_count;
        entry->doc_ids[doc_index] = doc_id;
        
        
        entry->positions[doc_index].positions = (int*)malloc(4 * sizeof(int));
        entry->positions[doc_index].count = 0;
        entry->positions[doc_index].capacity = 4;
        
        entry->doc_count++;
    }
    
    
    PositionList* pos_list = &entry->positions[doc_index];
    
    if (pos_list->count >= pos_list->capacity) {
        int new_capacity = pos_list->capacity * 2;
        int* new_positions = (int*)realloc(pos_list->positions, new_capacity * sizeof(int));
        if (!new_positions) return;
        
        pos_list->positions = new_positions;
        pos_list->capacity = new_capacity;
    }
    
    pos_list->positions[pos_list->count] = position;
    pos_list->count++;
}

IndexEntry* find_term(BooleanIndex* index, const char* term) {
    if (!index || !term) return nullptr;
    
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].term, term) == 0) {
            return &index->entries[i];
        }
    }
    
    return nullptr;
}


int* intersect_sorted_arrays(int* arr1, int count1, int* arr2, int count2, int* result_count) {
    if (!arr1 || !arr2 || count1 == 0 || count2 == 0) {
        *result_count = 0;
        return nullptr;
    }
    
    int max_result = count1 < count2 ? count1 : count2;
    int* result = (int*)malloc(max_result * sizeof(int));
    if (!result) {
        *result_count = 0;
        return nullptr;
    }
    
    int i = 0, j = 0, k = 0;
    while (i < count1 && j < count2) {
        if (arr1[i] < arr2[j]) {
            i++;
        } else if (arr1[i] > arr2[j]) {
            j++;
        } else {
            result[k++] = arr1[i];
            i++;
            j++;
        }
    }
    
    *result_count = k;
    
    
    if (k < max_result) {
        int* resized = (int*)realloc(result, k * sizeof(int));
        if (resized) {
            result = resized;
        }
    }
    
    return result;
}


int* union_sorted_arrays(int* arr1, int count1, int* arr2, int count2, int* result_count) {
    int max_result = count1 + count2;
    int* result = (int*)malloc(max_result * sizeof(int));
    if (!result) {
        *result_count = 0;
        return nullptr;
    }
    
    int i = 0, j = 0, k = 0;
    while (i < count1 && j < count2) {
        if (arr1[i] < arr2[j]) {
            result[k++] = arr1[i++];
        } else if (arr1[i] > arr2[j]) {
            result[k++] = arr2[j++];
        } else {
            result[k++] = arr1[i];
            i++;
            j++;
        }
    }
    
    while (i < count1) {
        result[k++] = arr1[i++];
    }
    
    while (j < count2) {
        result[k++] = arr2[j++];
    }
    
    *result_count = k;
    
    
    if (k < max_result) {
        int* resized = (int*)realloc(result, k * sizeof(int));
        if (resized) {
            result = resized;
        }
    }
    
    return result;
}

int* boolean_and(BooleanIndex* index, const char* term1, const char* term2, int* result_count) {
    *result_count = 0;
    
    if (!index || !term1 || !term2) return nullptr;
    
    
    IndexEntry* entry1 = find_term(index, term1);
    IndexEntry* entry2 = find_term(index, term2);
    
    if (!entry1 || !entry2) return nullptr;
    
    
    return intersect_sorted_arrays(entry1->doc_ids, entry1->doc_count, 
                                   entry2->doc_ids, entry2->doc_count, 
                                   result_count);
}

int* boolean_or(BooleanIndex* index, const char* term1, const char* term2, int* result_count) {
    *result_count = 0;
    
    if (!index || !term1 || !term2) return nullptr;
    
    
    IndexEntry* entry1 = find_term(index, term1);
    IndexEntry* entry2 = find_term(index, term2);
    
    if (!entry1 && !entry2) return nullptr;
    
    
    if (!entry1) {
        int* result = (int*)malloc(entry2->doc_count * sizeof(int));
        if (!result) return nullptr;
        
        memcpy(result, entry2->doc_ids, entry2->doc_count * sizeof(int));
        *result_count = entry2->doc_count;
        return result;
    }
    
    if (!entry2) {
        int* result = (int*)malloc(entry1->doc_count * sizeof(int));
        if (!result) return nullptr;
        
        memcpy(result, entry1->doc_ids, entry1->doc_count * sizeof(int));
        *result_count = entry1->doc_count;
        return result;
    }
    
    
    return union_sorted_arrays(entry1->doc_ids, entry1->doc_count,
                               entry2->doc_ids, entry2->doc_count,
                               result_count);
}

int* boolean_not(BooleanIndex* index, const char* term, DocumentCollection* docs, int* result_count) {
    *result_count = 0;
    
    if (!index || !term || !docs) return nullptr;
    
    
    IndexEntry* entry = find_term(index, term);
    
    if (!entry) {
        
        int* result = (int*)malloc(docs->count * sizeof(int));
        if (!result) return nullptr;
        
        for (int i = 0; i < docs->count; i++) {
            result[i] = docs->documents[i].id;
        }
        
        *result_count = docs->count;
        return result;
    }
    
    
    int* all_docs = (int*)malloc(docs->count * sizeof(int));
    if (!all_docs) return nullptr;
    
    for (int i = 0; i < docs->count; i++) {
        all_docs[i] = docs->documents[i].id;
    }
    
    
    int max_result = docs->count;
    int* result = (int*)malloc(max_result * sizeof(int));
    if (!result) {
        free(all_docs);
        return nullptr;
    }
    
    int k = 0;
    for (int i = 0; i < docs->count; i++) {
        bool found = false;
        for (int j = 0; j < entry->doc_count; j++) {
            if (all_docs[i] == entry->doc_ids[j]) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            result[k++] = all_docs[i];
        }
    }
    
    free(all_docs);
    *result_count = k;
    
    
    if (k < max_result) {
        int* resized = (int*)realloc(result, k * sizeof(int));
        if (resized) {
            result = resized;
        }
    }
    
    return result;
}

int* phrase_search(BooleanIndex* index, const char* phrase, int* result_count) {
    *result_count = 0;
    
    if (!index || !phrase) return nullptr;
    
    
    int token_count = 0;
    char** tokens = split_string(phrase, " ", &token_count);
    
    if (!tokens || token_count < 2) {
        if (tokens) free_string_array(tokens, token_count);
        return nullptr;
    }
    
    
    int* current_result = nullptr;
    int current_count = 0;
    
    for (int i = 0; i < token_count; i++) {
        IndexEntry* entry = find_term(index, tokens[i]);
        
        if (!entry) {
            
            if (current_result) free(current_result);
            free_string_array(tokens, token_count);
            *result_count = 0;
            return nullptr;
        }
        
        if (i == 0) {
            
            current_result = (int*)malloc(entry->doc_count * sizeof(int));
            if (!current_result) {
                free_string_array(tokens, token_count);
                return nullptr;
            }
            
            memcpy(current_result, entry->doc_ids, entry->doc_count * sizeof(int));
            current_count = entry->doc_count;
        } else {
            
            int* new_result = intersect_sorted_arrays(
                current_result, current_count,
                entry->doc_ids, entry->doc_count,
                &current_count
            );
            
            free(current_result);
            current_result = new_result;
            
            if (!current_result || current_count == 0) {
                break;
            }
        }
    }
    
    free_string_array(tokens, token_count);
    
    if (!current_result || current_count == 0) {
        *result_count = 0;
        return nullptr;
    }
    
    
    
    
    *result_count = current_count;
    return current_result;
}

void save_index(BooleanIndex* index, const char* filename) {
    if (!index || !filename) return;
    
    FILE* file = fopen(filename, "wb");
    if (!file) return;
    
    
    fwrite(&index->count, sizeof(int), 1, file);
    
    for (int i = 0; i < index->count; i++) {
        IndexEntry* entry = &index->entries[i];
        
        
        int term_len = strlen(entry->term);
        fwrite(&term_len, sizeof(int), 1, file);
        fwrite(entry->term, sizeof(char), term_len, file);
        
        
        fwrite(&entry->doc_count, sizeof(int), 1, file);
        
        
        for (int j = 0; j < entry->doc_count; j++) {
            fwrite(&entry->doc_ids[j], sizeof(int), 1, file);
            
            
            fwrite(&entry->positions[j].count, sizeof(int), 1, file);
            
            
            if (entry->positions[j].count > 0) {
                fwrite(entry->positions[j].positions, sizeof(int), 
                       entry->positions[j].count, file);
            }
        }
    }
    
    fclose(file);
}

BooleanIndex* load_index(const char* filename) {
    if (!filename) return nullptr;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return nullptr;
    
    BooleanIndex* index = (BooleanIndex*)malloc(sizeof(BooleanIndex));
    if (!index) {
        fclose(file);
        return nullptr;
    }
    
    
    int entry_count;
    if (fread(&entry_count, sizeof(int), 1, file) != 1) {
        free(index);
        fclose(file);
        return nullptr;
    }
    
    init_index(index, entry_count);
    index->count = entry_count;
    
    for (int i = 0; i < entry_count; i++) {
        IndexEntry* entry = &index->entries[i];
        
        
        int term_len;
        if (fread(&term_len, sizeof(int), 1, file) != 1) {
            free_index(index);
            fclose(file);
            return nullptr;
        }
        
        entry->term = (char*)malloc(term_len + 1);
        if (!entry->term) {
            free_index(index);
            fclose(file);
            return nullptr;
        }
        
        if (fread(entry->term, sizeof(char), term_len, file) != (size_t)term_len) {
            free_index(index);
            fclose(file);
            return nullptr;
        }
        entry->term[term_len] = '\0';
        
        
        if (fread(&entry->doc_count, sizeof(int), 1, file) != 1) {
            free_index(index);
            fclose(file);
            return nullptr;
        }
        
        entry->capacity = entry->doc_count;
        
        if (entry->doc_count > 0) {
            
            entry->doc_ids = (int*)malloc(entry->doc_count * sizeof(int));
            entry->positions = (PositionList*)malloc(entry->doc_count * sizeof(PositionList));
            
            if (!entry->doc_ids || !entry->positions) {
                free_index(index);
                fclose(file);
                return nullptr;
            }
            
            for (int j = 0; j < entry->doc_count; j++) {
                
                if (fread(&entry->doc_ids[j], sizeof(int), 1, file) != 1) {
                    free_index(index);
                    fclose(file);
                    return nullptr;
                }
                
                
                int pos_count;
                if (fread(&pos_count, sizeof(int), 1, file) != 1) {
                    free_index(index);
                    fclose(file);
                    return nullptr;
                }
                
                entry->positions[j].count = pos_count;
                entry->positions[j].capacity = pos_count;
                
                if (pos_count > 0) {
                    entry->positions[j].positions = (int*)malloc(pos_count * sizeof(int));
                    if (!entry->positions[j].positions) {
                        free_index(index);
                        fclose(file);
                        return nullptr;
                    }
                    
                    if (fread(entry->positions[j].positions, sizeof(int), pos_count, file) != (size_t)pos_count) {
                        free_index(index);
                        fclose(file);
                        return nullptr;
                    }
                } else {
                    entry->positions[j].positions = nullptr;
                }
            }
        } else {
            entry->doc_ids = nullptr;
            entry->positions = nullptr;
        }
    }
    
    fclose(file);
    return index;
}

void free_index(BooleanIndex* index) {
    if (!index) return;
    
    for (int i = 0; i < index->count; i++) {
        free(index->entries[i].term);
        
        if (index->entries[i].positions) {
            for (int j = 0; j < index->entries[i].doc_count; j++) {
                free(index->entries[i].positions[j].positions);
            }
            free(index->entries[i].positions);
        }
        
        free(index->entries[i].doc_ids);
    }
    
    free(index->entries);
    free(index);
}