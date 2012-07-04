#pragma once

template <class T> class Iterable {
public:
	virtual size_t copyTo(T* dest, size_t size) const = 0;
};

template <class T> class Iterator {
private:
	T* m_buffer;
	size_t m_size;
	size_t m_index;
public:
	Iterator(const Iterable<T>* iterable): m_index(0) {
		if (iterable != NULL) {
			m_size = iterable->copyTo(NULL, 0);
			if (m_size > 0) {
				m_buffer = new T[m_size];
				iterable->copyTo(m_buffer, m_size);
			}
			else {
				m_buffer = NULL;
			}
		}
		else {
			m_buffer = NULL;
			m_size = 0;
		}
	}

	Iterator(const Iterator& src): m_index(0), m_size(src.m_size) {
		if (m_size != 0) {
			m_buffer = new T[m_size];
			for (size_t i = 0; i < m_size; ++i) {
				m_buffer[i] = src.m_buffer[i];
			}
		}
		else {
			m_buffer = NULL;
		}
	}

	~Iterator() {
		if (m_buffer) {
			delete[] m_buffer;
		}
	}

	bool hasNext() const {
		return m_index < m_size;
	}

	T next() {
		dcassert(m_index < m_size);
		return m_buffer[m_index++];
	}
};

