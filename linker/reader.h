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

#ifndef READER_H
#define READER_H

#include <iostream>
#include <vector>
#include <map>
#include "dfg.h"

namespace dfg {

class reader
{
	struct {
		size_t line = 0;
		size_t column = 0;
	} loc;
	std::ostream &err;
	unit &dest;
	std::string block_name;
	std::vector<const node*> stack;
	std::map<std::string, const node*> symbols;
	void fail(std::string message);
	const node *error(std::string message);
	const node *literal(std::string body);
	const node *symbol(std::string name);
	const node *reloc(std::string name);
	const node *terminal(char prefix, std::string body);
	const node *operation(std::string text);
	const node *token(std::string token);
	void eval(std::string text);
	void stmt(std::string text);
	void end_block();
	void decl(std::string text);
	void line(std::string text);
public:
	reader(unit &dest, std::istream &src, std::ostream &err);
};

} // namespace dfg

#endif //READER_H
