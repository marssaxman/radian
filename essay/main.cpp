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
#include "logstream.h"
#include "token.h"

using std::endl;

int main(int argc, const char *argv[])
{
	logstream log(std::string(argv[0]) + ": ", std::cerr);
	if (argc <= 1) {
		log << "fail: no input files" << endl;
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
			const char *pos = text.c_str();
			const char *end = pos + text.size();
			while (pos != end) {
				token foo(pos, end);
				pos = foo.addr + foo.len;
				switch (foo.type) {
					case token::error:
						log << "error";
						break;
					case token::eof:
						log << "done";
						break;
					case token::number:
						log << "number(" << foo.value() << ")";
						break;
					case token::literal:
						log << "literal(" << foo.value() << ")";
						break;
					case token::symbol:
						log << "symbol(" << foo.value() << ")";
						break;
					case token::opcode:
						log << "opcode(" << foo.value() << ")";
						break;
					case token::lparen:
					case token::rparen:
					case token::lbracket:
					case token::rbracket:
					case token::lbrace:
					case token::rbrace:
					case token::comma:
					case token::semicolon:
						log << foo.value();
						continue;
					case token::newline:
						log << endl;
						continue;
				}
				log << " ";
			}


		} else {
			log << "fail: could not read " << path << std::endl;
		}
	}

	return log.empty()? EXIT_SUCCESS: EXIT_FAILURE;
}

