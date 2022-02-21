#include <gtest/gtest.h>
#include <tuple>
#include "reflection.hpp"

struct Parent {
    int iParent = 84;
    int fParent(double d) const noexcept { return (int)d + 84; }
};

#define refltype Parent
putils_reflection_info{
    putils_reflection_class_name;
    putils_reflection_attributes(
        putils_reflection_attribute(iParent)
    );
    putils_reflection_methods(
        putils_reflection_attribute(fParent)
    );
    putils_reflection_used_types(
        putils_reflection_type(int)
    );
};
#undef refltype

struct Reflectible : Parent {
    int i = 42;
    const char * s = "foo";

    int f(double d) const noexcept { return (int)d + 42; }
};

#define refltype Reflectible
putils_reflection_info{
    putils_reflection_class_name;
    putils_reflection_attributes(
        putils_reflection_attribute(i),
        putils_reflection_attribute(s)
    );
    putils_reflection_methods(
        putils_reflection_attribute(f)
    );
    putils_reflection_parents(
        putils_reflection_type(Parent)
    );
    putils_reflection_used_types(
        putils_reflection_type(const char *)
    );
};
#undef refltype

/*
 * Class name
 */

TEST(ReflectionTest, HasClassNameTrue) {
    static_assert(putils::reflection::has_class_name<Reflectible>());
    SUCCEED();
}

TEST(ReflectionTest, HasClassNameFalse) {
    static_assert(!putils::reflection::has_class_name<int>());
    SUCCEED();
}

TEST(ReflectionTest, GetClassName) {
    static_assert(putils::reflection::get_class_name<Reflectible>() == std::string_view(putils_nameof(Reflectible)));
    static_assert(putils::reflection::get_class_name<Parent>() == std::string_view(putils_nameof(Parent)));
}

/*
 * Attributes
 */

TEST(ReflectionTest, HasAttributesTrue) {
    static_assert(putils::reflection::has_attributes<Reflectible>());
    SUCCEED();
}

TEST(ReflectionTest, HasAttributesFalse) {
    static_assert(!putils::reflection::has_attributes<int>());
    SUCCEED();
}

TEST(ReflectionTest, AttributesSize) {
    {
        constexpr auto & attributes = putils::reflection::get_attributes<Parent>();
        constexpr auto size = std::tuple_size<putils_typeof(attributes)>();
        static_assert(size == 1);
    }
    {
        constexpr auto & attributes = putils::reflection::get_attributes<Reflectible>();
        constexpr auto size = std::tuple_size<putils_typeof(attributes)>();
        static_assert(size == 3);
    }
    SUCCEED();
}

TEST(ReflectionTest, GetAttributes) {
    constexpr auto attributes = putils::reflection::get_attributes<Reflectible>();
    static_assert(std::get<0>(attributes).name == std::string_view("i"));
    static_assert(std::get<0>(attributes).ptr == &Reflectible::i);
    static_assert(std::get<1>(attributes).name == std::string_view("s"));
    static_assert(std::get<1>(attributes).ptr == &Reflectible::s);
    static_assert(std::get<2>(attributes).name == std::string_view("iParent"));
    static_assert(std::get<2>(attributes).ptr == &Parent::iParent);
}

TEST(ReflectionTest, ForEachAttributePointers) {
    constexpr auto test = []() consteval {
        constexpr auto table = putils::make_table(
            "iParent", &Parent::iParent,
            "i", &Reflectible::i,
            "s", &Reflectible::s
        );
        int ret = 0;
        putils::reflection::for_each_attribute<Reflectible>([&](const auto & attr) {
            const auto ptr = putils::get_value<putils_typeof(attr.ptr)>(table, attr.name);
            if (*ptr == attr.ptr)
                ++ret;
        });
        return ret;
    };
    static_assert(test() == 3);
}

TEST(ReflectionTest, ForEachAttributeReferences) {
    const auto test = []() consteval {
        const Reflectible obj;
        const auto table = putils::make_table(
            "iParent", &obj.iParent,
            "i", &obj.i,
            "s", &obj.s
        );
        int ret = 0;
        putils::reflection::for_each_attribute(obj, [&](const auto & attr) {
            if (*putils::get_value<putils_typeof(&attr.member)>(table, attr.name) == &attr.member)
                ++ret;
        });
        return ret;
    };
    static_assert(test() == 3);
    SUCCEED();
}

/*
 * Methods
 */

TEST(ReflectionTest, HasMethodsTrue) {
    static_assert(putils::reflection::has_methods<Reflectible>());
    SUCCEED();
}

TEST(ReflectionTest, HasMethodsFalse) {
    static_assert(!putils::reflection::has_methods<int>());
    SUCCEED();
}

TEST(ReflectionTest, MethodsSize) {
    {
        constexpr auto & methods = putils::reflection::get_methods<Parent>();
        constexpr auto size = std::tuple_size<putils_typeof(methods)>();
        static_assert(size == 1);
    }
    {
        constexpr auto & methods = putils::reflection::get_methods<Reflectible>();
        constexpr auto size = std::tuple_size<putils_typeof(methods)>();
        static_assert(size == 2);
    }
    SUCCEED();
}

TEST(ReflectionTest, GetMethods) {
    constexpr auto methods = putils::reflection::get_methods<Reflectible>();
    static_assert(std::get<0>(methods).name == std::string_view("f"));
    static_assert(std::get<0>(methods).ptr == &Reflectible::f);
    static_assert(std::get<1>(methods).name == std::string_view("fParent"));
    static_assert(std::get<1>(methods).ptr == &Parent::fParent);
}

TEST(ReflectionTest, ForEachMethodPointers) {
    constexpr auto test = []() consteval {
        constexpr auto table = putils::make_table(
            "fParent", &Parent::fParent,
            "f", &Reflectible::f
        );
        int ret = 0;
        putils::reflection::for_each_method<Reflectible>([&](const auto & attr) {
            const auto ptr = putils::get_value<putils_typeof(attr.ptr)>(table, attr.name);
            if (*ptr == attr.ptr)
                ++ret;
        });
        return ret;
    };
    static_assert(test() == 2);
}

TEST(ReflectionTest, ForEachMethodReferences) {
    auto table = putils::make_table(
        "fParent", (double)0,
        "f", (double)0
    );
    const Reflectible obj;
    putils::reflection::for_each_method(obj, [&](const auto & attr) {
        *putils::get_value<double>(table, attr.name) = attr.method(42);
    });
    EXPECT_EQ(std::get<0>(table).second, 42 + 84);
    EXPECT_EQ(std::get<1>(table).second, 42 + 42);
    SUCCEED();
}

/*
 * Parents
 */

TEST(ReflectionTest, HasParentsTrue) {
    static_assert(putils::reflection::has_parents<Reflectible>());
    SUCCEED();
}

TEST(ReflectionTest, HasParentsFalse) {
    static_assert(!putils::reflection::has_parents<Parent>());
    SUCCEED();
}

TEST(ReflectionTest, ParentsSize) {
    {
        constexpr auto & parents = putils::reflection::get_parents<Parent>();
        constexpr auto size = std::tuple_size<putils_typeof(parents)>();
        static_assert(size == 0);
    }
    {
        constexpr auto & parents = putils::reflection::get_parents<Reflectible>();
        constexpr auto size = std::tuple_size<putils_typeof(parents)>();
        static_assert(size == 1);
    }
    SUCCEED();
}

TEST(ReflectionTest, GetParents) {
    constexpr auto parents = putils::reflection::get_parents<Reflectible>();
    static_assert(std::is_same_v<putils_wrapped_type(std::get<0>(parents).type), Parent>);
}

TEST(ReflectionTest, ForEachParent) {
    putils::reflection::for_each_parent<Reflectible>([](const auto & parent) {
        static_assert(std::is_same_v<putils_wrapped_type(parent.type), Parent>);
    });
}

/*
 * Used types
 */

TEST(ReflectionTest, HasUsedTypesTrue) {
    static_assert(putils::reflection::has_used_types<Reflectible>());
    SUCCEED();
}

TEST(ReflectionTest, HasUsedTypesFalse) {
    static_assert(!putils::reflection::has_used_types<int>());
    SUCCEED();
}

TEST(ReflectionTest, UsedTypesSize) {
    {
        constexpr auto & used_types = putils::reflection::get_used_types<Parent>();
        constexpr auto size = std::tuple_size<putils_typeof(used_types)>();
        static_assert(size == 1);
    }
    {
        constexpr auto & used_types = putils::reflection::get_used_types<Reflectible>();
        constexpr auto size = std::tuple_size<putils_typeof(used_types)>();
        static_assert(size == 2);
    }
    SUCCEED();
}

TEST(ReflectionTest, GetUsedTypes) {
    {
        constexpr auto types = putils::reflection::get_used_types<Parent>();
        static_assert(std::is_same_v<putils_wrapped_type(std::get<0>(types).type), int>);
    }
    {
        constexpr auto types = putils::reflection::get_used_types<Reflectible>();
        static_assert(std::is_same_v<putils_wrapped_type(std::get<0>(types).type), const char *>);
        static_assert(std::is_same_v<putils_wrapped_type(std::get<1>(types).type), int>);
    }
}

TEST(ReflectionTest, ForEachUsedType) {
    constexpr auto parentTest = []() consteval {
        auto table = putils::make_table(
            putils::meta::type<int>(), false
        );

        putils::reflection::for_each_used_type<Parent>([&](const auto & type) {
            *putils::get_value<bool>(table, type.type) = true;
        });
        return std::get<0>(table).second;
    };
    static_assert(parentTest());

    constexpr auto reflectibleTest = []() consteval {
        auto table = putils::make_table(
            putils::meta::type<int>(), false,
            putils::meta::type<const char *>(), false
        );

        putils::reflection::for_each_used_type<Reflectible>([&](const auto & type) {
            *putils::get_value<bool>(table, type.type) = true;
        });
        return std::get<0>(table).second && std::get<1>(table).second;
    };
    static_assert(reflectibleTest());
}
