#ifndef THREADSAFEHASHTABLE_H
#define THREADSAFEHASHTABLE_H

#include <mutex>
#include <vector>
#include <shared_mutex>
#include <list>
#include <algorithm>

template <typename K, typename V, typename hash_function = std::hash<K>>
class thread_safe_hash_table{
private:

  struct Bucket{
  private:
    using bucket_data = std::pair<K, V>;

    std::list<bucket_data> data;

    mutable std::shared_mutex m;

  public:
    void insert(K&& key, V&& value){
      std::lock_guard<std::shared_mutex> lock(m);
      data.push_back( { std::forward<K>(key), std::forward<V>(value) } );
    }

    template <typename... Args>
    void emplace(K&& key, Args&&... args){
      std::lock_guard<std::shared_mutex> lock(m);
      data.push_back( { std::forward<K>(key), V(std::forward<V>(args)...) } );
    }

    void erase(K&& key){
      std::lock_guard<std::shared_mutex> lock(m);

      auto it = find_if(data.begin(), data.end(), [&key](const bucket_data& data){
          return data.first == key;
        });

      if(it != data.end()){
        data.erase(it);
      }
    }

    V* get_value(const K& key){
      std::shared_lock lock(m);

      auto it = find_if(data.begin(), data.end(), [&key](const bucket_data& data){
          return data.first == key;
        });

      return it != data.end() ? &it->second : nullptr;
    }

    V& get_last_element(){
      std::shared_lock lock(m);

      return data.back().second;
    }
  };

  size_t get_index_by_key(const K& key) const{
    hash_function hash;
    return hash(key) % buckets_count;
  }

  size_t buckets_count;
  std::shared_mutex m;

  std::vector<Bucket> data;

public:
  thread_safe_hash_table() : buckets_count(10), data(buckets_count){}
  ~thread_safe_hash_table() = default;

  void insert(K&& key, V&& value){
    size_t index = get_index_by_key(key);

    std::shared_lock lock(m);
    data[index].insert(std::forward<K>(key), std::forward<V>(value));
  }

  template <typename... Args>
  void emplace(K&& key, Args&&... args){
    size_t index = get_index_by_key(key);

    std::shared_lock lock(m);
    data[index].emplace(std::forward<K>(key), V(std::forward<V>(args)...));
  }

  void erase(K&& key){
    size_t index = get_index_by_key(key);

    std::shared_lock lock(m);
    data[index].erase(std::forward<K>(key));
  }

  V& operator [] (K&& key){
    size_t index = get_index_by_key(key);

    std::shared_lock lock(m);
    V* value = data[index].get_value(std::forward<K>(key));

    if(!value){
      data[index].insert(std::forward<K>(key), V());
    }

    return value ? *value : data[index].get_last_element();
  }

  V& at(K&& key){
    size_t index = get_index_by_key(key);

    std::shared_lock lock(m);
    V* value = data[index]->get_value(std::forward<K>(key));

    if(!value)
        throw std::out_of_range("Key doesn't exist");

    return *value;
  }

  void clear(){
    std::lock_guard<std::shared_mutex> lock(m);
    data.clear();
  }

  size_t bucket_count() const{
    return buckets_count;
  }
};

#endif // THREADSAFEHASHTABLE_H
