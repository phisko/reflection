#pragma once

#include <string>

struct base {};

//! putils reflect all
//! parents: [base]
//! used_types: [int, std::string]
//! metadata: [("key", "value"), (("otherkey", "moreotherkey"), ["othervalue", "moreothervalue"])]
struct reflectible : base {
	int i = 0;
	//! metadata: [("key", "value")]
	std::string name = "hello";

	//! metadata: [("zero", 0), ("foo", "bar"), (("key", "morekey"), ("value", ["morevalue1", "morevalue2"]))]
	void f() noexcept;
	int g() const noexcept;
};