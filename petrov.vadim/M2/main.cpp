#include <csignal>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "command.hpp"
#include "parse.hpp"

int main()
{
  struct sigaction sa{};
  sa.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sa, nullptr);

  petrov::command_context_t ctx;
  ctx.next_calc_id = 1;

  const petrov::command_table_t table = petrov::makeCommandTable();

  std::string line;
  while (std::getline(std::cin, line)) {
    const std::vector< std::string > words = petrov::split(line);
    if (words.empty()) {
      continue;
    }

    auto it = table.find(words[0]);
    if (it == table.end()) {
      std::cout << "<UNKNOWN COMMAND>\n";
      continue;
    }

    it->second(words, ctx, std::cout);
  }

  for (const auto& pair : ctx.processes) {
    const petrov::process_t& proc = pair.second;
    close(proc.in_fd);
    close(proc.out_fd);
    waitpid(proc.pid, nullptr, 0);
  }

  return 0;
}
