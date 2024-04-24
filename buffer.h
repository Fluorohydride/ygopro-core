#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>

template<typename T>
inline void write_byte_buffer(unsigned char*& p, T value) {
	std::memcpy(p, &value, sizeof(T));
	p += sizeof(T);
}

#endif //BUFFER_H
