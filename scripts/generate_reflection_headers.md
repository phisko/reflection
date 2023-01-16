# [generate_reflection_headers](generate_reflection_headers.py)

Script to automatically generate reflection code for types using `libclang`. It relies on "brief comments", i.e. comments placed before a declaration and starting with a `!` (like `//! putils reflect`).

## CMake helper

A [putils_generate_reflection_headers](generate_reflection_headers.cmake) function is exposed that will automatically call the script for each of a target's headers, passing the required include and define arguments to clang.

## Marking a type as reflectible

The script will generate reflection code for any type that starts its "brief comment" with `putils reflect`. For instance, running the script for:

```cpp
#pragma once

// reflectible.hpp

//! putils reflect all
struct reflectible {
	int i;
	void f();
};
```

Will generate the following reflection header:

```cpp
#pragma once

#include "reflectible.hpp"
#include "putils/reflection.hpp"

#define refltype reflectible
putils_reflection_info {
	putils_reflection_class_name;
	putils_reflection_attributes(
		putils_reflection_attribute(i)
	);
	putils_reflection_methods(
		putils_reflection_methods(f)
	);

};
#undef refltype
```

## Selecting what you reflect

Attributes and methods may or may not be reflected depending on the "argument" passed to `!putils reflect`.

* `!putils reflect all`: reflects all attributes and methods
* `!putils reflect attributes`: reflects only attributes
* `!putils reflect methods`: reflects only methods

Note that operator overloads are automatically excluded.

You can also use `!putils reflect on` and `!putils reflect off` to individually enable or disable reflection for an attribute or method.

## Using a custom class name

To specify a custom class name, add `class_name: your class name` to your brief comment.

```cpp
//! putils reflect all
//! class_name: my_custom_class_name
struct reflectible {};

// will generate
#define refltype reflectible
putils_reflection_info {
	putils_reflection_custom_class_name(my_custom_class_name);
};
#undef refltype
```

## Reflecting parents and used types

To reflect parents or used types, add `parents: [type1, type2, ...]` to your brief comment.

```cpp
struct base {};

//! putils reflect all
//! parents: [base]
//! used_types: [int, float]
struct reflectible : base {
	int i;
	float f;
};

// will generate
#define refltype reflectible
putils_reflection_info {
	putils_reflection_class_name;
	putils_reflection_parents(
		putils_reflection_type(base)
	);
	putils_reflection_used_types(
		putils_reflection_type(int),
		putils_reflection_type(float)
	);
};
#undef refltype
```