#include "command.hpp"

#include <cctype>
#include <iomanip>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

#include "parse.hpp"

namespace petrov {

  command_table_t makeCommandTable()
  {
    command_table_t table;
    table.emplace("circle", cmdCircle);
    table.emplace("show", cmdShow);
    table.emplace("set", cmdSet);
    table.emplace("showset", cmdShowset);
    table.emplace("frame", cmdFrame);
    table.emplace("frameset", cmdFrameset);
    table.emplace("spawn", cmdSpawn);
    table.emplace("area-on", cmdAreaOn);
    table.emplace("wait", cmdWait);
    table.emplace("status", cmdStatus);

    return table;
  }

  void printCalcResult(const calc_t& calc, std::ostream& out)
  {
    if (calc.status == status_t::InProgress) {
      out << calc.process_name << " <IN PROGRESS>\n";
    } else {
      out << calc.process_name << " " << calc.time_ms << " " << std::fixed << std::setprecision(3) << calc.area << "\n";
    }
  }

  bool tryReadResponse(command_context_t& ctx, int fd)
  {
    response_t resp{};
    if (!recvFull(fd, &resp, sizeof(resp))) {
      return false;
    }

    auto it = ctx.calc_id_to_name.find(resp.calc_id);
    if (it == ctx.calc_id_to_name.end()) {
      return false;
    }

    auto calc_it = ctx.calcs.find(it->second);
    if (calc_it != ctx.calcs.end()) {
      calc_it->second.status = status_t::Done;
      calc_it->second.time_ms = resp.time_ms;
      calc_it->second.area = resp.area;
    }

    return true;
  }

  void cmdCircle(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out)
  {
    const size_t expected_args = 5;
    if (words.size() != expected_args || words[1].empty() || !std::isalpha(static_cast< unsigned char >(words[1][0]))) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    if (ctx.shapes.count(words[1]) > 0) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    long long r = 0, x = 0, y = 0;
    if (!parseLongLong(words[2], r) || !parseLongLong(words[3], x) || !parseLongLong(words[4], y) || r <= 0) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    ctx.shapes.emplace(words[1], shape_t{r, x, y});
  }

  void cmdShow(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out)
  {
    if (words.size() != 2) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    auto it = ctx.shapes.find(words[1]);
    if (it == ctx.shapes.end()) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    out << it->second.r << " (" << it->second.x << " " << it->second.y << ")\n";
  }

  void cmdSet(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out)
  {
    if (words.size() < 3 || words[1].empty() || !std::isalpha(static_cast< unsigned char >(words[1][0]))) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    long long n = 0;
    if (!parseLongLong(words[2], n) || n <= 0 || words.size() != 3 + static_cast< size_t >(n)) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    if (ctx.sets.count(words[1]) > 0) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    std::vector< std::string > v;
    v.reserve(static_cast< size_t >(n));
    for (size_t i = 0; i < static_cast< size_t >(n); ++i) {
      if (ctx.shapes.count(words[3 + i]) == 0) {
        out << "<INVALID COMMAND>\n";
        return;
      }
      v.push_back(words[3 + i]);
    }

    ctx.sets.emplace(words[1], std::move(v));
  }

  void cmdShowset(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out)
  {
    if (words.size() != 2) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    auto it = ctx.sets.find(words[1]);
    if (it == ctx.sets.end()) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    for (const auto& name : it->second) {
      const shape_t& s = ctx.shapes.at(name);
      out << s.r << " (" << s.x << " " << s.y << ")\n";
    }
  }

  void cmdFrame(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out)
  {
    if (words.size() != 2) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    auto it = ctx.shapes.find(words[1]);
    if (it == ctx.shapes.end()) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    const box_t box = boundingBox(it->second);
    out << "(" << box.x_min << " " << box.y_min << ") (" << box.x_max << " " << box.y_max << ")\n";
  }

  void cmdFrameset(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out)
  {
    if (words.size() != 2) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    auto it = ctx.sets.find(words[1]);
    if (it == ctx.sets.end()) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    std::vector< shape_t > shapes;
    shapes.reserve(it->second.size());
    for (const auto& name : it->second) {
      shapes.push_back(ctx.shapes.at(name));
    }

    const box_t box = boundingBox(shapes);
    out << "(" << box.x_min << " " << box.y_min << ") (" << box.x_max << " " << box.y_max << ")\n";
  }

  void cmdSpawn(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out)
  {
    if (words.size() != 3 || words[1].empty() || !std::isalpha(static_cast< unsigned char >(words[1][0]))) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    if (ctx.processes.count(words[1]) > 0) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    long long seed = 0;
    if (!parseLongLong(words[2], seed) || seed < 0) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    int in_pipe[2] = {-1, -1};
    int out_pipe[2] = {-1, -1};

    if (pipe(in_pipe) != 0 || pipe(out_pipe) != 0) {
      out << "<INVALID COMMAND>\n";
      if (in_pipe[0] != -1) {
        close(in_pipe[0]);
        close(in_pipe[1]);
      }
      if (out_pipe[0] != -1) {
        close(out_pipe[0]);
        close(out_pipe[1]);
      }
      return;
    }

    const pid_t pid = fork();
    if (pid < 0) {
      out << "<INVALID COMMAND>\n";
      close(in_pipe[0]);
      close(in_pipe[1]);
      close(out_pipe[0]);
      close(out_pipe[1]);
      return;
    }

    if (pid == 0) {
      close(in_pipe[1]);
      close(out_pipe[0]);

      for (auto& pair : ctx.processes) {
        close(pair.second.in_fd);
        close(pair.second.out_fd);
      }

      childLoop(in_pipe[0], out_pipe[1], static_cast< size_t >(seed));
      _exit(0);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);
    ctx.processes.emplace(words[1], process_t{pid, in_pipe[1], out_pipe[0]});
  }

  void cmdAreaOn(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out)
  {
    const size_t expected_args = 6;
    if (words.size() != expected_args || words[1].empty() || !std::isalpha(static_cast< unsigned char >(words[1][0]))) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    const std::string& proc_name = words[1];
    const std::string& calc_name = words[2];
    const std::string& set_name = words[3];

    auto proc_it = ctx.processes.find(proc_name);
    if (proc_it == ctx.processes.end() || ctx.calcs.count(calc_name) > 0) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    auto set_it = ctx.sets.find(set_name);
    if (set_it == ctx.sets.end()) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    long long threads_ll = 0, tries_ll = 0;
    const size_t threads_idx = 4;
    const size_t tries_idx = 5;
    if (!parseLongLong(words[threads_idx], threads_ll) || threads_ll <= 0 || !parseLongLong(words[tries_idx], tries_ll)
        || tries_ll <= 0) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    std::vector< shape_t > shapes;
    shapes.reserve(set_it->second.size());
    for (const auto& shape_name : set_it->second) {
      auto it = ctx.shapes.find(shape_name);
      if (it == ctx.shapes.end()) {
        out << "<INVALID COMMAND>\n";
        return;
      }
      shapes.push_back(it->second);
    }

    request_header_t header{};
    header.calc_id = ctx.next_calc_id++;
    header.tries = static_cast< size_t >(tries_ll);
    header.threads = static_cast< size_t >(threads_ll);
    header.n_shapes = shapes.size();

    if (!sendFull(proc_it->second.in_fd, &header, sizeof(header))) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    if (!shapes.empty()) {
      if (!sendFull(proc_it->second.in_fd, shapes.data(), sizeof(shape_t) * shapes.size())) {
        out << "<INVALID COMMAND>\n";
        return;
      }
    }

    calc_t calc{};
    calc.process_name = proc_name;
    calc.status = status_t::InProgress;
    calc.time_ms = 0;
    calc.area = 0.0;

    ctx.calcs.emplace(calc_name, calc);
    ctx.calc_id_to_name.emplace(header.calc_id, calc_name);
  }

  void cmdWait(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out)
  {
    if (words.size() != 2) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    auto calc_it = ctx.calcs.find(words[1]);
    if (calc_it == ctx.calcs.end()) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    if (calc_it->second.status != status_t::InProgress) {
      printCalcResult(calc_it->second, out);
      return;
    }

    auto proc_it = ctx.processes.find(calc_it->second.process_name);
    if (proc_it == ctx.processes.end()) {
      return;
    }

    const int fd = proc_it->second.out_fd;
    while (true) {
      struct pollfd pfd{};
      pfd.fd = fd;
      pfd.events = POLLIN;

      const int ret = poll(&pfd, 1, -1);
      if (ret < 0) {
        continue;
      }

      if (pfd.revents & (POLLIN | POLLHUP)) {
        if (!tryReadResponse(ctx, fd)) {
          break;
        }
      }

      if (calc_it->second.status != status_t::InProgress) {
        break;
      }
    }

    printCalcResult(calc_it->second, out);
  }

  void cmdStatus(const std::vector< std::string >& words, command_context_t& ctx, std::ostream& out)
  {
    if (words.size() != 2) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    auto calc_it = ctx.calcs.find(words[1]);
    if (calc_it == ctx.calcs.end()) {
      out << "<INVALID COMMAND>\n";
      return;
    }

    if (calc_it->second.status == status_t::InProgress) {
      auto proc_it = ctx.processes.find(calc_it->second.process_name);
      if (proc_it != ctx.processes.end()) {
        const int fd = proc_it->second.out_fd;
        while (true) {
          struct pollfd pfd{};
          pfd.fd = fd;
          pfd.events = POLLIN;

          const int ret = poll(&pfd, 1, 0);
          if (ret <= 0) {
            break;
          }

          if (pfd.revents & (POLLIN | POLLHUP)) {
            if (!tryReadResponse(ctx, fd)) {
              break;
            }
          }
        }
      }
    }

    printCalcResult(calc_it->second, out);
  }

}
