/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Steve Thomas <steve AT tobtu DOT com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <assert.h>
#include <stdint.h>
#include <algorithm>
#include <type_traits>

template <class T>
class BatchSet
{
public:
	BatchSet(size_t initSize = 0, size_t bufferSize = 1024)
	{
		if (bufferSize < 8)
		{
			bufferSize = 8;
		}
		if (initSize < 8)
		{
			initSize = 8;
		}
		m_data          = new T[initSize];
		m_tempData      = new T[bufferSize];
		m_dataCount     = 0;
		m_dataSize      = initSize;
		m_tempDataCount = 0;
		m_tempDataSize  = bufferSize;


		// Init binary search trade off
		// n+m vs n*log2(m)
		// m_tempDataSize + m_dataCount vs m_tempDataSize * log2(m_dataCount)
		size_t m = 4;
		size_t nlogm = 2 * bufferSize;

		while (bufferSize + m < nlogm && nlogm > bufferSize)
		{
			m *= 2;
			nlogm += bufferSize;
		}
		if (nlogm <= bufferSize) // overflow
		{
			m_bsTradeOff = SIZE_MAX;
		}
		else
		{
			m_bsTradeOff = m;
		}
	}

	BatchSet(const T *first, size_t count, size_t initSize = 0, size_t bufferSize = 1024)
	{
		// Init
		if (bufferSize < 8)
		{
			bufferSize = 8;
		}
		if (initSize < 8)
		{
			initSize = 8;
		}
		if (initSize < count)
		{
			initSize = count;
		}
		m_data          = new T[initSize];
		m_tempData      = new T[bufferSize];
		m_dataSize      = initSize;
		m_tempDataCount = 0;
		m_tempDataSize  = bufferSize;

		// Copy
		for (size_t i = 0; i < count; i++, ++first)
		{
			m_data[i] = *first;
		}

		// Sort
		std::sort(m_data, m_data + count);


		// Remove duplicates
		size_t j = 0;

		for (size_t i = 1; i < count; i++)
		{
			if (m_data[j] != m_data[i])
			{
				j++;
				if (j != i)
				{
					m_data[j] = m_data[i];
				}
			}
		}
		m_dataCount = j + 1;


		// Init binary search trade off
		// n+m vs n*log2(m)
		// m_tempDataSize + m_dataCount vs m_tempDataSize * log2(m_dataCount)
		size_t m = 4;
		size_t nlogm = 2 * bufferSize;

		while (bufferSize + m < nlogm)
		{
			m *= 2;
			nlogm += bufferSize;
			if (nlogm < bufferSize) // overflow
			{
				break;
			}
		}
		if (nlogm < bufferSize) // overflow
		{
			m_bsTradeOff = SIZE_MAX;
		}
		else
		{
			m_bsTradeOff = m;
		}
	}

	BatchSet(const BatchSet &set)
	{
		m_data          = new T[set.m_dataSize];
		m_tempData      = new T[set.m_tempDataSize];
		m_dataCount     = set.m_dataCount;
		m_dataSize      = set.m_dataSize;
		m_tempDataCount = set.m_tempDataCount;
		m_tempDataSize  = set.m_tempDataSize;
		m_bsTradeOff    = set.m_bsTradeOff;

		for (size_t i = 0, count = m_dataCount; i < count; i++)
		{
			m_data[i] = set.m_data[i];
		}
		for (size_t i = 0, count = m_tempDataCount; i < count; i++)
		{
			m_tempData[i] = set.m_tempData[i];
		}
	}

	~BatchSet()
	{
		delete [] m_data;
		delete [] m_tempData;
	}

	BatchSet &operator=(const BatchSet &set)
	{
		if (this != &set)
		{
			delete [] m_data;
			delete [] m_tempData;

			m_data          = new T[set.m_dataSize];
			m_tempData      = new T[set.m_tempDataSize];
			m_dataCount     = set.m_dataCount;
			m_dataSize      = set.m_dataSize;
			m_tempDataCount = set.m_tempDataCount;
			m_tempDataSize  = set.m_tempDataSize;
			m_bsTradeOff    = set.m_bsTradeOff;

			for (size_t i = 0, count = m_dataCount; i < count; i++)
			{
				m_data[i] = set.m_data[i];
			}
			for (size_t i = 0, count = m_tempDataCount; i < count; i++)
			{
				m_tempData[i] = set.m_tempData[i];
			}
		}
		return *this;
	}

	// O(1)
	// if the buffer is full then it needs to merge
	// merge costs (M = buffer size, N = current size):
	//   O(M*log(M))+O(M+N)+O(M+N)
	//   or
	//   O(M*log(M))+O(M*log(N))+O(M+N)
	void insert(const T &key)
	{
		m_tempData[m_tempDataCount] = key;
		if (++m_tempDataCount >= m_tempDataSize)
		{
			merge();
		}
	}

	// O(N)
	// if the buffer is full then it needs to merge
	// merge costs (M = buffer size, N = current size):
	//   O(M*log(M))+O(M+N)+O(M+N)
	//   or
	//   O(M*log(M))+O(M*log(N))+O(M+N)
	void erase(const T &key)
	{
		T *p;

		if (m_tempDataCount > 0)
		{
			merge();
		}
		p = std::lower_bound(m_data, m_data + m_dataCount, key);
		if (p != m_data + m_dataCount && key == *p)
		{
			T *end = m_data + m_dataCount;

			while (p != end)
			{
				if (std::is_arithmetic<T>::value)
				{
					p[0] = p[1];
				}
				else
				{
					// Prevent reallocating memory for std containers
					std::swap(p[0], p[1]);
				}
				p++;
			}
			m_dataCount--;
		}
	}

	// O(log(N))
	// if it has data in the buffer then it needs to merge
	// merge costs (M = buffer size, N = current size):
	//   O(M*log(M))+O(M+N)+O(M+N)
	//   or
	//   O(M*log(M))+O(M*log(N))+O(M+N)
	bool isSet(const T &key)
	{
		if (m_tempDataCount > 0)
		{
			merge();
		}
		return std::binary_search(m_data, m_data + m_dataCount, key);
	}

	// O(1)
	void clear()
	{
		m_dataCount     = 0;
		m_tempDataCount = 0;
	}

	// O(1)
	// if it has data in the buffer then it needs to merge
	// merge costs (M = buffer size, N = current size):
	//   O(M*log(M))+O(M+N)+O(M+N)
	//   or
	//   O(M*log(M))+O(M*log(N))+O(M+N)
	T *stealData(size_t &count, size_t &size)
	{
		T *ret;

		if (m_tempDataCount > 0)
		{
			merge();
		}
		ret   = m_data;
		count = m_dataCount;
		size  = m_dataSize;
		m_data = new T[8];
		m_dataCount = 0;
		m_dataSize = 8;
		return ret;
	}

	// O(1)
	// if it has data in the buffer then it needs to merge
	// merge costs (M = buffer size, N = current size):
	//   O(M*log(M))+O(M+N)+O(M+N)
	//   or
	//   O(M*log(M))+O(M*log(N))+O(M+N)
	T operator[](size_t pos)
	{
		if (m_tempDataCount > 0)
		{
			merge();
		}
		assert(pos >= 0 && pos < m_dataCount);
		return m_data[pos];
	}

	// O(1)
	// if it has data in the buffer then it needs to merge
	// merge costs (M = buffer size, N = current size):
	//   O(M*log(M))+O(M+N)+O(M+N)
	//   or
	//   O(M*log(M))+O(M*log(N))+O(M+N)
	size_t count()
	{
		if (m_tempDataCount > 0)
		{
			merge();
		}
		return m_dataCount;
	}

private:
	// Sort buffer, deduplicate, merge (M = buffer size, N = current size):
	// O(M*log(M))+O(M+N)+O(M+N)
	// or
	// O(M*log(M))+O(M*log(N))+O(M+N)
	void merge()
	{
		T      *data          = m_data;
		T      *tempData      = m_tempData;
		size_t  dataCount     = m_dataCount;
		size_t  tempDataCount = m_tempDataCount;
		size_t  i             = 0;
		size_t  j             = 0;
		size_t  outPos        = 0;

		assert(tempDataCount != 0);
		m_tempDataCount = 0;

		std::sort(tempData, tempData + tempDataCount);

		// Assume buffer is full, otherwise you'd need to do find the "m_bsTradeOff" for "m_tempDataCount"
		if (dataCount < m_bsTradeOff)
		{
			// Remove duplicates
			for (; j < dataCount; j++)
			{
				if (data[j] >= tempData[0])
				{
					break;
				}
			}
			if (j >= dataCount || data[j] > tempData[0])
			{
				outPos = 1;
			}
			for (i = 1; i < tempDataCount; i++)
			{
				if (tempData[i] != tempData[i - 1])
				{
					for (; j < dataCount; j++)
					{
						if (data[j] >= tempData[i])
						{
							break;
						}
					}
					if (j >= dataCount || data[j] > tempData[i])
					{
						if (outPos != i)
						{
							tempData[outPos] = tempData[i];
						}
						outPos++;
					}
				}
			}
			tempDataCount = outPos;
		}
		else
		{
			// Remove duplicates
			T *lo = data;
			T *hi = data + dataCount;

			lo = std::lower_bound(lo, hi, tempData[0]);
			if (lo == hi || *lo != tempData[0])
			{
				outPos = 1;
			}
			for (i = 1; i < tempDataCount; i++)
			{
				if (tempData[i] != tempData[i - 1])
				{
					lo = std::lower_bound(lo, hi, tempData[i]);
					if (lo == hi || *lo != tempData[i])
					{
						if (i != outPos)
						{
							tempData[outPos] = tempData[i];
						}
						outPos++;
					}
				}
			}
			tempDataCount = outPos;
		}

		// No new data
		if (tempDataCount == 0)
		{
			return;
		}

		// Merge
		if (dataCount + tempDataCount > m_dataSize)
		{
			if (dataCount + tempDataCount > 1024*1024)
			{
				// Add 1024*1024 and round up to nearest multiple of 1024*1024
				m_dataSize = (dataCount + tempDataCount + 2 * 1024 * 1024 - 1) & ~((size_t) 1024 * 1024 - 1);
			}
			else if (dataCount + tempDataCount > 1024)
			{
				// Double size and round up to nearest multiple of 1024
				m_dataSize = (2 * (dataCount + tempDataCount) + 1024 - 1) & ~((size_t) 1024 - 1);
			}
			else
			{
				// Double size and round up to nearest multiple of 8
				m_dataSize = (2 * (dataCount + tempDataCount) + 8 - 1) & ~((size_t) 8 - 1);
			}


			T *newData = new T[m_dataSize];

			// Do this to not break classes that have allocated memory
			if (std::is_arithmetic<T>::value)
			{
				memcpy(newData, data, sizeof(T) * dataCount);
			}
			else
			{
				// Prevent reallocating memory for std containers
				std::swap_ranges(data, data + dataCount, newData);
			}
			delete [] data;
			data = newData;
			m_data = newData;
		}
		i = tempDataCount;
		j = dataCount;
		dataCount += tempDataCount;
		m_dataCount = dataCount;
		outPos = dataCount - 1;
		while (i > 0 && j > 0)
		{
			if (tempData[i - 1] < data[j - 1])
			{
				data[outPos--] = data[--j];
			}
			else
			{
				data[outPos--] = tempData[--i];
			}
		}
		while (i > 0)
		{
			data[outPos--] = tempData[--i];
		}
	}

	T      *m_data;
	T      *m_tempData;
	size_t  m_dataCount;
	size_t  m_dataSize;
	size_t  m_tempDataCount;
	size_t  m_tempDataSize;
	size_t  m_bsTradeOff;
};
