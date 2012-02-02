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
	void resize(size_t size) { buf.resize(size); }
	BYTE* data() { return buf.data(); }
	void assign(size_t size) {buf.assign(size, 0);}
	void assign(BYTE* data, size_t size);
	void reserve(size_t size) {buf.reserve(size);}
	void append(void* data, size_t size) { if (size>=this->size()) resize(size); memcpy_s(this->data(), this->size(), data, size);}	
	void append(BarbaBuffer* buffer) { append(buffer->data(), buffer->size()); }
	void append(std::vector<BYTE>* data) { append(data->data(), data->size()); }
	void append(BYTE c) {buf.push_back(c);}
	bool empty() {return buf.empty();}

private:
	std::vector<BYTE> buf;
};

