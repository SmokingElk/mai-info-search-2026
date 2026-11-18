#include "../include/document_parser.h"
#include "../include/utils.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#ifdef _WIN32
    #include <direct.h>
    #define mkdir _mkdir
#else
    #include <sys/stat.h>
    #include <dirent.h>
#endif

void init_document_collection(DocumentCollection* collection, int initial_capacity) {
    collection->documents = (Document*)malloc(initial_capacity * sizeof(Document));
    collection->count = 0;
    collection->capacity = initial_capacity;
    
    for (int i = 0; i < initial_capacity; i++) {
        collection->documents[i].id = 0;
        collection->documents[i].title = nullptr;
        collection->documents[i].content = nullptr;
        collection->documents[i].original_html = nullptr;
        collection->documents[i].filepath = nullptr;
        collection->documents[i].word_count = 0;
    }
}

void add_document(DocumentCollection* collection, Document doc) {
    if (!collection) return;
    
    
    if (collection->count >= collection->capacity) {
        int new_capacity = collection->capacity * 2;
        Document* new_docs = (Document*)realloc(collection->documents, new_capacity * sizeof(Document));
        if (!new_docs) return;
        
        collection->documents = new_docs;
        collection->capacity = new_capacity;
    }
    
    
    collection->documents[collection->count] = doc;
    collection->count++;
}

char* read_file_content(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) return nullptr;
    
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    
    char* content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return nullptr;
    }
    
    
    size_t read_bytes = fread(content, 1, file_size, file);
    content[read_bytes] = '\0';
    
    fclose(file);
    return content;
}

char* decode_html_entities(const char* str) {
    if (!str) return nullptr;
    
    int len = strlen(str);
    char* result = (char*)malloc(len + 1);
    if (!result) return nullptr;
    
    char* dest = result;
    const char* src = str;
    
    while (*src) {
        if (*src == '&') {
            
            if (strncmp(src, "&lt;", 4) == 0) {
                *dest++ = '<';
                src += 4;
            } else if (strncmp(src, "&gt;", 4) == 0) {
                *dest++ = '>';
                src += 4;
            } else if (strncmp(src, "&amp;", 5) == 0) {
                *dest++ = '&';
                src += 5;
            } else if (strncmp(src, "&quot;", 6) == 0) {
                *dest++ = '"';
                src += 6;
            } else if (strncmp(src, "&#39;", 5) == 0 || strncmp(src, "&apos;", 6) == 0) {
                *dest++ = '\'';
                src += (*src == '&' && src[1] == '#' && src[2] == '3' && src[3] == '9') ? 5 : 6;
            } else if (strncmp(src, "&nbsp;", 6) == 0) {
                *dest++ = ' ';
                src += 6;
            } else {
                *dest++ = *src++;
            }
        } else {
            *dest++ = *src++;
        }
    }
    *dest = '\0';
    
    return result;
}

char* strip_html_tags(const char* html) {
    if (!html) return nullptr;
    
    int len = strlen(html);
    char* result = (char*)malloc(len + 1);
    if (!result) return nullptr;
    
    char* dest = result;
    const char* src = html;
    int in_tag = 0;
    int in_script = 0;
    int in_style = 0;
    int in_comment = 0;
    int last_char_was_space = 0;
    
    while (*src) {
        
        if (!in_comment && strncmp(src, "<!--", 4) == 0) {
            in_comment = 1;
            src += 4;
            continue;
        }
        
        
        if (in_comment && strncmp(src, "-->", 3) == 0) {
            in_comment = 0;
            src += 3;
            continue;
        }
        
        if (!in_comment) {
            
            if (*src == '<') {
                
                if (strncmp(src, "<script", 7) == 0) {
                    in_script = 1;
                } else if (strncmp(src, "<style", 6) == 0) {
                    in_style = 1;
                } else if (strncmp(src, "<!", 2) == 0) {
                    
                    in_tag = 1;
                }
                
                in_tag = 1;
                src++;
                continue;
            }
            
            
            if (*src == '>') {
                in_tag = 0;
                
                
                if (in_script && *(src-1) == 't' && *(src-2) == 'p' && *(src-3) == 'i' && 
                    *(src-4) == 'r' && *(src-5) == 'c' && *(src-6) == 's' && *(src-7) == '/') {
                    in_script = 0;
                } else if (in_style && *(src-1) == 'e' && *(src-2) == 'l' && *(src-3) == 'y' && 
                          *(src-4) == 't' && *(src-5) == 's' && *(src-6) == '/') {
                    in_style = 0;
                }
                
                src++;
                continue;
            }
            
            
            if (!in_tag && !in_script && !in_style) {
                
                if (isspace((unsigned char)*src)) {
                    if (!last_char_was_space) {
                        *dest++ = ' ';
                        last_char_was_space = 1;
                    }
                } else {
                    *dest++ = *src;
                    last_char_was_space = 0;
                }
            }
        }
        
        src++;
    }
    
    *dest = '\0';
    
    
    char* start = result;
    while (*start && isspace((unsigned char)*start)) start++;
    
    char* end = result + strlen(result) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    
    *(end + 1) = '\0';
    
    
    char* final = strdup(start);
    free(result);
    
    return final;
}

char* extract_title_from_html(const char* html) {
    if (!html) return strdup("Untitled");
    
    
    const char* title_start = strstr(html, "<title>");
    if (!title_start) {
        
        title_start = strstr(html, "<title ");
        if (title_start) {
            title_start = strchr(title_start, '>');
            if (title_start) title_start++;
        }
    } else {
        title_start += 7; 
    }
    
    if (title_start) {
        const char* title_end = strstr(title_start, "</title>");
        if (title_end) {
            int title_len = title_end - title_start;
            char* title = (char*)malloc(title_len + 1);
            if (title) {
                strncpy(title, title_start, title_len);
                title[title_len] = '\0';
                
                
                char* decoded = decode_html_entities(title);
                free(title);
                
                return decoded ? decoded : strdup("Untitled");
            }
        }
    }
    
    return strdup("Untitled");
}

char* extract_text_from_html(const char* html) {
    if (!html) return strdup("");
    
    
    char* stripped = strip_html_tags(html);
    if (!stripped) return strdup("");
    
    
    char* decoded = decode_html_entities(stripped);
    free(stripped);
    
    if (!decoded) return strdup("");
    
    
    char* result = decoded;
    
    
    char* dest = result;
    int last_was_space = 0;
    
    for (const char* src = result; *src; src++) {
        if (isspace((unsigned char)*src)) {
            if (!last_was_space) {
                *dest++ = ' ';
                last_was_space = 1;
            }
        } else {
            *dest++ = *src;
            last_was_space = 0;
        }
    }
    *dest = '\0';
    
    return result;
}

int is_html_file(const char* filename) {
    if (!filename) return 0;
    
    const char* dot = strrchr(filename, '.');
    if (!dot) return 0;
    
    return (strcmp(dot, ".html") == 0 || 
            strcmp(dot, ".htm") == 0 ||
            strcmp(dot, ".HTML") == 0 ||
            strcmp(dot, ".HTM") == 0);
}

int count_words(const char* text) {
    if (!text || !*text) return 0;
    
    int count = 0;
    int in_word = 0;
    
    for (const char* p = text; *p; p++) {
        if (isspace((unsigned char)*p)) {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            count++;
        }
    }
    
    return count;
}

Document parse_html_document(const char* filepath, int doc_id) {
    Document doc = {};
    doc.id = doc_id;
    doc.filepath = strdup(filepath);
    
    
    char* html_content = read_file_content(filepath);
    if (!html_content) {
        doc.title = strdup("Untitled");
        doc.content = strdup("");
        doc.original_html = strdup("");
        doc.word_count = 0;
        return doc;
    }
    
    doc.original_html = strdup(html_content);
    
    
    doc.title = extract_title_from_html(html_content);
    
    
    doc.content = extract_text_from_html(html_content);
    
    
    doc.word_count = count_words(doc.content);
    
    free(html_content);
    return doc;
}

#ifndef _WIN32

DocumentCollection load_documents_from_dir(const char* dir_path) {
    DocumentCollection collection;
    init_document_collection(&collection, 10);
    
    DIR* dir = opendir(dir_path);
    if (!dir) {
        printf("Cannot open directory: %s\n", dir_path);
        return collection;
    }
    
    struct dirent* entry;
    int doc_id = 1;
    
    while ((entry = readdir(dir)) != nullptr) {
        
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        
        if (is_html_file(entry->d_name)) {
            printf("Processing HTML file: %s\n", entry->d_name);
            Document doc = parse_html_document(full_path, doc_id);
            add_document(&collection, doc);
            doc_id++;
        }
    }
    
    closedir(dir);
    printf("Loaded %d HTML documents\n", collection.count);
    return collection;
}
#else

DocumentCollection load_documents_from_dir(const char* dir_path) {
    DocumentCollection collection;
    init_document_collection(&collection, 10);
    
    printf("Directory loading for HTML files...\n");
    
    
    
    printf("Creating test HTML documents...\n");
    
    
    const char* html1 = "<!DOCTYPE html>\n"
                       "<html>\n"
                       "<head><title>Music Document 1</title></head>\n"
                       "<body>\n"
                       "<h1>Back in Black - AC/DC</h1>\n"
                       "<div class=\"lyrics\">Back in black I hit the sack\n"
                       "I've been too long I'm glad to be back</div>\n"
                       "<p>Genre: Rock</p>\n"
                       "</body>\n"
                       "</html>";
    
    Document doc1;
    doc1.id = 1;
    doc1.filepath = strdup("test1.html");
    doc1.original_html = strdup(html1);
    doc1.title = extract_title_from_html(html1);
    doc1.content = extract_text_from_html(html1);
    doc1.word_count = count_words(doc1.content);
    add_document(&collection, doc1);
    
    
    const char* html2 = "<!DOCTYPE html>\n"
                       "<html lang=\"ru\">\n"
                       "<head><title>Музыкальный документ 2</title></head>\n"
                       "<body>\n"
                       "<h1>Bohemian Rhapsody - Queen</h1>\n"
                       "<div>Легендарная песня группы Queen</div>\n"
                       <p>Жанр: Рок, Прогрессив-рок</p>\n"
                       "</body>\n"
                       "</html>";
    
    Document doc2;
    doc2.id = 2;
    doc2.filepath = strdup("test2.html");
    doc2.original_html = strdup(html2);
    doc2.title = extract_title_from_html(html2);
    doc2.content = extract_text_from_html(html2);
    doc2.word_count = count_words(doc2.content);
    add_document(&collection, doc2);
    
    return collection;
}
#endif

void free_document_collection(DocumentCollection* collection) {
    if (!collection) return;
    
    for (int i = 0; i < collection->count; i++) {
        free(collection->documents[i].title);
        free(collection->documents[i].content);
        free(collection->documents[i].original_html);
        free(collection->documents[i].filepath);
    }
    
    free(collection->documents);
    collection->documents = nullptr;
    collection->count = 0;
    collection->capacity = 0;
}