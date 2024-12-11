#pragma once
#include <vector>


template <typename T>
class VectorPool
{
	std::vector<std::vector<T>> pool;
public:
	explicit VectorPool(size_t initialCapacity);
	~VectorPool();

	std::vector<T>&& acquire();
	void release(std::vector<T>& obj);
	void reserve(size_t newCapacity);
	void clear();

	size_t getSize() const;
};

template<typename T>
inline VectorPool<T>::VectorPool(size_t initialCapacity)
{
	pool.reserve(initialCapacity);
	for (size_t i = 0; i < initialCapacity; i++)
	{
		pool.emplace_back();
	}
}

template<typename T>
inline VectorPool<T>::~VectorPool()
{
	clear();
}

template<typename T>
inline std::vector<T>&& VectorPool<T>::acquire()
{
	if (pool.empty())
	{
		return std::move(std::vector<T>());
	}
	std::vector<T>& obj = pool.back();
	pool.pop_back();
	return std::move(obj);
}

template<typename T>
inline void VectorPool<T>::release(std::vector<T>& obj)
{
	pool.emplace_back();
	pool.back().swap(obj);
}

template<typename T>
inline void VectorPool<T>::reserve(size_t newCapacity)
{
	if (newCapacity <= pool.capacity())
	{
		return;
	}
	size_t add = newCapacity - pool.size();
	for (size_t i = 0; i < add; i++)
	{
		pool.emplace_back();
	}
}

template<typename T>
inline void VectorPool<T>::clear()
{
	for (std::vector<T>& obj : pool)
	{
		obj.clear();
	}
	pool.clear();
}

template<typename T>
inline size_t VectorPool<T>::getSize() const
{
	return pool.size();
}
