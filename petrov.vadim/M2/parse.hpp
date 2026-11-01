#ifndef PARSE_HPP
#define PARSE_HPP

#include <string>
#include <vector>

namespace petrov {

  std::vector< std::string > split(const std::string& line);
  bool parseLongLong(const std::string& s, long long& out);
  bool parseDouble(const std::string& s, double& out);

}

#endif
