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

#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include "logstream.h"
#include "token.h"
#include "ast.h"

static void parse(const lexer &input, std::ostream &log)
{
	for (lexer pos = input.begin(); pos; ++pos) {
		switch (pos->type) {
			case token::opening:
			case token::closing:
			case token::separator:
			default: break;
		}
	}
}

int main(int argc, const char *argv[])
{
	logstream log(std::string(argv[0]) + ": ", std::cerr);
	if (argc <= 1) {
		log << "fail: no input files" << std::endl;
	}
	for (int i = 1; i < argc; ++i) {
		std::string path(argv[i]);
		std::ifstream src(path);
		if (src) {
			logstream err(path + ":", log);
			std::stringstream buf;
			buf << src.rdbuf();
			src.close();
			std::string text = buf.str();
			lexer input(text);
			parse(input, log);
		} else {
			log << "fail: could not read " << path << std::endl;
		}
	}

	return log.empty()? EXIT_SUCCESS: EXIT_FAILURE;
}

