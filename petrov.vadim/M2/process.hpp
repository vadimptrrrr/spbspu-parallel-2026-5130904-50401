#ifndef PROCESS_HPP
#define PROCESS_HPP

#include <cstddef>
#include <map>
#include <string>
#include <sys/types.h>

namespace petrov {

  struct process_t {
    pid_t pid;
    int in_fd;
    int out_fd;
  };

  using process_map_t = std::map< std::string, process_t >;

  struct request_header_t {
    size_t calc_id;
    size_t tries;
    size_t threads;
    size_t n_shapes;
  };

  struct response_t {
    size_t calc_id;
    size_t time_ms;
    double area;
  };

  bool sendFull(int fd, const void* buf, size_t k);
  bool recvFull(int fd, void* buf, size_t k);

  void childLoop(int in_fd, int out_fd, size_t seed);

}

#endif
