all: pngtext.cc
	clang++ -std=c++20 -stdlib=libc++ -fexperimental-library pngtext.cc crc32.cc -lc++abi -o pngtext
