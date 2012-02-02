#pragma once
class BarbaBuffer
{
public:
	explicit BarbaBuffer(void){}
	explicit BarbaBuffer(size_t size) {assign(size);}
	explicit BarbaBuffer(void* data, size_t size) { append(data, size); }
	explicit BarbaBuffer(BarbaBuffer* buffer) { append(buffer); }
	explicit BarbaBuffer(std::vector<BYTE>* data) { append(data); }
	~BarbaBuffer(void){}
	size_t size() { return buf.size(); }
	size_t capacity() { return buf.capacity(); }
	void resize(size_t size) { buf.resize(size); }
	BYTE* data() { return buf.data(); }
	void assign(size_t size) {buf.assign(size, 0);}
	void assign(size_t size, BYTE value) {buf.assign(size, value);}
	void assign(BYTE* data, size_t size) {resize(0); append(data, size); }
	void reserve(size_t size) {buf.reserve(size);}
	void append(void* data, size_t size) { size_t oldSize = this->size(); resize(oldSize + size); memcpy_s(this->data() + oldSize, this->size()-oldSize, data, size);}	
	void append(BarbaBuffer* buffer) { append(buffer->data(), buffer->size()); }
	void append(std::vector<BYTE>* data) { append(data->data(), data->size()); }
	void append(BYTE value) {buf.push_back(value);}
	bool empty() {return buf.empty();}

private:
	std::vector<BYTE> buf;
};

