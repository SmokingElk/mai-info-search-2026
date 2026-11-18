CXX        ?= clang++
CXXFLAGS   ?= -std=c++17 -O2 -Wall -Wextra
CORPUS_DIR ?= ../lab1/lyrics_corpus/html
INDEX_FILE ?= build/index.bin

SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:src/%.cpp=build/%.o)
DEP := $(OBJ:.o=.d)

BIN := build/boolean_search

all: $(BIN)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Iinclude -MMD -MP -c $< -o $@

demo: $(BIN)
	./$(BIN) demo

# Индексация корпуса. Корпус по умолчанию ../lab1/lyrics_corpus/html,
# можно переопределить: make index CORPUS_DIR=../lab1/musicbrainz_corpus/html
# или через переменную окружения: CORPUS_DIR=... make index
index: $(BIN)
	./$(BIN) build $(CORPUS_DIR) $(INDEX_FILE)

search: $(BIN)
	@test -n "$(QUERY)" || { echo 'Usage: make search QUERY="your query here"'; exit 2; }
	@test -f $(INDEX_FILE) || { echo "Index $(INDEX_FILE) not found. Run: make index [CORPUS_DIR=<path>]"; exit 1; }
	./$(BIN) search $(INDEX_FILE) "$(QUERY)"

stats: $(BIN)
	./$(BIN) stats

clean:
	rm -rf build

-include $(DEP)

.PHONY: all demo index search stats clean

demo_search:
	./$(BIN) search $(INDEX_FILE) "love"