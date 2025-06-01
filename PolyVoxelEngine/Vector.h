#pragma once
#include <type_traits>
#include <iostream>

template<size_t count, typename Enable = void>
class VectorSizeTypeSelector;

template<size_t count>
class VectorSizeTypeSelector<count, typename std::enable_if<(count < ((size_t)1 << 8))>::type>
{
public:
	using Type = uint8_t;
};

template<size_t count>
class VectorSizeTypeSelector<count, typename std::enable_if<(count >= ((size_t)1 << 8) && count < ((size_t)1 << 16))>::type>
{
public:
	using Type = uint16_t;
};

template<size_t count>
class VectorSizeTypeSelector<count, typename std::enable_if<(count >= ((size_t)1 << 16) && count < ((size_t)1 << 32))>::type>
{
public:
	using Type = uint32_t;
};

template<size_t count>
class VectorSizeTypeSelector<count, typename std::enable_if<(count >= ((size_t)1 << 32))>::type>
{
public:
	using Type = uint64_t;
};

template<typename T, size_t maxSize>
class Vector
{
	using SizeType = typename VectorSizeTypeSelector<maxSize>::Type;
#pragma pack(push, 1)
	T* data = nullptr;
	SizeType size = 0, capacity = 0;
#pragma pack(pop)

	void reAllocate(size_t newCapacity)
	{
		T* newData = (T*)::operator new(newCapacity * sizeof(T));

		if (newCapacity < size)
		{
			size = (SizeType)newCapacity;
		}
		size_t t_size = (size_t)size;
		for (size_t i = 0; i < t_size; i++)
		{
			newData[i] = std::move(data[i]);
			data[i].~T();
		}

		::operator delete(data, capacity * sizeof(T));
		data = newData;
		capacity = (SizeType)newCapacity;
	}
public:
	Vector()
	{}

	Vector(size_t initialSize)
	{
		reAllocate(initialSize);
		size = initialSize;
	}

	~Vector()
	{
		clear();
		::operator delete(data, capacity * sizeof(T));
	}

	Vector(const Vector& other)
	{
		size_t newSize = other.getSize();
		size_t newCapacity = other.getCapacity();

		data = (T*)::operator new(newCapacity * sizeof(T));
		for (size_t i = 0; i < newSize; i++)
		{
			data[i] = other[i];
		}

		size = newSize;
		capacity = newCapacity;
	}

	Vector(Vector&& other) noexcept
		: data(other.data), size(other.size), capacity(other.capacity)
	{
		other.data = nullptr;
		other.size = 0;
		other.capacity = 0;
	}

	Vector& operator=(const Vector& other)
	{
		if (this != &other)
		{
			reAllocate(other.capacity);
			size = other.size;
			size_t t_size = (size_t)size;
			for (size_t i = 0; i < t_size; i++)
			{
				data[i] = other[i];
			}
		}
		return *this;
	}

	Vector& operator=(Vector&& other) noexcept
	{
		if (this != &other)
		{
			clear();
			::operator delete(data, capacity * sizeof(T));

			data = other.data;
			size = other.size;
			capacity = other.capacity;

			other.data = nullptr;
			other.size = 0;
			other.capacity = 0;
		}
		return *this;
	}


	T& operator[](size_t index)
	{
		if (index >= size)
		{
			throw std::exception("Out of range");
		}
		return data[index];
	};

	const T& operator[](size_t index) const
	{
		if (index >= size)
		{
			throw std::exception("Out of range");
		}
		return data[index];
	};


	T* begin() const
	{
		return data;
	}

	T* end() const
	{
		return data + size;
	}


	void push(const T& value)
	{
		if (size >= capacity)
		{
			size_t newCapacity = std::min((size_t)capacity + ((size_t)capacity >> 1) + 1, maxSize);
			if (capacity == newCapacity)
			{
				throw std::exception("Max size limit reached");
			}
			reAllocate(newCapacity);
		}
		data[size++] = value;
	}

	void push(T&& value)
	{
		if (size >= capacity)
		{
			size_t newCapacity = std::min((size_t)capacity + ((size_t)capacity >> 1) + 1, maxSize);
			if (capacity == newCapacity)
			{
				throw std::exception("Max size limit reached");
			}
			reAllocate(newCapacity);
		}
		data[size++] = std::move(value);
	}

	template<typename... Args>
	T& emplace(Args&&... args)
	{
		if (size >= capacity)
		{
			size_t newCapacity = std::min((size_t)capacity + ((size_t)capacity >> 1) + 1, maxSize);
			if (capacity == newCapacity)
			{
				throw std::exception("Max size limit reached");
			}
			reAllocate(newCapacity);
		}
		new(&data[size]) T(std::forward<Args>(args)...);
		return data[size++];
	}

	void pop()
	{
		if (size == 0)
		{
			throw std::exception("Nothing left to pop");
		}
		size--;
		data[size].~T();
	}

	void pop(size_t index)
	{
		if (size == 0)
		{
			throw std::exception("Nothing left to pop");
		}
		else if (index > size)
		{
			throw std::exception("Out of range");
		}
		size--;

		data[index].~T();
		size_t t_size = (size_t)size;
		for (size_t i = index; i < t_size; i++)
		{
			data[i] = std::move(data[i + 1]);
		}
	}

	T popReturn()
	{
		if (size == 0)
		{
			throw std::exception("Nothing left to pop");
		}
		size--;
		return data[size];
	}

	T popReturn(size_t index)
	{
		if (size == 0)
		{
			throw std::exception("Nothing left to pop");
		}
		else if (index > size)
		{
			throw std::exception("Out of range");
		}
		size--;

		T returnV = data[index];

		size_t t_size = (size_t)size;
		for (size_t i = index; i < t_size; i++)
		{
			data[i] = std::move(data[i + 1]);
		}
		return returnV;
	}

	void remove(const T& item)
	{
		size_t index;
		size_t t_size = (size_t)size;
		bool found = false;
		for (index = 0; index < t_size; index++)
		{
			if (data[index] == item)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			return;
		}

		size--;
		t_size--;
		data[index].~T();
		for (size_t i = index; i < t_size; i++)
		{
			data[i] = std::move(data[i + 1]);
		}
	}

	void clear()
	{
		for (size_t i = 0; i < size; i++)
		{
			data[i].~T();
		}
		size = 0;
	}

	void reserve(size_t newCapacity)
	{
		if (capacity < newCapacity)
		{
			reAllocate(newCapacity);
		}
	}

	void resize(size_t newSize)
	{
		if (newSize == size)
		{
			return;
		}
		if (capacity < newSize)
		{
			reAllocate(newSize);
		}
		size = newSize;
	}


	T* getData() const
	{
		return data;
	}

	size_t getSize() const 
	{
		return (size_t)size;
	}

	size_t getCapacity() const
	{
		return (size_t)capacity;
	}
};

template<typename T, size_t maxSize>
void printVector(const Vector<T, maxSize>& vector)
{
	size_t size = vector.getSize();
	for (size_t i = 0; i < size; i++)
	{
		std::cout << vector[i] << " ";
	}
	std::cout << "\n";
}

template<typename T>
void printVector(const T& vector)
{
	size_t size = vector.size();
	for (size_t i = 0; i < size; i++)
	{
		std::cout << vector[i] << " ";
	}
	std::cout << "\n";
}