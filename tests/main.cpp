//-----------------------------------------------------------------------------
// The MIT License (MIT)
// 
// Copyright (c) 2017 Jeff Hutchinson
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//-----------------------------------------------------------------------------

#include <cstdio>
#include <vector>
#include <duktape.h>

const char * script =
"var gSharedVariable = 0;\n"
"function add(a, b) {\n"
"   return a + b;\n"
"}\n"
"\n"
"Animal.prototype.test = function() {\n"
"   print('Test Call to javacript from C++.');\n"
"};\n"
"\n"
"function main() {\n"
"   print('Hello World!');\n"
"   print('Value of shared variable is ' + gSharedVariable);\n"
"}\n"
"\n"
"var animal = new Animal();\n"
"for (var i = 0; i < 5; i++) {\n"
"   animal.speak();\n"
"}\n";

duk_context *ctx = nullptr;

void viewStack(duk_context *ctx);

// https://github.com/svaarala/duktape-wiki/blob/master/HowtoNativeConstructor.md
class Animal {
public:
	Animal() {

	}

	void speak() {
		printf("SPEAK!!!!\n");
	}

	void *dukPtr;
};
std::vector<Animal*> gAnimals;

static duk_ret_t Animal_New(duk_context *ctx) {
	if (!duk_is_constructor_call(ctx))
		return DUK_RET_TYPE_ERROR;

	// Stack: []

	Animal *ptr = new Animal();
	gAnimals.push_back(ptr);
	
	duk_push_this(ctx); // [ this ] 
	ptr->dukPtr = duk_get_heapptr(ctx, -1); // [ this ]

	// Bind the native pointer to the js pointer.
	duk_push_string(ctx, "__ptr"); // [ this "__ptr" ]
	duk_push_pointer(ctx, ptr); // [ this "__ptr" 0xPTR ]
	duk_put_prop(ctx, -3); // [ this ]

	return 0;
}

static duk_ret_t Animal_speak(duk_context *ctx) {
	// Get the object we are modifiying, or the thisptr
	duk_push_this(ctx);

	// Get the native pointer handle.
	duk_get_prop_string(ctx, -1, "__ptr");
	void *ptr = duk_get_pointer(ctx, -1);

	// Call speak
	static_cast<Animal*>(ptr)->speak();
	return 0;
}

static void firePrototypeCb_Animal(Animal *animal) {
	duk_push_heapptr(ctx, animal->dukPtr); // [ ... dukPtr ]
	duk_get_prop_string(ctx, -1, "test"); // [ ... dukPtr test_fn ]
	duk_pcall(ctx, 0); // [ ... dukPtr return_value ]
	// Ignore return value
	duk_pop(ctx); // [ ... dukPtr ] 

	// pop obj that we pushed.
	duk_pop(ctx); // [ ... ]
}

void viewStack(duk_context *ctx) {
	// duk_to_string modifies the stack, so we have to duplicate the current
	// value that we are inspecting to the top of the stack by pushing it,
	// modifying it, and then poping it so we don't screw up the stack.
	// This is probably slow, but this function is used for inspecting the stack.
	// http://duktape.org/api.html#duk_to_string
	printf("----- Viewing Stack -----\n");
	int count = duk_get_top_index(ctx);
	for (int i = count; i > -1; i--) {
		duk_dup(ctx, i);
		printf("%s\n", duk_to_string(ctx, count + 1));
		duk_pop(ctx);
	}
	printf("---- end Stack ----\n");
}

// https://github.com/svaarala/duktape/blob/master/examples/guide/primecheck.c
static duk_ret_t script_print(duk_context *ctxx) {
	duk_push_string(ctxx, " ");
	duk_insert(ctxx, 0);
	duk_join(ctxx, duk_get_top(ctxx) - 1);
	printf("%s\n", duk_to_string(ctxx, -1));
	return 0;
}

// after each line is executed, the comments that follow is what the stack looks like
int main(int argc, const char **argv) {
	ctx = duk_create_heap_default(); // Stack: [ ... ]

	
	duk_push_global_object(ctx);// Stack: [ ... global_object ]
	duk_push_c_function(ctx, script_print, DUK_VARARGS); // [ ... global_object script_print_fn ]
	duk_put_prop_string(ctx, -2, "print"); // [ ... global_object ]

	/// { 
	/// Initialize Animal Prototype 

	// Bind the prototype constructor and object.
	duk_push_c_function(ctx, Animal_New, 0); // [ ... global_object AnimalCSTR ]
	duk_push_object(ctx); // [ ... global_object AnimalCSTR PrototypeObj ]

	// Bind methods to the prototype
	{
		duk_push_c_function(ctx, Animal_speak, 0);
		duk_put_prop_string(ctx, -2, "speak");
	}

	// Bind prototype property and the name of the prototype to use with new stmts.
	duk_put_prop_string(ctx, -2, "prototype"); // [ ... global_object AnimalCSTR ]
	duk_put_global_string(ctx, "Animal"); // [ ... global_object ]

	/// }

	duk_eval_string_noresult(ctx, script); // [ ... global_object ]
	duk_get_prop_string(ctx, -1, "main");  // [ ... global_object main_fn ]
	if (duk_pcall(ctx, 0) != 0) { // [ ... global_object return_value ]
		printf("Error: %s\n", duk_safe_to_string(ctx, -1)); // [ ... global_object return_value ]
	}
	duk_pop(ctx); // [ ... global_object ]

	// Call add
	// currently stack is just global object which has add fn.
	duk_get_prop_string(ctx, -1, "add"); // [ ... global_object add_fn ]
	duk_push_int(ctx, 4); // [ ... global_object add_fn 4 ]
	duk_push_int(ctx, 5); // [ ... global_object add_fn 4 5 ]
	if (duk_pcall(ctx, 2) == DUK_EXEC_SUCCESS) { // [ ... global_object return_value ]
		printf("Result of add() is %d\n", duk_get_int(ctx, -1)); // [ ... global_object return_value ]
	} else {
		printf("Error: %s\n", duk_safe_to_string(ctx, -1)); // [ ... global_object return_value ]
	}
	duk_pop(ctx); // [ ... global_object ]

	firePrototypeCb_Animal(gAnimals[0]);
	
	duk_pop(ctx); // [ ... ]
	duk_destroy_heap(ctx); // []

#ifdef _WIN32
	system("pause");
#endif
	return 0;
}