#include "kmer_dump.hpp"
using std::string;
using std::to_string;

struct StringToIntSerializer
{
  bool operator()(FILE *fp, const std::pair<const size_t, size_t> &value) const
  {
    // Write the key.
    if (fwrite(&value.first, sizeof(value.first), 1, fp) != 1)
      return false;
    // Write the value.
    if (fwrite(&value.second, sizeof(value.second), 1, fp) != 1)
      return false;
    return true;
  }
  bool operator()(FILE *fp, std::pair<const size_t, size_t> *value) const
  {
    // Read the key.  Note the need for const_cast to get around
    // the fact hash_map keys are always const.
    if (fread(const_cast<size_t *>(&value->first), sizeof(value->first), 1, fp) != 1)
      return false;
    // Read the value.
    if (fread(const_cast<size_t *>(&value->second), sizeof(value->second), 1, fp) != 1)
      return false;
    return true;
  }
};

void mergeHashmap(dense_hash_map<size_t, size_t> newHashMap, int partition, string base_path)
{
  // Assuming path exist
  string file_path = base_path + DIRECTORY_SEP + std::to_string(partition) + ".data";
  FILE *fp = fopen(file_path.c_str(), "r");
  dense_hash_map<size_t, size_t> oldHashMap;

  if (fp)
  {
    oldHashMap.unserialize(StringToIntSerializer(), fp);
    fclose(fp);
  }

  google::dense_hash_map<size_t, size_t>::iterator it = newHashMap.begin();

  for (; it != newHashMap.end(); ++it)
  {
    if (oldHashMap.count(it->first) > 0)
    {
      oldHashMap[it->first] += it->second;
    }
    else
    {
      oldHashMap[it->first] = it->second;
    }
  }
  // it = oldHashMap.begin();
  // for (; it != oldHashMap.end(); ++it){
  //     std::cout << it->first << " " << it->second << std::endl;
  // }

  // Writing back to file
  fp = fopen(file_path.c_str(), "w");
  oldHashMap.serialize(StringToIntSerializer(), fp);
  fclose(fp);
}

void mergeArrayToHashmap(size_t *dataArray, int dataArrayLength, int partition, string base_path)
{
  // Assuming path exist
  string file_path = base_path + DIRECTORY_SEP + to_string(partition) + ".data";
  FILE *fp = fopen(file_path.c_str(), "r");
  dense_hash_map<size_t, size_t> oldHashMap;
  oldHashMap.set_empty_key(-1);

  if (fp)
  {
    oldHashMap.unserialize(StringToIntSerializer(), fp);
    fclose(fp);
  }

  for (int i = 0; i < dataArrayLength; i += 2)
  {
    if (oldHashMap[dataArray[i]] > 0)
    {
      oldHashMap[dataArray[i]] += dataArray[i + 1];
    }
    else
    {
      oldHashMap[dataArray[i]] = dataArray[i + 1];
    }
  }

  // Writing back to file
  fp = fopen(file_path.c_str(), "w");
  oldHashMap.serialize(StringToIntSerializer(), fp);
  fclose(fp);
  oldHashMap.clear();
}

void dumpHashmap(dense_hash_map<size_t, size_t> hashMap, int partition, int partitionFileCounts[], string base_path)
{
  // Assumes file_path exist
  string file_path = base_path + DIRECTORY_SEP + to_string(partition) + DIRECTORY_SEP + to_string(partitionFileCounts[partition]++);

  FILE *fp = fopen(file_path.c_str(), "w");
  hashMap.serialize(StringToIntSerializer(), fp);
  fclose(fp);
}

void loadHashMap(dense_hash_map<std::size_t, std::size_t> *hashMap, int partition, string base_path)
{
  string file_path = base_path + DIRECTORY_SEP + to_string(partition) + ".data";
  FILE *fp = fopen(file_path.c_str(), "r");

  if (fp)
  {
    (*hashMap).unserialize(StringToIntSerializer(), fp);
    fclose(fp);
  }
}

/*
  Overwrites any existing file.
  */
void saveHashMap(dense_hash_map<std::size_t, std::size_t> *hashMap, int partition, string base_path)
{

  string file_path = base_path + DIRECTORY_SEP + to_string(partition) + ".data";
  FILE *fp = fopen(file_path.c_str(), "w");

  if (fp)
  {
    (*hashMap).serialize(StringToIntSerializer(), fp);
    fclose(fp);
  }
}