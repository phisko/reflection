#pragma once

// stl
#include <string_view>
#include <optional>
#include <type_traits>
#include <utility>
#include <functional>

// meta
#include "putils/meta/table.hpp"

namespace putils::reflection {
	template<typename T>
	struct type_info;
	// may have:
	// 		static constexpr auto class_name = const char *;
	// 		static constexpr auto attributes = std::tuple<attribute_info>;
	// 		static constexpr auto methods = std::tuple<attribute_info>;
	// 		static constexpr auto parents = std::tuple<used_type_info>;
	// 		static constexpr auto used_types = std::tuple<used_type_info>;

	template<typename MemberPtr, typename MetadataTable>
	struct attribute_info {
		const char * name;
		const MemberPtr ptr;
		const MetadataTable metadata; // putils::table<Key, Value...>
	};

	template<typename MemberPtr, typename MetadataTable>
	using method_info = attribute_info<MemberPtr, MetadataTable>;

	template<typename Member, typename MetadataTable>
	struct object_attribute_info {
		const char * name;
		Member & member;
		const MetadataTable & metadata; // putils::table<Key, Value...>
	};

	template<typename Callback, typename MetadataTable>
	struct object_method_info {
		const char * name;
		const Callback & method;
		const MetadataTable & metadata; // putils::table<Key, Value...>
	};

	template<typename T, typename MetadataTable>
	struct used_type_info {
		const putils::meta::type<T> type;
		const MetadataTable metadata; // putils::table<Key, Value...>
	};

	template<typename T>
	constexpr bool is_reflectible() noexcept;

	template<typename T>
	constexpr auto get_class_name() noexcept;

	// For each parent of T, get a used_type_info
	// Same behavior as putils::tuple_for_each
	template<typename T, typename Func>
	constexpr auto for_each_parent(Func && func) noexcept;

	template<typename T, typename Parent>
	constexpr bool has_parent() noexcept;

	// For each type used by T, get a used_type_info
	// Same behavior as putils::tuple_for_each
	template<typename T, typename Func>
	constexpr auto for_each_used_type(Func && func) noexcept;

	template<typename T, typename Used>
	constexpr bool has_used_type() noexcept;

	// For each attribute in T, get an attribute_info
	// Same behavior as putils::tuple_for_each
	template<typename T, typename Func>
	constexpr auto for_each_attribute(Func && func) noexcept;

	// For each attribute in T, get an object_attribute_info
	// Same behavior as putils::tuple_for_each
	template<typename T, typename Func>
	constexpr auto for_each_attribute(T && obj, Func && func) noexcept;

	template<typename T>
	constexpr bool has_attribute(std::string_view name) noexcept;

	// Try to find an attribute called "name" and get a member pointer to it, or nullopt
	template<typename Attribute, typename T>
	constexpr std::optional<Attribute T::*> get_attribute(std::string_view name) noexcept;

	// Try to find an attribute called "name" and get a pointer to it in obj, or nullptr
	template<typename Attribute, typename T>
	constexpr auto /* [const] Attribute * */ get_attribute(T && obj, std::string_view name) noexcept;

	// For each method in T, get an attribute_info
	// Same behavior as putils::tuple_for_each
	template<typename T, typename Func>
	constexpr auto for_each_method(Func && func) noexcept;

	// For each method in T, get an object_method_info
	// Same behavior as putils::tuple_for_each
	template<typename T, typename Func>
	constexpr auto for_each_method(T && obj, Func && func) noexcept;

	template<typename T>
	constexpr bool has_method(std::string_view name) noexcept;

	// Try to find a method called "name" and get an optional functor taking a `T` that calls the method on it
	template<typename Signature, typename T>
	constexpr auto get_method(std::string_view name) noexcept;
	// Alternatively, provide the complete member function type (i.e. `void (Type::*)()`). Required in a constexpr context with gcc
	template<typename Signature>
	constexpr auto get_method(std::string_view name) noexcept;

	// Try to find a method called "name" and get an optional functor calling it on obj
	template<typename Signature, typename T>
	constexpr auto get_method(T && obj, std::string_view name) noexcept;

	template<typename T, typename Key>
	constexpr bool has_attribute_metadata(std::string_view attribute, Key && key) noexcept;

	template<typename Ret, typename T, typename Key>
	constexpr const Ret * get_attribute_metadata(std::string_view attribute, Key && key) noexcept;

	template<typename T, typename Key>
	constexpr bool has_method_metadata(std::string_view method, Key && key) noexcept;

	template<typename Ret, typename T, typename Key>
	constexpr const Ret * get_method_metadata(std::string_view method, Key && key) noexcept;

	template<typename... Metadata, typename Key>
	constexpr bool has_metadata(const putils::table<Metadata...> & metadata, Key && key) noexcept;

	template<typename Ret, typename... Metadata, typename Key>
	constexpr const Ret * get_metadata(const putils::table<Metadata...> & metadata, Key && key) noexcept;
}

#include "reflection.inl"