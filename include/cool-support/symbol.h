

#ifndef SYMBOL_H
#define SYMBOL_H

#include <iosfwd>
#include <string>

class Entry
{
protected:
    std::string str;
    int index;

public:
    Entry() = default;
    Entry(std::string s, int i) : str(std::move(s)), index(i) {}

    std::string const &get_string() const { return str; }
    int get_index() const { return index; }
};

using Symbol = Entry *;

std::ostream &operator<<(std::ostream &s, Entry const &sym);


#endif //SYMBOL_H
