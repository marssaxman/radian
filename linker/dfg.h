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

#ifndef DFG_H
#define DFG_H

#include <string>
#include <iostream>

class DFG
{
	struct {
		std::string path;
		size_t line = 0;
		size_t column = 0;
		bool valid = true;
	} parse;
	std::ostream &fail();
	void read_op(std::string instruction);
	void read_term(char prefix, std::string body);
	void read_token(std::string token);
	void read_line(std::string text);
public:
	void read(std::string name, std::istream &src);
	bool valid() const { return parse.valid; }
};

#endif //DFG_H

