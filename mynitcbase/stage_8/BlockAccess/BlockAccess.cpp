#include "BlockAccess.h"

#include <cstring>
#include <iostream>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op)
{
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    int response;
    RecId prevRecId;
    response = RelCacheTable::getSearchIndex(relId, &prevRecId);
    // if the response is not SUCCESS, return the response
    // if (response != SUCCESS){
    //     printf("Invalid Search Index.\n");
    //     exit(1);
    // }

    RelCatEntry relCatEntry;
    response = RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    if (response != SUCCESS)
    {
        printf("Relation Catalogue Entry Not found.\n");
        exit(1);
    }
    // get the relation catalog entry using RelCacheTable::getRelCatEntry()

    // let block and slot denote the record id of the record being currently checked
    int block = -1, slot = -1;
    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)

        // block = first record block of the relation
        block = relCatEntry.firstBlk;
        // slot = 0
        slot = 0;
    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)

        // block = search index's block
        if (slot >= relCatEntry.numAttrs)
        {
            HeadInfo head;
            RecBuffer recBuffer(prevRecId.block);
            response = recBuffer.getHeader(&head);
            block = head.rblock;
            slot = 0;
            if (block == -1)
            {
                block = relCatEntry.firstBlk;
            }
        }
        else
        {
            block = prevRecId.block;
            // slot = search index's slot + 1
            slot = prevRecId.slot + 1;
        }
    }

    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */
    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */
        RecBuffer recBuffer(block);

        // get the record with id (block, slot) using RecBuffer::getRecord()
        Attribute recordEntry[relCatEntry.numAttrs];
        response = recBuffer.getRecord(recordEntry, slot);

        if (response != SUCCESS)
        {
            printf("Record not found.\n");
            exit(1);
        }

        // get header of the block using RecBuffer::getHeader() function
        struct HeadInfo head;
        response = recBuffer.getHeader(&head);
        if (response != SUCCESS)
        {
            printf("Header not found.\n");
            exit(1);
        }
        // get slot map of the block using RecBuffer::getSlotMap() function
        unsigned char slotMap[head.numSlots];
        response = recBuffer.getSlotMap(slotMap);
        if (response != SUCCESS)
        {
            printf("Slotmap not found.\n");
            exit(1);
        }

        // If slot >= the number of slots per block(i.e. no more slots in this block)
        if (slot >= head.numSlots)
        {
            // update block = right block of block
            block = head.rblock;
            // update slot = 0
            slot = 0;
            continue;
        }

        // if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        if (slotMap[slot] == SLOT_UNOCCUPIED)
        {
            // increment slot and continue to the next record slot
            slot++;
            continue;
        }

        // compare record's attribute value to the the given attrVal as below:
        /*
            firstly get the attribute offset for the attrName attribute
            from the attribute cache entry of the relation using
            AttrCacheTable::getAttrCatEntry()
        */
        AttrCatEntry attrCatEntry;
        response = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
        if (response != SUCCESS)
        {
            // TODO Make it linked list if needed
            printf("Attribute Catalogue Entry Not found.\n");
            exit(1);
        }
        // get the attribute offset

        /* use the attribute offset to get the value of the attribute from
           current record */
        Attribute currRecordAttr = recordEntry[attrCatEntry.offset];

        int cmpVal; // will store the difference between the attributes
        // set cmpVal using compareAttrs()
        cmpVal = compareAttrs(currRecordAttr, attrVal, attrCatEntry.attrType);

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) || // if op is "not equal to"
            (op == LT && cmpVal < 0) ||  // if op is "less than"
            (op == LE && cmpVal <= 0) || // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) || // if op is "equal to"
            (op == GT && cmpVal > 0) ||  // if op is "greater than"
            (op == GE && cmpVal >= 0)    // if op is "greater than or equal to"
        )
        {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
            RecId recId = {block, slot};
            RelCacheTable::setSearchIndex(relId, &recId);

            return recId;
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName; // set newRelationName with newName
    strcpy(newRelationName.sVal, newName);
    char relcatAttrName[] = RELCAT_ATTR_RELNAME;
    // search the relation catalog for an entry with "RelName" = newRelationName
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, newRelationName, EQ);

    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;
    if (recId.block != -1 && recId.slot != -1)
    {
        return E_RELEXIST;
    }

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName; // set oldRelationName with oldName
    strcpy(oldRelationName.sVal, oldName);

    // search the relation catalog for an entry with "RelName" = oldRelationName
    recId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, oldRelationName, EQ);

    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    //    return E_RELNOTEXIST;
    if (recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    RecBuffer recBuffer(RELCAT_BLOCK);
    Attribute recordEntry[RELCAT_NO_ATTRS];
    int response = recBuffer.getRecord(recordEntry, recId.slot);
    if (response != SUCCESS)
    {
        printf("Record not found.\n");
        exit(1);
    }
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    strcpy(recordEntry[RELCAT_REL_NAME_INDEX].sVal, newName);
    // set back the record value using RecBuffer.setRecord
    response = recBuffer.setRecord(recordEntry, recId.slot);
    if (response != SUCCESS)
    {
        printf("Record not saved successfully.\n");
        exit(1);
    }

    /*
    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    int numberOfAttributes = recordEntry[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    // for i = 0 to numberOfAttributes :
    //     linearSearch on the attribute catalog for relName = oldRelationName
    //     get the record using RecBuffer.getRecord
    //
    //     update the relName field in the record to newName
    //     set back the record using RecBuffer.setRecord
    char attrcatAttrName[] = ATTRCAT_ATTR_RELNAME;
    for (int i = 0; i < numberOfAttributes; i++)
    {
        recId = BlockAccess::linearSearch(ATTRCAT_RELID, attrcatAttrName, oldRelationName, EQ);
        if (recId.block == -1 && recId.slot == -1)
        {
            return E_RELNOTEXIST;
        }
        recBuffer = RecBuffer(recId.block);
        response = recBuffer.getRecord(recordEntry, recId.slot);
        if (response != SUCCESS)
        {
            printf("Record not found.\n");
            exit(1);
        }
        strcpy(recordEntry[ATTRCAT_REL_NAME_INDEX].sVal, newName);
        response = recBuffer.setRecord(recordEntry, recId.slot);
        if (response != SUCCESS)
        {
            printf("Record not saved successfully.\n");
            exit(1);
        }
    }

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr; // set relNameAttr to relName
    strcpy(relNameAttr.sVal, relName);

    // Search for the relation with name relName in relation catalog using linearSearch()
    // If relation with name relName does not exist (search returns {-1,-1})
    //    return E_RELNOTEXIST;
    char relcatAttrName[] = RELCAT_ATTR_RELNAME;
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, relNameAttr, EQ);
    if (recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    /* declare variable attrToRenameRecId used to store the attr-cat recId
    of the attribute to rename */
    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    char attrcatAttrName[] = ATTRCAT_ATTR_RELNAME;
    while (true)
    {
        // linear search on the attribute catalog for RelName = relNameAttr
        recId = BlockAccess::linearSearch(ATTRCAT_RELID, attrcatAttrName, relNameAttr, EQ);

        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        //     break;
        if (recId.block == -1 && recId.slot == -1)
        {
            break;
        }

        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
        RecBuffer recBuffer(recId.block);
        int response = recBuffer.getRecord(attrCatEntryRecord, recId.slot);

        if (response != SUCCESS)
        {
            printf("Record not found.\n");
            exit(1);
        }

        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0)
        {
            attrToRenameRecId = recId;
            break;
        }

        // if attrCatEntryRecord.attrName = newName
        //     return E_ATTREXIST;
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0)
        {
            return E_ATTREXIST;
        }
    }

    // if attrToRenameRecId == {-1, -1}
    //     return E_ATTRNOTEXIST;
    if (attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1)
    {
        return E_ATTRNOTEXIST;
    }

    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    RecBuffer recBuffer(attrToRenameRecId.block);
    int response = recBuffer.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);
    if (response != SUCCESS)
    {
        printf("Record not found.\n");
        exit(1);
    }
    //   update the AttrName of the record with newName
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    //   set back the record with RecBuffer.setRecord
    response = recBuffer.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record)
{
    // get the relation catalog entry from relation cache
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)
    RelCatEntry relCatEntry;
    int response = RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    if (response != SUCCESS)
    {
        printf("Failed to get relation catalogue entry.\n");
        exit(1);
    }
    /* first record block of the relation (from the rel-cat entry)*/;
    int blockNum = relCatEntry.firstBlk;

    // recId will be used to store where the new record will be inserted
    RecId recId = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk; /* number of slots per record block */
    int numOfAttributes = relCatEntry.numAttrs   /* number of attributes of the relation */

        /* block number of the last element in the linked list = -1 */;
    int prevBlockNum = -1;

    /*
        Traversing the linked list of existing record blocks of the relation
        until a free slot is found OR
        until the end of the list is reached
    */
    while (blockNum != -1)
    {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
        RecBuffer recBuffer(blockNum);
        // get header of block(blockNum) using RecBuffer::getHeader() function
        struct HeadInfo head;
        response = recBuffer.getHeader(&head);

        if (response != SUCCESS)
        {
            printf("Header not found.\n");
            exit(1);
        }

        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
        unsigned char slotMap[head.numSlots];
        response = recBuffer.getSlotMap(slotMap);

        if (response != SUCCESS)
        {
            printf("Slotmap not found.\n");
            exit(1);
        }

        // search for free slot in the block 'blockNum' and store it's rec-id in recId

        // (Free slot can be found by iterating over the slot map of the block)
        /* slot map stores SLOT_UNOCCUPIED if slot is free and
           SLOT_OCCUPIED if slot is occupied) */
        for (int i = 0; i < head.numSlots; i++)
        {
            if (slotMap[i] == SLOT_UNOCCUPIED)
            {
                recId.block = blockNum;
                recId.slot = i;
                break;
            }
        }

        /* if a free slot is found, set rec_id and discontinue the traversal
           of the linked list of record blocks (break from the loop) */
        if (recId.block != -1 && recId.slot != -1)
        {
            break;
        }

        /* otherwise, continue to check the next block by updating the
           block numbers as follows:
              update prevBlockNum = blockNum
              update blockNum = header.rblock (next element in the linked
                                               list of record blocks)
        */
        prevBlockNum = blockNum;
        blockNum = head.rblock;
    }

    //  if no free slot is found in existing record blocks (rec_id = {-1, -1})
    if (recId.block == -1 && recId.slot == -1)
    {
        // if relation is RELCAT, do not allocate any more blocks
        //     return E_MAXRELATIONS;
        if (relId == RELCAT_RELID)
        {
            return E_MAXRELATIONS;
        }

        // Otherwise,
        // get a new record block (using the appropriate RecBuffer constructor!)
        RecBuffer recBuffer;

        // get the block number of the newly allocated block
        // (use BlockBuffer::getBlockNum() function)
        // let ret be the return value of getBlockNum() function call
        int newBlockNum = recBuffer.getBlockNum();
        if (newBlockNum == E_DISKFULL)
        {
            return E_DISKFULL;
        }

        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
        recId.block = newBlockNum;
        recId.slot = 0;

        /*
            set the header of the new record block such that it links with
            existing record blocks of the relation
            set the block's header as follows:
            blockType: REC, pblock: -1
            lblock
                  = -1 (if linked list of existing record blocks was empty
                         i.e this is the first insertion into the relation)
                  = prevBlockNum (otherwise),
            rblock: -1, numEntries: 0,
            numSlots: numOfSlots, numAttrs: numOfAttributes
            (use BlockBuffer::setHeader() function)
        */
        HeadInfo head;
        response = recBuffer.getHeader(&head);
        if (response != SUCCESS)
        {
            printf("Header not found.\n");
            exit(1);
        }
        head.blockType = REC;
        head.pblock = -1;
        head.lblock = prevBlockNum;
        head.rblock = -1;
        head.numEntries = 0;
        head.numSlots = numOfSlots;
        head.numAttrs = numOfAttributes;
        response = recBuffer.setHeader(&head);
        if (response != SUCCESS)
        {
            printf("Header not saved successfully.\n");
            exit(1);
        }

        /*
            set block's slot map with all slots marked as free
            (i.e. store SLOT_UNOCCUPIED for all the entries)
            (use RecBuffer::setSlotMap() function)
        */
        unsigned char slotMap[head.numSlots];
        for (int i = 0; i < head.numSlots; i++)
        {
            slotMap[i] = SLOT_UNOCCUPIED;
        }
        response = recBuffer.setSlotMap(slotMap);
        if (response != SUCCESS)
        {
            printf("Slotmap not saved successfully.\n");
            exit(1);
        }

        // if prevBlockNum != -1
        if (prevBlockNum != -1)
        {
            // create a RecBuffer object for prevBlockNum
            RecBuffer recBuffer(prevBlockNum);
            // get the header of the block prevBlockNum and
            HeadInfo prevBlockHeader;
            response = recBuffer.getHeader(&prevBlockHeader);
            if (response != SUCCESS)
            {
                printf("Header not found.\n");
                exit(1);
            }
            // update the rblock field of the header to the new block
            prevBlockHeader.rblock = recId.block; // newBlockNum
            // number i.e. rec_id.block
            // (use BlockBuffer::setHeader() function)
            response = recBuffer.setHeader(&prevBlockHeader);
            if (response != SUCCESS)
            {
                printf("Header not saved successfully.\n");
                exit(1);
            }
        }
        // else
        else
        {
            // update first block field in the relation catalog entry to the
            // new block (using RelCacheTable::setRelCatEntry() function)
            relCatEntry.firstBlk = recId.block;
        }

        // update last block field in the relation catalog entry to the
        // new block (using RelCacheTable::setRelCatEntry() function)
        relCatEntry.lastBlk = recId.block;
        response = RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        if (response != SUCCESS)
        {
            printf("Failed to set relation catalogue entry.\n");
            exit(1);
        }
    }

    // create a RecBuffer object for rec_id.block
    RecBuffer recBuffer(recId.block);
    // insert the record into rec_id'th slot using RecBuffer.setRecord())
    response = recBuffer.setRecord(record, recId.slot);

    if (response != SUCCESS)
    {
        printf("Record not saved successfully.\n");
        exit(1);
    }

    /* update the slot map of the block by marking entry of the slot to
       which record was inserted as occupied) */

    // (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
    // (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)
    HeadInfo head;
    response = recBuffer.getHeader(&head);
    if (response != SUCCESS)
    {
        printf("Header not found.\n");
        exit(1);
    }
    unsigned char slotMap[head.numSlots];
    response = recBuffer.getSlotMap(slotMap);
    if (response != SUCCESS)
    {
        printf("Slotmap not found.\n");
        exit(1);
    }
    slotMap[recId.slot] = SLOT_OCCUPIED;

    // set slotmap
    response = recBuffer.setSlotMap(slotMap);
    if (response != SUCCESS)
    {
        printf("Slotmap not saved successfully.\n");
        exit(1);
    }

    // increment the numEntries field in the header of the block to which record was inserted
    head.numEntries++;
    // (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
    response = recBuffer.setHeader(&head);
    if (response != SUCCESS)
    {
        printf("Header not saved successfully.\n");
        exit(1);
    }
    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
    relCatEntry.numRecs++;
    response = RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;
}