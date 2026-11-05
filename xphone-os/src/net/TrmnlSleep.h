#pragma once
class Gfx;
namespace TrmnlSleep {
// SD configuration is read on entry; polling happens only during a nap.
void update();
bool due();
bool draw(Gfx& gfx);
}
