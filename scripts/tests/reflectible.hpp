#pragma once

#include <string>

struct base {};

//! putils reflect all
//! parents: [base]
//! used_types: [int, std::string]
struct reflectible : base {
	int i = 0;
	std::string name = "hello";

	void f() noexcept;
	int g() const noexcept;
};