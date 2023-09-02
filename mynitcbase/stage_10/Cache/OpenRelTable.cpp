#include "OpenRelTable.h"
#include <iostream>
#include <cstring>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable()
{

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; i++)
  {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
  }

  /************ Setting up Relation Cache entries ************/
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK);

  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

  struct RelCacheEntry *relCacheEntry = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry->relCatEntry);
  relCacheEntry->recId.block = RELCAT_BLOCK;
  relCacheEntry->recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;
  relCacheEntry->dirty = false;
  relCacheEntry->searchIndex = {-1, -1};

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = relCacheEntry;

  /**** setting up Attribute Catalog relation in the Relation Cache Table ****/

  // set up the relation cache entry for the attribute catalog similarly

  // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
  relCacheEntry = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));

  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry->relCatEntry);
  relCacheEntry->recId.block = RELCAT_BLOCK;
  relCacheEntry->recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
  relCacheEntry->dirty = false;
  relCacheEntry->searchIndex = {-1, -1};

  // set the value at RelCacheTable::relCache[ATTRCAT_RELID]
  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[ATTRCAT_RELID] = relCacheEntry;

  /************ Setting up Attribute cache entries ************/
  // (we need to populate attribute cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
  AttrCacheEntry *attrLinkedListHead = nullptr;
  AttrCacheEntry *currAttrCacheEntry;

  for (int j = 0; j < 6; j++)
  {
    if (attrLinkedListHead == nullptr)
    {
      attrLinkedListHead = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
      currAttrCacheEntry = attrLinkedListHead;
    }
    else
    {
      currAttrCacheEntry->next = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
      currAttrCacheEntry = currAttrCacheEntry->next;
    }
    attrCatBlock.getRecord(attrCatRecord, j);
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &currAttrCacheEntry->attrCatEntry);
    currAttrCacheEntry->recId.block = ATTRCAT_BLOCK;
    currAttrCacheEntry->recId.slot = j;
    currAttrCacheEntry->searchIndex = {-1, -1};
    currAttrCacheEntry->dirty = false;
  }
  AttrCacheTable::attrCache[RELCAT_RELID] = attrLinkedListHead; // head of the linked list
  attrLinkedListHead = nullptr;

  for (int j = 6; j < 12; j++)
  {
    if (attrLinkedListHead == nullptr)
    {
      attrLinkedListHead = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
      currAttrCacheEntry = attrLinkedListHead;
    }
    else
    {
      currAttrCacheEntry->next = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
      currAttrCacheEntry = currAttrCacheEntry->next;
    }
    attrCatBlock.getRecord(attrCatRecord, j);
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &currAttrCacheEntry->attrCatEntry);

    currAttrCacheEntry->recId.block = ATTRCAT_BLOCK;
    currAttrCacheEntry->recId.slot = j;
    currAttrCacheEntry->searchIndex = {-1, -1};
    currAttrCacheEntry->dirty = false;
  }
  AttrCacheTable::attrCache[ATTRCAT_RELID] = attrLinkedListHead; // head of the linked list
  attrLinkedListHead = nullptr;

  /************ Setting up tableMetaInfo entries ************/

  // in the tableMetaInfo array
  //   set free = false for RELCAT_RELID and ATTRCAT_RELID
  //   set relname for RELCAT_RELID and ATTRCAT_RELID
  tableMetaInfo[RELCAT_RELID].free = false;
  strcpy(tableMetaInfo[RELCAT_RELID].relName, RELCAT_RELNAME);

  tableMetaInfo[ATTRCAT_RELID].free = false;
  strcpy(tableMetaInfo[ATTRCAT_RELID].relName, ATTRCAT_RELNAME);
  // set rest as free
  for (int i = 2; i < MAX_OPEN; i++)
  {
    tableMetaInfo[i].free = true;
  }
}

OpenRelTable::~OpenRelTable()
{
  // close all open relations (from rel-id = 2 onwards. Why?)
  for (int i = 2; i < MAX_OPEN; ++i)
  {
    if (!tableMetaInfo[i].free)
    {
      OpenRelTable::closeRel(i); // we will implement this function later
    }
  }
  /**** Closing the catalog relations in the relation cache ****/

  // releasing the relation cache entry of the attribute catalog

  if (RelCacheTable::relCache[ATTRCAT_RELID]->dirty) /* RelCatEntry of the ATTRCAT_RELID-th RelCacheEntry has been modified */
  {

    /* Get the Relation Catalog entry from RelCacheTable::relCache
    Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
    RelCatEntry relCacheEntry = RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry;
    union Attribute record[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&relCacheEntry, record);

    // declaring an object of RecBuffer class to write back to the buffer
    RecId recId = RelCacheTable::relCache[ATTRCAT_RELID]->recId;

    RecBuffer relCatBlock(recId.block);

    // Write back to the buffer using relCatBlock.setRecord() with recId.slot
    relCatBlock.setRecord(record, recId.slot);
  }
  // free the memory dynamically allocated to this RelCacheEntry

  free(RelCacheTable::relCache[ATTRCAT_RELID]);

  // releasing the relation cache entry of the relation catalog

  if (RelCacheTable::relCache[RELCAT_RELID]->dirty) /* RelCatEntry of the RELCAT_RELID-th RelCacheEntry has been modified */
  {

    /* Get the Relation Catalog entry from RelCacheTable::relCache
    Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
    RelCatEntry relCacheEntry = RelCacheTable::relCache[RELCAT_RELID]->relCatEntry;
    union Attribute record[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&relCacheEntry, record);

    // declaring an object of RecBuffer class to write back to the buffer
    RecId recId = RelCacheTable::relCache[RELCAT_RELID]->recId;
    RecBuffer relCatBlock(recId.block);

    // Write back to the buffer using relCatBlock.setRecord() with recId.slot
    relCatBlock.setRecord(record, recId.slot);
  }
  // free the memory dynamically allocated for this RelCacheEntry
  free(RelCacheTable::relCache[RELCAT_RELID]);
}

int OpenRelTable::getFreeOpenRelTableEntry()
{

  /* traverse through the tableMetaInfo array,
    find a free entry in the Open Relation Table.*/

  for (int i = 2; i < MAX_OPEN; ++i)
  {
    if (tableMetaInfo[i].free)
    {
      return i;
    }
  }

  // if found return the relation id, else return E_CACHEFULL.
  return E_CACHEFULL;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE])
{

  /* traverse through the tableMetaInfo array,
    find the entry in the Open Relation Table corresponding to relName.*/
  for (int i = 0; i < MAX_OPEN; i++)
  {
    if (!tableMetaInfo[i].free && strcmp(tableMetaInfo[i].relName, relName) == 0)
    {
      return i;
    }
  }

  // if found return the relation id, else indicate that the relation do not
  // have an entry in the Open Relation Table.
  return E_RELNOTOPEN;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE])
{
  // let relId be used to store the  slot.
  int relId = getRelId(relName);
  /* the relation `relName` already has an entry in the Open Relation Table */
  if (relId != E_RELNOTOPEN)
  {
    // (checked using OpenRelTable::getRelId())
    // return that relation id;
    return relId;
  }

  /* find a free slot in the Open Relation Table
     using OpenRelTable::getFreeOpenRelTableEntry(). */
  relId = getFreeOpenRelTableEntry();

  if (relId == E_CACHEFULL)
  { /* free slot not available */
    return E_CACHEFULL;
  }

  /****** Setting up Relation Cache entry for the relation ******/
  RecBuffer relCatBlock(RELCAT_BLOCK);

  Attribute relCatRecord[RELCAT_NO_ATTRS];

  // get the record from the relation catalog for the relation relation name is equal to relName using linear search
  char relNameAttrConst[] = RELCAT_ATTR_RELNAME;
  Attribute relNameAttr;
  strcpy(relNameAttr.sVal, relName);
  // reset search index to -1
  RecId recId = {-1, -1};
  RelCacheTable::setSearchIndex(RELCAT_RELID, &recId);
  recId = BlockAccess::linearSearch(RELCAT_RELID, relNameAttrConst, relNameAttr, EQ);

  // if the record is not found, return E_RELNOTEXIST
  if (recId.block == -1 && recId.slot == -1)
  {
    return E_RELNOTEXIST;
  }
  // initialise BlockNumber with recId.block
  RecBuffer relCatBuffer(recId.block);
  relCatBuffer.getRecord(relCatRecord, recId.slot);

  struct RelCacheEntry *relCacheEntry = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry->relCatEntry);
  relCacheEntry->recId.block = recId.block;
  relCacheEntry->recId.slot = recId.slot;
  relCacheEntry->dirty = false;
  relCacheEntry->searchIndex = recId;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[relId] = relCacheEntry;
  int numOfAttrs = relCacheEntry->relCatEntry.numAttrs;

  /************ Setting up Attribute cache entries ************/
  // (we need to populate attribute cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
  AttrCacheEntry *attrLinkedListHead = nullptr;
  AttrCacheEntry *currAttrCacheEntry;

  recId = {-1, -1};
  RelCacheTable::setSearchIndex(ATTRCAT_RELID, &recId);
  recId = BlockAccess::linearSearch(ATTRCAT_RELID, relNameAttrConst, relNameAttr, EQ);

  for (int i = 0; i < numOfAttrs && recId.block != -1 && recId.slot != -1; i++)
  {
    if (attrLinkedListHead == nullptr)
    {
      attrLinkedListHead = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
      currAttrCacheEntry = attrLinkedListHead;
    }
    else
    {
      currAttrCacheEntry->next = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
      currAttrCacheEntry = currAttrCacheEntry->next;
    }
    RecBuffer attrCatBlock(recId.block);
    attrCatBlock.getRecord(attrCatRecord, recId.slot);
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &currAttrCacheEntry->attrCatEntry);
    currAttrCacheEntry->recId.block = recId.block;
    currAttrCacheEntry->recId.slot = recId.slot;
    currAttrCacheEntry->dirty = false;
    // currAttrCacheEntry->searchIndex = indexId;

    recId = BlockAccess::linearSearch(ATTRCAT_RELID, relNameAttrConst, relNameAttr, EQ);
  }

  AttrCacheTable::attrCache[relId] = attrLinkedListHead; // head of the linked list

  // update the relIdth entry of the tableMetaInfo with free as false and
  // relName as the input.
  tableMetaInfo[relId].free = false;
  strcpy(tableMetaInfo[relId].relName, relName);

  return relId;
}

int OpenRelTable::closeRel(int relId)
{
  /* rel-id corresponds to relation catalog or attribute catalog*/
  if (relId == RELCAT_RELID || relId == ATTRCAT_RELID)
  {
    return E_NOTPERMITTED;
  }
  /* 0 <= relId < MAX_OPEN */
  if (relId < 0 || relId >= MAX_OPEN)
  {
    return E_OUTOFBOUND;
  }
  /* rel-id corresponds to a free slot*/
  if (tableMetaInfo[relId].free)
  {
    return E_RELNOTOPEN;
  }
  /****** Releasing the Relation Cache entry of the relation ******/
  if (RelCacheTable::relCache[relId]->dirty) /* RelCatEntry of the relId-th Relation Cache entry has been modified */
  {

    /* Get the Relation Catalog entry from RelCacheTable::relCache
    Then convert it to a record using RelCacheTable::relCatEntryToRecord(). */
    RelCatEntry relCatEntry = RelCacheTable::relCache[relId]->relCatEntry;
    union Attribute record[RELCAT_NO_ATTRS];
    RelCacheTable::relCatEntryToRecord(&relCatEntry, record);
    RecId recId = RelCacheTable::relCache[relId]->recId;

    // declaring an object of RecBuffer class to write back to the buffer
    RecBuffer relCatBlock(recId.block);

    // Write back to the buffer using relCatBlock.setRecord() with recId.slot
    relCatBlock.setRecord(record, recId.slot);
  }
  /****** Releasing the Attribute Cache entry of the relation ******/

  // free the memory allocated in the relation and attribute caches which was
  // allocated in the OpenRelTable::openRel() function
  RelCacheEntry *relCacheEntry = RelCacheTable::relCache[relId];
  AttrCacheEntry *attrCacheEntry = AttrCacheTable::attrCache[relId];
  AttrCacheEntry *tempAttrCacheEntry = attrCacheEntry;
  // free all the linked list pointers
  while (tempAttrCacheEntry != nullptr)
  {
    attrCacheEntry = attrCacheEntry->next;
    free(tempAttrCacheEntry);
    tempAttrCacheEntry = attrCacheEntry;
  }

  free(relCacheEntry);
  // update `tableMetaInfo` to set `relId` as a free slot
  tableMetaInfo[relId].free = true;
  // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
  RelCacheTable::relCache[relId] = nullptr;
  AttrCacheTable::attrCache[relId] = nullptr;
   std::cout<<"Number of comparisons: "<<compareCount<<std::endl;

  return SUCCESS;
}
