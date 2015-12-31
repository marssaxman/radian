// Copyright 2009-2012 Mars Saxman
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the
// use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented); you must not
// claim that you wrote the original software. If you use this software in a
// product, an acknowledgment in the product documentation would be appreciated
// but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.



// This is the master list of symbols known by the Radian runtime support code.
// This file will be included multiple times with different definitions of the
// SYMBOL macro. Do not insert a multiple-inclusion guard around this file.
// Keep these symbols in alphabetical order, because it's prettier that way.

SYMBOL(add);
SYMBOL(append);
SYMBOL(assign);
SYMBOL(bit_and);
SYMBOL(bit_or);
SYMBOL(bit_xor);
SYMBOL(byte_size);
SYMBOL(call);
SYMBOL(chop);
SYMBOL(compare_to);
SYMBOL(concatenate);
SYMBOL(contains);
SYMBOL(ctype);
SYMBOL(current);
SYMBOL(denominator);
SYMBOL(describe_function);
SYMBOL(divide);
SYMBOL(exponentiate);
SYMBOL(from_bytes);
SYMBOL(head);
SYMBOL(input);
SYMBOL(insert);
SYMBOL(int8);
SYMBOL(int16);
SYMBOL(int32);
SYMBOL(int64);
SYMBOL(is_empty);
SYMBOL(is_integer);
SYMBOL(is_number);
SYMBOL(is_rational);
SYMBOL(is_running);
SYMBOL(is_valid);
SYMBOL(iterate);
SYMBOL(load_external);
SYMBOL(lookup);
SYMBOL(modulus);
SYMBOL(multiply);
SYMBOL(next);
SYMBOL(numerator);
SYMBOL(partition);
SYMBOL(pointer);
SYMBOL(pop);
SYMBOL(print);
SYMBOL(push);
SYMBOL(remove);
SYMBOL(response);
SYMBOL(reverse);
SYMBOL(send);
SYMBOL(shift_left);
SYMBOL(shift_right);
SYMBOL(size);
SYMBOL(start);
SYMBOL(subtract);
SYMBOL(tail);
SYMBOL(to_bytes);
SYMBOL(to_char);
SYMBOL(uint8);
SYMBOL(uint16);
SYMBOL(uint32);
SYMBOL(uint64);
SYMBOL(undefined);
SYMBOL(void);
SYMBOL(write_file);
