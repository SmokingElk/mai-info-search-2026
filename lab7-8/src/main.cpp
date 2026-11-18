#include "../include/boolean_index.h"
#include "../include/document_parser.h"
#include "../include/tokenizer.h"
#include "../include/utils.h"
#include <cstdio>
#include <cstdlib>

void print_help() {
    printf("HTML Boolean Search System\n");
    printf("Usage:\n");
    printf("  build <html_documents_dir> <index_file>  - Build index from HTML documents\n");
    printf("  search <index_file> <query>              - Search in index\n");
    printf("  demo                                     - Run demo with test HTML documents\n");
    printf("  stats                                    - Show document statistics\n");
}

void build_index(const char* docs_dir, const char* index_file) {
    printf("Building index from HTML directory: %s\n", docs_dir);
    
    
    DocumentCollection docs = load_documents_from_dir(docs_dir);
    printf("Loaded %d HTML documents\n", docs.count);
    
    if (docs.count == 0) {
        printf("No HTML documents found in directory.\n");
        free_document_collection(&docs);
        return;
    }
    
    
    printf("\nDocument Statistics:\n");
    for (int i = 0; i < docs.count && i < 5; i++) {
        printf("  Doc %d: '%s' (%d words)\n", 
               docs.documents[i].id, 
               docs.documents[i].title,
               docs.documents[i].word_count);
    }
    
    BooleanIndex* index = (BooleanIndex*)calloc(1, sizeof(BooleanIndex));
    init_index(index, 100);
    
    
    printf("\nIndexing documents...\n");
    for (int i = 0; i < docs.count; i++) {
        Document* doc = &docs.documents[i];
        
        
        TokenArray tokens = tokenize_text(doc->content);
        
        
        for (int j = 0; j < tokens.count; j++) {
            add_to_index(index, tokens.tokens[j], doc->id, j);
        }
        
        
        free_tokens(&tokens);
        
        
        if ((i + 1) % 5 == 0 || i == docs.count - 1) {
            printf("  Indexed %d/%d documents...\n", i + 1, docs.count);
        }
    }
    
    printf("Index built. Total unique terms: %d\n", index->count);
    
    
    save_index(index, index_file);
    printf("Index saved to: %s\n", index_file);
    
    
    free_index(index);
    free_document_collection(&docs);
}

void search_index(const char* index_file, const char* query) {
    printf("Searching for: '%s'\n", query);
    
    
    BooleanIndex* index = load_index(index_file);
    if (!index) {
        printf("Cannot load index from: %s\n", index_file);
        return;
    }
    
    printf("Index loaded. Total terms: %d\n", index->count);
    
    
    TokenArray query_tokens = tokenize_text(query);
    
    if (query_tokens.count == 0) {
        printf("No valid search terms in query.\n");
        free_index(index);
        return;
    }
    
    
    IndexEntry* entry = find_term(index, query_tokens.tokens[0]);
    if (entry) {
        printf("\nFound term: '%s'\n", entry->term);
        printf("Documents containing this term: %d\n", entry->doc_count);
        
        printf("Document IDs: ");
        for (int i = 0; i < entry->doc_count && i < 10; i++) {
            printf("%d ", entry->doc_ids[i]);
        }
        if (entry->doc_count > 10) printf("...");
        printf("\n");
    } else {
        printf("Term not found: '%s'\n", query_tokens.tokens[0]);
    }
    
    if (query_tokens.count > 1) {
        printf("\nBoolean AND search for all terms:\n");
        
        int result_count = 0;
        int* results = nullptr;
        
        for (int i = 0; i < query_tokens.count; i++) {
            const char* tok = query_tokens.tokens[i];
            
            // пропускаем булевы операторы, оставшиеся в запросе
            if (strcmp(tok, "and") == 0 || strcmp(tok, "or") == 0 || strcmp(tok, "not") == 0) {
                continue;
            }
            
            IndexEntry* entry = find_term(index, tok);
            if (!entry || entry->doc_count == 0) {
                free(results);
                results = nullptr;
                result_count = 0;
                break;
            }
            
            if (!results) {
                results = (int*)malloc(entry->doc_count * sizeof(int));
                memcpy(results, entry->doc_ids, entry->doc_count * sizeof(int));
                result_count = entry->doc_count;
            } else {
                int new_count = 0;
                int* new_results = intersect_sorted_arrays(results, result_count,
                                                            entry->doc_ids, entry->doc_count,
                                                            &new_count);
                free(results);
                results = new_results;
                result_count = new_count;
                if (!results) break;
            }
        }
        
        if (results && result_count > 0) {
            printf("Found %d documents with all terms\n", result_count);
            printf("Document IDs: ");
            for (int i = 0; i < result_count && i < 10; i++) {
                printf("%d ", results[i]);
            }
            if (result_count > 10) printf("...");
            printf("\n");
            free(results);
        } else {
            printf("No documents found with all terms\n");
        }
    }
    
    free_tokens(&query_tokens);
    free_index(index);
}

void show_stats() {
    printf("HTML Document Parser Statistics\n");
    printf("===============================\n\n");
    
    
    const char* test_html = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>Test Music Document &amp; Stats</title>\n"
        "    <style>body { color: #333; }</style>\n"
        "    <script>console.log('test');</script>\n"
        "</head>\n"
        "<body>\n"
        "    <h1>Music Analysis</h1>\n"
        "    <p>This is a <strong>test</strong> HTML document for &quot;music&quot; search.</p>\n"
        "    <p>Keywords: rock, pop, jazz, blues</p>\n"
        "    <!-- This is a comment -->\n"
        "</body>\n"
        "</html>";
    
    printf("Test HTML:\n");
    printf("----------\n%s\n\n", test_html);
    
    printf("Extracted Title:\n");
    printf("----------------\n");
    char* title = extract_title_from_html(test_html);
    printf("%s\n\n", title);
    free(title);
    
    printf("Extracted Text (without HTML tags):\n");
    printf("-----------------------------------\n");
    char* text = extract_text_from_html(test_html);
    printf("%s\n\n", text);
    
    printf("Word Count: %d\n", count_words(text));
    
    free(text);
}

void run_demo() {
    printf("=== HTML Boolean Search Demo ===\n\n");
    
    
    DocumentCollection docs;
    init_document_collection(&docs, 5);
    
    const char* html1 = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>Rock Music Collection</title></head>\n"
        "<body>\n"
        "<h1>AC/DC - Back in Black</h1>\n"
        "<div class=\"lyrics\">Back in black I hit the sack\n"
        "I've been too long I'm glad to be back</div>\n"
        "<p><strong>Genre:</strong> Hard Rock</p>\n"
        "<p><strong>Year:</strong> 1980</p>\n"
        "</body>\n"
        "</html>";
    
    const char* html2 = 
        "<!DOCTYPE html>\n"
        "<html lang=\"ru\">\n"
        "<head><title>Queen - Bohemian Rhapsody</title></head>\n"
        "<body>\n"
        "<h1>Bohemian Rhapsody</h1>\n"
        "<p>Легендарная песня группы <em>Queen</em> из альбома <strong>A Night at the Opera</strong></p>\n"
        "<p>Жанры: Прогрессив-рок, Хард-рок</p>\n"
        "</body>\n"
        "</html>";
    
    const char* html3 = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>Pop Music Hits</title></head>\n"
        "<body>\n"
        "<h1>Michael Jackson - Thriller</h1>\n"
        "<p>The best-selling album of all time</p>\n"
        "<ul>\n"
        "<li>Genre: Pop, Funk, Disco</li>\n"
        "<li>Year: 1982</li>\n"
        "</ul>\n"
        "</body>\n"
        "</html>";
    
    Document doc1 = parse_html_document("demo1.html", 1);
    free(doc1.original_html); doc1.original_html = nullptr;
    free(doc1.content);       doc1.content = nullptr;
    free(doc1.title);         doc1.title = nullptr;
    doc1.title = extract_title_from_html(html1);
    doc1.content = extract_text_from_html(html1);
    doc1.word_count = count_words(doc1.content);
    
    Document doc2 = parse_html_document("demo2.html", 2);
    free(doc2.original_html); doc2.original_html = nullptr;
    free(doc2.content);       doc2.content = nullptr;
    free(doc2.title);         doc2.title = nullptr;
    doc2.title = extract_title_from_html(html2);
    doc2.content = extract_text_from_html(html2);
    doc2.word_count = count_words(doc2.content);
    
    Document doc3 = parse_html_document("demo3.html", 3);
    free(doc3.original_html); doc3.original_html = nullptr;
    free(doc3.content);       doc3.content = nullptr;
    free(doc3.title);         doc3.title = nullptr;
    doc3.title = extract_title_from_html(html3);
    doc3.content = extract_text_from_html(html3);
    doc3.word_count = count_words(doc3.content);
    
    add_document(&docs, doc1);
    add_document(&docs, doc2);
    add_document(&docs, doc3);
    
    printf("Created %d test HTML documents:\n", docs.count);
    for (int i = 0; i < docs.count; i++) {
        printf("  %d. %s (%d words)\n", 
               docs.documents[i].id,
               docs.documents[i].title,
               docs.documents[i].word_count);
    }
    printf("\n");
    
    
    BooleanIndex* index = (BooleanIndex*)calloc(1, sizeof(BooleanIndex));
    init_index(index, 10);
    
    for (int i = 0; i < docs.count; i++) {
        Document* doc = &docs.documents[i];
        TokenArray tokens = tokenize_text(doc->content);
        
        for (int j = 0; j < tokens.count; j++) {
            add_to_index(index, tokens.tokens[j], doc->id, j);
        }
        
        free_tokens(&tokens);
    }
    
    printf("Index created. Total unique terms: %d\n\n", index->count);
    
    
    printf("1. Search for 'rock':\n");
    IndexEntry* rock_entry = find_term(index, "rock");
    if (rock_entry) {
        printf("   Found in %d documents: ", rock_entry->doc_count);
        for (int i = 0; i < rock_entry->doc_count; i++) {
            printf("%d ", rock_entry->doc_ids[i]);
        }
        printf("\n");
    }
    
    printf("\n2. Boolean AND search 'black AND back':\n");
    int and_count;
    int* and_results = boolean_and(index, "black", "back", &and_count);
    if (and_results) {
        printf("   Found %d documents: ", and_count);
        for (int i = 0; i < and_count; i++) {
            printf("%d ", and_results[i]);
        }
        printf("\n");
        free(and_results);
    }
    
    printf("\n3. Boolean OR search 'queen OR jackson':\n");
    int or_count;
    int* or_results = boolean_or(index, "queen", "jackson", &or_count);
    if (or_results) {
        printf("   Found %d documents: ", or_count);
        for (int i = 0; i < or_count; i++) {
            printf("%d ", or_results[i]);
        }
        printf("\n");
        free(or_results);
    }
    
    printf("\n4. Phrase search 'back in':\n");
    int phrase_count;
    int* phrase_results = phrase_search(index, "back in", &phrase_count);
    if (phrase_results) {
        printf("   Found %d documents: ", phrase_count);
        for (int i = 0; i < phrase_count; i++) {
            printf("%d ", phrase_results[i]);
        }
        printf("\n");
        free(phrase_results);
    }
    
    
    free_index(index);
    free_document_collection(&docs);
    
    printf("\n=== Demo completed ===\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }
    
    if (strcmp(argv[1], "build") == 0 && argc == 4) {
        build_index(argv[2], argv[3]);
    } else if (strcmp(argv[1], "search") == 0 && argc == 4) {
        search_index(argv[2], argv[3]);
    } else if (strcmp(argv[1], "demo") == 0) {
        run_demo();
    } else if (strcmp(argv[1], "stats") == 0) {
        show_stats();
    } else {
        print_help();
        return 1;
    }
    
    return 0;
}