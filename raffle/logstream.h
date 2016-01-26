// Copyright 2016 Mars Saxman.
//
// Radian is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 2 of the License, or (at your option) any later
// version.
//
// Radian is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// Radian. If not, see <http://www.gnu.org/licenses/>.

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
