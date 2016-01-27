// Copyright (C) 2016 Mars Saxman. All rights reserved.
// Permission is granted to use at your own risk and distribute this software
// in source and binary forms provided all source code distributions retain
// this paragraph and the above copyright notice. THIS SOFTWARE IS PROVIDED "AS
// IS" WITH NO EXPRESS OR IMPLIED WARRANTY.

#ifndef LOGSTREAM_H
#define LOGSTREAM_H

#include <iostream>

class prefixbuf: public std::streambuf
{
	std::string prefix;
	std::streambuf *dest;
	bool ready = true;
	bool has_written = false;
	int sync();
	int overflow(int c);
public:
	prefixbuf(const std::string &prefix, std::streambuf *dest);
	bool empty() const { return !has_written; }
};

class logstream : private virtual prefixbuf, public std::ostream
{
public:
	logstream(const std::string &prefix, std::ostream &dest);
	bool empty() const { return prefixbuf::empty(); }
};

#endif //LOGSTREAM_H
