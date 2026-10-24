CXX      = clang++
CXXFLAGS = -std=c++17 -O2 -Wall
TARGET   = lexer
CORPORA  = ../lab1/lyrics_corpus ../lab1/musicbrainz_corpus

all: $(TARGET)

$(TARGET): lexer.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET) $(CORPORA)

test: $(TARGET)
	./$(TARGET) --test

clean:
	rm -f $(TARGET)

.PHONY: all run test clean
