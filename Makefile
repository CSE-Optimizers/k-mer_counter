CC = mpiCC
CPPFLAGS = -Wall
CXXFLAGS = -std=c++11

main: main.o extractor.o com.o kmer_dump.o utils.hpp MurmurHash2.o
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -o kmer_counter.out main.o extractor.o com.o kmer_dump.o MurmurHash2.o

main.o: main.cpp extractor.hpp com.hpp kmer_dump.hpp utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c main.cpp

extractor.o: extractor.cpp extractor.hpp utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c extractor.cpp

com.o: com.cpp com.hpp utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c com.cpp

kmer_dump.o: kmer_dump.cpp kmer_dump.hpp utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c kmer_dump.cpp

MurmurHash2.o: MurmurHash2.cpp MurmurHash2.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c MurmurHash2.cpp