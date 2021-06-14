CC = mpiCC
# CPPFLAGS = -pg
CXXFLAGS = -std=c++11

main: main.o extractor.o com.o kmer_dump.o
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -o kmer_counter.out main.o extractor.o com.o kmer_dump.o

main.o: main.cpp extractor.hpp com.hpp kmer_dump.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c main.cpp

extractor.o: extractor.cpp extractor.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c extractor.cpp

com.o: com.cpp com.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c com.cpp

kmer_dump.o: kmer_dump.cpp kmer_dump.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c kmer_dump.cpp