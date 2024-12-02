#pragma once
#include <vector>
#include <memory>


template <typename T>
class ObjectPool
{
	std::vector<T*> pool;
public:
	explicit ObjectPool(size_t initialCapacity);
	~ObjectPool();

	T* acquire();
	void release(T* obj);
	void reserve(size_t newCapacity);
	void clear();
};

template<typename T>
inline ObjectPool<T>::ObjectPool(size_t initialCapacity)
{
	pool.reserve(initialCapacity);
	for (size_t i = 0; i < initialCapacity; i++)
	{
		pool.push_back(new T());
	}
}

template<typename T>
inline ObjectPool<T>::~ObjectPool()
{
	clear();
}

template<typename T>
inline T* ObjectPool<T>::acquire() 
{
	if (pool.empty()) 
	{
		return new T();
	}
	T* obj = pool.back();
	pool.pop_back();
	return obj;
}

template<typename T>
inline void ObjectPool<T>::release(T* obj)
{
	pool.push_back(obj);
}

template<typename T>
inline void ObjectPool<T>::reserve(size_t newCapacity)
{
	if (newCapacity <= pool.capacity())
	{
		return;
	}
	size_t add = newCapacity - pool.size();
	for (size_t i = 0; i < add; i++)
	{
		pool.push_back(new T());
	}
}

template<typename T>
inline void ObjectPool<T>::clear()
{
	for (T* obj : pool)
	{
		delete obj;
	}
	pool.clear();
}
