#ifndef _COMPRESSION_TREE_
#define _COMPRESSION_TREE_

#include <vector>
#include <map>
#include <iostream>
#include "stream_buffer.h"


namespace chrigi {
	// read meta data from chrigi files
	struct meta {
		std::string filename;
		size_t Nbytes_compressed;
	};
	meta read_meta(ifstream__& ifile);

	// function to compress a file (into a chrigi file)
	void compress(const std::string& ifilename, const std::string& ofilename="",
			const int verbosity=0, std::ostream& os=std::cout);
	// function to decompress a chrigi file
	void decompress(const std::string& ifilename);
}

// class for the tree structure used to compress bytes
class compression_tree {
private:
	// helper functions for the constructor
	void from_file_(ifstream__& ifile, const int verbosity, std::ostream& os);
	void from_compressed_(ifstream__& ifile);

public:
	// enum for the constructor
	enum read_mode {
		from_file,
		from_compressed
	};

	// constructor
	inline compression_tree(ifstream__& ifile, read_mode mode, const int verbosity=0, std::ostream& os=std::cout) {
		switch (mode) {
			case from_file:
			{
				from_file_(ifile,verbosity,os);
			}
			break;
			case from_compressed:
			{
				from_compressed_(ifile);
			}
			break;
		}
	}
	inline compression_tree(ifstream__&& ifile, read_mode mode, const int verbosity=0, std::ostream& os=std::cout):
		compression_tree(ifile,mode,verbosity,os) {}


	// write tree to stream
	void write(ofstream__& ofile) const;


	// translation functions
	void compress(byte_buffered_ifstream<>& ifile, bit_buffered_ofstream<>& ofile, const size_t Nbytes) const noexcept;
	void decompress(bit_buffered_ifstream<>& ifile, byte_buffered_ofstream<>& ofile, const size_t Nbytes) const noexcept;

private:
	// compression tree and leaf bits
	// for translation compressed -> uncompressed
	struct lr { uint8_t left, right; };
	std::vector<lr> tree_;
	std::bitset<256> lleaf_;
	std::bitset<256> rleaf_;

	// translation map for uncompressed -> compressed
	std::map<uint8_t,std::vector<bool>> tmap_;
};

#endif // _COMPRESSION_TREE_
