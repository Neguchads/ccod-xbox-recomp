"""Hypothesis fuzzing of the public source-only dev-menu index parser."""
import re
from hypothesis import given, settings, strategies as st
from tools import menu_search_index as index

CI = settings(max_examples=130, deadline=None, derandomize=True)

@CI
@given(st.text(max_size=600))
def fuzz_source_parser(source):
    parsed = index.entries(source)
    assert all(isinstance(page, int) and isinstance(label, str) for page, label in parsed)
    assert index.render(source).startswith("#pragma once\n")
    assert parsed == index.entries(source)

@CI
@given(st.text(alphabet=st.characters(min_codepoint=32, max_codepoint=126), max_size=180))
def fuzz_quoted_label(raw):
    safe = raw.replace("\\", "").replace('"', "").replace("%", "")
    source = f'void PageA() {{ Toggle("{safe}", flag); }}\nvoid Draw() {{case 3:PageA();}}'
    parsed = index.entries(source)
    # A label may be empty or filtered, but must never leak an unescaped quote.
    assert all('"' not in label for _, label in parsed)

if __name__ == "__main__":
    fuzz_source_parser()
    fuzz_quoted_label()
    print("PASS: Xbox source-only search index properties")
