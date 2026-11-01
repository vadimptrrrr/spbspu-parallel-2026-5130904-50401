#ifndef COMMAND_HPP
#define COMMAND_HPP

#include <map>
#include <ostream>
#include <string>
#include <vector>

#include "process.hpp"
#include "shape.hpp"

namespace petrov {

  enum class status_t { InProgress, Done };

  struct calc_t {
    std::string process_name;
    status_t status;
    size_t time_ms;
    double area;
  };

  using calc_map_t = std::map< std::string, calc_t >;

  struct command_context_t {
    shape_map_t shapes;
    set_map_t sets;
    process_map_t processes;
    calc_map_t calcs;
    std::map< size_t, std::string > calc_id_to_name;
    size_t next_calc_id = 1;
  };

  using command_handler_t = void (*)(const std::vector< std::string >&, command_context_t&, std::ostream&);
  using command_table_t = std::map< std::string, command_handler_t >;

  command_table_t makeCommandTable();

  void cmdCircle(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out);
  void cmdShow(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out);
  void cmdSet(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out);
  void cmdShowset(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out);
  void cmdFrame(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out);
  void cmdFrameset(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out);
  void cmdSpawn(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out);
  void cmdAreaOn(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out);
  void cmdWait(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out);
  void cmdStatus(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out);

  bool tryReadResponse(command_context_t& ctx, int fd);
  void printCalcResult(const calc_t& calc, std::ostream& out);

}

#endif
