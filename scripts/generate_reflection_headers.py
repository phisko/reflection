#!/usr/bin/env python3

import argparse
import os
import re
from clang.cindex import *

import clang_helpers

parser = argparse.ArgumentParser(description='Generate putils reflection headers')
parser.add_argument('files', help = 'input headers to parse', nargs = '+')
parser.add_argument('--extension', help = 'output file extension', default = '.reflection.hpp')
parser.add_argument('--clang-args', help = 'extra arguments to pass to clang', nargs = argparse.REMAINDER, required = False)
parser.add_argument('--diagnostics', help = 'Print clang diagnostic messages', action = 'store_true', required = False)

args = parser.parse_args()

def get_reflection_info_for_type(node, reflection_type):
	reflection_info = { 'type': clang_helpers.get_fully_qualified_symbol(node) }

	match = re.match(r'.*class_name:\s*([^\s]+).*', node.brief_comment)
	if match:
		reflection_info['class_name'] = match.group(1)

	def add_list_property_to_reflection_infos(prop_name):
		match = re.match(f'.*{prop_name}:\s*\[([^\]]*)\].*', node.brief_comment)
		if match:
			reflection_info[prop_name] = [s.strip() for s in match.group(1).split(',')]
	add_list_property_to_reflection_infos('parents')
	add_list_property_to_reflection_infos('used_types')

	def add_children_to_reflection_infos(node, target_reflection_type, target_kind):
		for child in node.get_children():
			def should_reflect():
				if child.brief_comment == 'putils reflect off':
					return False

				if re.match(r'^operator[^\w]', child.spelling):
					return False

				if reflection_type in ['all', target_reflection_type]:
					return True
				if child.brief_comment == 'putils reflect':
					return True
				return False

			if not should_reflect():
				continue

			if child.is_anonymous():
				add_children_to_reflection_infos(child, target_reflection_type, target_kind)

			if child.kind != target_kind:
				continue

			if not target_reflection_type in reflection_info:
				reflection_info[target_reflection_type] = []
			reflection_info[target_reflection_type].append(child.spelling)

	add_children_to_reflection_infos(node, 'attributes', CursorKind.FIELD_DECL)
	add_children_to_reflection_infos(node, 'methods', CursorKind.CXX_METHOD)
	return reflection_info

def visit_node(node):
	reflection_infos = []

	def parse_reflection_info():
		if node.kind not in [CursorKind.STRUCT_DECL, CursorKind.CLASS_DECL]:
			return

		# Ignore forward declarations
		if not node.is_definition():
			return

		if not node.brief_comment:
			return

		match = re.match(r'.*putils reflect (\w+)', node.brief_comment)
		if match:
			reflection_info = get_reflection_info_for_type(node, match.group(1))
			reflection_infos.append(reflection_info)

	parse_reflection_info()
	for child in node.get_children():
		reflection_infos += visit_node(child)
	return reflection_infos

def generate_reflection_info(reflection_info):
	result = f'\n\n#define refltype {reflection_info["type"]}\n'
	result += 'putils_reflection_info {\n'

	if not 'class_name' in reflection_info:
		result += '\tputils_reflection_class_name;\n'
	else:
		result += f'\tputils_reflection_custom_class_name({reflection_info["class_name"]});\n'

	def generate_property_list(prop_name, item_macro):
		result = ''
		if not prop_name in reflection_info:
			return result

		result += f'\tputils_reflection_{prop_name}(\n'
		for index, attr in enumerate(reflection_info[prop_name]):
			result += f'\t\t{item_macro}({attr})'
			if index != len(reflection_info[prop_name]) - 1:
				result += ','
			result += '\n'
		result += '\t);\n'
		return result

	result += generate_property_list('attributes', 'putils_reflection_attribute')
	result += generate_property_list('methods', 'putils_reflection_attribute')
	result += generate_property_list('parents', 'putils_reflection_type')
	result += generate_property_list('used_types', 'putils_reflection_type')

	result += '};\n'
	result += '#undef refltype'
	return result

def get_output_file(input_file):
	base_file, extension = os.path.splitext(input_file)
	if extension == args.extension:
		return None
	return base_file + args.extension

#
# Main
#

# Truncate all existing output files
for input_file in args.files:
	output_file = get_output_file(input_file)
	if not output_file:
		continue

	with open(output_file, 'w'):
		pass

parsed_files = clang_helpers.parse_files(args.files, args.clang_args)

for file_name, parsed_file in parsed_files.items():
	output_file = get_output_file(file_name)
	if not output_file:
		continue

	if args.diagnostics:
		diagnostics = parsed_file['diagnostics']
		if diagnostics:
			print(f'Diagnostics for {file_name}:')
			for diagnostic in diagnostics:
				print(f'\t{diagnostic}')

	reflection_infos = []
	for node in parsed_file['nodes']:
		reflection_infos += visit_node(node)

	if not reflection_infos:
		os.remove(output_file)
		continue

	result = '#pragma once\n\n#include "putils/reflection.hpp"'
	for reflection_info in reflection_infos:
		result += generate_reflection_info(reflection_info)

	with open(output_file, 'w') as f:
		f.write(result)