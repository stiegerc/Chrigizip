#ifndef _STREAM_BUFFER_
#define _STREAM_BUFFER_

#include <fstream>
#include <array>
#include <bitset>
#include <filesystem>
#include <cassert>


#ifndef CHUNKSIZE
#define CHUNKSIZE 1024	// default chunk size
#endif



// std::ifstream wrapper keeping track of the filename, bytes left in the file and bytes already read
class ifstream__ {
public:
	// constructor
	inline ifstream__(const std::string& filename):
			file_(filename, std::ios::in | std::ios::binary),
			path_(filename), Nbytes_left_(file_size(path_)), Nbytes_read_(0) {
	
		if (!is_regular_file(path_))
			throw(std::invalid_argument("'"+path_.filename().string()+"' is not a regular file"));
		if (!file_)
			throw(std::invalid_argument("failed to open file '"+path_.filename().string()+"'"));
	}

	// io
	inline void read(char* ptr, const size_t N) {
		assert(N<=Nbytes_left());
		
		file_.read(ptr,N);
		Nbytes_read_ += N, Nbytes_left_ -= N;

		if (file_.eof())
			throw(std::runtime_error("unexpected end of file in file '"+path().filename().string()+"'"));
		if (!file_)
			throw(std::runtime_error("error reading from file '"+path().filename().string()+"'"));
	}

	// information
	inline const std::filesystem::path& path() const noexcept { return path_; }
	inline size_t Nbytes_left() const noexcept { return Nbytes_left_; }
	inline size_t Nbytes_read() const noexcept { return Nbytes_read_; }
	inline size_t Nbytes_total() const noexcept { return Nbytes_left()+Nbytes_read(); }

private:
	std::ifstream file_;
	std::filesystem::path path_;
	size_t Nbytes_left_;
	size_t Nbytes_read_;
};


// std::ofstream wrapper keeping track of the filename and bytes written
class ofstream__ {
public:
	// constructor
	inline ofstream__(const std::string& filename):
			file_(filename, std::ios::trunc | std::ios::out | std::ios::binary),
			path_(filename), Nbytes_written_(0) {
	
		if (!is_regular_file(path_))
			throw(std::invalid_argument("'"+path_.filename().string()+"' is not a regular file"));
		if (!file_)
			throw(std::invalid_argument("failed to open file '"+path_.filename().string()+"'"));
	}

	// io
	inline void write(const char* ptr, const size_t N) {
		file_.write(ptr,N);
		Nbytes_written_ += N;
		
		if (!file_)
			throw(std::runtime_error("error writing to file '"+path().filename().string()+"'"));
	}

	// information
	inline const std::filesystem::path& path() const noexcept { return path_; }
	inline size_t Nbytes_written() const noexcept { return Nbytes_written_; }

private:
	std::ofstream file_;
	std::filesystem::path path_;
	size_t Nbytes_written_;
};



// wrapper for easy access to bytes from file, data is read in chunks and buffered
template <size_t N=CHUNKSIZE>
class byte_buffered_ifstream {
public:
	// constructor
	inline byte_buffered_ifstream(ifstream__& stream): stream_(stream) {
		load_buffer_();
	}

	// information
	inline bool eof() const noexcept {
		return !stream_.Nbytes_left() && byte_index_==bytes_in_buffer_;
	}

	// data access
	inline uint8_t next_byte() noexcept {
		assert(!eof());

		if (byte_index_ == bytes_in_buffer_)
			load_buffer_();

		return bytes_[byte_index_++];
	}

private:
	inline void load_buffer_() {
		const size_t Nread = N<stream_.Nbytes_left() ? N: stream_.Nbytes_left();
		stream_.read((char*) bytes_.data(), Nread);
		bytes_in_buffer_ = Nread;
		byte_index_ = 0;
	}
	
private:
	ifstream__& stream_;
	std::array<uint8_t,N> bytes_;
	size_t bytes_in_buffer_;
	size_t byte_index_;
};

// wrapper for easy access to bits from file, data is read in chunks and buffered
template <size_t N=CHUNKSIZE>
class bit_buffered_ifstream {
public:
	// constructor
	inline bit_buffered_ifstream(ifstream__& stream): stream_(stream) {
		load_buffer_();
	}

	// information
	inline bool eof() const noexcept {
		return !stream_.Nbytes_left() && bit_index_==bits_in_buffer_;
	}

	// data access
	inline bool next_bit() noexcept {
		assert(!eof());

		if (bit_index_ == bits_in_buffer_)
			load_buffer_();

		return bits_[bit_index_++];
	}

private:
	inline void load_buffer_() {
		const size_t Nread = N<stream_.Nbytes_left() ? N: stream_.Nbytes_left();
		stream_.read((char*) &bits_, Nread);
		bits_in_buffer_ = 8*Nread;
		bit_index_ = 0;
	}
	
private:
	ifstream__& stream_;
	std::bitset<8*N> bits_;
	size_t bits_in_buffer_;
	size_t bit_index_;
};



// wrapper for easy writing bytes to file, data is written in chunks and buffered
template <size_t N=CHUNKSIZE>
class byte_buffered_ofstream {
public:
	// constructor/destructor
	inline byte_buffered_ofstream(ofstream__& stream): stream_(stream), byte_index_(0) {}
	inline ~byte_buffered_ofstream() {
		flush();
	}

	// write to file
	inline void flush() {
		if (byte_index_>0)
			stream_.write((char*) bytes_.data(), byte_index_);
		byte_index_ = 0;
	}

	// data access
	inline void put_byte(const uint8_t byte) noexcept {

		if (byte_index_ == N)
			flush();
		
		bytes_[byte_index_++] = byte;
	}
	
private:
	ofstream__& stream_;
	std::array<uint8_t,N> bytes_;
	size_t byte_index_;
};

// wrapper for easy writing bits to file, data is written in chunks and buffered
template <size_t N=CHUNKSIZE>
class bit_buffered_ofstream {
public:
	// constructor/destructor
	inline bit_buffered_ofstream(ofstream__& stream): stream_(stream), bit_index_(0) {}
	inline ~bit_buffered_ofstream() {
		flush();
	}

	// write to file
	inline void flush() {
		if (bit_index_>0)
			stream_.write((char*) &bits_, bit_index_/8 + (bit_index_%8 ? 1: 0));
		bit_index_ = 0;
	}

	// data access
	inline void put_bit(const bool bit) noexcept {

		if (bit_index_ == 8*N)
			flush();
		
		bits_[bit_index_++] = bit;
	}
	
private:
	ofstream__& stream_;
	std::bitset<8*N> bits_;
	size_t bit_index_;
};



#endif // _STREAM_BUFFER_
