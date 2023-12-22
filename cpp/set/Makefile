CXX := g++
CXXFLAGS := -std=c++17 -Wall -Werror -Wextra

src := $(wildcard *.cpp)
obj := $(addsuffix .o, $(basename $(src)))
bin := wordle_solver

.PHONY: all clean

all: $(bin)

$(bin): $(obj)
	$(CXX) $(LDFLAGS) $^ -o $@

main.o: main.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

clean:
	rm -f $(obj) $(bin)
