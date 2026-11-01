#include "process.hpp"

#include <chrono>
#include <cstddef>
#include <unistd.h>
#include <vector>

#include "shape.hpp"

namespace petrov {
  namespace {

    size_t send(int& err, int wr, const char* msg, size_t k)
    {
      size_t r = 0;
      while (r < k) {
        err = write(wr, msg + r, k - r);
        if (err <= 0) {
          break;
        }
        r += static_cast< size_t >(err);
      }

      return r;
    }

    size_t recv(int& err, int rd, char* buf, size_t k)
    {
      size_t r = 0;
      while (r < k) {
        err = read(rd, buf + r, k - r);
        if (err <= 0) {
          break;
        }
        r += static_cast< size_t >(err);
      }

      return r;
    }

  }

  bool sendFull(int fd, const void* buf, size_t k)
  {
    int err = 0;
    return send(err, fd, static_cast< const char* >(buf), k) == k;
  }

  bool recvFull(int fd, void* buf, size_t k)
  {
    int err = 0;
    return recv(err, fd, static_cast< char* >(buf), k) == k;
  }

  void childLoop(int in_fd, int out_fd, size_t seed)
  {
    while (true) {
      request_header_t header{};
      if (!recvFull(in_fd, &header, sizeof(header))) {
        break;
      }

      std::vector< shape_t > shapes(header.n_shapes);
      if (header.n_shapes > 0) {
        if (!recvFull(in_fd, shapes.data(), sizeof(shape_t) * header.n_shapes)) {
          break;
        }
      }

      const auto start = std::chrono::steady_clock::now();
      double area = 0.0;

      if (!shapes.empty() && header.tries > 0 && header.threads > 0) {
        const box_t box = boundingBox(shapes);
        const result_t hits = countTotalHits(shapes, box, header.threads, header.tries, seed);

        const double square = (static_cast< double >(box.x_max) - static_cast< double >(box.x_min))
            * (static_cast< double >(box.y_max) - static_cast< double >(box.y_min));

        area = square * (static_cast< double >(hits.hits) / static_cast< double >(header.tries));
      }

      const auto end = std::chrono::steady_clock::now();
      const auto time_ms = std::chrono::duration_cast< std::chrono::milliseconds >(end - start).count();

      response_t resp{};
      resp.calc_id = header.calc_id;
      resp.time_ms = static_cast< size_t >(time_ms);
      resp.area = area;

      if (!sendFull(out_fd, &resp, sizeof(resp))) {
        break;
      }
    }

    close(in_fd);
    close(out_fd);
  }

}
