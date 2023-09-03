#include "BPlusTree.h"
#include <iostream>
#include <cstring>

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    // declare searchIndex which will be used to store search index for attrName.
    IndexId searchIndex;

    /* get the search index corresponding to attribute with name attrName
       using AttrCacheTable::getSearchIndex(). */
    int response = AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);

    if (response != SUCCESS) {
        printf("failed to get search index for %s\n", attrName);
        exit(1);
    }

    AttrCatEntry attrCatEntry;

    /* load the attribute cache entry into attrCatEntry using
     AttrCacheTable::getAttrCatEntry(). */
    response = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

    if (response != SUCCESS) {
        printf("failed to get attrCatEntry for %s\n", attrName);
        exit(1);
    }
    


    // declare variables block and index which will be used during search
    int block, index;
    /* searchIndex == {-1, -1}*/
    if (searchIndex.block == -1 && searchIndex.index == -1){
        // (search is done for the first time)

        // start the search from the first entry of root.
        block = attrCatEntry.rootBlock;
        index = 0;
        /* attrName doesn't have a B+ tree (block == -1)*/
        if (block==-1) {
            return RecId{-1, -1};
        }

    } else {
        /*a valid searchIndex points to an entry in the leaf index of the attribute's
        B+ Tree which had previously satisfied the op for the given attrVal.*/

        block = searchIndex.block;
        index = searchIndex.index + 1;  // search is resumed from the next index.

        // load block into leaf using IndLeaf::IndLeaf().
        IndLeaf leaf(block);

        // declare leafHead which will be used to hold the header of leaf.
        HeadInfo leafHead;

        // load header into leafHead using BlockBuffer::getHeader().
        response = leaf.getHeader(&leafHead);

        if (response != SUCCESS) {
            printf("failed to get header for block %d\n", block);
            exit(1);
        }

        if (index >= leafHead.numEntries) {
            /* (all the entries in the block has been searched; search from the
            beginning of the next leaf index block. */

            // block = rblock of leafHead.
            block = leafHead.rblock;
            index = 0;
            // update block to rblock of current block and index to 0.

            if (block == -1) {
                // (end of linked list reached - the search is done.)
                return RecId{-1, -1};
            }
        }
    }

    /******  Traverse through all the internal nodes according to value
             of attrVal and the operator op                             ******/

    /* (This section is only needed when
        - search restarts from the root block (when searchIndex is reset by caller)
        - root is not a leaf
        If there was a valid search index, then we are already at a leaf block
        and the test condition in the following loop will fail)
    */
    /* block is of type IND_INTERNAL */
    while (StaticBuffer::getStaticBlockType(block) == IND_INTERNAL){
        //use StaticBuffer::getStaticBlockType()

        // load the block into internalBlk using IndInternal::IndInternal().
        IndInternal internalBlk(block);

        HeadInfo intHead;

        // load the header of internalBlk into intHead using BlockBuffer::getHeader()
        response = internalBlk.getHeader(&intHead);

        // declare intEntry which will be used to store an entry of internalBlk.
        InternalEntry intEntry;
        /* op is one of NE, LT, LE */
        if (op == NE || op == LT || op == LE) {
            /*
            - NE: need to search the entire linked list of leaf indices of the B+ Tree,
            starting from the leftmost leaf index. Thus, always move to the left.

            - LT and LE: the attribute values are arranged in ascending order in the
            leaf indices of the B+ Tree. Values that satisfy these conditions, if
            any exist, will always be found in the left-most leaf index. Thus,
            always move to the left.
            */


            // load entry in the first slot of the block into intEntry
            // using IndInternal::getEntry().
            response = internalBlk.getEntry (&intEntry, 0);

            block = intEntry.lChild;

        } else {
            /*
            - EQ, GT and GE: move to the left child of the first entry that is
            greater than (or equal to) attrVal
            (we are trying to find the first entry that satisfies the condition.
            since the values are in ascending order we move to the left child which
            might contain more entries that satisfy the condition)
            */


            /*
             traverse through all entries of internalBlk and find an entry that
             satisfies the condition.
             if op == EQ or GE, then intEntry.attrVal >= attrVal
             if op == GT, then intEntry.attrVal > attrVal
             Hint: the helper function compareAttrs() can be used for comparing
            */
           int index = 0;
           while(index<MAX_KEYS_INTERNAL){
                response = internalBlk.getEntry (&intEntry, index);
                if (compareAttrs(intEntry.attrVal, attrVal, attrCatEntry.attrType) >= 0){
                     break;
                }
                index++;
            }
    
            

            /* such an entry is found*/
            if (index < MAX_KEYS_INTERNAL) {
                // move to the left child of that entry
                // left child of the entry
                block = intEntry.lChild;  

            } else {
                // move to the right child of the last entry of the block
                // i.e numEntries - 1 th entry of the block
                response = internalBlk.getEntry (&intEntry, intHead.numEntries - 1);
                if (response != SUCCESS) {
                    printf("failed to get entry %d of block %d\n", intHead.numEntries - 1, block);
                    exit(1);
                }

                 // right child of last entry
                block =  intEntry.rChild;
            }
        }
    }

    // NOTE: `block` now has the block number of a leaf index block.

    /******  Identify the first leaf index entry from the current position
                that satisfies our condition (moving right)             ******/

    while(block != -1) {
        // load the block into leafBlk using IndLeaf::IndLeaf().
        IndLeaf leafBlk(block);
        HeadInfo leafHead;

        // load the header to leafHead using BlockBuffer::getHeader().
        response = leafBlk.getHeader(&leafHead);

        // declare leafEntry which will be used to store an entry from leafBlk
        Index leafEntry;
        /*index < numEntries in leafBlk*/
        while (index < leafHead.numEntries) {

            // load entry corresponding to block and index into leafEntry
            // using IndLeaf::getEntry().
            response = leafBlk.getEntry(&leafEntry, index);

            /* comparison between leafEntry's attribute value
                            and input attrVal using compareAttrs()*/
            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, attrCatEntry.attrType);
            if (
                (op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0)
            ) {
                // (entry satisfying the condition found)

                // set search index to {block, index}
                searchIndex.block = block;
                searchIndex.index = index;
                response = AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);
                if(response!=SUCCESS){
                    printf("failed to set search index for %s\n", attrName);
                    exit(1);
                }



                // return the recId {leafEntry.block, leafEntry.slot}.
                return RecId{leafEntry.block, leafEntry.slot};

            } else if ((op == EQ || op == LE || op == LT) && cmpVal > 0) {
                /*future entries will not satisfy EQ, LE, LT since the values
                    are arranged in ascending order in the leaves */

                // return RecId {-1, -1};
                return RecId{-1, -1};
            }

            // search next index.
            index++;
        }

        /*only for NE operation do we have to check the entire linked list;
        for all the other op it is guaranteed that the block being searched
        will have an entry, if it exists, satisying that op. */
        if (op != NE) {
            break;
        }

        // block = next block in the linked list, i.e., the rblock in leafHead.
        block = leafHead.rblock;
        // update index to 0.
        index = 0;
        
    }

    // no entry satisying the op was found; return the recId {-1,-1}
    return RecId{-1, -1};
}