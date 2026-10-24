# Writes a .cpp defining a char array with a text file's bytes, for the
# compiled-in PS2 names:
#   cmake -DIN=<file> -DOUT=<cpp> -DNAME=<symbol> -DNS=<namespace> -P embed_text.cmake
file(READ "${IN}" hex HEX)
string(LENGTH "${hex}" n)
math(EXPR bytes "${n} / 2")
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," arr "${hex}")
string(REGEX REPLACE "(0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,)" "\\1\n" arr "${arr}")
file(WRITE "${OUT}" "// Generated from ${IN}; don't edit.\n#include <cstddef>\nnamespace ${NS} {\nextern const char ${NAME}[];\nextern const std::size_t ${NAME}Size;\nconst char ${NAME}[] = {\n${arr}0};\nconst std::size_t ${NAME}Size = ${bytes};\n}\n")
