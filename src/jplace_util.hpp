#ifndef EPA_JPLACE_UTIL_H_
#define EPA_JPLACE_UTIL_H_

#include <string>
#include <sstream>

#include "Placement.hpp"
#include "Placement_Set.hpp"

#define TAB "  ";
#define NEWL "\n";

std::string placement_to_jplace_string(const Placement& p);
std::string placement_set_to_jplace_string(const Placement_Set& ps, std::string& invocation);

#endif
