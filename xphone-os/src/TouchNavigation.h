#pragma once
#include <cmath>
namespace touch_navigation {
enum class Direction { Up, Down, Left, Right };
// Normalized native-panel positions -> Flowe portrait navigation.
inline Direction swipe(float x0, float y0, float x1, float y1) {
  const float dx = -(y1 - y0), dy = x1 - x0;
  return std::fabs(dx) > std::fabs(dy) ? (dx > 0 ? Direction::Right : Direction::Left)
                                       : (dy > 0 ? Direction::Down : Direction::Up);
}
}
