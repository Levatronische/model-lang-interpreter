CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2

all: itr

itr: itr.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

stages: sa pl

sa: sa.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

pl: pl.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

la: la.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f itr sa pl la

.PHONY: all stages clean
