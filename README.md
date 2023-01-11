# [Reflection](reflection.hpp)

[![tests](https://github.com/phisko/reflection/workflows/tests/badge.svg)](https://github.com/phisko/reflection/actions/workflows/tests.yml)

A simple, stand-alone, header-only and easily pluggable reflection system.

This provides an API that you can implement for any type so that you can introspect its attributes, methods, parent types and any types it "uses".

The "used types" concept can refer to anything really, but one interesting use case is when registering a type with scripting languages. If you're going to be accessing a type's attributes through a scripting language, chances are you also want to register those attributes' types.

An example use case for this API is the register_type function provided for [Lua](https://github.com/phisko/putils/blob/master/lua/lua_helper.hpp#L80) and [Python](https://github.com/phisko/putils/blob/master/python/python_helper.inl) in my putils library, that inspects a type and registers all its attributes and methods to the scripting language.

## Helpers

A [generate_reflection_headers](generate_reflection_headers.md) script is provided to automatically generate reflection info. This is however completely optional, and you might prefer writing your reflection info by hand to start with.

## Overview

Making a type reflectible is done like so:

```cpp
struct parent {};

struct reflectible : parent {
    int i = 0;

    int get_value() const { return i; }
};

#define refltype reflectible
putils_reflection_info {
    putils_reflection_class_name;
    putils_reflection_attributes(
        putils_reflection_attribute(i)
    );
    putils_reflection_methods(
        putils_reflection_attribute(get_value)
    );
    putils_reflection_parents(
        putils_reflection_type(parent)
    );
    putils_reflection_used_types(
        putils_reflection_type(int)
    );
};
#undef refltype
```

Note that all the information does not need to be present. For instance, for a type with only two attributes, and for which we don't want to expose the class name:

```cpp
struct simple {
    int i = 0;
    double d = 0;
};

#define refltype simple
putils_reflection_info {
    putils_reflection_attributes(
        putils_reflection_attribute(i),
        putils_reflection_attribute(d)
    );
};
#undef refltype
```

Accessing a type's name, attributes, methods and used types is done like so:

```cpp
int main() {
    reflectible obj;

    std::cout << putils::reflection::get_class_name<reflectible>() << std::endl;

    // Obtaining member pointers
    {
        putils::reflection::for_each_attribute<reflectible>(
            [&](const auto & attr) {
                assert(attr.ptr == &reflectible::i);
                std::cout << attr.name << ": " << obj.*attr.ptr << std::endl;
            }
        );
        constexpr auto member_ptr = putils::reflection::get_attribute<int, reflectible>("i");
        static_assert(member_ptr == &reflectible::i);
    }

    // Obtaining attributes of a specific object
    {
        putils::reflection::for_each_attribute(obj,
            [&](const auto & attr) {
                assert(&attr.member == &obj.i);
                std::cout << attr.name << ": " << attr.member << std::endl;
            }
        );
        const auto attr = putils::reflection::get_attribute<int>(obj, "i");
        assert(attr == &obj.i);
    }

    // Obtaining member function pointers
    putils::reflection::for_each_method<reflectible>(
        [&](const auto & method) {
            assert(method.ptr == &reflectible::get_value);
            std::cout << method.name << ": " << (obj.*method.ptr)() << std::endl;
        }
    );

    // Obtaining functors to call the method on a given object
    putils::reflection::for_each_method(obj,
        [](const auto & method) {
            // func is a functor that calls obj.get_value()
            std::cout << method.name << ": " << method.member() << std::endl;
        }
    );

    putils::reflection::for_each_parent<reflectible>(
        [](const auto & type) {
            // type: putils::meta::type<parent>
            using T = putils_wrapped_type(type.type);
            std::cout << typeid(T).name() << std::endl;
        }
    );

    putils::reflection::for_each_used_type<reflectible>(
        [](const auto & type) {
            // type: putils::meta::type<int>
            using T = putils_wrapped_type(type.type);
            std::cout << typeid(T).name() << std::endl;
        }
    );

    return 0;
}
```

All these operations can also be done at compile-time:

```cpp
constexpr reflectible obj;

constexpr auto & attributes = putils::reflection::get_attributes<reflectible>();
using expected_type = std::tuple<std::pair<const char *, int reflectible:: *>>;
static_assert(std::is_same_v<std::decay_t<decltype(attributes)>, expected_type>);
static_assert(std::get<0>(attributes).second == &reflectible::i);

constexpr auto attr = putils::reflection::get_attribute<int>(obj, "i");
static_assert(attr == &obj.i);
static_assert(*attr == 0);

constexpr size_t count_attributes() {
    size_t ret = 0;
    putils::reflection::for_each_attribute<reflectible>(
        [&](const auto & attr) {
            ++ret;
        }
	);
    return ret;
}

static_assert(count_attributes() == 1);
```

Functions are also provided to check if a type exposes a given property:

```cpp
static_assert(putils::reflection::has_class_name<reflectible>());
static_assert(putils::reflection::has_attributes<reflectible>());
static_assert(putils::reflection::has_methods<reflectible>());
static_assert(putils::reflection::has_parents<reflectible>());
static_assert(putils::reflection::has_used_types<reflectible>());
```

## Metadata

Attributes and methods can be annotated with custom metadata like so:

```cpp
struct with_metadata {
    int i = 0;
    void f() const;
};

#define refltype with_metadata
putils_reflection_info {
    putils_reflection_attributes(
        putils_reflection_attribute(i, putils_reflection_metadata("meta_key", "meta_value"))
    );
    putils_reflection_methods(
        putils_reflection_attribute(f, putils_reflection_metadata(42, std::string("value")))
    );
};
#undef refltype
```

Metadata are key-value pairs, with no specific constraint regarding the types for either the key or the value.

Metadata can then be accessed directly from the `metadata` table in the `attribute_info` returned by `get_attributes`/`get_methods` and iterated on by `for_each_attribute`/`for_each_method`.

They may also be queried and accessed through helper functions:
```cpp
template<typename T, typename Key>
constexpr bool has_attribute_metadata(std::string_view attribute, Key && key) noexcept;

template<typename Ret, typename T, typename Key>
constexpr const Ret * get_attribute_metadata(std::string_view attribute, Key && key) noexcept;

template<typename T, typename Key>
constexpr bool has_method_metadata(std::string_view method, Key && key) noexcept;

template<typename Ret, typename T, typename Key>
constexpr const Ret * get_method_metadata(std::string_view method, Key && key) noexcept;

template<typename ... Metadata, typename Key>
constexpr bool has_metadata(const putils::table<Metadata...> & metadata, Key && key) noexcept;

template<typename Ret, typename ... Metadata, typename Key>
constexpr const Ret * get_metadata(const putils::table<Metadata...> & metadata, Key && key) noexcept;
```

## API

Making a type reflectible consists in specializing the `putils::reflection::type_info` template with (up to) 5 static members that provide type information.

```cpp
namespace putils::reflection {
	template<typename T>
	struct type_info {
		static constexpr auto class_name = const char *;
		static constexpr auto attributes = std::tuple<std::pair<const char *, member_pointer>...>;
		static constexpr auto methods = std::tuple<std::pair<const char *, member_pointer>...>;
		static constexpr auto parents = std::tuple<putils::meta::type<parent>...>;
		static constexpr auto used_types = std::tuple<putils::meta::type<used_type>...>;
	};
}
```

For instance, for the `reflectible` struct given as an example above:

```cpp
template<>
struct putils::reflection::type_info<reflectible> {
    static constexpr auto class_name = "reflectible";
    static constexpr auto attributes = std::make_tuple(
        std::make_pair("i", &reflectible::i)
    );
    static constexpr auto methods = std::make_tuple(
        std::make_pair("get_value", &reflectible::get_value)
    );
    static constexpr auto parents = std::make_tuple(
        putils::meta::type<parent>{}
    );
    static constexpr auto used_types = std::make_tuple(
        putils::meta::type<int>{}
    );
};
```

The `type_info` specialization can be easily defined through the use of helper macros, described below.

### class_name

```cpp
static constexpr auto class_name = "my_class";
```
Can be easily generated with `putils_reflection_class_name`.

### attributes

```cpp
static constexpr auto attributes = std::make_tuple(
    std::make_pair("attribute", &MyClass::attribute),
    ...
);
```
[table](https://github.com/phisko/meta/blob/main/table.md) mapping strings to pointers to the attributes.
Can be easily generated with `putils_reflection_attributes`.

### methods

```cpp
static constexpr auto methods = std::make_tuple(
    std::make_pair("method", &MyClass::method),
    ...
);
```
[table](https://github.com/phisko/meta/blob/main/table.md) mapping strings to pointers to the methods.
Can be easily generated with `putils_reflection_methods`.

### parents
```cpp
static constexpr auto parents = std::make_tuple(
    putils::meta::type<parent>{},
    ...
);
```
`std::tuple` of `putils::meta::type` objects for each of the type's parents.
Can be easily generated with `putils_reflection_parents`.

### used_types
```cpp
static constexpr auto used_types = std::make_tuple(
    putils::meta::type<UsedType>{},
    ...
);
```
`std::tuple` of `putils::meta::type` objects for each type used by the class (which should also be registered with scripting systems, for instance).
Can be easily generated with `putils_reflection_used_types`.

## Detector functions

The following functions are defined to let client code check whether a given type is reflectible.

```cpp
namespace putils::reflection {
    template<typename T>
    constexpr bool is_reflectible() noexcept; // Returns true if reflection info, even empty, was provided

    template<typename T>
    constexpr bool has_class_name() noexcept;

    template<typename T>
    constexpr bool has_attributes() noexcept;

    template<typename T>
    constexpr bool has_methods() noexcept;

    template<typename T>
    constexpr bool has_parents() noexcept;

    template<typename T>
    constexpr bool has_used_types() noexcept;
}
```

## Iterating attributes

Once a type is declared reflectible, iterating over any of its reflectible properties is made easy by the following helper functions. Note that calling these functions with a non-reflectible type is supported, and will do nothing.

### for_each_attribute

```cpp
namespace putils::reflection {
    template<typename T, typename Func> // Func: void(const attribute_info & attr)
    void for_each_attribute(Func && func) noexcept;

    template<typename T, typename Func> // Func: void(const object_attribute_info & attr)
    void for_each_attribute(T && obj, Func && func) noexcept;
}
```

Lets client code iterate over the attributes for a given type.

### for_each_method

```cpp
namespace putils::reflection {
    template<typename T, typename Func> // Func: void(const attribute_info & attr)
    void for_each_method(Func && func) noexcept;

    template<typename T, typename Func> // Func: void(const object_attribute_info & attr)
    void for_each_method(T && obj, Func && func) noexcept;
}
```

Lets client code iterate over the methods for a given type.

### for_each_parent

```cpp
namespace putils::reflection {
    template<typename T, typename Func> // Func: void(const used_type_info & attr)
    void for_each_parent(Func && func) noexcept;
}
```

Lets client code iterate over the parents for a given type.

### for_each_used_type

```cpp
namespace putils::reflection {
    template<typename T, typename Func> // Func: void(const used_type_info & attr)
    void for_each_used_type(Func && func) noexcept;
}
```

Lets client code iterate over the types used by a given type.

## Querying attributes

```cpp
template<typename T, typename Parent>
constexpr bool has_parent();

template<typename T, typename Used>
constexpr bool has_used_type();

template<typename T>
constexpr bool has_attribute(std::string_view name);

template<typename T>
constexpr bool has_method(std::string_view name);
```

Returns whether `T` has the specified parent, used type, attribute or method.

## Getting specific attributes

### get_attribute

```cpp
template<typename Member, typename T>
std::optional<Member T::*> get_attribute(std::string_view name) noexcept;

template<typename Member, typename T>
Member * get_attribute(T && obj, std::string_view name) noexcept;
```

Returns the attribute called `name` if there is one.
* The first overload returns an `std::optional` member pointer (or `std::nullopt`)
* The second overload returns a pointer to `obj`'s attribute (or `nullptr`)

### get_method

```cpp
template<typename Signature, typename T>
std::optional<Signature T::*> get_method(std::string_view name) noexcept;

template<typename Signature, typename T>
std::optional<Functor> get_method(T && obj, std::string_view name) noexcept;
```

Returns the method called `name` if there is one.
* The first overload returns an `std::optional` member pointer (or `std::nullopt`)
* The second overload returns an `std::optional` functor which calls the method on `obj` (or `std::nullopt`)

## Helper macros

The following macros can be used to greatly simplify defining the `putils::reflection::type_info` specialization for a type.

These macros expect a `refltype` macro to be defined for the given type:
```cpp
#define refltype ReflectibleType
...
#undef refltype
```

### putils_reflection_info

Declares a specialization of `putils::reflection::type_info` for `refltype`.

### putils_reflection_info_template

Declares a specialization of `putils::reflection_type_info` for a template type, e.g.:
```cpp
template<typename T>
struct my_type {};

template<typename T>
#define refltype my_type<T>
putils_reflection_info_template {
    ...
};
#undef refltype
```

### putils_reflection_friend(type)

Used inside a reflectible type to mark the corresponding `type_info` as `friend`, in order to reflect private fields. Takes as parameter the name of the type.

### putils_reflection_class_name

Defines a `class_name` static string with `refltype` as its value.

### putils_reflection_custom_class_name(name)

Defines a `class_name` static string with the macro parameter as its value.

### putils_reflection_attributes(attributes...)

Defines an `attributes` static table of `std::pair<const char *, member_pointer>`.

### putils_reflection_methods(methods...)

Defines a `methods` static table of `std::pair<const char *, member_pointer>`.

### putils_reflection_parents(parents...)

Defines a `parents` static tuple of `putils::meta::type`.

### putils_reflection_attribute(attributeName)

Takes the name of an attribute as parameter and generates of pair of parameters under the form `"var", &refltype::var` to avoid redundancy when passing parameters to `putils::make_table`. For instance:

```cpp
const auto table = putils::make_table(
    "x", &point::x,
    "y", &point::y
);
```

can be refactored to:

```cpp
#define refltype point
const auto table = putils::make_table(
    putils_reflection_attribute(x),
    putils_reflection_attribute(y)
);
#undef refltype
```

### putils_reflection_attribute_private(member_ptr)

Provides the same functionality as `putils_reflection_attribute`, but skips the first character of the attribute's name (such as an `_` or `m`) that would mark a private member. For instance:

```cpp
const auto table = putils::make_table(
    "name", &human::_name,
    "age", &human::_age
);
```

can be refactored to:

```cpp
#define refltype human
const auto table = putils::make_table(
    putils_reflection_attribute_private(_name),
    putils_reflection_attribute_private(_age)
);
#undef refltype
```

### putils_reflection_type(name)

Provides the same functionality as `putils_reflection_attribute`, but for types. It takes a type name as parameter and expands to `putils::meta::type<class_name>{}` to avoid redundancy when passing parameters to `putils::make_table`.
