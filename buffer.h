#ifndef CORE_BUFFER_H
#define CORE_BUFFER_H

#include <cstdio>
#include <cstring>
#include <vector>

inline void buffer_read_block(unsigned char*& p, void* dest, size_t size) {
	std::memcpy(dest, p, size);
	p += size;
}
template<typename T>
inline T buffer_read(unsigned char*& p) {
	T ret{};
	std::memcpy(&ret, p, sizeof(T));
	p += sizeof(T);
	return ret;
}

inline void buffer_write_block(unsigned char*& p, const void* src, size_t size) {
	std::memcpy(p, src, size);
	p += size;
}
template<typename T>
inline void buffer_write(unsigned char*& p, const T& value) {
	std::memcpy(p, &value, sizeof(T));
	p += sizeof(T);
}

inline void vector_write_block(std::vector<unsigned char>& buffer, const void* src, size_t size) {
	const auto len = buffer.size();
	buffer.resize(len + size);
	std::memcpy(buffer.data() + len, src, size);
}
template<typename T>
inline void vector_write(std::vector<unsigned char>& buffer, const T& value) {
	vector_write_block(buffer, &value, sizeof(T));
}

inline void vector_fread(std::vector<unsigned char>& buffer, FILE* fp) {
	unsigned char temp[4096]{};
	while (size_t len = std::fread(temp, 1, sizeof temp, fp))
		vector_write_block(buffer, temp, len);
	std::fclose(fp);
}

#endif // !CORE_BUFFER_H
