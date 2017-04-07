gtest_include_dir = ./deps/googletest/googletest/include
CXX_FLAGS = -Wall -Wextra -pthread -std=c++11 -I$(gtest_include_dir) -I TinySTL
CC = g++
ODIR = obj
LDIR = lib

# manul:
#
# prerequisite:
# 1. make crawl   --- crawl web sites
# 2. make map     --- generate map files
# 3. make index   --- generate inverted index files
# 4. make grank   --- generate pagerank
#
# searching:
# 4. make search  --- console search
# 4. make query   --- python search
# 4. make server  --- start server and search in web page

.PHONY: crawl clean test map rank index search grank query

all: map index

crawl: crawler/crawler.py
	@cd crawler && python3 crawler.py

map: generate_map.py
	@python3 generate_map.py

rank: map pagerank.cpp
	g++ $(CXX_FLAGS) pagerank.cpp -o $(ODIR)/pagerank
	$(ODIR)/pagerank


niters = 15
../data/tmp/graph.txt: ../data/graph.txt
	@cp ../data/graph.txt ../data/tmp/graph.txt
$(ODIR)/grank: ../data/tmp/graph.txt grank.cpp
	#g++ -std=c++11 -g -O3 -I ./deps/graphchi-cpp/src/  -fopenmp -Wall -Wno-strict-aliasing grank.cpp -o $(ODIR)/grank -lz
	@cp ./deps/graphchi-cpp/conf $(ODIR)/ -r
grank: $(ODIR)/grank
	#@cd $(ODIR) && ./grank file ../../data/tmp/graph.txt niters $(niters) top 0
	@cd $(ODIR) && ./grank file ../../data/tmp/graph.txt niters $(niters) top 0 > ../../data/pagerank.txt 2> /dev/null

index: generate_index.cpp
	g++ $(CXX_FLAGS) generate_index.cpp -o $(ODIR)/generate_index -Ideps/cppjieba/include -Ideps/cppjieba/deps
	$(ODIR)/generate_index

$(ODIR)/search: search.cpp util.hpp
	g++ $(CXX_FLAGS) search.cpp -o $(ODIR)/search -Ideps/cppjieba/include -Ideps/cppjieba/deps

search:$(ODIR)/search
	$(ODIR)/search

query: $(ODIR)/search
	python3 server/query.py

server: $(ODIR)/search
	$(shell export FLASK_APP=server/index.py && flask run)

test_obj = $(ODIR)/test/TinySTLTest.o
$(test_obj): TinySTL/test/TinySTLTest.cpp TinySTL/test/*.h TinySTL/*.hpp
	-mkdir $(ODIR)/test
	$(CC) $(CXX_FLAGS) -c TinySTL/test/TinySTLTest.cpp -o $(ODIR)/test/TinySTLTest.o

test: $(test_obj)
	$(CC) $(CXX_FLAGS) $(ODIR)/test/TinySTLTest.o $(LDIR)/libgtest.a -o  $(ODIR)/test/TinySTLTest
	$(ODIR)/test/TinySTLTest

clean:
	-rm -rf $(ODIR)/*
