# Chrigizip

This is an implementation of a simple compression algorithm. Something I heard a long time ago and now finally got around to trying. Turns out this actually just Huffman Coding. It is not meant for serious use.


### Structure

Implemented here you will find what I call a compression tree. This implements the binary tree used for Huffman Coding.
My own versions of ifstream and ofstream made such that they hold a buffer of 1024 bytes by default. Whenever the buffer is full or exhausted, another chunk of this size is read/written to/from the file. This is because a call to read or write for every single byte is much slower than reading multiple bytes at once. We don't want to read whole files at once either since they could be very large, thus the approach with the buffer.


### Prerequisites

This needs a recent C++ compiler, nothing more.


## Authors

Christian Stieger
