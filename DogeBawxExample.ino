#include "DogeBawx.h"

DogeBawx bawx;

void setup()
{
  bawx.init();
}

void loop()
{
  bawx.readBtns();
  bawx.readCmd();
  bawx.write();
}
