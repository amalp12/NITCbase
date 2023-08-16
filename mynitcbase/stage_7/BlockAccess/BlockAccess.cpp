#include "BlockAccess.h"

#include <cstring>
#include <iostream>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
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
    if(response != SUCCESS){
        printf("Relation Catalogue Entry Not found.\n");
        exit(1);
    }
    // get the relation catalog entry using RelCacheTable::getRelCatEntry()

    // let block and slot denote the record id of the record being currently checked
    int block=-1, slot=-1;
    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1){
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
        if(slot>=relCatEntry.numAttrs){
            HeadInfo head;
            RecBuffer recBuffer(prevRecId.block);
            response = recBuffer.getHeader(&head);
            block = head.rblock;
            slot = 0;
            if(block==-1){
                block=relCatEntry.firstBlk;
            }

        }
        else{
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

        if (response != SUCCESS){
            printf("Record not found.\n");
            exit(1);
        }

        // get header of the block using RecBuffer::getHeader() function
        struct HeadInfo head;
        response = recBuffer.getHeader(&head);
        if(response != SUCCESS){
            printf("Header not found.\n");
            exit(1);
        }
        // get slot map of the block using RecBuffer::getSlotMap() function
        unsigned char slotMap[head.numSlots];
        response = recBuffer.getSlotMap(slotMap);
        if(response != SUCCESS){
            printf("Slotmap not found.\n");
            exit(1);
        }


        // If slot >= the number of slots per block(i.e. no more slots in this block)
        if(slot >= head.numSlots){
            // update block = right block of block
            block = head.rblock;
            // update slot = 0
            slot = 0;
            continue;
        }

        // if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        if(slotMap[slot] == SLOT_UNOCCUPIED){
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
        if(response != SUCCESS){
            // TODO Make it linked list if needed
            printf("Attribute Catalogue Entry Not found.\n");
            exit(1);
        }
        // get the attribute offset
    
        /* use the attribute offset to get the value of the attribute from
           current record */
        Attribute currRecordAttr = recordEntry[attrCatEntry.offset];

        int cmpVal;  // will store the difference between the attributes
        // set cmpVal using compareAttrs()
        cmpVal = compareAttrs(currRecordAttr, attrVal, attrCatEntry.attrType);

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
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

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]){
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;    // set newRelationName with newName
    strcpy(newRelationName.sVal, newName);
    char relcatAttrName[]=RELCAT_ATTR_RELNAME;
    // search the relation catalog for an entry with "RelName" = newRelationName
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, newRelationName, EQ);

    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;
    if(recId.block != -1 && recId.slot != -1){
        return E_RELEXIST;
    }


    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;    // set oldRelationName with oldName
    strcpy(oldRelationName.sVal, oldName);

    // search the relation catalog for an entry with "RelName" = oldRelationName
    recId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, oldRelationName, EQ);

    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    //    return E_RELNOTEXIST;
    if(recId.block == -1 && recId.slot == -1){
        return E_RELNOTEXIST;
    }

    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    RecBuffer recBuffer(RELCAT_BLOCK);
    Attribute recordEntry[RELCAT_NO_ATTRS];
    int response = recBuffer.getRecord(recordEntry, recId.slot);
    if(response != SUCCESS){
        printf("Record not found.\n");
        exit(1);
    }
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    strcpy(recordEntry[RELCAT_REL_NAME_INDEX].sVal, newName);
    // set back the record value using RecBuffer.setRecord
    response = recBuffer.setRecord(recordEntry, recId.slot);
    if(response != SUCCESS){
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
    int numberOfAttributes=recordEntry[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    //for i = 0 to numberOfAttributes :
    //    linearSearch on the attribute catalog for relName = oldRelationName
    //    get the record using RecBuffer.getRecord
    //
    //    update the relName field in the record to newName
    //    set back the record using RecBuffer.setRecord
    char attrcatAttrName[]=ATTRCAT_ATTR_RELNAME;
    for(int i=0;i<numberOfAttributes;i++){
        recId = BlockAccess::linearSearch(ATTRCAT_RELID, attrcatAttrName, oldRelationName, EQ);
        if(recId.block == -1 && recId.slot == -1){
            return E_RELNOTEXIST;
        }
        recBuffer = RecBuffer(recId.block);
        response = recBuffer.getRecord(recordEntry, recId.slot);
        if(response != SUCCESS){
            printf("Record not found.\n");
            exit(1);
        }
        strcpy(recordEntry[ATTRCAT_REL_NAME_INDEX].sVal, newName);
        response = recBuffer.setRecord(recordEntry, recId.slot);
        if(response != SUCCESS){
            printf("Record not saved successfully.\n");
            exit(1);
        }

    }

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;    // set relNameAttr to relName
    strcpy(relNameAttr.sVal, relName);

    // Search for the relation with name relName in relation catalog using linearSearch()
    // If relation with name relName does not exist (search returns {-1,-1})
    //    return E_RELNOTEXIST;
    char relcatAttrName[]=RELCAT_ATTR_RELNAME;
    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, relcatAttrName, relNameAttr, EQ);    
    if(recId.block == -1 && recId.slot == -1){
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
    char attrcatAttrName[]=ATTRCAT_ATTR_RELNAME;
    while (true) {
        // linear search on the attribute catalog for RelName = relNameAttr
        recId = BlockAccess::linearSearch(ATTRCAT_RELID, attrcatAttrName, relNameAttr, EQ);

        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        //     break;
        if(recId.block == -1 && recId.slot == -1){
            break;
        }

        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */
        RecBuffer recBuffer(recId.block);
        int response = recBuffer.getRecord(attrCatEntryRecord, recId.slot);

        if (response != SUCCESS){
            printf("Record not found.\n");
            exit(1);
        }

        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record
        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0){
            attrToRenameRecId = recId;
            break;
        }

        // if attrCatEntryRecord.attrName = newName
        //     return E_ATTREXIST;
        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0){
            return E_ATTREXIST;
        }
    }

    // if attrToRenameRecId == {-1, -1}
    //     return E_ATTRNOTEXIST;
    if(attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1){
        return E_ATTRNOTEXIST;
    }



    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    RecBuffer recBuffer(attrToRenameRecId.block);
    int response = recBuffer.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);
    if (response != SUCCESS){
        printf("Record not found.\n");
        exit(1);
    }
    //   update the AttrName of the record with newName
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    //   set back the record with RecBuffer.setRecord
    response = recBuffer.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    return SUCCESS;
}