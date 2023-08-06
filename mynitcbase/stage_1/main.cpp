#include "../Buffer/StaticBuffer.h"
#include "../Cache/OpenRelTable.h"
#include "../Disk_Class/Disk.h"
#include "../FrontendInterface/FrontendInterface.h"
#include <iostream>
int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;

  // unsigned char has size 1 byte
  unsigned char buffer[BLOCK_SIZE];//BLOCK_SIZE is a constant that has value 2048
  
  Disk::readBlock(buffer, 7000);// 7000 is a random block number that's unused.
  
  char message[] = "hello";
  memcpy(buffer + 20, message, 6); // We'll be using the memcpy function provided in the cstring header for this. The function takes three arguments; the destination pointer, the source pointer, and the number of bytes. It copies the required number of bytes from the source to the destination.
  //Now, buffer[20] = 'h', buffer[21] = 'e' ...
  Disk::writeBlock(buffer, 7000);

  //  our modified buffer has been written to the disk. Let's try reading this back from the disk into a new buffer to see if our changes have been made.
  unsigned char buffer2[BLOCK_SIZE];
  char message2[6];
  Disk::readBlock(buffer2, 7000);
  memcpy(message2, buffer2 + 20, 6);
  std::cout << message2;
  // StaticBuffer buffer;
  // OpenRelTable cache;

  return FrontendInterface::handleFrontend(argc, argv);
}