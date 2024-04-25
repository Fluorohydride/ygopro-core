#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>
#include <vector>

namespace Buffer {
	template<typename T>
	inline T read(unsigned char*& p) {
		T ret = 0;
		std::memcpy(&ret, p, sizeof(T));
		p += sizeof(T);
		return ret;
	}
	inline void read_array(unsigned char*& p, void* dest, int size) {
		if (size <= 0)
			return;
		std::memcpy(dest, p, size);
		p += size;
	}

	template<typename T>
	inline void write(unsigned char*& p, T value) {
		std::memcpy(p, &value, sizeof(T));
		p += sizeof(T);
	}
	template<typename T>
	inline void write(std::vector<unsigned char>& buffer, T value) {
		const auto len = buffer.size();
		buffer.resize(len + sizeof(T));
		std::memcpy(&buffer[len], &value, sizeof(T));
	}
	inline void write_array(unsigned char*& p, const void* src, int size) {
		if (size <= 0)
			return;
		std::memcpy(p, src, size);
		p += size;
	}
	inline void write_array(std::vector<unsigned char>& buffer, const void* src, int size) {
		if (size <= 0)
			return;
		const auto len = buffer.size();
		buffer.resize(len + size);
		std::memcpy(&buffer[len], src, size);
	}
}

#endif //BUFFER_H
