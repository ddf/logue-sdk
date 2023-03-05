#pragma once

#include <cmath>
#include "Noise.hpp"

template<int dim>
class Noise2D
{
  const float step = 4.0f / dim;
  float table[dim * dim];
public:
  Noise2D()
  {
    for (int x = 0; x < dim; ++x)
    {
      for (int y = 0; y < dim; ++y)
      {
        int i = x * dim + y;
        table[i] = perlin2d(x * step, y * step, 1, 4) * 2 - 1;
      }
    }
  }

  float sample(float x, float y) const
  {
    int nx = (int)(fabs(x) / step) % dim;
    int ny = (int)(fabs(y) / step) % dim;
    int ni = nx * dim + ny;
    return table[ni];
  }
};