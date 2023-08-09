#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>

int getNumberOfCatalogEntries(int blockNum)
{
  int currBlockNum = blockNum;
  int count = 0;
  unsigned char buffer[BLOCK_SIZE];
  const int catalogNumberOfEntriesStart = 16;
  const int catalogRightBlockStart = 12;
  while (currBlockNum != -1)
  {
    Disk::readBlock(buffer, blockNum);
    count += (*(int32_t *)(buffer + catalogNumberOfEntriesStart * sizeof(unsigned char)));
    currBlockNum = (*(int32_t *)(buffer + catalogRightBlockStart * sizeof(unsigned char)));
  }
  return count;
}

int main(int argc, char *argv[])
{
  Disk disk_run;

  // create objects for the relation catalog and attribute catalog
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;

  // load the headers of both the blocks into relCatHeader and attrCatHeader.
  // (we will implement these functions later)
  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);

  int relationCount = getNumberOfCatalogEntries(RELCAT_BLOCK);
  int attributeCount = getNumberOfCatalogEntries(ATTRCAT_BLOCK);
  for (int i = 0; i < relationCount; i++)
  { /* i = 0 to total relation count */

    Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    for (int j = 0; j < attributeCount; j++)
    { /* j = 0 to number of entries in the attribute catalog */

      // declare attrCatRecord and load the attribute catalog entry into it
      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
      attrCatBuffer.getRecord(attrCatRecord, j);

      if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0)
      { /* attribute catalog entry corresponds to the current relation */
        const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
        printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
      }
    }
    printf("\n");
  }

  return 0;
}