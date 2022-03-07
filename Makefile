CC = mpic++
CPPFLAGS = -Wall
CXXFLAGS = -O2 -std=c++11 -pthread -lboost_thread -lboost_system
RM = rm

USER_LIBS = -I /home/ruchin/WorkSpace/Campus/FYP/tools/sparsehash/src

kmer_distributor: distributor_main.o encoder.o encoder_writer.o utils.hpp thread_safe_queue.o file_reader.o MurmurHash2.o
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -o kmer_distributor.out distributor_main.o encoder.o encoder_writer.o thread_safe_queue.o file_reader.o MurmurHash2.o $(USER_LIBS)

kmer_counter: counter_main.o counter.o extractor.o generator.o kmer_dump.o kmer_writer.o thread_safe_queue.o writer.o MurmurHash2.o utils.hpp 
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -o kmer_counter.out counter_main.o counter.o extractor.o generator.o kmer_dump.o kmer_writer.o thread_safe_queue.o writer.o MurmurHash2.o $(USER_LIBS)

# kmer_counter_with_queries: count_with_query_main.o binary_counter.o extractor.o generator_with_query.o thread_safe_queue.o utils.hpp 
# 	$(CC) $(CXXFLAGS) $(CPPFLAGS) -o kmer_counter_with_queries.out count_with_query_main.o binary_counter.o extractor.o generator_with_query.o thread_safe_queue.o utils.hpp $(USER_LIBS)

# count_with_query_main.o: count_with_query_main.cpp utils.hpp thread_safe_queue.hpp extractor.hpp generator_with_query.hpp 
# 	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c count_with_query_main.cpp $(USER_LIBS)

# binary_counter.o: binary_counter.cpp binary_counter.hpp thread_safe_queue.hpp 
# 	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c binary_counter.cpp $(USER_LIBS)

# generator_with_query.o: generator_with_query.cpp generator_with_query.hpp utils.hpp thread_safe_queue.hpp
# 	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c generator_with_query.cpp $(USER_LIBS)

counter_main.o: counter_main.cpp utils.hpp thread_safe_queue.hpp extractor.hpp generator.hpp kmer_writer.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c counter_main.cpp $(USER_LIBS)

extractor.o: extractor.cpp extractor.hpp utils.hpp thread_safe_queue.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c extractor.cpp $(USER_LIBS)

thread_safe_queue.o: thread_safe_queue.cpp thread_safe_queue.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c thread_safe_queue.cpp $(USER_LIBS)

counter.o: counter.cpp counter.hpp utils.hpp thread_safe_queue.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c counter.cpp $(USER_LIBS)

generator.o: generator.cpp generator.hpp utils.hpp thread_safe_queue.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c generator.cpp $(USER_LIBS)

kmer_writer.o: kmer_writer.cpp kmer_writer.hpp utils.hpp thread_safe_queue.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c kmer_writer.cpp $(USER_LIBS)

writer.o: writer.cpp writer.hpp utils.hpp kmer_dump.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c writer.cpp $(USER_LIBS)

kmer_dump.o: kmer_dump.cpp kmer_dump.hpp utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c kmer_dump.cpp $(USER_LIBS)

distributor_main.o: distributor_main.cpp utils.hpp thread_safe_queue.hpp file_reader.hpp encoder.hpp encoder_writer.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c distributor_main.cpp $(USER_LIBS)

encoder.o: encoder.cpp thread_safe_queue.hpp utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c encoder.cpp $(USER_LIBS)

encoder_writer.o: encoder_writer.cpp encoder_writer.hpp utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c encoder_writer.cpp $(USER_LIBS)

MurmurHash2.o: MurmurHash2.cpp MurmurHash2.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c MurmurHash2.cpp $(USER_LIBS)

file_reader.o: file_reader.cpp file_reader.hpp extractor.hpp utils.hpp
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c file_reader.cpp $(USER_LIBS)

clean: 
	$(RM) *.o *.out 