#!/usr/bin/env python3
"""One-shot rebrand: DQuest/Qivot  ->  Qivot.

  - DQ  class/macro prefix   -> Qi   (QiModel -> QiModel, QI_MODEL -> QI_MODEL)
  - dq  function/file prefix -> qi   (qiField -> qiField, qimodel.h -> qimodel.h)
  - product name Qivot/QIVOT -> Qivot/QIVOT
  - umbrella qivot.h        -> qivot.h,  header-only qivot.hpp -> qivot.hpp
  - the *attribution* "DQuest" (mixed case) is preserved on purpose.

Run once from the repo root:  python3 tools/rebrand.py
"""
import os
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

SKIP_DIRS = {".git", "dist"}
TEXT_EXT = {".h", ".cpp", ".hpp", ".pri", ".pro", ".txt", ".md", ".py",
            ".in", ".qml", ".qrc", ".json", ".patch", ".sh", ".cmake", ".pro.user"}
TEXT_NAMES = {"CMakeLists.txt", "Doxyfile", "install.sh", "control", "rules",
              "changelog", "compat", "copyright"}

_DQ_MACRO   = re.compile(r'\bDQ_')
_DQ_CLASS   = re.compile(r'\bDQ(?=[A-Z])')
_dq_lower   = re.compile(r'(?<![A-Za-z0-9])dq(?=[A-Za-z])')
_dq_us      = re.compile(r'_dq(?=[A-Za-z])')


def transform(text):
    # 1) umbrella header + any lowercase "qivot"  -> qivot   (keeps mixed-case DQuest)
    text = text.replace("qivot", "qivot")
    # 2) all-caps guards / pri vars: QIVOT_* -> QIVOT_*
    text = text.replace("QIVOT", "QIVOT")
    # 3) product name (3 casings)
    text = text.replace("QIVOT", "QIVOT")
    text = text.replace("Qivot", "Qivot")
    text = text.replace("qivot", "qivot")
    # 4) symbol prefixes
    text = _DQ_MACRO.sub("QI_", text)     # QI_MODEL -> QI_MODEL
    text = _DQ_CLASS.sub("Qi", text)      # QiModel  -> QiModel   (DQuest untouched: 'u' is lc)
    text = _dq_us.sub("_qi", text)        # _qiMetaInfoCreateFields -> _qiMetaInfoCreateFields
    text = _dq_lower.sub("qi", text)      # qiField  -> qiField
    return text


def is_text(name):
    _, ext = os.path.splitext(name)
    return ext in TEXT_EXT or name in TEXT_NAMES


def main():
    changed, renamed = 0, 0
    # --- pass 1: rewrite file contents ---
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for fn in filenames:
            if not is_text(fn):
                continue
            path = os.path.join(dirpath, fn)
            try:
                with open(path, "r", encoding="utf-8") as f:
                    src = f.read()
            except (UnicodeDecodeError, IsADirectoryError):
                continue
            out = transform(src)
            if out != src:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(out)
                changed += 1

    # --- pass 2: rename files whose basename carries the old brand ---
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
        for fn in filenames:
            new = transform(fn)
            if new != fn:
                os.rename(os.path.join(dirpath, fn), os.path.join(dirpath, new))
                renamed += 1
                print("  renamed %s -> %s" % (fn, new))

    print("rebrand: %d files edited, %d files renamed" % (changed, renamed))


if __name__ == "__main__":
    main()
