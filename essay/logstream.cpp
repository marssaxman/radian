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

