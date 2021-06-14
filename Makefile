CC = mpiCC
# CPPFLAGS = -pg

main: main.o extractor.o com.o kmer_dump.o
	$(CC) $(CPPFLAGS) -o kmer_counter.out main.o extractor.o com.o kmer_dump.o

main.o: main.cpp extractor.hpp com.hpp kmer_dump.hpp
	$(CC) $(CPPFLAGS) -c main.cpp

extractor.o: extractor.cpp extractor.hpp
	$(CC) $(CPPFLAGS) -c extractor.cpp

com.o: com.cpp com.hpp
	$(CC) $(CPPFLAGS) -c com.cpp

kmer_dump.o: kmer_dump.cpp kmer_dump.hpp
	$(CC) $(CPPFLAGS) -c kmer_dump.cpp