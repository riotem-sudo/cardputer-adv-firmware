import io, os, sys

tsv = r"C:\launcher\zh.tsv"
out_h = r"C:\launcher\Launcher-3.0.0\lib\DisplayDrivers\src\zh_lang.h"
chars_txt = r"C:\launcher\chars.txt"

pairs = []
seen = {}
with io.open(tsv, encoding="utf-8") as f:
    for line in f:
        line = line.rstrip("\n")
        if not line.strip():
            continue
        if "\t" not in line:
            print("skip bad line:", line)
            continue
        en, zh = line.split("\t", 1)
        if not en or not zh:
            continue
        if en in seen:
            continue
        seen[en] = True
        pairs.append((en, zh.strip()))

pairs.sort(key=lambda p: p[0])

cjk = sorted({c for _, z in pairs for c in z if ord(c) > 0x2E7F})

with io.open(out_h, "w", encoding="utf-8", newline="\n") as f:
    f.write("// Generated from zh.tsv - do not edit\n")
    f.write("#ifndef ZH_LANG_H\n#define ZH_LANG_H\n\n")
    f.write("static const char *const ZH_EN[] = {\n")
    for en, _ in pairs:
        f.write('  "%s",\n' % en.replace('\\', '\\\\').replace('"', '\\"'))
    f.write("};\n\n")
    f.write("static const char *const ZH_ZH[] = {\n")
    for _, zh in pairs:
        f.write('  "%s",\n' % zh.replace('\\', '\\\\').replace('"', '\\"'))
    f.write("};\n\n")
    f.write("static const int ZH_PAIR_COUNT = %d;\n\n" % len(pairs))
    f.write("#endif\n")

with io.open(chars_txt, "w", encoding="utf-8") as f:
    f.write("".join(cjk))

print("pairs:", len(pairs), "unique cjk chars:", len(cjk))
print("".join(cjk))
