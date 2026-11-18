#ifndef DOCUMENT_PARSER_H
#define DOCUMENT_PARSER_H


typedef struct {
    int id;
    char* title;
    char* content;      
    char* original_html; 
    char* filepath;
    int word_count;
} Document;


typedef struct {
    Document* documents;
    int count;
    int capacity;
} DocumentCollection;


void init_document_collection(DocumentCollection* collection, int initial_capacity);

void add_document(DocumentCollection* collection, Document doc);

DocumentCollection load_documents_from_dir(const char* dir_path);

void free_document_collection(DocumentCollection* collection);

Document parse_html_document(const char* filepath, int doc_id);

char* extract_text_from_html(const char* html);

char* extract_title_from_html(const char* html);

char* strip_html_tags(const char* html);

char* decode_html_entities(const char* str);

int is_html_file(const char* filename);

int count_words(const char* text);

#endif