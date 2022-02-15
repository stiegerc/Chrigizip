#include <iostream>
#include <fstream>
#include <getopt.h>
#include "stream_buffer.h"
#include "compression_tree.h"

#define OPTS__ "e:x:v:"
#define ERRMSG__ "valid options are:\n" \
		 "-e: compress a file into a chrigi file\n" \
		 "-x: extract file from a chrigi file\n"


int main(const int argc, char** argv) {

	// define default stream here
	std::ostream& os = std::cout;


	// print errmsg and quit if there are no arguments
	if (argc==1) {
		os << ERRMSG__;
		return 0;
	}

	// verbosity setting for compression
	int verbosity=0;

	// parse commands
	try {
		int cmd=0;
		while (cmd!=-1 && cmd!='?') {

			switch (cmd = getopt(argc,argv,OPTS__)) {
				case 'v':
				{
					verbosity = std::stoi(optarg);
				}
				break;
				case 'e':
				{
					chrigi::compress(optarg,"",verbosity,os);
				}
				break;
				case 'x':
				{
					chrigi::decompress(optarg);
				}
				break;

			}
		}
	} catch (const std::exception& e) {
		os << "ERROR: " << e.what() << "\n";
	}


/*
	{
		ctree CT("test");
		CT.compress("test","gaggi.chrigi");
	}
	{
		std::ifstream ifile("gaggi.chrigi", std::ios::binary);
		if (!ifile)
			throw(std::invalid_argument("bad file"));

		ctree CT(ifile);
		std::cout << "\n\n" << CT.filename() << "\n\n";
	}
*/

//	chrigi::compress("testfiles/test","test.chrigi");
//	chrigi::compress("testfiles/kenny.jpeg","kenny.chrigi",2,std::cout);
//	chrigi::compress("testfiles/test2","test2.chrigi",2,std::cout);
//	chrigi::compress("testfiles/test","test.chrigi",2,std::cout);
//	chrigi::compress("test2.chrigi","test2.chrigi.chrigi");
//	chrigi::compress("test2.chrigi.chrigi","test2.chrigi.chrigi.chrigi");
//	chrigi::decompress("test2.chrigi.chrigi.chrigi");
//	chrigi::decompress("test2.chrigi.chrigi");
//	chrigi::decompress("test2.chrigi");

//	ifstream__ ifile("test");
//	compression_tree T(ifile, compression_tree::read_mode::from_file);



/*
	{
		ifstream__ ifile("test");
		char* blob = new char [ifile.Nbytes_left()];
		std::cout << "\n\n" << ifile.Nbytes_left() << "\n\n";
		ifile.read(blob,ifile.Nbytes_left());
		std::cout << "\n\n" << ifile.Nbytes_left() << "\n\n";
		delete[] blob;

		ifstream__ ifile2("test");
		std::cout << "\n\n" << ifile2.Nbytes_left() << "\n\n";
	}
	{
		ifstream__ ifile("test");
		std::cout << "\n\n" << ifile.Nbytes_left() << "\n\n";
	}
*/


	return 0;
}
