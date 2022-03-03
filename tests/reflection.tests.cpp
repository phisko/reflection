#include <gtest/gtest.h>
#include <tuple>
#include "reflection.hpp"

struct Parent {
    int iParent = 84;
    const int ciParent = -84;
    int fParent(double d) noexcept { return (int)d + iParent; }
    constexpr int cfParent(double d) const noexcept { return (int)d - ciParent; }
};

#define refltype Parent
putils_reflection_info{
    putils_reflection_class_name;
    putils_reflection_attributes(
        putils_reflection_attribute(iParent, putils_reflection_metadata("metaKey", "metaValue")),
        putils_reflection_attribute(ciParent)
    );
    putils_reflection_methods(
        putils_reflection_attribute(fParent, putils_reflection_metadata("metaKey", "metaValue")),
        putils_reflection_attribute(cfParent)
    );
    putils_reflection_used_types(
        putils_reflection_type(int)
    );
};
#undef refltype

struct Reflectible : Parent {
    int i = 42;
    const int ci = -42;
    const char * s = "foo";
    const char * const cs = "constfoo";

    int f(double d) noexcept { return (int)d + i; }
    constexpr int cf(double d) const noexcept { return (int)d + ci; }
};

#define refltype Reflectible
putils_reflection_info{
    putils_reflection_class_name;
    putils_reflection_attributes(
        putils_reflection_attribute(i),
        putils_reflection_attribute(ci),
        putils_reflection_attribute(s),
        putils_reflection_attribute(cs)
    );
    putils_reflection_methods(
        putils_reflection_attribute(f),
        putils_reflection_attribute(cf)
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

TEST(reflection, has_class_name_true) {
    static_assert(putils::reflection::has_class_name<Reflectible>());
    SUCCEED();
}

TEST(reflection, has_class_name_false) {
    static_assert(!putils::reflection::has_class_name<int>());
    SUCCEED();
}

TEST(reflection, get_class_name) {
    static_assert(putils::reflection::get_class_name<Reflectible>() == std::string_view(putils_nameof(Reflectible)));
    static_assert(putils::reflection::get_class_name<Parent>() == std::string_view(putils_nameof(Parent)));
}

/*
 * Attributes
 */

TEST(reflection, has_attributes_true) {
    static_assert(putils::reflection::has_attributes<Reflectible>());
    SUCCEED();
}

TEST(reflection, has_attributes_false) {
    static_assert(!putils::reflection::has_attributes<int>());
    SUCCEED();
}

TEST(reflection, attributes_size) {
    {
        constexpr auto & attributes = putils::reflection::get_attributes<Parent>();
        constexpr auto size = std::tuple_size<putils_typeof(attributes)>();
        static_assert(size == 2);
    }
    {
        constexpr auto & attributes = putils::reflection::get_attributes<Reflectible>();
        constexpr auto size = std::tuple_size<putils_typeof(attributes)>();
        static_assert(size == 6);
    }
    SUCCEED();
}

TEST(reflection, get_attributes) {
    constexpr auto attributes = putils::reflection::get_attributes<Reflectible>();

    static_assert(std::get<0>(attributes).name == std::string_view("i"));
    static_assert(std::get<0>(attributes).ptr == &Reflectible::i);

    static_assert(std::get<1>(attributes).name == std::string_view("ci"));
    static_assert(std::get<1>(attributes).ptr == &Reflectible::ci);

    static_assert(std::get<2>(attributes).name == std::string_view("s"));
    static_assert(std::get<2>(attributes).ptr == &Reflectible::s);

    static_assert(std::get<3>(attributes).name == std::string_view("cs"));
    static_assert(std::get<3>(attributes).ptr == &Reflectible::cs);

    static_assert(std::get<4>(attributes).name == std::string_view("iParent"));
    static_assert(std::get<4>(attributes).ptr == &Parent::iParent);

    static_assert(std::get<5>(attributes).name == std::string_view("ciParent"));
    static_assert(std::get<5>(attributes).ptr == &Parent::ciParent);
}

TEST(reflection, for_each_attribute_pointers) {
    constexpr auto test = []() consteval {
        constexpr auto table = putils::make_table(
            "iParent", &Parent::iParent,
            "ciParent", &Parent::ciParent,
            "i", &Reflectible::i,
            "ci", &Reflectible::ci,
            "s", &Reflectible::s,
            "cs", &Reflectible::cs
        );
        int ret = 0;
        putils::reflection::for_each_attribute<Reflectible>([&](const auto & attr) {
            const auto ptr = putils::get_value<putils_typeof(attr.ptr)>(table, attr.name);
            if (*ptr == attr.ptr)
                ++ret;
        });
        return ret;
    };
    static_assert(test() == 6);
}

TEST(reflection, for_each_attribute_references) {
    const auto test = []() consteval {
        const Reflectible obj;
        const auto table = putils::make_table(
            "iParent", &obj.iParent,
            "ciParent", &obj.ciParent,
            "i", &obj.i,
            "ci", &obj.ci,
            "s", &obj.s,
            "cs", &obj.cs
        );
        int ret = 0;
        putils::reflection::for_each_attribute(obj, [&](const auto & attr) {
            if (*putils::get_value<putils_typeof(&attr.member)>(table, attr.name) == &attr.member)
                ++ret;
        });
        return ret;
    };
    static_assert(test() == 6);
    SUCCEED();
}

TEST(reflection, get_attribute_pointer) {
    using Wrapped = putils::detail::MemberTypeWrapper<decltype(&Reflectible::iParent)>::type;
    static_assert(std::is_same<Wrapped, int>());

    constexpr auto iParentAttr = putils::reflection::get_attribute<int, Reflectible>("iParent");
    static_assert(*iParentAttr == &Reflectible::iParent);

    // TODO: I'd prefer not have to specify the second const here
    constexpr auto ciParentAttr = putils::reflection::get_attribute<const int, Reflectible>("ciParent");
    static_assert(*ciParentAttr == &Reflectible::ciParent);

    constexpr auto iAttr = putils::reflection::get_attribute<int, Reflectible>("i");
    static_assert(*iAttr == &Reflectible::i);

    // TODO: I'd prefer not have to specify the second const here
    constexpr auto ciAttr = putils::reflection::get_attribute<const int, Reflectible>("ci");
    static_assert(*ciAttr == &Reflectible::ci);

    constexpr auto sAttr = putils::reflection::get_attribute<const char *, Reflectible>("s");
    static_assert(*sAttr == &Reflectible::s);

    // TODO: I'd prefer not have to specify the second const here
    constexpr auto csAttr = putils::reflection::get_attribute<const char * const, Reflectible>("cs");
    static_assert(*csAttr == &Reflectible::cs);
}

TEST(reflection, get_attribute_pointer_missing) {
    constexpr auto attr = putils::reflection::get_attribute<int, Reflectible>("foo");
    static_assert(attr == std::nullopt);
}

TEST(reflection, get_attribute_reference) {
    static constexpr Reflectible obj;

    constexpr auto iParentAttr = putils::reflection::get_attribute<int>(obj, "iParent");
    static_assert(iParentAttr == &obj.iParent);

    constexpr auto ciParentAttr = putils::reflection::get_attribute<const int>(obj, "ciParent");
    static_assert(ciParentAttr == &obj.ciParent);

    constexpr auto iAttr = putils::reflection::get_attribute<int>(obj, "i");
    static_assert(iAttr == &obj.i);

    constexpr auto ciAttr = putils::reflection::get_attribute<const int>(obj, "ci");
    static_assert(ciAttr == &obj.ci);

    constexpr auto sAttr = putils::reflection::get_attribute<const char *>(obj, "s");
    static_assert(sAttr == &obj.s);

    // TODO: I'd prefer not have to specify the second const here
    constexpr auto csAttr = putils::reflection::get_attribute<const char * const>(obj, "cs");
    static_assert(csAttr == &obj.cs);

    Reflectible obj2;
    const auto attr = putils::reflection::get_attribute<int>(obj2, "iParent");
    *attr = -1;
    EXPECT_EQ(obj2.iParent, -1);

    // TODO: I'd prefer not have to specify the const here
    const auto cattr = putils::reflection::get_attribute<const int>(obj2, "ci");
    EXPECT_EQ(cattr, &obj2.ci);
}

TEST(reflection, get_attribute_reference_missing) {
    constexpr Reflectible obj;
    constexpr auto attr = putils::reflection::get_attribute<int>(obj, "foo");
    static_assert(attr == nullptr);
}

/*
 * Methods
 */

TEST(reflection, has_methods_true) {
    static_assert(putils::reflection::has_methods<Reflectible>());
    SUCCEED();
}

TEST(reflection, has_methods_false) {
    static_assert(!putils::reflection::has_methods<int>());
    SUCCEED();
}

TEST(reflection, methods_size) {
    {
        constexpr auto & methods = putils::reflection::get_methods<Parent>();
        constexpr auto size = std::tuple_size<putils_typeof(methods)>();
        static_assert(size == 2);
    }
    {
        constexpr auto & methods = putils::reflection::get_methods<Reflectible>();
        constexpr auto size = std::tuple_size<putils_typeof(methods)>();
        static_assert(size == 4);
    }
    SUCCEED();
}

TEST(reflection, get_methods) {
    constexpr auto methods = putils::reflection::get_methods<Reflectible>();

    static_assert(std::get<0>(methods).name == std::string_view("f"));
    static_assert(std::get<0>(methods).ptr == &Reflectible::f);

    static_assert(std::get<1>(methods).name == std::string_view("cf"));
    static_assert(std::get<1>(methods).ptr == &Reflectible::cf);

    static_assert(std::get<2>(methods).name == std::string_view("fParent"));
    static_assert(std::get<2>(methods).ptr == &Parent::fParent);

    static_assert(std::get<3>(methods).name == std::string_view("cfParent"));
    static_assert(std::get<3>(methods).ptr == &Parent::cfParent);
}

TEST(reflection, for_each_method_pointers) {
    constexpr auto test = []() consteval {
        constexpr auto table = putils::make_table(
            "fParent", &Parent::fParent,
            "cfParent", &Parent::cfParent,
            "f", &Reflectible::f,
            "cf", &Reflectible::cf
        );
        int ret = 0;
        putils::reflection::for_each_method<Reflectible>([&](const auto & attr) {
            const auto ptr = putils::get_value<putils_typeof(attr.ptr)>(table, attr.name);
            if (*ptr == attr.ptr)
                ++ret;
        });
        return ret;
    };
    static_assert(test() == 4);
}

TEST(reflection, for_each_method_references) {
    auto table = putils::make_table(
        "fParent", (double)0,
        "cfParent", (double)0,
        "f", (double)0,
        "cf", (double)0
    );
    Reflectible obj;
    putils::reflection::for_each_method(obj, [&](const auto & attr) {
        *putils::get_value<double>(table, attr.name) = attr.method(0);
    });
    EXPECT_EQ(std::get<0>(table).second, obj.fParent(0));
    EXPECT_EQ(std::get<1>(table).second, obj.cfParent(0));
    EXPECT_EQ(std::get<2>(table).second, obj.f(0));
    EXPECT_EQ(std::get<3>(table).second, obj.cf(0));
    SUCCEED();
}

TEST(reflection, get_method_constexpr_pointer) {
    constexpr auto fParentMethod = putils::reflection::get_method<int(double), Parent>("fParent");
    static_assert(fParentMethod != std::nullopt);
    static_assert(*fParentMethod == &Reflectible::fParent);

    // TODO: I'd prefer not have to cast here, and have get_method return the const qualified member pointer
#ifdef _MSC_VER // TODO: I wish this compiled on gcc
    constexpr auto cfParentMethod = putils::reflection::get_method<int(double), Reflectible>("cfParent");
    static_assert(cfParentMethod != std::nullopt);
    static_assert(*cfParentMethod == (int (Reflectible::*)(double))&Reflectible::cfParent);
#endif

    constexpr auto fMethod = putils::reflection::get_method<int(double), Reflectible>("f");
    static_assert(fMethod != std::nullopt);
    static_assert(*fMethod == &Reflectible::f);

    // TODO: I'd prefer not have to cast here, and have get_method return the const qualified member pointer
#ifdef _MSC_VER // TODO: I wish this compiled on gcc
    constexpr auto cfMethod = putils::reflection::get_method<int(double), Reflectible>("cf");
    static_assert(cfMethod != std::nullopt);
    static_assert(*cfMethod == (int (Reflectible::*)(double))&Reflectible::cf);
#endif
}

TEST(reflection, get_method_constexpr_pointer_full_signature) {
    // TODO: I'd prefer not have to specify the noexcept in all these
    // TODO: I'd prefer being able to use Reflectible::* instead of Parent::*
    constexpr auto fParentMethod = putils::reflection::get_method<int (Parent::*)(double) noexcept>("fParent");
    static_assert(fParentMethod != std::nullopt);
    static_assert(*fParentMethod == &Reflectible::fParent);

    constexpr auto cfParentMethod = putils::reflection::get_method<int (Parent::*)(double) const noexcept>("cfParent");
    static_assert(cfParentMethod != std::nullopt);
    static_assert(*cfParentMethod == &Reflectible::cfParent);

    constexpr auto fMethod = putils::reflection::get_method<int (Reflectible::*)(double) noexcept>("f");
    static_assert(fMethod != std::nullopt);
    static_assert(*fMethod == &Reflectible::f);

    constexpr auto cfMethod = putils::reflection::get_method<int (Reflectible::*)(double) const noexcept>("cf");
    static_assert(cfMethod != std::nullopt);
    static_assert(*cfMethod == &Reflectible::cf);
}

TEST(reflection, get_method_constexpr_pointer_missing) {
    constexpr auto method = putils::reflection::get_method<int(double), Reflectible>("foo");
    static_assert(method == std::nullopt);
}

TEST(reflection, get_method_pointer) {
    // TODO: I'd prefer not have to cast here, and have get_method return the const/noexcept qualified member pointer

    const auto fParentMethod = putils::reflection::get_method<int(double), Reflectible>("fParent");
    EXPECT_EQ(*fParentMethod, (int (Reflectible::*)(double))&Reflectible::fParent);

    const auto cfParentMethod = putils::reflection::get_method<int(double), Reflectible>("cfParent");
    EXPECT_EQ(*cfParentMethod, (int (Reflectible::*)(double))&Reflectible::cfParent);

    const auto fMethod = putils::reflection::get_method<int(double), Reflectible>("f");
    EXPECT_EQ(*fMethod, (int (Reflectible::*)(double))&Reflectible::f);

    const auto cfMethod = putils::reflection::get_method<int(double), Reflectible>("cf");
    EXPECT_EQ(*cfMethod, (int (Reflectible::*)(double))&Reflectible::cf);
}

TEST(reflection, get_method_pointer_missing) {
    const auto method = putils::reflection::get_method<int(), Reflectible>("foo");
    EXPECT_EQ(method, std::nullopt);
}

TEST(reflection, get_method_constexpr_reference) {
    static constexpr Reflectible obj;

    static constexpr auto cfParentMethod = putils::reflection::get_method<int (Parent::*)(double) const noexcept>(obj, "cfParent");
    static_assert(cfParentMethod != std::nullopt);
    static_assert((*cfParentMethod)(42) == obj.cfParent(42));

    static constexpr auto cfMethod = putils::reflection::get_method<int (Reflectible::*)(double) const noexcept>(obj, "cf");
    static_assert(cfMethod != std::nullopt);
    static_assert((*cfMethod)(42) == obj.cf(42));
}

TEST(reflection, get_method_reference) {
    Reflectible obj;

    const auto fParentMethod = putils::reflection::get_method<int(double)>(obj, "fParent");
    EXPECT_EQ((*fParentMethod)(42), obj.fParent(42));

    const auto cfParentMethod = putils::reflection::get_method<int(double)>(obj, "cfParent");
    EXPECT_EQ((*cfParentMethod)(42), obj.cfParent(42));

    const auto fMethod = putils::reflection::get_method<int(double)>(obj, "f");
    EXPECT_EQ((*fMethod)(42), obj.f(42));

    const auto cfMethod = putils::reflection::get_method<int(double)>(obj, "cf");
    EXPECT_EQ((*cfMethod)(42), obj.cf(42));
}

TEST(reflection, get_method_constexpr_reference_missing) {
    static constexpr Reflectible obj;
    static constexpr auto method = putils::reflection::get_method<int(double)>(obj, "foo");
    static_assert(method == std::nullopt);
}

TEST(reflection, get_method_reference_missing) {
    const Reflectible obj;
    const auto method = putils::reflection::get_method<int(double)>(obj, "foo");
    EXPECT_EQ(method, std::nullopt);
}

/*
 * Parents
 */

TEST(reflection, has_parents_true) {
    static_assert(putils::reflection::has_parents<Reflectible>());
    SUCCEED();
}

TEST(reflection, has_parents_false) {
    static_assert(!putils::reflection::has_parents<Parent>());
    SUCCEED();
}

TEST(reflection, parents_size) {
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

TEST(reflection, get_parents) {
    constexpr auto parents = putils::reflection::get_parents<Reflectible>();
    static_assert(std::is_same_v<putils_wrapped_type(std::get<0>(parents).type), Parent>);
}

TEST(reflection, for_each_parent) {
    putils::reflection::for_each_parent<Reflectible>([](const auto & parent) {
        static_assert(std::is_same_v<putils_wrapped_type(parent.type), Parent>);
    });
}

/*
 * Used types
 */

TEST(reflection, has_used_types_true) {
    static_assert(putils::reflection::has_used_types<Reflectible>());
    SUCCEED();
}

TEST(reflection, has_used_types_false) {
    static_assert(!putils::reflection::has_used_types<int>());
    SUCCEED();
}

TEST(reflection, used_types_size) {
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

TEST(reflection, get_used_types) {
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

TEST(reflection, for_each_used_type) {
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

TEST(reflection, has_attribute_metadata) {
    static_assert(putils::reflection::has_attribute_metadata<Reflectible>("iParent", "metaKey"));
    static_assert(!putils::reflection::has_attribute_metadata<Reflectible>("iParent", "badKey"));
    static_assert(!putils::reflection::has_attribute_metadata<Reflectible>("i", "metaKey"));
}

TEST(reflection, get_attribute_metadata) {
    static_assert(*putils::reflection::get_attribute_metadata<const char *, Reflectible>("iParent", "metaKey") == std::string_view("metaValue"));
    static_assert(putils::reflection::get_attribute_metadata<const char *, Reflectible>("iParent", "badKey") == nullptr);
    static_assert(putils::reflection::get_attribute_metadata<const char *, Reflectible>("i", "metaKey") == nullptr);
}

TEST(reflection, has_method_metadata) {
    static_assert(putils::reflection::has_method_metadata<Reflectible>("fParent", "metaKey"));
    static_assert(!putils::reflection::has_method_metadata<Reflectible>("fParent", "badKey"));
    static_assert(!putils::reflection::has_method_metadata<Reflectible>("f", "metaKey"));
}

TEST(reflection, get_method_metadata) {
    static_assert(*putils::reflection::get_method_metadata<const char *, Reflectible>("fParent", "metaKey") == std::string_view("metaValue"));
    static_assert(putils::reflection::get_method_metadata<const char *, Reflectible>("fParent", "badKey") == nullptr);
    static_assert(putils::reflection::get_method_metadata<const char *, Reflectible>("f", "metaKey") == nullptr);
}
