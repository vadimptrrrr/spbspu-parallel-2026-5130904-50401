#ifndef SHAPE_HPP
#define SHAPE_HPP

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace petrov {

  struct shape_t {
    long long r;
    long long x;
    long long y;
  };

  struct box_t {
    long long x_min;
    long long x_max;
    long long y_min;
    long long y_max;
  };

  struct result_t {
    size_t hits;
  };

  using shape_map_t = std::map< std::string, shape_t >;
  using set_map_t = std::map< std::string, std::vector< std::string > >;

  box_t boundingBox(const shape_t& shape);
  box_t boundingBox(const std::vector< shape_t >& shapes);

  bool isInside(double x, double y, const shape_t& shape);

  result_t calc(const std::vector< shape_t >& shapes, const box_t& box, size_t tests, size_t seed);
  result_t countTotalHits(
      const std::vector< shape_t >& shapes, const box_t& box, size_t threads, size_t tries, size_t seed);

}

#endif
