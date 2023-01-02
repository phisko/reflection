// stl
#include <tuple>

// gtest
#include <gtest/gtest.h>

// reflection
#include "putils/reflection.hpp"

namespace {
	struct parent {
		int iparent = 84;
		const int ciparent = -84;

		int fparent(double d) noexcept { return (int)d + iparent; }
		constexpr int cfparent(double d) const noexcept { return (int)d - ciparent; }
	};
}

#define refltype parent
putils_reflection_info {
	putils_reflection_class_name;
	putils_reflection_attributes(
		putils_reflection_attribute(iparent, putils_reflection_metadata("meta_key", "meta_value")),
		putils_reflection_attribute(ciparent)
	);
	putils_reflection_methods(
		putils_reflection_attribute(fparent, putils_reflection_metadata("meta_key", "meta_value")),
		putils_reflection_attribute(cfparent)
	);
	putils_reflection_used_types(
		putils_reflection_type(int)
	);
};
#undef refltype

namespace {
	struct reflectible : parent {
		int i = 42;
		const int ci = -42;
		const char * s = "foo";
		const char * const cs = "constfoo";

		int f(double d) noexcept { return (int)d + i; }
		constexpr int cf(double d) const noexcept { return (int)d + ci; }
	};
}

#define refltype reflectible
putils_reflection_info {
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
		putils_reflection_type(parent)
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
	static_assert(putils::reflection::has_class_name<reflectible>());
	SUCCEED();
}

TEST(reflection, has_class_name_false) {
	static_assert(!putils::reflection::has_class_name<int>());
	SUCCEED();
}

TEST(reflection, get_class_name) {
	static_assert(putils::reflection::get_class_name<reflectible>() == std::string_view(putils_nameof(reflectible)));
	static_assert(putils::reflection::get_class_name<parent>() == std::string_view(putils_nameof(parent)));
}

/*
 * Attributes
 */

TEST(reflection, has_attributes_true) {
	static_assert(putils::reflection::has_attributes<reflectible>());
	SUCCEED();
}

TEST(reflection, has_attributes_false) {
	static_assert(!putils::reflection::has_attributes<int>());
	SUCCEED();
}

TEST(reflection, attributes_size) {
	{
		constexpr auto & attributes = putils::reflection::get_attributes<parent>();
		constexpr auto size = std::tuple_size<putils_typeof(attributes)>();
		static_assert(size == 2);
	}
	{
		constexpr auto & attributes = putils::reflection::get_attributes<reflectible>();
		constexpr auto size = std::tuple_size<putils_typeof(attributes)>();
		static_assert(size == 6);
	}
	SUCCEED();
}

TEST(reflection, get_attributes) {
	constexpr auto attributes = putils::reflection::get_attributes<reflectible>();

	static_assert(std::get<0>(attributes).name == std::string_view("i"));
	static_assert(std::get<0>(attributes).ptr == &reflectible::i);

	static_assert(std::get<1>(attributes).name == std::string_view("ci"));
	static_assert(std::get<1>(attributes).ptr == &reflectible::ci);

	static_assert(std::get<2>(attributes).name == std::string_view("s"));
	static_assert(std::get<2>(attributes).ptr == &reflectible::s);

	static_assert(std::get<3>(attributes).name == std::string_view("cs"));
	static_assert(std::get<3>(attributes).ptr == &reflectible::cs);

	static_assert(std::get<4>(attributes).name == std::string_view("iparent"));
	static_assert(std::get<4>(attributes).ptr == &parent::iparent);

	static_assert(std::get<5>(attributes).name == std::string_view("ciparent"));
	static_assert(std::get<5>(attributes).ptr == &parent::ciparent);
}

TEST(reflection, for_each_attribute_pointers) {
	constexpr auto test = []() consteval {
		constexpr auto table = putils::make_table(
			"iparent", &parent::iparent,
			"ciparent", &parent::ciparent,
			"i", &reflectible::i,
			"ci", &reflectible::ci,
			"s", &reflectible::s,
			"cs", &reflectible::cs
		);
		int ret = 0;
		putils::reflection::for_each_attribute<reflectible>([&](const auto & attr) {
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
		const reflectible obj;
		const auto table = putils::make_table(
			"iparent", &obj.iparent,
			"ciparent", &obj.ciparent,
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

TEST(reflection, has_attribute) {
	static_assert(putils::reflection::has_attribute<parent>("iparent"));
	static_assert(putils::reflection::has_attribute<reflectible>("iparent"));
	static_assert(!putils::reflection::has_attribute<reflectible>("unknown"));
}

TEST(reflection, get_attribute_pointer) {
	using wrapped = putils::detail::member_type_wrapper<decltype(&reflectible::iparent)>::type;
	static_assert(std::is_same<wrapped, int>());

	constexpr auto iparent_attr = putils::reflection::get_attribute<int, reflectible>("iparent");
	static_assert(*iparent_attr == &reflectible::iparent);

	// TODO: I'd prefer not have to specify the second const here
	constexpr auto ciparent_attr = putils::reflection::get_attribute<const int, reflectible>("ciparent");
	static_assert(*ciparent_attr == &reflectible::ciparent);

	constexpr auto i_attr = putils::reflection::get_attribute<int, reflectible>("i");
	static_assert(*i_attr == &reflectible::i);

	// TODO: I'd prefer not have to specify the second const here
	constexpr auto ci_attr = putils::reflection::get_attribute<const int, reflectible>("ci");
	static_assert(*ci_attr == &reflectible::ci);

	constexpr auto s_attr = putils::reflection::get_attribute<const char *, reflectible>("s");
	static_assert(*s_attr == &reflectible::s);

	// TODO: I'd prefer not have to specify the second const here
	constexpr auto cs_attr = putils::reflection::get_attribute<const char * const, reflectible>("cs");
	static_assert(*cs_attr == &reflectible::cs);
}

TEST(reflection, get_attribute_pointer_missing) {
	constexpr auto attr = putils::reflection::get_attribute<int, reflectible>("foo");
	static_assert(attr == std::nullopt);
}

TEST(reflection, get_attribute_reference) {
	static constexpr reflectible obj;

	constexpr auto iparent_attr = putils::reflection::get_attribute<int>(obj, "iparent");
	static_assert(iparent_attr == &obj.iparent);

	constexpr auto ciparent_attr = putils::reflection::get_attribute<const int>(obj, "ciparent");
	static_assert(ciparent_attr == &obj.ciparent);

	constexpr auto i_attr = putils::reflection::get_attribute<int>(obj, "i");
	static_assert(i_attr == &obj.i);

	constexpr auto ci_attr = putils::reflection::get_attribute<const int>(obj, "ci");
	static_assert(ci_attr == &obj.ci);

	constexpr auto s_attr = putils::reflection::get_attribute<const char *>(obj, "s");
	static_assert(s_attr == &obj.s);

	// TODO: I'd prefer not have to specify the second const here
	constexpr auto cs_attr = putils::reflection::get_attribute<const char * const>(obj, "cs");
	static_assert(cs_attr == &obj.cs);

	reflectible obj2;
	const auto attr = putils::reflection::get_attribute<int>(obj2, "iparent");
	*attr = -1;
	EXPECT_EQ(obj2.iparent, -1);

	// TODO: I'd prefer not have to specify the const here
	const auto cattr = putils::reflection::get_attribute<const int>(obj2, "ci");
	EXPECT_EQ(cattr, &obj2.ci);
}

TEST(reflection, get_attribute_reference_missing) {
	constexpr reflectible obj;
	constexpr auto attr = putils::reflection::get_attribute<int>(obj, "foo");
	static_assert(attr == nullptr);
}

/*
 * Methods
 */

TEST(reflection, has_methods_true) {
	static_assert(putils::reflection::has_methods<reflectible>());
	SUCCEED();
}

TEST(reflection, has_methods_false) {
	static_assert(!putils::reflection::has_methods<int>());
	SUCCEED();
}

TEST(reflection, methods_size) {
	{
		constexpr auto & methods = putils::reflection::get_methods<parent>();
		constexpr auto size = std::tuple_size<putils_typeof(methods)>();
		static_assert(size == 2);
	}
	{
		constexpr auto & methods = putils::reflection::get_methods<reflectible>();
		constexpr auto size = std::tuple_size<putils_typeof(methods)>();
		static_assert(size == 4);
	}
	SUCCEED();
}

TEST(reflection, get_methods) {
	constexpr auto methods = putils::reflection::get_methods<reflectible>();

	static_assert(std::get<0>(methods).name == std::string_view("f"));
	static_assert(std::get<0>(methods).ptr == &reflectible::f);

	static_assert(std::get<1>(methods).name == std::string_view("cf"));
	static_assert(std::get<1>(methods).ptr == &reflectible::cf);

	static_assert(std::get<2>(methods).name == std::string_view("fparent"));
	static_assert(std::get<2>(methods).ptr == &parent::fparent);

	static_assert(std::get<3>(methods).name == std::string_view("cfparent"));
	static_assert(std::get<3>(methods).ptr == &parent::cfparent);
}

TEST(reflection, for_each_method_pointers) {
	constexpr auto test = []() consteval {
		constexpr auto table = putils::make_table(
			"fparent", &parent::fparent,
			"cfparent", &parent::cfparent,
			"f", &reflectible::f,
			"cf", &reflectible::cf
		);
		int ret = 0;
		putils::reflection::for_each_method<reflectible>([&](const auto & attr) {
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
		"fparent", (double)0,
		"cfparent", (double)0,
		"f", (double)0,
		"cf", (double)0
	);
	reflectible obj;
	putils::reflection::for_each_method(obj, [&](const auto & attr) {
		*putils::get_value<double>(table, attr.name) = attr.method(0);
	});
	EXPECT_EQ(std::get<0>(table).second, obj.fparent(0));
	EXPECT_EQ(std::get<1>(table).second, obj.cfparent(0));
	EXPECT_EQ(std::get<2>(table).second, obj.f(0));
	EXPECT_EQ(std::get<3>(table).second, obj.cf(0));
	SUCCEED();
}

TEST(reflection, has_method) {
	static_assert(putils::reflection::has_method<parent>("fparent"));
	static_assert(putils::reflection::has_method<reflectible>("fparent"));
	static_assert(!putils::reflection::has_method<reflectible>("unknown"));
}

TEST(reflection, get_method_constexpr_pointer) {
	constexpr auto fparent_method = putils::reflection::get_method<int(double), parent>("fparent");
	static_assert(fparent_method != std::nullopt);
	static_assert(*fparent_method == &reflectible::fparent);

	// TODO: I'd prefer not have to cast here, and have get_method return the const qualified member pointer
#ifdef _MSC_VER // TODO: I wish this compiled on gcc
	constexpr auto cfparent_method = putils::reflection::get_method<int(double), reflectible>("cfparent");
	static_assert(cfparent_method != std::nullopt);
	static_assert(*cfparent_method == (int(reflectible::*)(double)) & reflectible::cfparent);
#endif

	constexpr auto f_method = putils::reflection::get_method<int(double), reflectible>("f");
	static_assert(f_method != std::nullopt);
	static_assert(*f_method == &reflectible::f);

	// TODO: I'd prefer not have to cast here, and have get_method return the const qualified member pointer
#ifdef _MSC_VER // TODO: I wish this compiled on gcc
	constexpr auto cf_method = putils::reflection::get_method<int(double), reflectible>("cf");
	static_assert(cf_method != std::nullopt);
	static_assert(*cf_method == (int(reflectible::*)(double)) & reflectible::cf);
#endif
}

TEST(reflection, get_method_constexpr_pointer_full_signature) {
	// TODO: I'd prefer not have to specify the noexcept in all these
	// TODO: I'd prefer being able to use reflectible::* instead of parent::*
	constexpr auto fparent_method = putils::reflection::get_method<int (parent::*)(double) noexcept>("fparent");
	static_assert(fparent_method != std::nullopt);
	static_assert(*fparent_method == &reflectible::fparent);

	constexpr auto cfparent_method = putils::reflection::get_method<int (parent::*)(double) const noexcept>("cfparent");
	static_assert(cfparent_method != std::nullopt);
	static_assert(*cfparent_method == &reflectible::cfparent);

	constexpr auto f_method = putils::reflection::get_method<int (reflectible::*)(double) noexcept>("f");
	static_assert(f_method != std::nullopt);
	static_assert(*f_method == &reflectible::f);

	constexpr auto cf_method = putils::reflection::get_method<int (reflectible::*)(double) const noexcept>("cf");
	static_assert(cf_method != std::nullopt);
	static_assert(*cf_method == &reflectible::cf);
}

TEST(reflection, get_method_constexpr_pointer_missing) {
	constexpr auto method = putils::reflection::get_method<int(double), reflectible>("foo");
	static_assert(method == std::nullopt);
}

TEST(reflection, get_method_pointer) {
	// TODO: I'd prefer not have to cast here, and have get_method return the const/noexcept qualified member pointer

	const auto fparent_method = putils::reflection::get_method<int(double), reflectible>("fparent");
	EXPECT_EQ(*fparent_method, (int(reflectible::*)(double)) & reflectible::fparent);

	const auto cfparent_method = putils::reflection::get_method<int(double), reflectible>("cfparent");
	EXPECT_EQ(*cfparent_method, (int(reflectible::*)(double)) & reflectible::cfparent);

	const auto f_method = putils::reflection::get_method<int(double), reflectible>("f");
	EXPECT_EQ(*f_method, (int(reflectible::*)(double)) & reflectible::f);

	const auto cf_method = putils::reflection::get_method<int(double), reflectible>("cf");
	EXPECT_EQ(*cf_method, (int(reflectible::*)(double)) & reflectible::cf);
}

TEST(reflection, get_method_pointer_missing) {
	const auto method = putils::reflection::get_method<int(), reflectible>("foo");
	EXPECT_EQ(method, std::nullopt);
}

TEST(reflection, get_method_constexpr_reference) {
	static constexpr reflectible obj;

	static constexpr auto cfparent_method = putils::reflection::get_method<int (parent::*)(double) const noexcept>(obj, "cfparent");
	static_assert(cfparent_method != std::nullopt);
	static_assert((*cfparent_method)(42) == obj.cfparent(42));

	static constexpr auto cf_method = putils::reflection::get_method<int (reflectible::*)(double) const noexcept>(obj, "cf");
	static_assert(cf_method != std::nullopt);
	static_assert((*cf_method)(42) == obj.cf(42));
}

TEST(reflection, get_method_reference) {
	reflectible obj;

	const auto fparent_method = putils::reflection::get_method<int(double)>(obj, "fparent");
	EXPECT_EQ((*fparent_method)(42), obj.fparent(42));

	const auto cfparent_method = putils::reflection::get_method<int(double)>(obj, "cfparent");
	EXPECT_EQ((*cfparent_method)(42), obj.cfparent(42));

	const auto f_method = putils::reflection::get_method<int(double)>(obj, "f");
	EXPECT_EQ((*f_method)(42), obj.f(42));

	const auto cf_method = putils::reflection::get_method<int(double)>(obj, "cf");
	EXPECT_EQ((*cf_method)(42), obj.cf(42));
}

TEST(reflection, get_method_constexpr_reference_missing) {
	static constexpr reflectible obj;
	static constexpr auto method = putils::reflection::get_method<int(double)>(obj, "foo");
	static_assert(method == std::nullopt);
}

TEST(reflection, get_method_reference_missing) {
	const reflectible obj;
	const auto method = putils::reflection::get_method<int(double)>(obj, "foo");
	EXPECT_EQ(method, std::nullopt);
}

/*
 * parents
 */

TEST(reflection, has_parents_true) {
	static_assert(putils::reflection::has_parents<reflectible>());
	SUCCEED();
}

TEST(reflection, has_parents_false) {
	static_assert(!putils::reflection::has_parents<parent>());
	SUCCEED();
}

TEST(reflection, parents_size) {
	{
		constexpr auto & parents = putils::reflection::get_parents<parent>();
		constexpr auto size = std::tuple_size<putils_typeof(parents)>();
		static_assert(size == 0);
	}
	{
		constexpr auto & parents = putils::reflection::get_parents<reflectible>();
		constexpr auto size = std::tuple_size<putils_typeof(parents)>();
		static_assert(size == 1);
	}
	SUCCEED();
}

TEST(reflection, get_parents) {
	constexpr auto parents = putils::reflection::get_parents<reflectible>();
	static_assert(std::is_same_v<putils_wrapped_type(std::get<0>(parents).type), parent>);
}

TEST(reflection, for_each_parent) {
	putils::reflection::for_each_parent<reflectible>([](const auto & p) {
		static_assert(std::is_same_v<putils_wrapped_type(p.type), parent>);
	});
}

TEST(reflection, has_parent) {
	static_assert(putils::reflection::has_parent<reflectible, parent>());
	static_assert(!putils::reflection::has_parent<parent, parent>());
}

/*
 * Used types
 */

TEST(reflection, has_used_types_true) {
	static_assert(putils::reflection::has_used_types<reflectible>());
	SUCCEED();
}

TEST(reflection, has_used_types_false) {
	static_assert(!putils::reflection::has_used_types<int>());
	SUCCEED();
}

TEST(reflection, used_types_size) {
	{
		constexpr auto & used_types = putils::reflection::get_used_types<parent>();
		constexpr auto size = std::tuple_size<putils_typeof(used_types)>();
		static_assert(size == 1);
	}
	{
		constexpr auto & used_types = putils::reflection::get_used_types<reflectible>();
		constexpr auto size = std::tuple_size<putils_typeof(used_types)>();
		static_assert(size == 2);
	}
	SUCCEED();
}

TEST(reflection, get_used_types) {
	{
		constexpr auto types = putils::reflection::get_used_types<parent>();
		static_assert(std::is_same_v<putils_wrapped_type(std::get<0>(types).type), int>);
	}
	{
		constexpr auto types = putils::reflection::get_used_types<reflectible>();
		static_assert(std::is_same_v<putils_wrapped_type(std::get<0>(types).type), const char *>);
		static_assert(std::is_same_v<putils_wrapped_type(std::get<1>(types).type), int>);
	}
}

TEST(reflection, for_each_used_type) {
	constexpr auto parent_test = []() consteval {
		auto table = putils::make_table(
			putils::meta::type<int>(), false
		);

		putils::reflection::for_each_used_type<parent>([&](const auto & type) {
			*putils::get_value<bool>(table, type.type) = true;
		});
		return std::get<0>(table).second;
	};
	static_assert(parent_test());

	constexpr auto reflectible_test = []() consteval {
		auto table = putils::make_table(
			putils::meta::type<int>(), false,
			putils::meta::type<const char *>(), false
		);

		putils::reflection::for_each_used_type<reflectible>([&](const auto & type) {
			*putils::get_value<bool>(table, type.type) = true;
		});
		return std::get<0>(table).second && std::get<1>(table).second;
	};
	static_assert(reflectible_test());
}

TEST(reflection, has_used_type) {
	static_assert(putils::reflection::has_used_type<parent, int>());
	static_assert(putils::reflection::has_used_type<reflectible, int>());
	static_assert(!putils::reflection::has_used_type<reflectible, void *>());
}

/*
 * Metadata
 */

TEST(reflection, has_attribute_metadata) {
	static_assert(putils::reflection::has_attribute_metadata<reflectible>("iparent", "meta_key"));
	static_assert(!putils::reflection::has_attribute_metadata<reflectible>("iparent", "bad_key"));
	static_assert(!putils::reflection::has_attribute_metadata<reflectible>("i", "meta_key"));
}

TEST(reflection, get_attribute_metadata) {
	static_assert(*putils::reflection::get_attribute_metadata<const char *, reflectible>("iparent", "meta_key") == std::string_view("meta_value"));
	static_assert(putils::reflection::get_attribute_metadata<const char *, reflectible>("iparent", "bad_key") == nullptr);
	static_assert(putils::reflection::get_attribute_metadata<const char *, reflectible>("i", "meta_key") == nullptr);
}

TEST(reflection, has_method_metadata) {
	static_assert(putils::reflection::has_method_metadata<reflectible>("fparent", "meta_key"));
	static_assert(!putils::reflection::has_method_metadata<reflectible>("fparent", "bad_key"));
	static_assert(!putils::reflection::has_method_metadata<reflectible>("f", "meta_key"));
}

TEST(reflection, get_method_metadata) {
	static_assert(*putils::reflection::get_method_metadata<const char *, reflectible>("fparent", "meta_key") == std::string_view("meta_value"));
	static_assert(putils::reflection::get_method_metadata<const char *, reflectible>("fparent", "bad_key") == nullptr);
	static_assert(putils::reflection::get_method_metadata<const char *, reflectible>("f", "meta_key") == nullptr);
}
