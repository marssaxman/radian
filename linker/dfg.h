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
#include <memory>
#include <vector>
#include <map>

class dfg
{
public:
	class node
	{
	public:
		virtual ~node() {}
	};
	class literal : public node
	{
		int64_t value;
	public:
		literal(int64_t v): value(v) {}
	};
	class block
	{
	public:
		std::vector<std::unique_ptr<node>> nodes;
	};
	void read(std::istream &src, std::ostream &log);
private:
	std::map<std::string, std::unique_ptr<block>> blocks;
	class reader
	{
		struct {
			size_t line = 1;
			size_t column = 0;
		} pos;
		std::ostream &err;
		dfg &dest;
		block current;
		size_t stack = 0;
		void fail(std::string message);
		void literal(std::string body);
		void token(std::string token);
		void line(std::string text);
	public:
		reader(dfg &dest, std::istream &src, std::ostream &err);
	};
};

#endif //DFG_H

