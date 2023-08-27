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

/*
NOTE: This function will copy the result of the search to the `record` argument.
      The caller should ensure that space is allocated for `record` array
      based on the number of attributes in the relation.
*/
int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op)
{
    // Declare a variable called recid to store the searched record
    RecId recId;

    /* search for the record id (recid) corresponding to the attribute with
    attribute name attrName, with value attrval and satisfying the condition op
    using linearSearch() */
    recId = BlockAccess::linearSearch(relId, attrName, attrVal, op);

    // if there's no record satisfying the given condition (recId = {-1, -1})
    //    return E_NOTFOUND;
    if (recId.block == -1 && recId.slot == -1)
    {
        return E_NOTFOUND;
    }

    /* Copy the record with record id (recId) to the record buffer (record)
       For this Instantiate a RecBuffer class object using recId and
       call the appropriate method to fetch the record
    */
    RecBuffer recBuffer(recId.block);
    int response = recBuffer.getRecord(record, recId.slot);
    if (response != SUCCESS)
    {
        printf("Record not found.\n");
        exit(1);
    }

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE])
{
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    //     return E_NOTPERMITTED
    // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
    // you may use the following constants: RELCAT_NAME and ATTRCAT_NAME)
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
    {
        return E_NOTPERMITTED;
    }

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr; // (stores relName as type union Attribute)
    // assign relNameAttr.sVal = relName
    strcpy(relNameAttr.sVal, relName);

    //  linearSearch on the relation catalog for RelName = relNameAttr
    const char relcatAttrName[] = RELCAT_ATTR_RELNAME;
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, (char *) relcatAttrName, relNameAttr, EQ);

    // if the relation does not exist (linearSearch returned {-1, -1})
    //     return E_RELNOTEXIST
    if (recId.block == -1 && recId.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    /* store the relation catalog record corresponding to the relation in
       relCatEntryRecord using RecBuffer.getRecord */
    RecBuffer recBuffer(recId.block);
    int response = recBuffer.getRecord(relCatEntryRecord, recId.slot);
    if (response != SUCCESS)
    {
        printf("Record not found.\n");
        exit(1);
    }

    /* get the first record block of the relation (firstBlock) using the
       relation catalog entry record */
    int firstBlock = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;

    /* get the number of attributes corresponding to the relation (numAttrs)
       using the relation catalog entry record */
    int numAttrs = relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    /*
     Delete all the record blocks of the relation
    */

    // for each record block of the relation:
    int recBlockNum = firstBlock;
    while (recBlockNum != -1)
    {

        RecBuffer recBuffer(recBlockNum);
        //     get block header using BlockBuffer.getHeader
        HeadInfo header;
        response = recBuffer.getHeader(&header);
        //     get the next block from the header (rblock)
        recBlockNum = header.rblock;
        //     release the block using BlockBuffer.releaseBlock
        recBuffer.releaseBlock();
        //
        //     Hint: to know if we reached the end, check if nextBlock = -1
    }

    /***
        Deleting attribute catalog entries corresponding the relation and index
        blocks corresponding to the relation with relName on its attributes
    ***/


    // reset the searchIndex of the attribute catalog

    int numberOfAttributesDeleted = 0;

    while (true)
    {
        RecId attrCatRecId;
        // attrCatRecId = linearSearch on attribute catalog for RelName = relNameAttr
        const char constRelNameAttr[] = ATTRCAT_ATTR_RELNAME;
        attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *) constRelNameAttr, relNameAttr, EQ);

        // if no more attributes to iterate over (attrCatRecId == {-1, -1})
        //     break;
        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1)
        {
            break;
        }

        numberOfAttributesDeleted++;

        // create a RecBuffer for attrCatRecId.block
        RecBuffer recBuffer(attrCatRecId.block);
        // get the header of the block
        HeadInfo header;
        response = recBuffer.getHeader(&header);
        if (response != SUCCESS)
        {
            printf("Header not found.\n");
            exit(1);
        }
        // get the record corresponding to attrCatRecId.slot
        Attribute recordEntry[ATTRCAT_NO_ATTRS];
        response = recBuffer.getRecord(recordEntry, attrCatRecId.slot);
        if (response != SUCCESS)
        {
            printf("Record not found.\n");
            exit(1);
        }

        // declare variable rootBlock which will be used to store the root
        // block field from the attribute catalog record.
        /* get root block from the record */;
        int rootBlock = recordEntry[ATTRCAT_ROOT_BLOCK_INDEX].nVal;
        // (This will be used later to delete any indexes if it exists)

        // Update the Slotmap for the block by setting the slot as SLOT_UNOCCUPIED
        // Hint: use RecBuffer.getSlotMap and RecBuffer.setSlotMap
        unsigned char slotMap[header.numSlots];
        response = recBuffer.getSlotMap(slotMap);
        if (response != SUCCESS)
        {
            printf("Slotmap not found.\n");
            exit(1);
        }
        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
        response = recBuffer.setSlotMap(slotMap);
        if (response != SUCCESS)
        {
            printf("Slotmap not saved successfully.\n");
            exit(1);
        }

        /* Decrement the numEntries in the header of the block corresponding to
           the attribute catalog entry and then set back the header
           using RecBuffer.setHeader */
        header.numEntries--;
        response = recBuffer.setHeader(&header);
        if (response != SUCCESS)
        {
            printf("Header not saved successfully.\n");
            exit(1);
        }

        /* If number of entries become 0, releaseBlock is called after fixing
           the linked list.
        */
        if (header.numSlots==0)
        {
            /* Standard Linked List Delete for a Block
               Get the header of the left block and set it's rblock to this
               block's rblock
            */
            HeadInfo leftBlockHeader;
           
            // create a RecBuffer for lblock and call appropriate methods
            RecBuffer leftBlockRecBuffer(header.lblock);
            response = leftBlockRecBuffer.getHeader(&leftBlockHeader);
            
           


            if (header.rblock != -1)/* header.rblock != -1 */
            {
                /* Get the header of the right block and set it's lblock to
                   this block's lblock */
                HeadInfo rightBlockHeader;
                // create a RecBuffer for rblock and call appropriate methods
                RecBuffer rightBlockRecBuffer(header.rblock);
                response = rightBlockRecBuffer.getHeader(&rightBlockHeader);
                if (response != SUCCESS)
                {
                    printf("Header not found.\n");
                    exit(1);
                }
            }
            else
            {
                // (the block being released is the "Last Block" of the relation.)
                /* update the Relation Catalog entry's LastBlock field for this
                   relation with the block number of the previous block. */
                // (use RelCacheTable::getRelCatEntry() and
                //  RelCacheTable::setRelCatEntry() functions)
                RelCatEntry relCatEntry;
                response = RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);
                if (response != SUCCESS)
                {
                    printf("Failed to get relation catalogue entry.\n");
                    exit(1);
                }
                relCatEntry.lastBlk = header.lblock;
                response = RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntry);
                if (response != SUCCESS)
                {
                    printf("Failed to set relation catalogue entry.\n");
                    exit(1);
                }

                
            }

            // (Since the attribute catalog will never be empty(why?), we do not
            //  need to handle the case of the linked list becoming empty - i.e
            //  every block of the attribute catalog gets released.)

            // call releaseBlock()
            recBuffer.releaseBlock();
        }

        // (the following part is only relevant once indexing has been implemented)
        // if index exists for the attribute (rootBlock != -1), call bplus destroy
        // if (rootBlock != -1)
        // {
            // delete the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
        // }
    }

    /*** Delete the entry corresponding to the relation from relation catalog ***/
    // Fetch the header of Relcat block
    RecBuffer relCatRecBuffer(RELCAT_BLOCK);
    HeadInfo relCatHeader;
    response = relCatRecBuffer.getHeader(&relCatHeader);

    /* Decrement the numEntries in the header of the block corresponding to the
       relation catalog entry and set it back */
    relCatHeader.numEntries--;

    /* Get the slotmap in relation catalog, update it by marking the slot as
       free(SLOT_UNOCCUPIED) and set it back. */
    unsigned char relCatSlotMap[relCatHeader.numSlots];
    response = relCatRecBuffer.getSlotMap(relCatSlotMap);
    if (response != SUCCESS)
    {
        printf("Slotmap not found.\n");
        exit(1);
    }
    relCatSlotMap[recId.slot] = SLOT_UNOCCUPIED;
    response = relCatRecBuffer.setSlotMap(relCatSlotMap);
    if (response != SUCCESS)
    {
        printf("Slotmap not saved successfully.\n");
        exit(1);
    }


    /*** Updating the Relation Cache Table ***/

    /** Update relation catalog record entry (number of records in relation
        catalog is decreased by 1) **/
    // Get the entry corresponding to relation catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)
    RelCatEntry relCatEntry;
    response = RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);
    if (response != SUCCESS)
    {
        printf("Failed to get relation catalogue entry.\n");
        exit(1);
    }
    relCatEntry.numRecs--;
    response = RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);
    if (response != SUCCESS)
    {
        printf("Failed to set relation catalogue entry.\n");
        exit(1);
    }

    /** Update attribute catalog entry (number of records in attribute catalog
        is decreased by numberOfAttributesDeleted) **/
    // i.e., #Records = #Records - numberOfAttributesDeleted
    // Get the entry corresponding to attribute catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)
    response = RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);
    if (response != SUCCESS)
    {
        printf("Failed to get relation catalogue entry.\n");
        exit(1);
    }
    relCatEntry.numRecs -= numberOfAttributesDeleted;
    response = RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntry);
    if (response != SUCCESS)
    {
        printf("Failed to set relation catalogue entry.\n");
        exit(1);
    }

    return SUCCESS;
}

/*
NOTE: the caller is expected to allocate space for the argument `record` based
      on the size of the relation. This function will only copy the result of
      the projection onto the array pointed to by the argument.
*/
int BlockAccess::project(int relId, Attribute *record) {
    // get the previous search index of the relation relId from the relation    
    // cache (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    int response = RelCacheTable::getSearchIndex(relId, &prevRecId);

    // declare block and slot which will be used to store the record id of the
    // slot we need to check.
    int block=prevRecId.block, slot=prevRecId.slot;

    /* if the current search index record is invalid(i.e. = {-1, -1})
       (this only happens when the caller reset the search index)
    */
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (new project operation. start from beginning)


        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        RelCatEntry relCatEntry;
        response = RelCacheTable::getRelCatEntry(relId, &relCatEntry);  
        if (response != SUCCESS)
        {
            printf("Failed to get relation catalogue entry.\n");
            exit(1);
        }

        // block = first record block of the relation
        block = relCatEntry.firstBlk;
        // slot = 0
        slot = 0;
    }
    else
    {
        // (a project/search operation is already in progress)

        // block = previous search index's block
        block = prevRecId.block;
        // slot = previous search index's slot + 1
        slot = prevRecId.slot + 1;
    }


    // The following code finds the next record of the relation
    /* Start from the record id (block, slot) and iterate over the remaining
       records of the relation */
    while (block != -1)
    {
        // create a RecBuffer object for block (using appropriate constructor!)
        RecBuffer recBuffer(block);

        // get header of the block using RecBuffer::getHeader() function
        struct HeadInfo head;
        response = recBuffer.getHeader(&head);
        // get slot map of the block using RecBuffer::getSlotMap() function
        unsigned char slotMap[head.numSlots];
        response = recBuffer.getSlotMap(slotMap);

        /* slot >= the number of slots per block*/
        if(slot >= head.numSlots)
        {
            // (no more slots in this block)
            // update block = right block of block
            block = head.rblock;
            // update slot = 0
            slot = 0;
            // (NOTE: if this is the last block, rblock would be -1. this would
            //        set block = -1 and fail the loop condition )
        }
        else if (slotMap[slot] == SLOT_UNOCCUPIED)/* slot is free */
        { // (i.e slot-th entry in slotMap contains SLOT_UNOCCUPIED)
            // increment slot
            slot++;
        }
        else {
            // (the next occupied slot / record has been found)
            break;
        }
    }

    if (block == -1){
        // (a record was not found. all records exhausted)
        return E_NOTFOUND;
    }

    // declare nextRecId to store the RecId of the record found
    RecId nextRecId={block, slot};

    // set the search index to nextRecId using RelCacheTable::setSearchIndex
    response = RelCacheTable::setSearchIndex(relId, &nextRecId);


    /* Copy the record with record id (nextRecId) to the record buffer (record)
       For this Instantiate a RecBuffer class object by passing the recId and
       call the appropriate method to fetch the record
    */ 
    RecBuffer recBuffer(nextRecId.block);
    response = recBuffer.getRecord(record, nextRecId.slot);
    if (response != SUCCESS)
    {
        printf("Record not found.\n");
        exit(1);
    }




    return SUCCESS;
}