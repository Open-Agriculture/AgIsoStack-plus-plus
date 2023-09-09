/*
  MIT License

  Copyright (c) 2018 Antonio Alexander Brewer (tonton81) - https://github.com/tonton81

  Contributors:
  Tim - https://github.com/Defragster
  Mike - https://github.com/mjs513

  Designed and tested for PJRC Teensy 3.2, 3.5, and 3.6 boards.
  May or may not work on other microcontrollers, support for them will not be provided.
  Use at your own risk.

  Forum link : https://forum.pjrc.com/threads/50395-Circular_Buffer

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H
#include <algorithm>

template<typename T, uint16_t _size, uint16_t multi = 0>
class Circular_Buffer
{
public:
	void push_back(T value)
	{
		return write(value);
	}
	void push_front(T value);
	T pop_front()
	{
		return read();
	}
	T pop_back();
	void write(T value);
	void push_back(const T *buffer, uint16_t length)
	{
		write(buffer, length);
	}
	void write(const T *buffer, uint16_t length);
	void push_front(const T *buffer, uint16_t length);
	T peek(uint16_t pos = 0);
	T peekBytes(T *buffer, uint16_t length);
	T read();
	T pop_front(T *buffer, uint16_t length)
	{
		return readBytes(buffer, length);
	}
	T peek_front(T *buffer, uint16_t length, uint32_t entry = 0);
	T read(T *buffer, uint16_t length)
	{
		return readBytes(buffer, length);
	}
	T readBytes(T *buffer, uint16_t length);
	void flush()
	{
		clear();
	}
	void clear()
	{
		head = tail = _available = 0;
	}
	void print(const char *p);
	void println(const char *p);
	uint16_t size()
	{
		return _available;
	}
	uint16_t available()
	{
		return _available;
	}
	uint16_t capacity()
	{
		return _size;
	}
	uint16_t length_back()
	{
		return (((int)_cabuf[((head + size() - 1) & (_size - 1))][0] << 8 * sizeof(T)) | (int)_cabuf[((head + size() - 1) & (_size - 1))][1]);
	}
	uint16_t length_front()
	{
		return (((int)_cabuf[((head) & (_size - 1))][0] << 8 * sizeof(T)) | (int)_cabuf[((head) & (_size - 1))][1]);
	}
	T list();
	T variance();
	T deviation();
	T average();
	bool remove(uint16_t pos);
	T median(bool override = 0);
	void sort_ascending();
	void sort_descending();
	T sum();
	T min();
	T max();
	T mean()
	{
		return average();
	}
	uint16_t max_size()
	{
		return multi;
	}
	T pop_back(T *buffer, uint16_t length);
	T *peek_front()
	{
		return front();
	}
	T *peek_back()
	{
		return back();
	}
	T *front()
	{
		return _cabuf[((head) & (_size - 1))] + 2;
	}
	T *back()
	{
		return _cabuf[((tail - 1) & (_size - 1))] + 2;
	}
	bool replace(T *buffer, uint16_t length, int pos1, int pos2, int pos3, int pos4 = -1, int pos5 = -1);
	bool isEqual(const T *buffer);
	bool find(T *buffer, uint16_t length, int pos1, int pos2, int pos3, int pos4 = -1, int pos5 = -1);
	bool findRemove(T *buffer, uint16_t length, int pos1, int pos2, int pos3, int pos4 = -1, int pos5 = -1);

protected:
private:
	volatile uint16_t head = 0;
	volatile uint16_t tail = 0;
	volatile uint16_t _available = 0;

	T _cbuf[_size];
	T _cabuf[_size][multi + 2];
};

template<typename T, uint16_t _size, uint16_t multi>
bool Circular_Buffer<T, _size, multi>::remove(uint16_t pos)
{
	if (multi)
	{
		if (pos >= _size)
			return 0;

		int32_t find_area = -1;

		for (uint16_t i = 0; i < _size; i++)
		{
			if (((head + i) & (_size - 1)) == pos)
			{
				find_area = i;
				break;
			}
		}
		if (find_area == -1)
			return 0;

		while (((head + find_area) & (_size - 1)) != ((head) & (_size - 1)))
		{
			memmove(_cabuf[((head + find_area) & (_size - 1))], _cabuf[((head + find_area - 1) & (_size - 1))], (2 + multi) * sizeof(T));
			find_area--;
		}
		head = ((head + 1) & (2 * _size - 1));
		_available--;
		return 1;
	}
	return 0;
}

template<typename T, uint16_t _size, uint16_t multi>
bool Circular_Buffer<T, _size, multi>::findRemove(T *buffer, uint16_t length, int pos1, int pos2, int pos3, int pos4, int pos5)
{
	uint8_t input_count = 3;
	int32_t found = -1;
	if (pos4 != -1)
		input_count = 4;
	if (pos5 != -1)
		input_count = 5;
	for (uint16_t j = 0; j < _available; j++)
	{
		switch (input_count)
		{
			case 3:
			{
				if (_cabuf[((head + j) & (_size - 1))][pos1 + 2] == buffer[pos1] && _cabuf[((head + j) & (_size - 1))][pos2 + 2] == buffer[pos2] &&
				    _cabuf[((head + j) & (_size - 1))][pos3 + 2] == buffer[pos3])
				{
					found = j;
					break;
				}
			}
			case 4:
			{
				if (_cabuf[((head + j) & (_size - 1))][pos1 + 2] == buffer[pos1] && _cabuf[((head + j) & (_size - 1))][pos2 + 2] == buffer[pos2] &&
				    _cabuf[((head + j) & (_size - 1))][pos3 + 2] == buffer[pos3] && _cabuf[((head + j) & (_size - 1))][pos4 + 2] == buffer[pos4])
				{
					found = j;
					break;
				}
			}
			case 5:
			{
				if (_cabuf[((head + j) & (_size - 1))][pos1 + 2] == buffer[pos1] && _cabuf[((head + j) & (_size - 1))][pos2 + 2] == buffer[pos2] &&
				    _cabuf[((head + j) & (_size - 1))][pos3 + 2] == buffer[pos3] && _cabuf[((head + j) & (_size - 1))][pos4 + 2] == buffer[pos4] &&
				    _cabuf[((head + j) & (_size - 1))][pos5 + 2] == buffer[pos5])
				{
					found = j;
					break;
				}
			}
		}
		if (found >= 0)
		{
			remove((head + found) & (_size - 1));
			return 1;
		}
	}
	return 0;
}

template<typename T, uint16_t _size, uint16_t multi>
bool Circular_Buffer<T, _size, multi>::find(T *buffer, uint16_t length, int pos1, int pos2, int pos3, int pos4, int pos5)
{
	uint8_t input_count = 3;
	bool found = 0;
	if (pos4 != -1)
		input_count = 4;
	if (pos5 != -1)
		input_count = 5;
	for (uint16_t j = 0; j < _available; j++)
	{
		switch (input_count)
		{
			case 3:
			{
				if (_cabuf[((head + j) & (_size - 1))][pos1 + 2] == buffer[pos1] && _cabuf[((head + j) & (_size - 1))][pos2 + 2] == buffer[pos2] &&
				    _cabuf[((head + j) & (_size - 1))][pos3 + 2] == buffer[pos3])
				{
					found = 1;
					break;
				}
			}
			case 4:
			{
				if (_cabuf[((head + j) & (_size - 1))][pos1 + 2] == buffer[pos1] && _cabuf[((head + j) & (_size - 1))][pos2 + 2] == buffer[pos2] &&
				    _cabuf[((head + j) & (_size - 1))][pos3 + 2] == buffer[pos3] && _cabuf[((head + j) & (_size - 1))][pos4 + 2] == buffer[pos4])
				{
					found = 1;
					break;
				}
			}
			case 5:
			{
				if (_cabuf[((head + j) & (_size - 1))][pos1 + 2] == buffer[pos1] && _cabuf[((head + j) & (_size - 1))][pos2 + 2] == buffer[pos2] &&
				    _cabuf[((head + j) & (_size - 1))][pos3 + 2] == buffer[pos3] && _cabuf[((head + j) & (_size - 1))][pos4 + 2] == buffer[pos4] &&
				    _cabuf[((head + j) & (_size - 1))][pos5 + 2] == buffer[pos5])
				{
					found = 1;
					break;
				}
			}
		}
		if (found)
		{
			memmove(buffer, _cabuf[((head + j) & (_size - 1))] + 2, length * sizeof(T));
			break;
		}
	}
	return found;
}

template<typename T, uint16_t _size, uint16_t multi>
bool Circular_Buffer<T, _size, multi>::replace(T *buffer, uint16_t length, int pos1, int pos2, int pos3, int pos4, int pos5)
{
	uint8_t input_count = 3;
	bool found = 0;
	if (pos4 != -1)
		input_count = 4;
	if (pos5 != -1)
		input_count = 5;
	for (uint16_t j = 0; j < _available; j++)
	{
		switch (input_count)
		{
			case 3:
			{
				if (_cabuf[((head + j) & (_size - 1))][pos1 + 2] == buffer[pos1] && _cabuf[((head + j) & (_size - 1))][pos2 + 2] == buffer[pos2] &&
				    _cabuf[((head + j) & (_size - 1))][pos3 + 2] == buffer[pos3])
				{
					found = 1;
					break;
				}
			}
			case 4:
			{
				if (_cabuf[((head + j) & (_size - 1))][pos1 + 2] == buffer[pos1] && _cabuf[((head + j) & (_size - 1))][pos2 + 2] == buffer[pos2] &&
				    _cabuf[((head + j) & (_size - 1))][pos3 + 2] == buffer[pos3] && _cabuf[((head + j) & (_size - 1))][pos4 + 2] == buffer[pos4])
				{
					found = 1;
					break;
				}
			}
			case 5:
			{
				if (_cabuf[((head + j) & (_size - 1))][pos1 + 2] == buffer[pos1] && _cabuf[((head + j) & (_size - 1))][pos2 + 2] == buffer[pos2] &&
				    _cabuf[((head + j) & (_size - 1))][pos3 + 2] == buffer[pos3] && _cabuf[((head + j) & (_size - 1))][pos4 + 2] == buffer[pos4] &&
				    _cabuf[((head + j) & (_size - 1))][pos5 + 2] == buffer[pos5])
				{
					found = 1;
					break;
				}
			}
		}
		if (found)
		{
			_cabuf[((head + j) & (_size - 1))][0] = length & 0xFF00;
			_cabuf[((head + j) & (_size - 1))][1] = length & 0xFF;
			memmove(_cabuf[((head + j) & (_size - 1))] + 2, buffer, length * sizeof(T));
			break;
		}
	}
	return found;
}

template<typename T, uint16_t _size, uint16_t multi>
bool Circular_Buffer<T, _size, multi>::isEqual(const T *buffer)
{
	if (multi)
	{
		bool success = 1;
		for (uint16_t j = 0; j < _available; j++)
		{
			success = 1;
			for (uint16_t k = 0; k < (_cabuf[((head + j) & (_size - 1))][0] | _cabuf[((head + j) & (_size - 1))][1]); k++)
			{
				if (_cabuf[((head + j) & (_size - 1))][k + 2] != buffer[k])
				{
					success = 0;
					break;
				}
			}
			if (success)
				return 1;
		}
	}
	return 0;
}

template<typename T, uint16_t _size, uint16_t multi>
void Circular_Buffer<T, _size, multi>::print(const char *p)
{
	if (multi)
		return;
	write((T *)p, strlen(p));
}

template<typename T, uint16_t _size, uint16_t multi>
void Circular_Buffer<T, _size, multi>::println(const char *p)
{
	if (multi)
		return;
	write((T *)p, strlen(p));
	write('\n');
}

template<typename T, uint16_t _size, uint16_t multi>
void Circular_Buffer<T, _size, multi>::push_front(const T *buffer, uint16_t length)
{
	if (multi)
	{
		if (tail == (head ^ _size))
			tail = ((tail - 1) & (2 * _size - 1));
		head = ((head - 1) & (2 * _size - 1));
		_cabuf[(head & (_size - 1))][0] = length & 0xFF00;
		_cabuf[(head & (_size - 1))][1] = length & 0xFF;
		memmove(_cabuf[((head) & (_size - 1))] + 2, buffer, length * sizeof(T));
		if (_available < _size)
			_available++;
		return;
	}
	for (uint16_t i = length - 1; i > 0; i--)
		push_front(buffer[i]);
	push_front(buffer[0]);
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::pop_back()
{
	if (_available)
	{
		if (_available)
			_available--;
		tail = ((tail - 1) & (2 * _size - 1));
		return _cbuf[((tail - 1) & (_size - 1))];
	}
	return -1;
}

template<typename T, uint16_t _size, uint16_t multi>
void Circular_Buffer<T, _size, multi>::push_front(T value)
{
	if (multi)
		return;
	head = ((head - 1) & (2 * _size - 1));
	_cbuf[((head) & (_size - 1))] = value;
	if (_available < _size)
		_available++;
}

template<typename T, uint16_t _size, uint16_t multi>
void Circular_Buffer<T, _size, multi>::write(const T *buffer, uint16_t length)
{
	if (multi)
	{
		_cabuf[((tail) & (_size - 1))][0] = length >> 8;
		_cabuf[((tail) & (_size - 1))][1] = length & 0xFF;
		memmove(_cabuf[((tail) & (_size - 1))] + 2, buffer, length * sizeof(T));
		if (tail == ((head ^ _size)))
			head = ((head + 1) & (2 * _size - 1));
		tail = ((tail + 1) & (2 * _size - 1));
		if (_available < _size)
			_available++;
		return;
	}
	if ((_available += length) >= _size)
		_available = _size;
	if (length < (_size - tail))
	{
		memmove(_cbuf + tail, buffer, length * sizeof(T));
		tail = ((tail + length) & (2 * _size - 1));
	}
	else
		for (uint16_t i = 0; i < length; i++)
			write(buffer[i]);
}

template<typename T, uint16_t _size, uint16_t multi>
void Circular_Buffer<T, _size, multi>::write(T value)
{
	if (multi)
		return;
	if (_available < _size)
		_available++;
	_cbuf[((tail) & (_size - 1))] = value;
	if (tail == ((head ^ _size)))
		head = ((head + 1) & (2 * _size - 1));
	tail = ((tail + 1) & (2 * _size - 1));
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::list()
{
	if (multi)
	{
		if (!size())
		{
			Serial.println("There are no queues available...");
			return 0;
		}

		Serial.print("\nCircular Array Buffer Queue Size: ");
		Serial.print(size());
		Serial.print(" / ");
		Serial.println(_size);

		Serial.print("\nFirst Entry: ");
		for (uint16_t i = 2; i <= (((int)_cabuf[((head) & (_size - 1))][0] << 8 * sizeof(T)) | (int)_cabuf[((head) & (_size - 1))][1]) + 1; i++)
		{
			if ((int)(_cabuf[((head + i) & (_size - 1))][i]) != (T)(_cabuf[((head + i) & (_size - 1))][i]))
			{ // possible float?
				Serial.print(_cabuf[((head) & (_size - 1))][i], 7);
			}
			else
				Serial.print(_cabuf[((head) & (_size - 1))][i]);
			Serial.print("    ");
		}
		Serial.print("(");
		Serial.print((((int)_cabuf[((head) & (_size - 1))][0] << 8 * sizeof(T)) | (int)_cabuf[((head) & (_size - 1))][1]));
		Serial.println(" entries.)");

		Serial.print("Last Entry:  ");
		for (uint16_t i = 2; i <= (((int)_cabuf[((head + size() - 1) & (_size - 1))][0] << 8 * sizeof(T)) | (int)_cabuf[((head + size() - 1) & (_size - 1))][1]) + 1; i++)
		{
			if ((int)(_cabuf[((head + size() - 1) & (_size - 1))][i]) != (T)(_cabuf[((head + size() - 1) & (_size - 1))][i]))
			{ // possible float?
				Serial.print(_cabuf[((head + size() - 1) & (_size - 1))][i], 7);
			}
			else
				Serial.print(_cabuf[((head + size() - 1) & (_size - 1))][i]);
			Serial.print("    ");
		}
		Serial.print("(");
		Serial.print((((int)_cabuf[((head + size() - 1) & (_size - 1))][0] << 8 * sizeof(T)) | (int)_cabuf[((head + size() - 1) & (_size - 1))][1]));
		Serial.println(" entries.)");

		Serial.print("\n[Indice]      [Entries]\n\n");
		for (uint16_t i = 0; i < size(); i++)
		{
			Serial.print("    ");
			Serial.print(((head + i) & (_size - 1)));
			Serial.print("\t\t");
			for (uint16_t j = 2; j <= (((int)_cabuf[((head + i) & (_size - 1))][0] << 8 * sizeof(T)) | (int)_cabuf[((head + i) & (_size - 1))][1]) + 1; j++)
			{
				if ((int)(_cabuf[((head + i) & (_size - 1))][j]) != (T)(_cabuf[((head + i) & (_size - 1))][j]))
				{ // possible float?
					Serial.print(_cabuf[((head + i) & (_size - 1))][j], 7);
					Serial.print("\t");
				}
				else
				{
					Serial.print(_cabuf[((head + i) & (_size - 1))][j]);
					Serial.print("\t");
				}
			}
			Serial.print("(");
			Serial.print((((int)_cabuf[((head + i) & (_size - 1))][0] << 8 * sizeof(T)) | (int)_cabuf[((head + i) & (_size - 1))][1]));
			Serial.println(" entries.)");
		}
		return _available;
	}

	if (!size())
	{
		Serial.println("There are no queues available...");
		return 0;
	}
	Serial.print("\nCircular Ring Buffer Queue Size: ");
	Serial.print(size());
	Serial.print(" / ");
	Serial.println(_size);

	Serial.print("\nIndice:  \t");

	for (uint16_t i = 0; i < _available; i++)
	{
		Serial.print("[");
		Serial.print((head + i) & (_size - 1));
		Serial.print("]      \t");
	}

	Serial.print("\nEntries:\t");

	for (uint16_t i = 0; i < _available; i++)
	{
		if ((int)_cbuf[((head + i) & (_size - 1))] != (T)_cbuf[((head + i) & (_size - 1))])
		{ // possible float?
			Serial.print(_cbuf[((head + i) & (_size - 1))], 7);
			Serial.print("\t");
		}
		else
		{
			Serial.print(_cbuf[((head + i) & (_size - 1))]);
			Serial.print("\t\t");
		}
	}
	Serial.println('\n');

	return _available;
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::read()
{
	if (multi)
	{
		head = ((head + 1) & (2 * _size - 1));
		if (_available)
			_available--;
		return 0;
	}
	if (_available)
		_available--;
	T value = _cbuf[((head) & (_size - 1))];
	head = ((head + 1) & (2 * _size - 1));
	return value;
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::sum()
{
	if (multi || !_available)
		return 0;
	T value = 0;
	for (uint16_t i = 0; i < _available; i++)
		value += _cbuf[((head + i) & (_size - 1))];
	return value;
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::average()
{
	if (multi || !_available)
		return 0;
	return sum() / _available;
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::variance()
{
	if (multi || !_available)
		return 0;
	T _mean = average();
	T value = 0;
	for (uint16_t i = 0; i < _available; i++)
	{
		value += ((_cbuf[((head + i) & (_size - 1))] - _mean) * (_cbuf[((head + i) & (_size - 1))] - _mean));
	}
	value /= _available;
	return value;
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::deviation()
{
	if (multi || !_available)
		return 0;
	return sqrt(variance());
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::peek(uint16_t pos)
{
	if (multi)
		return 0;
	if (pos > _size)
		return 0;
	return _cbuf[((head + pos) & (_size - 1))];
}

template<typename T, uint16_t _size, uint16_t multi>
void Circular_Buffer<T, _size, multi>::sort_ascending()
{
	if (multi || !_available)
		return;
	T buffer[_available];
	for (uint16_t i = 0; i < _available; i++)
		buffer[i] = _cbuf[((head + i) & (_size - 1))];
	std::sort(&buffer[0], &buffer[_available]); // sort ascending
	for (uint16_t i = 0; i < _available; i++)
		_cbuf[((head + i) & (_size - 1))] = buffer[i];
}

template<typename T, uint16_t _size, uint16_t multi>
void Circular_Buffer<T, _size, multi>::sort_descending()
{
	if (multi || !_available)
		return;
	sort_ascending();
	T buffer[_available];
	for (uint16_t i = 0; i < _available; i++)
		buffer[i] = _cbuf[((head + i) & (_size - 1))];
	std::reverse(&buffer[0], &buffer[_available]); // sort descending
	for (uint16_t i = 0; i < _available; i++)
		_cbuf[((head + i) & (_size - 1))] = buffer[i];
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::median(bool override)
{
	if (multi || !_available)
		return 0;
	if (override)
		sort_ascending();
	else
	{
		T buffer[_available];
		for (uint16_t i = 0; i < _available; i++)
			buffer[i] = _cbuf[((head + i) & (_size - 1))];
		std::sort(&buffer[0], &buffer[_available]); // sort ascending
	}
	if (!(_available % 2))
	{
		return (_cbuf[((head + ((_available / 2) - 1)) & (_size - 1))] + _cbuf[((head + (_available / 2)) & (_size - 1))]) / 2;
	}
	else
		return _cbuf[((head + ((_available / 2))) & (_size - 1))];
	return 0;
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::max()
{
	if (multi || !_available)
		return 0;
	T buffer[_available];
	for (uint16_t i = 0; i < _available; i++)
		buffer[i] = _cbuf[((head + i) & (_size - 1))];
	std::sort(&buffer[0], &buffer[_available]); // sort ascending
	return buffer[_available - 1];
}
template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::min()
{
	if (multi || !_available)
		return 0;
	T buffer[_available];
	for (uint16_t i = 0; i < _available; i++)
		buffer[i] = _cbuf[((head + i) & (_size - 1))];
	std::sort(&buffer[0], &buffer[_available]); // sort ascending
	return buffer[0];
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::peekBytes(T *buffer, uint16_t length)
{
	if (multi)
		return 0;
	uint16_t _count;
	(_available < length) ? _count = _available : _count = length;
	if (_count < (_size - head))
		memmove(buffer, _cbuf, _count * sizeof(T));
	else
		for (uint16_t i = 0; i < _count; i++)
			buffer[i] = peek(i);
	return _count;
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::peek_front(T *buffer, uint16_t length, uint32_t entry)
{
	if (multi)
	{
		memmove(&buffer[0], &_cabuf[((head + entry) & (_size - 1))][2], length * sizeof(T)); // update CA buffer
		return 0;
	}
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::readBytes(T *buffer, uint16_t length)
{
	if (multi)
	{
		memmove(&buffer[0], &_cabuf[((head) & (_size - 1))][2], length * sizeof(T)); // update CA buffer
		read();
		return 0;
	}
	uint16_t _count;
	(_available < length) ? _count = _available : _count = length; // memmove if aligned
	if (_count < (_size - head))
	{
		_available -= length;
		memmove(buffer, _cbuf, _count * sizeof(T));
		head = ((head + _count) & (2 * _size - 1));
	}
	else
		for (uint16_t i = 0; i < _count; i++)
			buffer[i] = read(); // if buffer rollover
	return _count;
}

template<typename T, uint16_t _size, uint16_t multi>
T Circular_Buffer<T, _size, multi>::pop_back(T *buffer, uint16_t length)
{
	if (multi)
	{
		memmove(&buffer[0], &_cabuf[((tail - 1) & (_size - 1))][2], length * sizeof(T));
		tail = (tail - 1) & (2 * _size - 1);
		if (_available)
			_available--;
		return 0;
	}
}

#endif // Circular_Buffer_H
