// Copyright (C) 2016 Mars Saxman. All rights reserved.
// Permission is granted to use at your own risk and distribute this software
// in source and binary forms provided all source code distributions retain
// this paragraph and the above copyright notice. THIS SOFTWARE IS PROVIDED "AS
// IS" WITH NO EXPRESS OR IMPLIED WARRANTY.

#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include "lexer.h"
#include "parser.h"

int main(int argc, const char *argv[])
{
	for (std::string line; std::getline(std::cin, line);) {
		for (lexer::iterator input(text); input; ++input) {
			//
		}
	}


		std::string path(argv[i]);
		std::ifstream src(path);
		if (src) {
			logstream err(path + ":", log);
			std::stringstream buf;
			buf << src.rdbuf();
			src.close();
			std::string text = buf.str();
			lexer::iterator input(text);
			parser astgen(input, log);
		} else {
			log << "fail: could not read " << path << std::endl;
		}
	}

	return log.empty()? EXIT_SUCCESS: EXIT_FAILURE;
}

