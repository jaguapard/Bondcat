#include "BondcatException.h"
#include <sstream>

BondcatException::BondcatException(std::string s, std::source_location loc)
{
	std::stringstream ss(s);
	ss << "Uncaught exception at " << loc.file_name() << ", line " << loc.line() << ", col " << loc.column() << ": " << s;
	this->desc = ss.str();
}

std::string BondcatException::asString() const
{
	return desc;
}
