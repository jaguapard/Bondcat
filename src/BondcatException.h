#include <stdexcept>
#include <source_location>
class BondcatException : std::exception
{
public:
	BondcatException(std::string s, std::source_location = std::source_location::current());
	std::string asString() const;
private:
	std::string desc;
};