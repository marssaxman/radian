
struct node
{
	// binops: arg, val -> result
	enum {
		// block frame
		enter,	// type -> param
		leave,	// arg, proc -> proc

		// arithmetic
		add, sub, mul, div, quo, rem, neg,

		// comparison
		ceq, cne, cgt, clt, cge, cle,

		// logic
		andl, orl, xorl, notl,

		// conversion
		test,	// int -> bool
		fcvt,	// int -> float

		// creating structures
		tuple,	// type -> seq
		array,	// type -> seq
		clone,	// val -> seq
		push,	// arg, seq -> seq
		skip,	// count, seq -> seq
		merge,	// seq, seq -> seq
		alloc,	// seq -> struct

		// using tuples
		field,	// index, tuple -> val

		// using arrays
		size,	// array -> val
		item,	// index, array -> val
		peek,	// default, array -> val
		head,	// count, array -> array
		next,	// count, array -> array
		tail,	// count, array -> array
		drop,	// count, array -> array

		// conditional evaluation
		branch,	// val, val -> branch
		cond,	// bool, branch -> val
		proc,	// name -> proc

	} op;
};

struct leaf: public node
{
	// enter, tuple, array, proc
	std::string data; // type string, static data, or link name
};

struct unary: public node
{
	// neg, notl, test, fcvt, clone, alloc, size
	const node *source;
};

struct binary: public node
{
	
	const node *value;
	const node *source;
};

struct immed: public node
{
	uint32_t value;
	const node *source;
};


