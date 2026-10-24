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

stem-demo: $(TARGET)
	./$(TARGET) --stem-demo

stem: $(TARGET)
	./$(TARGET) $(CORPORA) --stem

freq: $(TARGET)
	./$(TARGET) $(CORPORA) --freq term_freq.csv

zipf: freq
	python3 plot_zipf.py
	cp zipf.pdf report/

clean:
	rm -f $(TARGET) term_freq.csv

.PHONY: all run test clean
