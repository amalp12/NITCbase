#include "../Buffer/StaticBuffer.h"
#include "../Cache/OpenRelTable.h"
#include "../Disk_Class/Disk.h"
#include "../FrontendInterface/FrontendInterface.h"
#include <iostream>
int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;

  // unsigned char has size 1 byte
  // unsigned char buffer[BLOCK_SIZE];//BLOCK_SIZE is a constant that has value 2048
  

  //  our modified buffer has been written to the disk. Let's try reading this back from the disk into a new buffer to see if our changes have been made.
  unsigned char buffer2[BLOCK_SIZE];
  Disk::readBlock(buffer2, 0);
  for(int i=0;i<BLOCK_SIZE;i++){

    std::cout << (int)buffer2[i];
  }
  // StaticBuffer buffer;
  // OpenRelTable cache;
  return 0;
}