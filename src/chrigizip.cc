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

	return 0;
}
