#include <iostream>

#include "stringtab.h"

std::ostream &operator<<(std::ostream &s, Entry const &sym)
{
    return s << "{" << sym.get_string() << ", " << sym.get_index() << "}\n";
}

IntTable inttable;
IdTable idtable;
StrTable stringtable;
