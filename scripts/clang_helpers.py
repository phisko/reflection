import re
from clang.cindex import *

# Return a dictionary mapping the file name to { 'nodes': [], 'diagnostics': [] }
# 'nodes' will only contain the nodes actually defined in the file (not those included from headers)
def parse_files(files, clang_args):
    results = {}

    index = Index.create()

    split_clang_args = []
    if clang_args:
        for arg in clang_args:
            split_clang_args += arg.split()

    for file_name in files:
        tu = index.parse(file_name, args = split_clang_args)

        results[file_name] = { 'diagnostics': tu.diagnostics }
        results[file_name]['nodes'] = []

        for child in tu.cursor.get_children():
            if str(child.location.file) == file_name:
                results[file_name]['nodes'].append(child)

    return results

# Gets a class or function name with all parent types and namespaces
def get_fully_qualified_symbol(c):
    if c is None:
        return ''
    elif c.kind == CursorKind.TRANSLATION_UNIT:
        return ''
    else:
        res = get_fully_qualified_symbol(c.semantic_parent)
        if res != '':
            return res + '::' + c.spelling
        return c.spelling

open_brackets = '([{\'"'
closed_brackets = ')]}\'"'

# Parses a node's comment for a value like
# //! key: value
def parse_value_from_comment(node, key):
    if not node.brief_comment:
        return None

    def try_match(open, close):
        match = re.match(f'.*{key}:\s*(.*)$', node.brief_comment)
        if not match:
            return None
        remainder = match.group(1)

        if not remainder.startswith(open):
            return None

        if open == close:
            remainder = remainder[1:] # Skip open quote
            return remainder[0:remainder.find(close)]

        # We're dealing with brackets, handle nested groups
        open_count = 0
        for i, c in enumerate(remainder):
            if c == open:
                open_count += 1
            elif c == close:
                open_count -= 1

            if open_count == 0:
                return remainder[1:i]

        return None

    for i, c in enumerate(open_brackets):
        result = try_match(c, closed_brackets[i])
        if result:
            return result

    return None

def parse_array_from_comment(node, key):
    value = parse_value_from_comment(node, key)
    if not value:
        return None

    array = []
    last_element_start = 0
    open_stack = []
    for i, c in enumerate(value):
        if c == ',' and not open_stack:
            array.append(value[last_element_start:i].strip())
            last_element_start = i + 1

        open_index = open_brackets.find(c)
        if open_index != -1:
            open_stack.append(c)
        if open_stack:
            last_open = open_stack[-1]
            bracket_index = open_brackets.find(last_open)
            expected_close = closed_brackets[bracket_index]
            if c == expected_close:
                open_stack.pop()

    assert(not open_stack)
    array.append(value[last_element_start:].strip())

    return array
