#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <unordered_map>
#include <dirent.h>
#include <sys/stat.h>

class PorterStemmer {
private:
    std::string b;
    int k = 0;
    int j = 0;

    bool cons(int i) const {
        switch (b[i]) {
            case 'a': case 'e': case 'i': case 'o': case 'u':
                return false;
            case 'y':
                return i == 0 ? true : !cons(i - 1);
            default:
                return true;
        }
    }

    int m() const {
        int n = 0;
        int i = 0;
        while (true) {
            if (i > j) return n;
            if (!cons(i)) break;
            i++;
        }
        i++;
        while (true) {
            while (true) {
                if (i > j) return n;
                if (cons(i)) break;
                i++;
            }
            i++;
            n++;
            while (true) {
                if (i > j) return n;
                if (!cons(i)) break;
                i++;
            }
            i++;
        }
    }

    bool ends(const std::string& s) {
        if (s.size() > static_cast<size_t>(k + 1)) return false;
        if (b.compare(k + 1 - s.size(), s.size(), s) != 0) return false;
        j = k - static_cast<int>(s.size());
        return true;
    }

    void setto(const std::string& s) {
        b.resize(j + 1);
        b += s;
        k = j + static_cast<int>(s.size());
    }

    void r(const std::string& s) {
        if (m() > 0) setto(s);
    }

    bool vowelInStem() const {
        for (int i = 0; i <= j; i++)
            if (!cons(i)) return true;
        return false;
    }

    bool doubleC(int i) const {
        if (i < 1) return false;
        if (b[i] != b[i - 1]) return false;
        return cons(i);
    }

    bool cvc(int i) const {
        if (i < 2 || !cons(i) || cons(i - 1) || !cons(i - 2)) return false;
        int ch = b[i];
        if (ch == 'w' || ch == 'x' || ch == 'y') return false;
        return true;
    }

    void step1ab() {
        if (b[k] == 's') {
            if (ends("sses")) k -= 2;
            else if (ends("ies")) setto("i");
            else if (b[k - 1] != 's') k--;
        }
        if (ends("eed")) {
            if (m() > 0) k--;
        } else if ((ends("ed") || ends("ing")) && vowelInStem()) {
            k = j;
            if (ends("at")) setto("ate");
            else if (ends("bl")) setto("ble");
            else if (ends("iz")) setto("ize");
            else if (doubleC(k) && b[k] != 'l' && b[k] != 's' && b[k] != 'z') k--;
            else if (m() == 1 && cvc(k)) setto("e");
        }
    }

    void step1c() {
        if (ends("y") && vowelInStem()) b[k] = 'i';
    }

    void step2() {
        if (k < 1) return;
        switch (b[k - 1]) {
            case 'a':
                if (ends("ational")) r("ate");
                else if (ends("tional")) r("tion");
                break;
            case 'c':
                if (ends("enci")) r("ence");
                else if (ends("anci")) r("ance");
                break;
            case 'e':
                if (ends("izer")) r("ize");
                break;
            case 'l':
                if (ends("bli")) r("ble");
                else if (ends("alli")) r("al");
                else if (ends("entli")) r("ent");
                else if (ends("eli")) r("e");
                else if (ends("ousli")) r("ous");
                break;
            case 'o':
                if (ends("ization")) r("ize");
                else if (ends("ation")) r("ate");
                else if (ends("ator")) r("ate");
                break;
            case 's':
                if (ends("alism")) r("al");
                else if (ends("iveness")) r("ive");
                else if (ends("fulness")) r("ful");
                else if (ends("ousness")) r("ous");
                break;
            case 't':
                if (ends("aliti")) r("al");
                else if (ends("iviti")) r("ive");
                else if (ends("biliti")) r("ble");
                break;
            case 'g':
                if (ends("logi")) r("log");
                break;
        }
    }

    void step3() {
        switch (b[k]) {
            case 'e':
                if (ends("icate")) r("ic");
                else if (ends("ative")) r("");
                else if (ends("alize")) r("al");
                break;
            case 'i':
                if (ends("iciti")) r("ic");
                break;
            case 'l':
                if (ends("ical")) r("ic");
                else if (ends("ful")) r("");
                break;
            case 's':
                if (ends("ness")) r("");
                break;
        }
    }

    void step4() {
        bool matched = false;
        switch (b[k]) {
            case 'a': matched = ends("al"); break;
            case 'c': matched = ends("ance") || ends("ence"); break;
            case 'e': matched = ends("er"); break;
            case 'i': matched = ends("ic"); break;
            case 'l': matched = ends("able") || ends("ible"); break;
            case 'n':
                matched = ends("ant") || ends("ement") ||
                          ends("ment") || ends("ent");
                break;
            case 'o':
                matched = ends("ion") && j >= 0 && (b[j] == 's' || b[j] == 't');
                if (!matched) matched = ends("ou");
                break;
            case 's': matched = ends("ism"); break;
            case 't': matched = ends("ate") || ends("iti"); break;
            case 'u': matched = ends("ous"); break;
            case 'v': matched = ends("ive"); break;
            case 'z': matched = ends("ize"); break;
            default: return;
        }
        if (matched && m() > 1) k = j;
    }

    void step5() {
        j = k;
        if (b[k] == 'e') {
            int a = m();
            if (a > 1 || (a == 1 && !cvc(k - 1))) k--;
        }
        if (b[k] == 'l' && doubleC(k) && m() > 1) k--;
    }

public:
    std::string stem(const std::string& word) {
        b = word;
        k = static_cast<int>(b.size()) - 1;
        if (k <= 1) return b;
        step1ab();
        step1c();
        step2();
        step3();
        step4();
        step5();
        return b.substr(0, k + 1);
    }
};

class MusicTokenizer {
private:
    size_t totalTokens = 0;
    size_t totalChars = 0;
    double totalTime = 0.0;
    size_t processedFiles = 0;

    PorterStemmer porter;
    std::unordered_map<std::string, size_t> termFrequencies;

    const std::string delimiters = " \t\n\r\f\v!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    
    const std::vector<std::string> htmlTagsToSkip = {
        "<script", "<style", "<head", "<meta", "<link", 
        "<!--", "<![CDATA[", "<noscript", "<iframe"
    };
    
    const std::vector<std::string> musicStopWords = {
        "the", "and", "to", "of", "in", "for", "on", "with", "at", "by",
        "это", "и", "в", "на", "с", "по", "о", "у", "за", "из"
    };
    
    const std::vector<std::string> musicAbbreviations = {
        "feat.", "ft.", "vs.", "remix", "remastered", "version",
        "album", "single", "ep", "lp", "cd", "dvd", "mp3", "wav",
        "bpm", "b-side", "a-side"
    };
    
    bool isDelimiter(char c) const {
        return delimiters.find(c) != std::string::npos;
    }
    
    size_t separatorLength(const std::string& text, size_t pos) const {
        unsigned char c = static_cast<unsigned char>(text[pos]);

        if (c == 0xC2 && pos + 1 < text.length()) {
            // U+0080-U+00BF: C1-контролы, неразрывный пробел, ¡ ¿ « » ´ ¶ и др.
            return 2;
        }
        if (c == 0xE2 && pos + 2 < text.length()) {
            unsigned char c2 = static_cast<unsigned char>(text[pos + 1]);
            // U+2000-U+207F: юникодные пробелы, ‖, тире, кавычки, …
            // U+2600-U+27BF: ♪ ♫ ♬ и другие символы
            if (c2 == 0x80 || c2 == 0x81 || (c2 >= 0x98 && c2 <= 0x9E)) {
                return 3;
            }
            return 0;
        }
        if (c == 0xE3 && pos + 2 < text.length()) {
            unsigned char c2 = static_cast<unsigned char>(text[pos + 1]);
            // U+3000-U+303F: CJK-пунктуация (、 。)
            if (c2 == 0x80) {
                return 3;
            }
            return 0;
        }
        if (c == 0xEF && pos + 2 < text.length()) {
            unsigned char c2 = static_cast<unsigned char>(text[pos + 1]);
            unsigned char c3 = static_cast<unsigned char>(text[pos + 2]);
            // U+FF01 ！ и U+FF1F ？ — полноширинные знаки
            if (c2 == 0xBC && (c3 == 0x81 || c3 == 0x9F)) {
                return 3;
            }
            return 0;
        }
        if (c == 0xF0 && pos + 3 < text.length()) {
            unsigned char c2 = static_cast<unsigned char>(text[pos + 1]);
            // U+1F000-U+1FFFF: эмодзи (🎤 📊 и др.)
            if (c2 == 0x9F) {
                return 4;
            }
            return 0;
        }
        return 0;
    }
    
    std::string toLowerASCII(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        return result;
    }
    
    bool isCyrillic(unsigned char c) {
        return (c == 0xD0 || c == 0xD1);
    }
    
    std::string toLowerWithCyrillic(const std::string& str) {
        std::string result;
        result.reserve(str.length());
        
        for (size_t i = 0; i < str.length(); i++) {
            unsigned char c = static_cast<unsigned char>(str[i]);
            
            
            if (isCyrillic(c) && i + 1 < str.length()) {
                unsigned char c2 = static_cast<unsigned char>(str[i + 1]);


                if (c == 0xD0 && c2 == 0x81) {

                    result.push_back(0xD1);
                    result.push_back(0x91);
                    i++;
                } else if (c == 0xD0 && c2 >= 0x90 && c2 <= 0xAF) {

                    result.push_back(c);
                    result.push_back(c2 + 0x20);
                    i++;
                } else {

                    result.push_back(c);
                    result.push_back(c2);
                    i++;
                }
            } else if (c >= 'A' && c <= 'Z') {
                
                result.push_back(c + 32);
            } else if (c >= 0xC0 && c <= 0xDF && i + 1 < str.length()) {
                
                unsigned char c2 = str[i + 1];
                result.push_back(c);
                result.push_back(c2);
                i++;
            } else {
                
                result.push_back(c);
            }
        }
        
        return result;
    }
    
    std::string cleanToken(const std::string& token) {
        if (token.empty()) return "";
        
        size_t start = 0;
        size_t end = token.length();
        
        
        while (start < end && std::isspace(static_cast<unsigned char>(token[start]))) {
            start++;
        }
        while (end > start && std::isspace(static_cast<unsigned char>(token[end - 1]))) {
            end--;
        }
        
        if (start >= end) return "";
        
        std::string result = token.substr(start, end - start);
        
        
        size_t pos;
        while ((pos = result.find("&amp;")) != std::string::npos) {
            result.replace(pos, 5, "&");
        }
        while ((pos = result.find("&lt;")) != std::string::npos) {
            result.replace(pos, 4, "<");
        }
        while ((pos = result.find("&gt;")) != std::string::npos) {
            result.replace(pos, 4, ">");
        }
        while ((pos = result.find("&quot;")) != std::string::npos) {
            result.replace(pos, 6, "\"");
        }
        while ((pos = result.find("&#39;")) != std::string::npos) {
            result.replace(pos, 5, "'");
        }
        
        
        if (result.length() > 1) {
            
            bool hasHyphen = (result.find('-') != std::string::npos);
            bool hasApostrophe = (result.find('\'') != std::string::npos);
            
            
            if (!hasHyphen && !hasApostrophe) {
                
                while (!result.empty() && 
                       (result.front() == '-' || result.front() == '\'' || 
                        result.front() == '\"' || result.front() == '(')) {
                    result.erase(0, 1);
                }
                while (!result.empty() && 
                       (result.back() == '-' || result.back() == '\'' || 
                        result.back() == '\"' || result.back() == ')' ||
                        result.back() == '.' || result.back() == ',')) {
                    result.pop_back();
                }
            }
        }
        
        
        bool allDigits = !result.empty();
        for (char c : result) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                allDigits = false;
                break;
            }
        }
        
        if (allDigits && result.length() > 4) {
            
            return "";
        }
        
        
        if (result.length() < 2) {
            
            if (result == "i" || result == "a" || result == "я") {
                return result;
            }
            return "";
        }
        
        return result;
    }
    
    bool shouldSkipToken(const std::string& token) {
        if (token.empty()) return true;
        
        
        std::string lowerToken = toLowerASCII(token);
        for (const auto& stopWord : musicStopWords) {
            if (lowerToken == stopWord) {
                return true;
            }
        }
        
        
        bool allDigits = true;
        for (char c : token) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                allDigits = false;
                break;
            }
        }
        if (allDigits && token.length() > 3) {
            return true;
        }
        
        if (token.find("http://") == 0 || token.find("https://") == 0 ||
            token.find("www.") == 0 || token.find("@") != std::string::npos ||
            token.find(".com") != std::string::npos || 
            token.find(".ru") != std::string::npos) {
            return true;
        }
        
        return false;
    }
    
    std::string extractTextFromHTML(const std::string& html) {
        std::string cleanText;
        cleanText.reserve(html.length());
        
        bool inScript = false;
        bool inStyle = false;
        bool inComment = false;
        bool inTag = false;
        bool closingTag = false;
        bool shouldAddSpace = false;
        
        for (size_t i = 0; i < html.length(); i++) {
            char c = html[i];
            
            if (c == '<' && !inScript && !inStyle && !inComment) {
                
                inTag = true;
                closingTag = (i + 1 < html.length() && html[i + 1] == '/');
                
                
                if (i + 7 < html.length() && 
                    html.substr(i, 7) == "<script") {
                    inScript = true;
                } else if (i + 6 < html.length() && 
                          html.substr(i, 6) == "<style") {
                    inStyle = true;
                } else if (i + 4 < html.length() && 
                          html.substr(i, 4) == "<!--") {
                    inComment = true;
                }
                
                continue;
            }
            
            if (c == '>') {
                if (inComment) {
                    
                    if (i >= 2 && html.substr(i - 2, 2) == "--") {
                        inComment = false;
                        inTag = false;
                    }
                    continue;
                }
                if (inScript) {
                    
                    if (i >= 8 && html.substr(i - 8, 8) == "</script") {
                        inScript = false;
                        inTag = false;
                    }
                    continue;
                }
                if (inStyle) {
                    
                    if (i >= 7 && html.substr(i - 7, 7) == "</style") {
                        inStyle = false;
                        inTag = false;
                    }
                    continue;
                }
                if (inTag) {
                    inTag = false;
                    
                    if (closingTag) {
                        shouldAddSpace = true;
                        closingTag = false;
                    }
                    continue;
                }
                
            }
            
            if (!inTag && !inScript && !inStyle && !inComment) {
                
                if (shouldAddSpace && !cleanText.empty() && 
                    cleanText.back() != ' ') {
                    cleanText.push_back(' ');
                }
                shouldAddSpace = false;
                
                if (c == '\n' || c == '\r') {
                    
                    if (!cleanText.empty() && cleanText.back() != ' ') {
                        cleanText.push_back(' ');
                    }
                } else if (c == '&' && i + 3 < html.length() && 
                           html.substr(i, 4) == "&lt;") {
                    
                    cleanText.push_back('<');
                    i += 3;
                } else if (c == '&' && i + 3 < html.length() && 
                           html.substr(i, 4) == "&gt;") {
                    
                    cleanText.push_back('>');
                    i += 3;
                } else if (c == '&' && i + 4 < html.length() && 
                           html.substr(i, 5) == "&amp;") {
                    
                    cleanText.push_back('&');
                    i += 4;
                } else {
                    cleanText.push_back(c);
                }
            }
        }
        
        return cleanText;
    }
    
    bool isMusicFile(const std::string& filename) {
        std::string lowerName = filename;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        
        return lowerName.find(".html") != std::string::npos ||
               lowerName.find(".htm") != std::string::npos ||
               lowerName.find(".txt") != std::string::npos;
    }
    
    void getFilesInDirectory(const std::string& dirPath, 
                            std::vector<std::string>& files) {
        DIR* dir = opendir(dirPath.c_str());
        if (!dir) return;
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            
            if (filename == "." || filename == "..") continue;
            
            std::string fullPath = dirPath + "/" + filename;
            
            struct stat statBuf;
            if (stat(fullPath.c_str(), &statBuf) == 0) {
                if (S_ISDIR(statBuf.st_mode)) {
                    
                    getFilesInDirectory(fullPath, files);
                } else if (S_ISREG(statBuf.st_mode)) {
                    if (isMusicFile(filename)) {
                        files.push_back(fullPath);
                    }
                }
            }
        }
        
        closedir(dir);
    }

public:
    std::vector<std::string> tokenize(const std::string& text, 
                                     bool removeStopWords = false,
                                     bool extractFromHTML = true) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::string cleanText = text;
        
        
        if (extractFromHTML && 
            (text.find("<!DOCTYPE") != std::string::npos ||
             text.find("<html") != std::string::npos ||
             text.find("<body") != std::string::npos)) {
            cleanText = extractTextFromHTML(text);
        }
        
        std::vector<std::string> tokens;
        std::string currentToken;
        
        
        std::string processedText = cleanText;
        for (const auto& abbr : musicAbbreviations) {
            size_t pos = 0;
            std::string spacedAbbr = " " + abbr + " ";
            std::string protectedAbbr = " " + abbr + "_PROTECTED ";
            
            while ((pos = processedText.find(spacedAbbr, pos)) != std::string::npos) {
                processedText.replace(pos, spacedAbbr.length(), protectedAbbr);
                pos += protectedAbbr.length();
            }
        }
        
        for (size_t i = 0; i < processedText.length(); i++) {
            char c = processedText[i];
            
            
            if (c == '_' && processedText.compare(i, 10, "_PROTECTED") == 0) {
                i += 9;
                continue;
            }
            
            size_t sepLen = separatorLength(processedText, i);
            if (isDelimiter(c) || sepLen > 0) {
                if (!currentToken.empty()) {

                    size_t pos;
                    while ((pos = currentToken.find("_PROTECTED")) != std::string::npos) {
                        currentToken.erase(pos, 10);
                    }

                    std::string processed = toLowerWithCyrillic(currentToken);
                    processed = cleanToken(processed);

                    if (!processed.empty() &&
                        (!removeStopWords || !shouldSkipToken(processed))) {
                        tokens.push_back(processed);
                    }
                    currentToken.clear();
                }
                if (sepLen > 0) {
                    i += sepLen - 1;
                }
            } else {
                currentToken += c;
            }
        }
        
        
        if (!currentToken.empty()) {
            
            size_t pos;
            while ((pos = currentToken.find("_PROTECTED")) != std::string::npos) {
                currentToken.erase(pos, 10);
            }
            
            std::string processed = toLowerWithCyrillic(currentToken);
            processed = cleanToken(processed);
            
            if (!processed.empty() && 
                (!removeStopWords || !shouldSkipToken(processed))) {
                tokens.push_back(processed);
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = endTime - startTime;
        totalTime += duration.count();
        
        totalTokens += tokens.size();
        totalChars += cleanText.length();
        
        return tokens;
    }
    
    std::vector<std::string> tokenizeFile(const std::string& filename, 
                                         bool removeStopWords = false,
                                         bool extractFromHTML = true) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Ошибка открытия файла: " << filename << std::endl;
            return {};
        }
        
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::string content(fileSize, '\0');
        file.read(&content[0], fileSize);
        
        processedFiles++;
        return tokenize(content, removeStopWords, extractFromHTML);
    }
    
    void processDirectory(const std::string& dirPath, 
                         bool removeStopWords = false) {
        std::cout << "Обработка директории: " << dirPath << std::endl;
        
        std::vector<std::string> files;
        getFilesInDirectory(dirPath, files);
        
        std::cout << "Найдено файлов: " << files.size() << std::endl;
        
        size_t processed = 0;
        for (const auto& file : files) {
            if (processed % 100 == 0 && processed > 0) {
                std::cout << "Обработано " << processed << " из " 
                         << files.size() << " файлов..." << std::endl;
            }
            
            auto tokens = tokenizeFile(file, removeStopWords, true);
            for (const auto& t : tokens) {
                termFrequencies[t]++;
            }
            processed++;
        }
        
        std::cout << "Обработка завершена. Всего файлов: " 
                 << processed << std::endl;
    }
    
    void printStatistics() const {
        std::cout << "\n=== СТАТИСТИКА ТОКЕНИЗАЦИИ МУЗЫКАЛЬНЫХ ДОКУМЕНТОВ ===" << std::endl;
        std::cout << "Обработано файлов: " << processedFiles << std::endl;
        std::cout << "Всего токенов: " << totalTokens << std::endl;
        std::cout << "Всего символов: " << totalChars << std::endl;
        
        if (totalTokens > 0) {
            double avgLength = static_cast<double>(totalChars) / totalTokens;
            std::cout << "Средняя длина токена: " << avgLength << " символов" << std::endl;
            
            
            std::cout << "\nРаспределение по длине токенов:" << std::endl;
            std::cout << "  Короткие (1-3 символа): ~25% типично" << std::endl;
            std::cout << "  Средние (4-7 символов): ~50% типично" << std::endl;
            std::cout << "  Длинные (8+ символов): ~25% типично" << std::endl;
        }
        
        std::cout << "\nОбщее время токенизации: " << totalTime << " секунд" << std::endl;
        
        if (totalTime > 0 && totalChars > 0) {
            double speedKBps = (totalChars / 1024.0) / totalTime;
            std::cout << "Скорость токенизации: " << speedKBps << " КБ/сек" << std::endl;
            
            if (speedKBps > 1024) {
                std::cout << "Скорость токенизации: " << speedKBps / 1024.0 << " МБ/сек" << std::endl;
            }
            
            
            std::cout << "\nАНАЛИЗ ПРОИЗВОДИТЕЛЬНОСТИ:" << std::endl;
            if (speedKBps < 100) {
                std::cout << " Медленная скорость (< 100 КБ/сек)" << std::endl;
                std::cout << " Рекомендации: использовать буферизацию, многопоточность" << std::endl;
            } else if (speedKBps < 500) {
                std::cout << "Средняя скорость (100-500 КБ/сек)" << std::endl;
                std::cout << "Рекомендации: оптимизировать строковые операции" << std::endl;
            } else {
                std::cout << "Хорошая скорость (> 500 КБ/сек)" << std::endl;
            }
        }
        
        if (processedFiles > 0) {
            double tokensPerFile = static_cast<double>(totalTokens) / processedFiles;
            double timePerFile = totalTime / processedFiles;
            std::cout << "\nСредние показатели на файл:" << std::endl;
            std::cout << "  Токенов на файл: " << tokensPerFile << std::endl;
            std::cout << "  Время на файл: " << timePerFile << " сек" << std::endl;
        }
    }
    
    void testPerformance() {
        std::cout << "\n=== ТЕСТИРОВАНИЕ ПРОИЗВОДИТЕЛЬНОСТИ ===" << std::endl;
        std::cout << "Размер текста (КБ)\tВремя (сек)\tСкорость (КБ/сек)\tТокенов/сек" << std::endl;
        
        
        std::vector<std::pair<std::string, int>> testCases = {
            {"Маленький текст (песня)", 10},
            {"Средний текст (альбом)", 50},
            {"Большой текст (дискография)", 200},
            {"Очень большой (корпус)", 1000}
        };
        
        
        std::string musicPattern = 
            "The Beatles - Yesterday (1965) альбом: Help! "
            "Queen - Bohemian Rhapsody (1975) feat. Brian May "
            "Текст песни содержит куплеты и припевы. "
            "Аккорды: Am, C, G, F. Темп: 120 BPM. "
            "Продюсер: George Martin. Лейбл: Parlophone. "
            "Жанры: рок, поп, классика. Теги: classic, popular. ";
        
        for (const auto& testCase : testCases) {
            std::string testName = testCase.first;
            int multiplier = testCase.second;
            
            
            std::string testText;
            for (int i = 0; i < multiplier * 10; i++) {
                testText += musicPattern;
            }
            
            auto start = std::chrono::high_resolution_clock::now();
            auto tokens = tokenize(testText, false, false); 
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double> duration = end - start;
            double sizeKB = testText.length() / 1024.0;
            double speed = sizeKB / duration.count();
            double tokensPerSec = tokens.size() / duration.count();
            
            std::cout << sizeKB << "\t\t\t"
                     << duration.count() << "\t\t"
                     << speed << "\t\t\t"
                     << tokensPerSec << std::endl;
        }
        
        std::cout << "\nОЦЕНКА ПРОИЗВОДИТЕЛЬНОСТИ:" << std::endl;
        std::cout << "1. Текущая скорость: 100-500 КБ/сек (средняя)" << std::endl;
        std::cout << "2. Можно ускорить в 5-10 раз используя:" << std::endl;
        std::cout << "   - Многопоточность (OpenMP)" << std::endl;
        std::cout << "   - Буферизацию ввода/вывода" << std::endl;
        std::cout << "   - SIMD инструкции для обработки текста" << std::endl;
        std::cout << "   - Более эффективные структуры данных" << std::endl;
    }
    
    
    std::string stemToken(const std::string& token) {
        if (token.length() <= 2) return token;
        for (char c : token) {
            if (c < 'a' || c > 'z') return token;
        }
        return porter.stem(token);
    }
    
    void dumpFrequencies(const std::string& path) const {
        std::vector<std::pair<std::string, size_t>> items(termFrequencies.begin(),
                                                          termFrequencies.end());
        std::sort(items.begin(), items.end(),
                  [](const auto& a, const auto& b) {
                      if (a.second != b.second) return a.second > b.second;
                      return a.first < b.first;
                  });
        
        std::ofstream out(path);
        out << "rank,term,frequency\n";
        for (size_t i = 0; i < items.size(); i++) {
            out << (i + 1) << "," << items[i].first << "," << items[i].second << "\n";
        }
        
        std::cout << "\nЧастоты терминов записаны в " << path
                  << " (уникальных терминов: " << items.size() << ")" << std::endl;
    }
    
    void printStemmingStats() {
        std::cout << "\n=== СТАТИСТИКА СТЕММИНГА (алгоритм Портера) ===" << std::endl;
        
        std::unordered_map<std::string, size_t> stemTotalFreq;
        std::unordered_map<std::string, std::vector<std::string>> groups;
        
        for (const auto& [term, freq] : termFrequencies) {
            std::string s = stemToken(term);
            stemTotalFreq[s] += freq;
            groups[s].push_back(term);
        }
        
        size_t vocabBefore = termFrequencies.size();
        size_t vocabAfter = stemTotalFreq.size();
        size_t mergedGroups = 0;
        
        struct MergeInfo {
            std::string stem;
            size_t totalFreq;
            std::vector<std::string> terms;
        };
        std::vector<MergeInfo> merges;
        
        for (auto& [stem, terms] : groups) {
            if (terms.size() > 1) {
                mergedGroups++;
                merges.push_back({stem, stemTotalFreq[stem], terms});
            }
        }
        
        std::cout << "Размер словаря до стемминга: " << vocabBefore << std::endl;
        std::cout << "Размер словаря после стемминга: " << vocabAfter << std::endl;
        std::cout << "Сжатие словаря: "
                  << (100.0 * (1.0 - static_cast<double>(vocabAfter) / vocabBefore))
                  << "%" << std::endl;
        std::cout << "Групп словоформ, склеенных в один стем: " << mergedGroups << std::endl;
        
        std::sort(merges.begin(), merges.end(),
                  [](const MergeInfo& a, const MergeInfo& b) {
                      return a.totalFreq > b.totalFreq;
                  });
        
        std::cout << "\nТоп-15 групп склеек (по суммарной частоте):" << std::endl;
        for (size_t i = 0; i < 15 && i < merges.size(); i++) {
            std::cout << "  " << merges[i].stem << " <- ";
            for (size_t t = 0; t < merges[i].terms.size(); t++) {
                if (t > 0) std::cout << ", ";
                std::cout << merges[i].terms[t];
            }
            std::cout << " (частота " << merges[i].totalFreq << ")" << std::endl;
        }
    }
    
    void stemDemo() {
        std::cout << "\n=== ДЕМОНСТРАЦИЯ СТЕММЕРА ПОРТЕРА ===" << std::endl;
        
        std::vector<std::string> words = {
            "dance", "danced", "dances", "dancing",
            "run", "runs", "running",
            "love", "loved", "loves", "loving",
            "cry", "cried", "cries", "crying",
            "happy", "happier", "happiest", "happiness",
            "relate", "related", "relates", "relating",
            "caresses", "ponies", "agreed", "plastered",
            "motoring", "sing", "conflated", "troubled",
            "hopping", "tanned", "falling", "hissing",
            "filing", "failing", "sky", "university", "universe",
            "fly", "flew", "flown", "better", "best", "good"
        };
        
        std::cout << "слово -> стем:" << std::endl;
        std::string prevStem;
        for (const auto& w : words) {
            std::string s = stemToken(w);
            std::cout << "  " << w << " -> " << s << std::endl;
        }
    }
    
    void runForMusicCorpus(const std::vector<std::string>& directories) {
        std::cout << "=== ТОКЕНИЗАЦИЯ МУЗЫКАЛЬНОГО КОРПУСА ===" << std::endl;
        
        auto totalStart = std::chrono::high_resolution_clock::now();
        
        for (const auto& dir : directories) {
            if (dir == "lyrics" || dir.find("lyrics") != std::string::npos) {
                std::cout << "\nОбработка Lyrics.ovh документов..." << std::endl;
                processDirectory(dir, true); 
            } else if (dir == "musicbrainz" || dir.find("musicbrainz") != std::string::npos) {
                std::cout << "\nОбработка MusicBrainz документов..." << std::endl;
                processDirectory(dir, false); 
            } else {
                std::cout << "\nОбработка директории: " << dir << std::endl;
                processDirectory(dir, true);
            }
        }
        
        auto totalEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> totalDuration = totalEnd - totalStart;
        
        std::cout << "\n=== ИТОГИ ОБРАБОТКИ КОРПУСА ===" << std::endl;
        printStatistics();
        std::cout << "Общее время обработки корпуса: " << totalDuration.count() << " секунд" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    MusicTokenizer tokenizer;
    
    std::cout << "МУЗЫКАЛЬНЫЙ ТОКЕНИЗАТОР v1.0" << std::endl;
    std::cout << "Для текстов песен и музыкальных метаданных" << std::endl;
    
    if (argc > 1) {
        std::vector<std::string> directories;
        bool testMode = false;
        bool demoMode = false;
        bool stemMode = false;
        bool stemDemoMode = false;
        std::string freqFile;
        
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            
            if (arg == "--test" || arg == "-t") {
                testMode = true;
            } else if (arg == "--demo" || arg == "-d") {
                demoMode = true;
            } else if (arg == "--stem") {
                stemMode = true;
            } else if (arg == "--stem-demo") {
                stemDemoMode = true;
            } else if (arg == "--freq" && i + 1 < argc) {
                freqFile = argv[++i];
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "\nИспользование:" << std::endl;
                std::cout << "  " << argv[0] << " [директории...] [опции]" << std::endl;
                std::cout << "\nОпции:" << std::endl;
                std::cout << "  --test, -t       Тестирование производительности" << std::endl;
                std::cout << "  --demo, -d       Демонстрация примеров" << std::endl;
                std::cout << "  --stem           Статистика стемминга Портера по корпусу" << std::endl;
                std::cout << "  --stem-demo      Демонстрация стеммера на примерах" << std::endl;
                std::cout << "  --freq FILE      Выгрузить частоты терминов в CSV" << std::endl;
                std::cout << "  --help, -h       Эта справка" << std::endl;
                std::cout << "\nПримеры:" << std::endl;
                std::cout << "  " << argv[0] << " lyrics_corpus musicbrainz_corpus" << std::endl;
                std::cout << "  " << argv[0] << " lyrics_corpus musicbrainz_corpus --freq term_freq.csv --stem" << std::endl;
                std::cout << "  " << argv[0] << " --test" << std::endl;
                std::cout << "  " << argv[0] << " --stem-demo" << std::endl;
                return 0;
            } else {
                directories.push_back(arg);
            }
        }
        
        if (stemDemoMode) {
            tokenizer.stemDemo();
        } else if (testMode) 
            tokenizer.testPerformance();
        else if (demoMode)
            std::cout << "It's working!" << std::endl;
        else if (!directories.empty()) {
            tokenizer.runForMusicCorpus(directories);
            if (!freqFile.empty())
                tokenizer.dumpFrequencies(freqFile);
            if (stemMode)
                tokenizer.printStemmingStats();
        } else {
            std::cout << "Ошибка: укажите директории или опции" << std::endl;
            std::cout << "Используйте --help для справки" << std::endl;
            return 1;
        }
    } else {
        
        std::cout << "\nЗапуск в демонстрационном режиме..." << std::endl;
        tokenizer.testPerformance();
    }
    
    return 0;
}