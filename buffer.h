#ifndef CORE_BUFFER_H
#define CORE_BUFFER_H

#include <cstring>
#include <vector>

inline void buffer_read_block(unsigned char*& p, void* dest, size_t size) {
	std::memcpy(dest, p, size);
	p += size;
}
template<typename T>
inline T buffer_read(unsigned char*& p) {
	T ret{};
	buffer_read_block(p, &ret, sizeof(T));
	return ret;
}

inline void buffer_write_block(unsigned char*& p, const void* src, size_t size) {
	std::memcpy(p, src, size);
	p += size;
}
template<typename T>
inline void buffer_write(unsigned char*& p, T value) {
	buffer_write_block(p, &value,sizeof(T));
}

inline void vector_write_block(std::vector<unsigned char>& buffer, const void* src, size_t size) {
	const auto len = buffer.size();
	buffer.resize(len + size);
	std::memcpy(&buffer[len], src, size);
}
template<typename T>
inline void vector_write(std::vector<unsigned char>& buffer, T value) {
	vector_write_block(buffer, &value, sizeof(T));
}

#endif // !CORE_BUFFER_H
