#include "shape.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <future>
#include <random>
#include <vector>

namespace petrov {

  box_t boundingBox(const shape_t& shape)
  {
    return box_t{shape.x - shape.r, shape.x + shape.r, shape.y - shape.r, shape.y + shape.r};
  }

  box_t boundingBox(const std::vector< shape_t >& shapes)
  {
    box_t box = boundingBox(shapes.front());

    for (size_t i = 1; i < shapes.size(); ++i) {
      const box_t b = boundingBox(shapes[i]);
      box.x_min = std::min(box.x_min, b.x_min);
      box.x_max = std::max(box.x_max, b.x_max);
      box.y_min = std::min(box.y_min, b.y_min);
      box.y_max = std::max(box.y_max, b.y_max);
    }

    return box;
  }

  bool isInside(double x, double y, const shape_t& shape)
  {
    const double dx = x - static_cast< double >(shape.x);
    const double dy = y - static_cast< double >(shape.y);
    const double r = static_cast< double >(shape.r);

    return (dx * dx) + (dy * dy) <= (r * r);
  }

  result_t calc(const std::vector< shape_t >& shapes, const box_t& box, size_t tests, size_t seed)
  {
    std::default_random_engine engine(seed);
    const double width = static_cast< double >(box.x_max - box.x_min);
    const double height = static_cast< double >(box.y_max - box.y_min);
    const double x_min = static_cast< double >(box.x_min);
    const double y_min = static_cast< double >(box.y_min);
    const double max_val = static_cast< double >(engine.max());

    size_t hits = 0;

    for (size_t i = 0; i < tests; ++i) {
      const double x = x_min + (static_cast< double >(engine()) / max_val) * width;
      const double y = y_min + (static_cast< double >(engine()) / max_val) * height;

      for (size_t j = 0; j < shapes.size(); ++j) {
        if (isInside(x, y, shapes[j])) {
          ++hits;
          break;
        }
      }
    }

    return result_t{hits};
  }

  result_t countTotalHits(
      const std::vector< shape_t >& shapes, const box_t& box, size_t threads, size_t tries, size_t seed)
  {
    if (threads == 1) {
      return calc(shapes, box, tries, seed);
    }

    const size_t base = tries / threads;
    const size_t rem = tries % threads;

    std::vector< std::future< result_t > > futures;
    futures.reserve(threads);

    for (size_t i = 0; i < threads; ++i) {
      size_t local_tries = base + (i < rem ? 1 : 0);
      size_t local_seed = seed + i;
      futures.emplace_back(
          std::async(std::launch::async, calc, std::cref(shapes), std::cref(box), local_tries, local_seed));
    }

    result_t res{0};

    for (size_t i = 0; i < threads; ++i) {
      const result_t local_res = futures[i].get();
      res.hits += local_res.hits;
    }

    return res;
  }

}
