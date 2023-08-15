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

  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  /**** setting up Attribute Catalog relation in the Relation Cache Table ****/

  // set up the relation cache entry for the attribute catalog similarly

  // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);

  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

  // set the value at RelCacheTable::relCache[ATTRCAT_RELID]
  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;

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
  for(int i=2;i<MAX_OPEN;i++){
    tableMetaInfo[i].free=true;
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

  // free the memory allocated for rel-id 0 and 1 in the caches
  free(RelCacheTable::relCache[RELCAT_RELID]);
  free(RelCacheTable::relCache[ATTRCAT_RELID]);
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
    if (strcmp(tableMetaInfo[i].relName, relName) == 0)
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

  /* search for the entry with relation name, relName, in the Relation Catalog using
      BlockAccess::linearSearch().
      Care should be taken to reset the searchIndex of the relation RELCAT_RELID
      before calling linearSearch().*/
  char relNameConstAttr[]=RELCAT_ATTR_RELNAME;
  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
  RecId relcatRecId = {-1, -1};
  // reset the searchIndex of the relation relId
  RelCacheTable::setSearchIndex(relId, &relcatRecId);
  RelCacheTable::setSearchIndex(RELCAT_RELID, &relcatRecId);
  // search for the entry with relation name, relName, in the Relation Catalog
  // create Attribute with value relName
  Attribute relNameAttr;
  strcpy(relNameAttr.sVal, relName);
  
  relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, relNameConstAttr, relNameAttr, EQ);

  if (relcatRecId.block == -1 && relcatRecId.slot == -1)
  { /* relcatRecId == {-1, -1} */
    // (the relation is not found in the Relation Catalog.)
    return E_RELNOTEXIST;
  }

  /* read the record entry corresponding to relcatRecId and create a relCacheEntry
      on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
      update the recId field of this Relation Cache entry to relcatRecId.
      use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
    NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  */
  RelCacheEntry *relCacheEntry = (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  RecBuffer relCatBlock(RELCAT_BLOCK);
  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, relcatRecId.slot);

  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry->relCatEntry);
  relCacheEntry->recId.block = relcatRecId.block;
  relCacheEntry->recId.slot = relcatRecId.slot;

  int numAttrs = relCacheEntry->relCatEntry.numAttrs;

  /****** Setting up Attribute Cache entry for the relation ******/
  // let listHead be used to hold the head of the linked list of attrCache entries.
  AttrCacheEntry *attrLinkedListHead = nullptr, *currAttrCacheEntry;

  /*iterate over all the entries in the Attribute Catalog corresponding to each
  attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
  care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
  corresponding to Attribute Catalog before the first call to linearSearch().*/
  RecId attrcatRecId = {-1, -1};
  RecBuffer attrCatBlock(relCacheEntry->recId.block);
  Attribute attrCatRecord[numAttrs];

  RelCacheTable::setSearchIndex(relId, &attrcatRecId);
  RelCacheTable::setSearchIndex(ATTRCAT_RELID, &attrcatRecId);
  attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, relNameConstAttr, relNameAttr, EQ);
  while (attrcatRecId.block != 1 && attrcatRecId.slot != -1)
  {
    /* let attrcatRecId store a valid record id an entry of the relation, relName,
    in the Attribute Catalog.*/

    /* read the record entry corresponding to attrcatRecId and create an
    Attribute Cache entry on it using RecBuffer::getRecord() and
    AttrCacheTable::recordToAttrCatEntry().
    update the recId field of this Attribute Cache entry to attrcatRecId.
    add the Attribute Cache entry to the linked list of listHead .*/
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
    attrCatBlock.getRecord(attrCatRecord, attrcatRecId.slot);
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &currAttrCacheEntry->attrCatEntry);
    currAttrCacheEntry->recId.block = attrcatRecId.block;
    currAttrCacheEntry->recId.slot = attrcatRecId.slot;
    // NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
    attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, relNameConstAttr, relNameAttr, EQ);
  }
  // set the relIdth entry of the AttrCacheTable to listHead.
  AttrCacheTable::attrCache[relId] = attrLinkedListHead; // head of the linked list

  /****** Setting up metadata in the Open Relation Table for the relation******/

  // update the relIdth entry of the tableMetaInfo with free as false and
  // relName as the input.
  tableMetaInfo[relId].free = false;
  strcpy(tableMetaInfo[relId].relName, relName);

  return relId;
}


int OpenRelTable::closeRel(int relId) {
  /* rel-id corresponds to relation catalog or attribute catalog*/
  if (relId==RELCAT_RELID || relId==ATTRCAT_RELID) {
    return E_NOTPERMITTED;
  }
  /* 0 <= relId < MAX_OPEN */
  if (relId < 0 || relId >= MAX_OPEN) {
    return E_OUTOFBOUND;
  }
  /* rel-id corresponds to a free slot*/
  if (tableMetaInfo[relId].free) {
    return E_RELNOTOPEN;
  }

  // free the memory allocated in the relation and attribute caches which was
  // allocated in the OpenRelTable::openRel() function
  RelCacheEntry *relCacheEntry = RelCacheTable::relCache[relId];
  AttrCacheEntry *attrCacheEntry = AttrCacheTable::attrCache[relId];
  RelCacheEntry *tempRelCacheEntry;
  AttrCacheEntry *tempAttrCacheEntry;
  while (tempAttrCacheEntry != nullptr)
  {
    tempAttrCacheEntry = attrCacheEntry;
    attrCacheEntry = attrCacheEntry->next;
    free(tempAttrCacheEntry);
  }
  
  free(relCacheEntry);
  free(attrCacheEntry);

  // update `tableMetaInfo` to set `relId` as a free slot
  tableMetaInfo[relId].free = true;
  // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
  RelCacheTable::relCache[relId] = nullptr;
  AttrCacheTable::attrCache[relId] = nullptr;

  return SUCCESS;
}
