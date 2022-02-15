#include "compression_tree.h"
#include <functional>
#include <numeric>
#include <iostream>

// function to read meta data from chrigi files
chrigi::meta chrigi::read_meta(ifstream__& ifile) {
	
	chrigi::meta res;
	
	// read header
	{
		std::string header(9,'x');
		ifile.read((char*) header.c_str(), 9);
		if (header != std::string("chrigizip"))
			throw(std::invalid_argument("invalid format"));
	}
	// read original filename
	{
		uint16_t N;
		ifile.read((char*) &N, sizeof(N));

		res.filename.resize(N);
		ifile.read((char*) res.filename.c_str(), N);
	}
	// read number of compressed bytes
	{
		uint64_t N;
		ifile.read((char*) &N, sizeof(N));

		res.Nbytes_compressed = N;
	}

	return res;
}



// function to compress a file (into a chrigi file)
void chrigi::compress(const std::string& ifilename, const std::string& ofilename,
			const int verbosity, std::ostream& os) {
	
	// generate compression tree
	compression_tree T(ifstream__(ifilename),compression_tree::read_mode::from_file,verbosity,os);

	// write chrigi meta data
	ifstream__ ifile(ifilename);
	ofstream__ ofile(ofilename.empty() ? ifile.path().filename().string()+".chrigi": ofilename);
	{
		// header
		const std::string header("chrigizip");
		ofile.write(header.c_str(),9);
	}
	{
		// original filename (without path)
		const auto filename_ = ifile.path().filename().string();
		const uint16_t N = filename_.size();
		ofile.write((char*) &N, sizeof(N));
		ofile.write((char*) filename_.c_str(), int(N));
	}
	{
		// number of compressed bytes
		const uint64_t N = ifile.Nbytes_left();
		ofile.write((char*) &N, sizeof(N));
	}

	// write compression tree
	T.write(ofile);
	
	// setup streams
	const size_t N = ifile.Nbytes_left();	// need to store this here because the byte_buffered_stream will
						// modify this by reading some of the data into the buffer
	byte_buffered_ifstream<> bf_ifile(ifile);
	bit_buffered_ofstream<> bf_ofile(ofile);

	// write compressed data
	T.compress(bf_ifile,bf_ofile,N);
}

// function to decompress a chrigi file
void chrigi::decompress(const std::string& ifilename) {

	// read meta data
	ifstream__ ifile(ifilename);
	const auto meta = chrigi::read_meta(ifile);

	// generate compression tree
	compression_tree T(ifile,compression_tree::read_mode::from_compressed);

	// setup streams
	ofstream__ ofile(meta.filename);
	byte_buffered_ofstream<> bf_ofile(ofile);
	bit_buffered_ifstream<> bf_ifile(ifile);

	// write decompressed data
	T.decompress(bf_ifile,bf_ofile,meta.Nbytes_compressed);
}



// generate compression tree from normal file
void compression_tree::from_file_(ifstream__& ifile, const int verbosity, std::ostream& os) {
	
	if (verbosity)
		os << "analyzing file... ";

	// generate byte statistic
	struct bc_ {
		uint8_t byte;
		uint64_t count;
	};
	std::vector<bc_> stat_;
	{
		// initialize
		stat_.reserve(256);
		for (int i=0; i!=256; ++i)
			stat_.push_back({uint8_t(i),uint64_t(0)});

		// read whole file, count bytes
		byte_buffered_ifstream bf(ifile);
		while (!bf.eof())
			++stat_[bf.next_byte()].count;
		
		// sort and resize statistic
		std::sort(stat_.begin(),stat_.end(),
			[](const auto& lhs, const auto& rhs) -> bool {
				return lhs.count > rhs.count;
			});
		stat_.resize(std::distance(stat_.begin(),
			std::find_if(stat_.begin(),stat_.end(),[](const auto& i)->bool{return !i.count;})));
	}
	if (verbosity)
		os << "done! found " << stat_.size() << " unique bytes\n";
	
	
	// generate the compression tree from the statistic
	tree_.reserve(255);
	
	// set bitsets to false
	lleaf_.set(false), rleaf_.set(false);
	
	// recursive lambda
	typedef decltype(stat_)::const_iterator itr;
	std::function<uint8_t(itr,itr,size_t)> rec_;
	rec_ = [this,&rec_](const itr b, const itr e, const size_t S) -> uint8_t {

		// half the sum of S
		const size_t HS = S/2;

		// find iterator where we have the best fit for half the sum of S
		auto i=b+1; size_t s=b->count, ns;
		if (s<HS)
			for (; i!=e; s=ns, ++i) {
				ns = s + i->count;
				if (ns > HS) {
					if (ns-HS < HS-s)
						s=ns, ++i;
					break;
				}
			}

		// construct new node
		const auto dl = std::distance(b,i), dr = std::distance(i,e);
		lr node = {dl==1 ? b->byte: rec_(b,i,s),
			   dr==1 ? i->byte: rec_(i,e,S-s)};

		const auto ind = tree_.size();
		if (dl==1) lleaf_[ind] = true;
		if (dr==1) rleaf_[ind] = true;

		tree_.push_back(node);
		
		return uint8_t(ind);
	};

	// call the lambda for more than 1 byte in stat_
	if (stat_.size()>1)
		rec_(stat_.cbegin(),stat_.cend(),
			std::accumulate(stat_.cbegin(),stat_.cend(),size_t(0),
			[](const size_t s, const auto& i)->size_t{ return s+i.count; }));
	else {
		// for just one byte, 0 and 1 are degenerate (mean the same)
		tree_.push_back({stat_.front().byte,stat_.front().byte});
		lleaf_[0] = rleaf_[0] = true;
	}

	// shrink tree_
	tree_.shrink_to_fit();

	
	// generate the translation map
	// recursive lambda
	std::function<void(const size_t,std::vector<bool>)> trec_;
	trec_ = [this,&trec_](const size_t ind, std::vector<bool> trans) -> void {
		
		// left leaf or recursion
		{
			auto ltrans = trans; ltrans.push_back(false);
			if (lleaf_[ind]) tmap_.insert({tree_[ind].left,std::move(ltrans)});
			else		 trec_(tree_[ind].left,std::move(ltrans));
		}

		// right leaf or recursion
		{
			auto rtrans = trans; rtrans.push_back(true);
			if (rleaf_[ind]) tmap_.insert({tree_[ind].right,std::move(rtrans)});
			else		 trec_(tree_[ind].right,std::move(rtrans));
		}
	};

	// call lambda
	trec_(tree_.size()-1,{});

	if (verbosity>1) {
		os << "stats are:\n"
		   << std::setw(12) << "occurrence" << std::setw(12) << "byte"
		   << std::setw(16) << "byte[binary]" << std::setw(24) << "translation\n";

		// convert std::vector<bool> to string lambda
		const auto vbts = [](const std::vector<bool>& inp) -> std::string {
			std::string res; res.reserve(inp.size());
			for (auto i=inp.crbegin(),e=inp.crend(); i!=e; ++i)
				if (*i)	res.push_back('1');
				else    res.push_back('0');
			return res;
		};
		
		// print stats
		size_t osum=0, tsum=0;
		for (const auto& i: stat_) {
			const auto& j = tmap_.find(i.byte);

			osum += i.count*8;
			tsum += i.count*j->second.size();

			os << std::setw(12) << i.count << std::setw(12) << int(i.byte)
			   << std::setw(16) << std::bitset<8>(i.byte)
			   << std::setw(24) << vbts(j->second) << "\n";
		}

		// print compression factor
		os << "compression factor of " << std::setprecision(2) << std::fixed
		   << (double(tsum)/double(osum)*100.0) << "%\n";
	}
}

// read compression tree from file
void compression_tree::from_compressed_(ifstream__& ifile) {
	
	// number of nodes
	{
		uint8_t N;
		ifile.read((char*) &N, sizeof(N));
		tree_.resize(N);
	}

	// tree data and leaf bits
	ifile.read((char*) tree_.data(), tree_.size()*sizeof(lr));
	ifile.read((char*) &lleaf_, sizeof(lleaf_));
	ifile.read((char*) &rleaf_, sizeof(lleaf_));
}


// write tree to stream
void compression_tree::write(ofstream__& ofile) const {
	
	const uint8_t N = tree_.size();
	ofile.write((char*) &N, sizeof(N));
	ofile.write((char*) tree_.data(), tree_.size()*sizeof(lr));
	ofile.write((char*) &lleaf_, sizeof(lleaf_));
	ofile.write((char*) &rleaf_, sizeof(rleaf_));
}


// translation functions
void compression_tree::compress(byte_buffered_ifstream<>& ifile, bit_buffered_ofstream<>& ofile, const size_t Nbytes) const noexcept {
	size_t cnt=0;
	while (cnt!=Nbytes) {
		assert(!ifile.eof());

		const auto i = tmap_.find(ifile.next_byte());
		assert(i!=tmap_.end());

		for (auto bit: i->second)
			ofile.put_bit(bit);

		++cnt;
	}
}
void compression_tree::decompress(bit_buffered_ifstream<>& ifile, byte_buffered_ofstream<>& ofile, const size_t Nbytes) const noexcept {
	size_t cnt=0;
	while (cnt!=Nbytes) {
		assert(!ifile.eof());
		
		size_t ind = tree_.size()-1;
		while (true)
			if (ifile.next_bit()) {
				if (rleaf_[ind]) {
					ofile.put_byte(tree_[ind].right);
					++cnt;
					break;
				}
				ind = size_t(tree_[ind].right);
			} else {
				if (lleaf_[ind]) {
					ofile.put_byte(tree_[ind].left);
					++cnt;
					break;
				}
				ind = size_t(tree_[ind].left);
			}
	}
}
