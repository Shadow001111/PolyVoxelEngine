#pragma once
#include <vector>


template <typename T>
class AllocatedObjectPool
{
	std::vector<T*> pool;
public:
	explicit AllocatedObjectPool(size_t initialCapacity);
	~AllocatedObjectPool();

	T* acquire();
	void release(T* obj);
	void reserve(size_t newCapacity);
	void clear();

	size_t getSize() const;
};

template<typename T>
inline AllocatedObjectPool<T>::AllocatedObjectPool(size_t initialCapacity)
{
	pool.reserve(initialCapacity);
	for (size_t i = 0; i < initialCapacity; i++)
	{
		pool.push_back(new T());
	}
}

template<typename T>
inline AllocatedObjectPool<T>::~AllocatedObjectPool()
{
	clear();
}

template<typename T>
inline T* AllocatedObjectPool<T>::acquire() 
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
inline void AllocatedObjectPool<T>::release(T* obj)
{
	pool.push_back(obj);
}

template<typename T>
inline void AllocatedObjectPool<T>::reserve(size_t newCapacity)
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
inline void AllocatedObjectPool<T>::clear()
{
	for (T* obj : pool)
	{
		delete obj;
	}
	pool.clear();
}

template<typename T>
inline size_t AllocatedObjectPool<T>::getSize() const
{
	return pool.size();
}
