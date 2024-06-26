/**
 * @file lrucache.hpp
 * @brief Simple LRU (Least Recently Used) cache implementation.
 * 
 * This file contains the implementation of a simple LRU cache. The LRU cache is a data structure
 * that holds a fixed number of key-value pairs. When the cache reaches its capacity and a new
 * key-value pair needs to be inserted, the least recently used (LRU) key-value pair is evicted
 * from the cache to make room for the new pair.
 * 
 * This implementation uses a combination of a std::list and a std::unordered_map to achieve
 * O(1) time complexity for insert, delete, and access operations. The std::list is used to keep
 * track of the order of elements (with the most recently used at the front), and the
 * std::unordered_map is used for fast lookup of elements in the list.
 * 
 * @tparam key_t The type of the keys in the cache.
 * @tparam value_t The type of the values in the cache.
 * 
 * Usage example:
 * 
 * cache::lru_cache<int, std::string> my_cache(5); // Create a cache for up to 5 key-value pairs
 * my_cache.put(1, "one"); // Insert a key-value pair into the cache
 * try {
 *     std::string value = my_cache.get(1); // Access a value by its key
 *     std::cout << value << std::endl;
 * } catch (const std::range_error& e) {
 *     std::cerr << e.what() << std::endl;
 * }
 * 
 * @author Alexander Ponomarev
 * @date June 20, 2013
 */

#ifndef _LRUCACHE_HPP_INCLUDED_
#define	_LRUCACHE_HPP_INCLUDED_

#include <unordered_map>
#include <list>
#include <cstddef>
#include <stdexcept>
#include <mutex>

namespace cache {

template<typename key_t, typename value_t>
class lru_cache {
public:
	typedef typename std::pair<key_t, value_t> key_value_pair_t;
	typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

	lru_cache(size_t max_size) :
		_max_size(max_size) {
	}
	
	void put(const key_t& key, const value_t& value) {
		std::lock_guard<std::mutex> lock(_mutex);
		auto it = _cache_items_map.find(key);
		_cache_items_list.push_front(key_value_pair_t(key, value));
		if (it != _cache_items_map.end()) {
			_cache_items_list.erase(it->second);
			_cache_items_map.erase(it);
		}
		_cache_items_map[key] = _cache_items_list.begin();
		
		if (_cache_items_map.size() > _max_size) {
			auto last = _cache_items_list.end();
			last--;
			_cache_items_map.erase(last->first);
			_cache_items_list.pop_back();
		}
	}
	
	const value_t& get(const key_t& key) {
		std::lock_guard<std::mutex> lock(_mutex);
		auto it = _cache_items_map.find(key);
		if (it == _cache_items_map.end()) {
			throw std::range_error("There is no such key in cache");
		} else {
			_cache_items_list.splice(_cache_items_list.begin(), _cache_items_list, it->second);
			return it->second->second;
		}
	}
	
	bool exists(const key_t& key) const {
		std::lock_guard<std::mutex> lock(_mutex);
		return _cache_items_map.find(key) != _cache_items_map.end();
	}
	
	size_t size() const {
		std::lock_guard<std::mutex> lock(_mutex);
		return _cache_items_map.size();
	}
	
private:
	std::list<key_value_pair_t> _cache_items_list;
	std::unordered_map<key_t, list_iterator_t> _cache_items_map;
	size_t _max_size;
	mutable std::mutex _mutex;
};

} // namespace cache

#endif	/* _LRUCACHE_HPP_INCLUDED_ */