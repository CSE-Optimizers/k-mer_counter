CC = mpiCC
CPPFLAGS = -Wall
CXXFLAGS = -std=c++11 -L/home/ruchin/boost-1.77.0/built/lib -lboost_thread -lboost_system

RM = rm

main: main.o extractor.o com.o kmer_dump.o utils.hpp MurmurHash2.o thread_safe_queue.o counter.o writer.o
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -o kmer_counter.out main.o extractor.o com.o kmer_dump.o MurmurHash2.o thread_safe_queue.o counter.o writer.o

main.o: main.cpp extractor.hpp com.hpp kmer_dump.hpp utils.hpp counter.hpp thread_safe_queue.hpp writer.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c main.cpp

extractor.o: extractor.cpp extractor.hpp utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c extractor.cpp

com.o: com.cpp com.hpp utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c com.cpp

writer.o: writer.cpp writer.hpp utils.hpp kmer_dump.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c writer.cpp

thread_safe_queue.o: thread_safe_queue.cpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c thread_safe_queue.cpp

counter.o: counter.cpp thread_safe_queue.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c counter.cpp

finalizer: finalizer.o MurmurHash2.o kmer_dump.o utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -o finalizer.out finalizer.o kmer_dump.o MurmurHash2.o

finalizer.o: finalizer.cpp kmer_dump.hpp utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c finalizer.cpp

kmer_dump.o: kmer_dump.cpp kmer_dump.hpp utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c kmer_dump.cpp

MurmurHash2.o: MurmurHash2.cpp MurmurHash2.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c MurmurHash2.cpp

clean: 
	$(RM) kmer_counter.out *.o
