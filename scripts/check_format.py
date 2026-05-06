#!/usr/bin/env python3

import sys
import re

def format_info(filename):

    def space_indent_quantity_ok(s): return re.match("^(\ \ \ \ )*[^\ ]?", s)
    def indented_with_tab(s): return re.match("^\t+", s)
    def has_trailing_space(s): return re.fullmatch(".*[\ \t\r\n]+\n$", s)

    def basename(filepath): return re.search("[^/\\\\]+$", filepath).group()
    def report(line_s, line_no, reason): print("%s.%u: %s /* => %s */" % (filename, line_no, line.strip("\r\n"), reason))

    with open(filename, "r") as f:
        for n, line in enumerate(f.readlines()):
            if not space_indent_quantity_ok(line):
                report(line, n, "Wrong indentation width")
            if indented_with_tab(line):
                report(line, n, "Indented using tabs")
            if has_trailing_space(line):
                report(line, n, "Trailing whitespace")

if __name__ == "__main__":
    for filename in sys.argv[1:]:
        format_info(filename)
