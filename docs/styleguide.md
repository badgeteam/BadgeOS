# BadgerOS style guide
- [C code and header style guide](#c-code-style)



# C code style
- [Naming conventions](#naming-conventions)
- [Type definitions](#type-definitions)
- [Variable declarations](#variable-declarations)
- [Function declarations](#function-declarations)
- [Function definitions](#function-definitions)
- [File structure](#file-structure)
- [Spacing](#spacing)
- [Indentation](#indentation)


## Naming conventions
All names should be in `snake_case`, some of which are `SNAKE_UPPERCASE`.
Specifically, macros may be either, other most namable things are only `snake_case`.

The following are always `snake_case`:
- Function names
- Variable names
- Struct/union/enum names
- Typedef names

Enum values and macro names are mostly `SNAKE_UPPERCASE` except macros that replace functions, types or other lowercase items, in which case the macro name should also be lowercase.

It is also good to establish the words that make up the name, take the following example:
```c
// Returns the amount of GPIO pins present.
#define io_count() (31)
// Sets the mode of GPIO pin `pin` to `mode`.
void    io_mode (badge_err_t *ec, int pin, io_mode_t mode);
```

This demonstrates the macro exception as well as the prefix given to the GPIO API.
In general, every API has a prefix, which may be extended if an API is split into multiple sub-APIs.

In general, names should:
- Have a prefix indicating what API is being referred to.
- Roughly describe their purpose:
    - Especially if the type of a variable is something nameless, like `int pin`.
    - This may be simplified if the context makes it clear, like `badge_err_t *ec`.
- Not use terms that are too abstract:
    - Things like `int *list` are not recommended.
    - Exact duplication of the type name like `badge_err_t *badge_err` is not recommended.
- Not start with `_`:
    - This is typically a workaround for equal names.
    - Names starting with `__` are reserved.
    - Names starting with `_` followed by a capital letter are reserved.

## Type definitions
Type definitions should be simple. If a type is a complex composite, consider using typedefs and structs. Here is a general example of how to define types:
```c
// If you need the longest integer the CPU has.
typedef unsigned long long my_type0;
// Prefer int8_t if possible.
typedef signed char        my_type1;
// Prefer uint8_t if possible.
typedef unsigned char      my_type2;
// For strings and other things to print.
typedef char               my_type3;
// For string constants.
typedef const char        *my_type4;

// Structs, unions and enums should be in a typedef.
typedef struct my_struct   my_struct_t;
// Same comment as at the typedef.
struct my_struct {
	// Same rules as variable declarations in here.
};

// Structs, unions and enums may also be anonymous.
typedef struct {
	// Same rules as variable declarations in here.
} my_anonymous_struct_t;

// Always use typedefs for function pointers.
typedef void (*my_func_1_t)(int some_parameter, char *another_parameter);
// If there are no parameters, write void.
typedef int  (*my_func_2_t)(void);
```

The following rules apply to types in general:
- The column at which the name starts is constant among a block of declarations / definitions.
- The padding is written with spaces.
- The `*` is at the function name and has at least one space from the type before it.
- The `const` is to the left of the type (unless the pointer is also a constant).
- When a specific resolution is required, use the appropriate (u)int[bits]_t.
- When referring to a length, use `size_t`.

The following rules apply to structs, unions and enums:
- The content of the definition is indented with four spaces.
- The `{` is on the same line as the `struct`, `union` or `enum` keyword.

The following rules apply to function pointer types:
- Always use typedefs for function pointers.
- If there are no parameters, write void.

The following rules apply to integers in types:
- When the integer refers to a `long`, `short`, `char`, etc. the `int` keyword is omitted.
- The `signed` keyword is only permitted before `char`.
- The `signed` and `unsigned` keywords are before the length of the integer.

The `bool` type is not to be treated as a regular integer:
    - `true` and `false` should be used where applicable.
    - Multiplication with a constant is allowed.
    - Multiplication with a variable or expression is not.
    - Bitwise OR, bitwise AND and bitwise complement are not allowed on booleans.


## Variable declarations
Variable declarations should be on one line unless that means exceeding 120 columns.
Every variable should have at least a one-line comment explaining the purpose.
Here is a general example of how to declare variables:
```c
// This string will be printed when XYZ happens.
const char *my_string_constant = "Hello, World!";
// This string will be printed when UVW happens.
const char *the_other_string   = "FooBar.";
// How many magic numbers are in `magic_numbers`.
size_t      magic_numbers_len;
// A list of carefully selected magic numbers.
const int  *magic_numbers;
```

The following rules apply:
- [The rules for type definitions](#type-definitions).
- The column at which the name starts is constant among a block of declarations / definitions.
- The column at which the `=` starts is constant among a block of definitions.
- The padding is written with spaces.
- The `=` has at least one space to the left and one space to the right.
- Arrays have a `_len` suffixed length counterpart of type `size_t`.
- The length of an array is declared before the array if that is possible.
- The variable name should approximately describe the purpose.


## Function declarations
Function declarations should be on one line unless that means exceeding 120 columns.
Every function should have at least a one-line comment explaining the purpose and functions with return values should explain what is returned on error.
Here is a general example of how to declare functions:
```c
// Computes an approximation of the square root of `x`.
// Does not accept negative inputs for `x`.
// Returns -1 on error.
int    fast_square_root(int x) __attribute__((const));
// Computes the integer square root of `x` (rounded down).
// Does not accept negative inputs for `x`.
// Returns -1 on error.
int    square_root     (int x) __attribute__((const));
// Counts the number of times `c` occurs in `str`.
// Returns 0 if `c` is 0 or if `str` is NULL.
size_t count_occurances(const char *str, char c)
		__attribute__((pure));
```

The following rules apply:
- [The rules for type definitions](#type-definitions).
- The column at which the name starts is constant among a block of declarations.
- The column at which the `(` starts is constant among a block of definitions.
- The padding is written with spaces.
- The attributes are behind the `)`.
    - With exactly one space of padding OR
    - On a newline, preceded by exactly eight spaces.
- The function name should approximately describe the purpose.


## Function definitions
Function definitions should be on one line unless that means exceeding 120 columns.
Every function should have at least a one-line comment explaining the purpose and functions with return values should explain what is returned on error.
Function definitions should have the same exact comment as their matching declaration.
Here is a general example of how to define functions:
```c
// Counts the number of times `c` occurs in `str`.
// Returns 0 if `c` is 0 or if `str` is NULL.
size_t count_occurances(const char *str, char c) {
	// Null checks.
	if (!str || !c) return 0;
	
	// Count the amount of times `c` occurs in `str`.
	size_t count = 0;
	// A comment here is not required.
	while (*str) {
		// Compare current character.
		// This type of counting would also work but is not recommended:
		// count += *str == c;
		if (*str == c) count ++;
		// Next character.
		str ++;
	}
	
	// A comment at such a return is not required.
	return count;
}
```

The following rules apply:
- [The rules for type definitions](#type-definitions).
- [The rules for functions declarations](#function-declarations), except:
    - No horizontal alignment is required or allowed.
    - If attributes are required, they must be on the declaration, but not definition.
- The `{` is on the same line as the `)`.

## File structure
The order of things to write in a C source file is:

1. The license identifier.
2. Includes.
    1. Project includes.
	2. System libraries.
	3. External libraries.
3. Local types, if any.
4. Functions.

Things like headers should be sorted alphabetically.


## Spacing
In order to make a high-level overview of a file more readable, there should be a certain number of empty lines between different parts of the file.
For any two given things, there should always be at least one empty line between them.

Things that are unrelated (like functions between two different APIs) should have exactly 3 empty lines between them. This also applies to space between the defines and types, and between the types and functions.

## Indentation
Indentation is written with size 4 tabs unless the required indent is not aligned to 4, in which case spaces may be added after the tabs.
