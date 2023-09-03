#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int main(int argc, char *argv[])
{
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  /* W/O Index
  # SELECT * FROM NumbersC INTO BigNumbersC WHERE key > 165000;
  Number of comparisons: 166510
  Selected successfully into BigNumbersC
  */

 /*Indexed
 # SELECT * FROM Numbers INTO BigNumbers WHERE key > 165000;
  Number of comparisons: 1719
  Selected successfully into BigNumbers
 */
 compareCount=0;
  return FrontendInterface::handleFrontend(argc, argv);
}
