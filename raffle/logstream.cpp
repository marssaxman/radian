// Copyright (C) 2016 Mars Saxman. All rights reserved.
// Permission is granted to use at your own risk and distribute this software
// in source and binary forms provided all source code distributions retain
// this paragraph and the above copyright notice. THIS SOFTWARE IS PROVIDED "AS
// IS" WITH NO EXPRESS OR IMPLIED WARRANTY.

#include "logstream.h"

int prefixbuf::sync()
{
	return dest->pubsync();
}

int prefixbuf::overflow(int c)
{
	has_written = true;
	if (ready) {
		int len = dest->sputn(&prefix[0], prefix.size());
		if (len != prefix.size()) {
			return std::char_traits<char>::eof();
		}
		ready = false;
	}
	ready = c == '\n';
	return dest->sputc(c);
}

prefixbuf::prefixbuf(const std::string &_prefix, std::streambuf *_dest):
	prefix(_prefix),
	dest(_dest)
{
}

logstream::logstream(const std::string &prefix, std::ostream &dest):
	prefixbuf(prefix, dest.rdbuf()),
	std::ios(static_cast<std::streambuf*>(this)),
	std::ostream(static_cast<std::streambuf*>(this))
{
}

