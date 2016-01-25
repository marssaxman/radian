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

#ifndef TYPES_H
#define TYPES_H

// the fundamental type is word, "w", representing a count or offset within
// the machine: addresses, array lengths, and structure sizes are words.
// basically size_t from C, but also uintptr_t if you think of memory as one
// big array of bytes.

// does it make sense to think of a word as having a scale, which is the
// size in bytes of the object to which it refers?
// we could then express multiplication and division as rescaling operations
// or am I going totally the wrong way here? maybe the ranged-pair arrays
// are a better solution for this class of problem

// what are the fundamental sorts of numbers? unsigned quantities representing
// specific things, actually - unicode characters, network addreses, protocol
// identifiers, bitmasks, that sort of thing

// the fundamental work of software is this: extract information from a stream
// of bytes by matching the specific values against a set of abstract patterns,
// yielding a data structure; perform a computation on that data structure,
// creating new data; and render the new data out as a bytestream.

// the elements of language we need, then, are these:
// - a grammar for extracting data from input
// - a data structure for describing data (tables, indexes, etc)
// - a set of transformations on that data structure
// - a grammar for producing output from data

// so.... the data structures should be abstract
// the transformations should be expressed in terms of map/filter/reduce
// this way, the grammar is an abstraction applied to a stream of input, and
// the transformation can be applied asynchronously, as input arrives

// so what's going on at the low level?
// well, we're looking at bytes and categorizing them to match patterns.
// what does a regex engine need? or a peg parser, maybe?
// pattern matching will necessarily proceed in layers
// many data formats have some structure with blobs of some other structure
// inside, to be interpreted by another layer higher up

// byte buffers need to be a fundamental datatype.
// switch tables need to be a fundamental control construct.
// we can represent packed structures as a series of byte reads, shifts,
// and masks, and must have efficient operations for doing this

// let's describe objects in terms of their data layouts and the constraints
// on the values in their fields
// a type consists of: here's a bunch of bytes, in this arrangement, with
// these names, which can have these ranges of values
// the compiler can then generate code which matches that pattern against some
// byte buffer and tells you whether it matches
// but more likely, you'd say: here's a byte buffer, which of these patterns
// does it match?
// type specialization would therefore consist of placing additional
// constraints on the contents of an existing structure
// for example, we could say that an "IP packet" contains at least 20
// bytes, which are laid out in thus and such a way
// we could have one specialization which includes the options field and
// another which does not
// we could say that a "TCP packet" consists of an "IP" packet whose protocol
// field value equals 6, which adds some number of additional fields, and
// that an "ICMP packet" is an "IP packet" with a protocol field equal to 1,
// etc. we could then have the compiler automatically generate code which
// would analyze an incoming blob and route it to the appropriate protocol
// handler

// we could continue this kind of process all the way up the stack

// the pattern-matcher could ignore any fields it didn't actually need in
// order to resolve the specific routing problem

// this mechanism seems like it would be general enough to support a whole
// array of higher-level overloading and method dispatch protocols
// it's a natural fit with tagged unions and algebraic types

// ok, so let's say we have a fundamental "buffer" type
// we can break a buffer down into a series of fields
// fields have a length, in bytes, with an optional bitfield syntax
// atomic types: bit, byte
// bigendian adds word16, int16, word32, int32, etc
ipv4 = bigendian(buffer):
	field version = bit*4
	assert version == 4
	field ihl = bit*4
	field dscp = bit*6
	field ecn = bit*2
	field length = word16
	field identification = word16
	field flags = bit*2
	field frag_offset = bit*14
	field ttl = byte
	field protocol = byte
	field checksum = word16
	field source = word32
	field destination = word32
	if ihl > 5:
		field options = word32
	end
	def header_size = length
	body = buffer
	assert body.size = length - header_size
end

tcp = ipv4:
	assert protocol = 6
	... more fields
end

icmp = ipv4:
	assert protocol = 1
	... more fields
end

// perhaps we can do type unions?
def packet = tcp or icmp

// this is not the same as ipv4: where ipv4 could be anything with the right
// values in the protocol and length fields, "packet" would be specifically
// either tcp or icmp packets only

// we could define text character classes the same way
digit = char:
	assert self >= '0'
	assert self <= '9'
end
lower = char:
	assert self >= 'a'
	assert self <= 'z'
end
upper = char:
	assert self >= 'A'
	assert self <= 'Z'
end
alpha = lower or upper
alnum = alpha or digit
ident_start = alnum
ident_body = alnum or (char = '_')

// now we can define sequences of patterns
ident = ident_start + optional(repeat(ident_body))
number = repeat(digit)

// this matches an array of bytes having certain properties
// so now we can do something like this:
match input:
case ident:
	yield "identifier"
case number:
	yield "number"
else:
	fail
end







#endif //TYPES_H

