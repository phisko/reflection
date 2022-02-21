#pragma once

#include <string_view>
#include <optional>
#include "meta/table.hpp"

namespace putils::reflection {
	template<typename T>
	struct type_info {
		// static constexpr auto class_name = const char *;
		// static constexpr auto attributes = std::tuple<attribute_info>;
		// static constexpr auto methods = std::tuple<attribute_info>;
		// static constexpr auto parents = std::tuple<used_type_info>;
		// static constexpr auto used_types = std::tuple<used_type_info>;
	};

	template<typename MemberPtr, typename MetadataTable>
	struct attribute_info {
		const char * name;
		const MemberPtr ptr;
		const MetadataTable metadata; // putils::table<Key, Value...>
	};

	template<typename Member, typename MetadataTable>
	struct object_attribute_info {
		const char * name;
		Member & member;
		const MetadataTable & metadata; // putils::table<Key, Value...>
	};

	template<typename T, typename MetadataTable>
	struct used_type_info {
		const putils::meta::type<T> type;
		const MetadataTable metadata; // putils::table<Key, Value...>
	};

	template<typename T>
	constexpr auto get_class_name() noexcept;

	// For each parent of T, get a `putils::type<Parent>` object
	template<typename T, typename Func>
	constexpr void for_each_parent(Func && func) noexcept;

	// For each type used by T, get a `putils::type<Type>` object
	template<typename T, typename Func>
	constexpr void for_each_used_type(Func && func) noexcept;

	// For each attribute in T, get its name and a member pointer to it
	template<typename T, typename Func>
	constexpr void for_each_attribute(Func && func) noexcept;

	// For each attribute in T, get its name and a reference to it in obj
	template<typename T, typename Func>
	constexpr void for_each_attribute(T && obj, Func && func) noexcept;

	// Try to find an attribute called "name" and get a member pointer to it
	template<typename Ret, typename T>
	constexpr std::optional<Ret T:: *> get_attribute(std::string_view name) noexcept;

	// Try to find an attribute called "name" and get a reference to it in obj
	template<typename Ret, typename T>
	constexpr auto get_attribute(T && obj, std::string_view name) noexcept;

	// For each method in T, get its name and a member pointer to it
	template<typename T, typename Func>
	constexpr void for_each_method(Func && func) noexcept;

	// For each method in T, get its name and a functor calling it on obj
	template<typename T, typename Func>
	constexpr void for_each_method(T && obj, Func && func) noexcept;

	// Try to find a method called "name" and get a member function pointer to it
	template<typename Ret, typename T>
	constexpr std::optional<Ret T:: *> get_method(std::string_view name) noexcept;

	// Try to find a method called "name" and get a functor calling it on obj
	template<typename Ret, typename T>
	constexpr auto get_method(T && obj, std::string_view name) noexcept;

	template<typename ... Metadata, typename Key>
	constexpr bool has_metadata(const putils::table<Metadata...> & metadata, Key && key) noexcept;

    template<typename Ret, typename ... Metadata, typename Key>
    constexpr const Ret & get_metadata(const putils::table<Metadata...> & metadata, Key && key) noexcept;
	
    template<typename Ret, typename ... Metadata, typename Key>
    constexpr const Ret * try_get_metadata(const putils::table<Metadata...> & metadata, Key && key) noexcept;
}

#include "reflection.inl"