#pragma once
#include <asteroids/gameLoop.h>

namespace level
{
  void initialize(ezRectFloat levelBounds);
  void shutdown();
  void update(GameLoopData& gameLoop);
}
