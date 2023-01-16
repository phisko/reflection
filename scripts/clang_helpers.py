from clang.cindex import *

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