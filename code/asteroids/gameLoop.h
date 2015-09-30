#pragma once

#include <Foundation/Time/Time.h>

struct GameLoopData
{
  ezTime dt;
  bool stop = false;
};
