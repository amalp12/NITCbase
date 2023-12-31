#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

// the declarations for these functions can be found in "BlockBuffer.h"
/*
f the block could not be allocatted in the disk, then the blockNum field of this class will contain the appropriate error code. The callers of this constructor and the following constructors: RecBuffer :: RecBuffer() (Constructor 1), IndBuffer :: IndBuffer() (Constructor 1), IndInternal :: IndInternal() (Constructor1) and IndLeaf :: IndLeaf() (Constructor 1) should check the value of blockNum field to verify if the disk block was allocatted succesfully.
*/
BlockBuffer::BlockBuffer(char blockType){
    // allocate a block on the disk and a buffer in memory to hold the new block of
    // given type using getFreeBlock function and get the return error codes if any.
    int blockTypeInt;
    if(blockType=='R'){
        blockTypeInt = REC;
    }
    else if(blockType=='I'){
        blockTypeInt = IND_INTERNAL;
    }
    else if(blockType=='L'){
        blockTypeInt = IND_LEAF;
    }
    else{
        printf("Invalid block type.\n");
        exit(1);
    }
    int blockNum = getFreeBlock(blockTypeInt);

    // set the blockNum field of the object to that of the allocated block
    // number if the method returned a valid block number,
    // otherwise set the error code returned as the block number.
    this->blockNum = blockNum;

    // (The caller must check if the constructor allocatted block successfully
    // by checking the value of block number field.)
}
BlockBuffer::BlockBuffer(int blockNum)
{
    // initialise this.blockNum with the argument
    this->blockNum = blockNum;
}

// call parent non-default constructor with 'R' denoting record block.
RecBuffer::RecBuffer() : BlockBuffer('R'){}

// calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}
// the declarations for these functions can be found in "BlockBuffer.h"

/*
Used to get the header of the block into the location pointed to by `head`
NOTE: this function expects the caller to allocate memory for `head`
*/
int BlockBuffer::getHeader(struct HeadInfo *head)
{

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret; // return any errors that might have occured in the process
    }
    // rest of the logic is as in stage 2
    memset(head, 0, sizeof(struct HeadInfo));
    // populate the numEntries, numAttrs and numSlots fields in *head
    memcpy(&head->reserved, bufferPtr + 28, 4);
    memcpy(&head->numSlots, bufferPtr + 24, 4);
    memcpy(&head->numAttrs, bufferPtr + 20, 4);
    memcpy(&head->numEntries, bufferPtr + 16, 4);
    memcpy(&head->rblock, bufferPtr + 12, 4);
    memcpy(&head->lblock, bufferPtr + 8, 4);
    memcpy(&head->pblock, bufferPtr + 4, 4);
    memcpy(&head->blockType, bufferPtr, 4);

    return SUCCESS;
}

/*
Used to get the record at slot `slotNum` into the array `rec`
NOTE: this function expects the caller to allocate memory for `rec`
*/
int RecBuffer::getRecord(union Attribute *rec, int slotNum)
{
    struct HeadInfo head;

    // get the header using this.getHeader() function
    this->getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret;
    }
    // ... (the rest of the logic is as in stage 2
    /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
       - each record will have size attrCount * ATTR_SIZE
       - slotMap will be of size slotCount
    */

    int recordSize = attrCount * ATTR_SIZE;
    int slotNumRecordOffset = (HEADER_SIZE + slotCount) + (recordSize * slotNum);
    /* calculate buffer + offset */;
    unsigned char *slotPointer = bufferPtr + slotNumRecordOffset;

    // load the record into the rec data structure
    memcpy(rec, slotPointer, recordSize);

    return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    if (ret != SUCCESS){
        return ret;
    }

    /* get the header of the block using the getHeader() function */
    struct HeadInfo head;
    ret = BlockBuffer::getHeader(&head);

    // get number of attributes in the block.
    int numberOfAttributes = head.numAttrs;

    // get the number of slots in the block.
    int numberOfSlots = head.numSlots;

    // if input slotNum is not in the permitted range return E_OUTOFBOUND.
    if (slotNum < 0 || slotNum >= numberOfSlots){
        return E_OUTOFBOUND;
    }

    /* offset bufferPtr to point to the beginning of the record at required
       slot. the block contains the header, the slotmap, followed by all
       the records. so, for example,
       record at slot x will be at bufferPtr + HEADER_SIZE + (x*recordSize)
       copy the record from `rec` to buffer using memcpy
       (hint: a record will be of size ATTR_SIZE * numAttrs)
    */
    int recordSize = numberOfAttributes * ATTR_SIZE;
    int slotNumRecordOffset = (HEADER_SIZE + numberOfSlots) + (recordSize * slotNum);
    unsigned char *slotPointer = bufferPtr + slotNumRecordOffset;
    memcpy(slotPointer, rec, recordSize);

    // update dirty bit using setDirtyBit()
    ret = StaticBuffer::setDirtyBit(this->blockNum);
    /* (the above function call should not fail since the block is already
       in buffer and the blockNum is valid. If the call does fail, there
       exists some other issue in the code) */
    if (ret != SUCCESS){
        printf("Error in setting dirty bit.\n");
        exit(1);
    }
   
    

    // return SUCCESS
    return SUCCESS;
}

/*
Used to load a block to the buffer and get a pointer to it.
NOTE: this function expects the caller to allocate memory for the argument
*/
/* NOTE: This function will NOT check if the block has been initialised as a
   record or an index block. It will copy whatever content is there in that
   disk block to the buffer.
   Also ensure that all the methods accessing and updating the block's data
   should call the loadBlockAndGetBufferPtr() function before the access or
   update is done. This is because the block might not be present in the
   buffer due to LRU buffer replacement. So, it will need to be bought back
   to the buffer before any operations can be done.
 */
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr)
{
    // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if (bufferNum == E_BLOCKNOTINBUFFER)
    {
        // get a free buffer using StaticBuffer.getFreeBuffer()

        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        // if the call returns E_OUTOFBOUND, return E_OUTOFBOUND here as
        // the blockNum is invalid
        if (bufferNum == E_OUTOFBOUND)
        {
            return E_OUTOFBOUND;
        }

        Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
    }
    else
    {

        // if present ,
        // timestamps of all other occupied buffers in BufferMetaInfo.
        for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++)
        {
            if (StaticBuffer::metainfo[bufferIndex].free == false)
            {
                StaticBuffer::metainfo[bufferIndex].timeStamp++;
            }
        }
        // set the timestamp of the corresponding buffer to 0
        StaticBuffer::metainfo[bufferNum].timeStamp = 0;
    }

    // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
    (*buffPtr) = StaticBuffer::blocks[bufferNum];

    return SUCCESS;
}
/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
*/
int RecBuffer::getSlotMap(unsigned char *slotMap)
{
    unsigned char *bufferPtr;

    // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS)
    {
        return ret;
    }

    struct HeadInfo head;
    // get the header of the block using getHeader() function
    ret = BlockBuffer::getHeader(&head);

    /* number of slots in block from header */;
    int slotCount = head.numSlots;

    // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

    // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
    memcpy(slotMap, slotMapInBuffer, slotCount);

    return SUCCESS;
}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType)
{

    double diff;
    // if attrType == STRING
    //     diff = strcmp(attr1.sval, attr2.sval)

    // else
    //     diff = attr1.nval - attr2.nval
    if (attrType == STRING)
    {
        diff = strcmp(attr1.sVal, attr2.sVal);
    }
    else
    {
        diff = attr1.nVal - attr2.nVal;
    }

    /*
    if diff > 0 then return 1
    if diff < 0 then return -1
    if diff = 0 then return 0
    */
    if (diff > 0)
    {
        return 1;
    }
    else if (diff < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int BlockBuffer::setHeader(struct HeadInfo *head){

    unsigned char *bufferPtr;
    // get the starting address of the buffer containing the block using
    // loadBlockAndGetBufferPtr(&bufferPtr).
    int response = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    if (response != SUCCESS){
        return response;
    }


    // cast bufferPtr to type HeadInfo*
    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

    // copy the fields of the HeadInfo pointed to by head (except reserved) to
    // the header of the block (pointed to by bufferHeader)
    //(hint: bufferHeader->numSlots = head->numSlots )
    bufferHeader->numSlots = head->numSlots;
    bufferHeader->numAttrs = head->numAttrs;
    bufferHeader->numEntries = head->numEntries;
    bufferHeader->lblock = head->lblock;
    bufferHeader->rblock = head->rblock;
    bufferHeader->pblock = head->pblock;
    bufferHeader->blockType = head->blockType;


    // update dirty bit by calling StaticBuffer::setDirtyBit()
    response = StaticBuffer::setDirtyBit(this->blockNum);

    // if setDirtyBit() failed, return the error code
    if(response != SUCCESS){
        return response;
    }

    // return SUCCESS;
    return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType){

    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block
       using loadBlockAndGetBufferPtr(&bufferPtr). */
    int response = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    if (response != SUCCESS){
        return response;
    }

    // store the input block type in the first 4 bytes of the buffer.
    // (hint: cast bufferPtr to int32_t* and then assign it)
    // *((int32_t *)bufferPtr) = blockType;
    *((int32_t *)bufferPtr) = blockType;

    // update the StaticBuffer::blockAllocMap entry corresponding to the
    // object's block number to `blockType`.
    StaticBuffer::blockAllocMap[this->blockNum] = blockType;

    // update dirty bit by calling StaticBuffer::setDirtyBit()
    response = StaticBuffer::setDirtyBit(this->blockNum);
    // if setDirtyBit() failed
        // return the returned value from the call
    if (response != SUCCESS){
        return response;
    }

    // return SUCCESS
    return SUCCESS;
}

int BlockBuffer::getFreeBlock(int blockType){

    // iterate through the StaticBuffer::blockAllocMap and find the block number
    // of a free block in the disk.
    int freeBlock = -1;
    for (int blockNum = 0; blockNum < DISK_BLOCKS; blockNum++){
        if (StaticBuffer::blockAllocMap[blockNum] == UNUSED_BLK){
            freeBlock = blockNum;
            break;
        }
    }

    // if no block is free, return E_DISKFULL.
    if (freeBlock == -1){
        return E_DISKFULL;
    }

    // set the object's blockNum to the block number of the free block.
    this->blockNum = freeBlock;

    // find a free buffer using StaticBuffer::getFreeBuffer() .
    int bufferNum = StaticBuffer::getFreeBuffer(freeBlock);

    // initialize the header of the block passing a struct HeadInfo with values
    // pblock: -1, lblock: -1, rblock: -1, numEntries: 0, numAttrs: 0, numSlots: 0
    // to the setHeader() function.
    struct HeadInfo head;
    head.pblock = -1;
    head.lblock = -1;
    head.rblock = -1;
    head.numEntries = 0;
    head.numAttrs = 0;
    head.numSlots = 0;

    int response = setHeader(&head);

    if(response != SUCCESS){
        printf("Error in setting header.\n");
        exit(1);
    }

    // update the block type of the block to the input block type using setBlockType().
    response = setBlockType(blockType);
    if (response != SUCCESS){
        printf("Error in setting block type.\n");
        exit(1);
    }


    // return block number of the free block.
    return freeBlock;
}

int RecBuffer::setSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;
    /* get the starting address of the buffer containing the block using
       loadBlockAndGetBufferPtr(&bufferPtr). */

    int response = loadBlockAndGetBufferPtr(&bufferPtr);

    // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
        // return the value returned by the call.
    if (response != SUCCESS){
        return response;
    }

    // get the header of the block using the getHeader() function
    struct HeadInfo head;
    response = BlockBuffer::getHeader(&head);

    if(response != SUCCESS){
        printf("Error in getting header.\n");
        exit(1);
    }

    int numSlots = head.numSlots;

    // the slotmap starts at bufferPtr + HEADER_SIZE. Copy the contents of the
    // argument `slotMap` to the buffer replacing the existing slotmap.
    // Note that size of slotmap is `numSlots`
    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
    memcpy(slotMapInBuffer, slotMap, numSlots);

    // update dirty bit using StaticBuffer::setDirtyBit
    response = StaticBuffer::setDirtyBit(this->blockNum);
    // if setDirtyBit failed, return the value returned by the call
    if (response != SUCCESS){
        return response;
    }

    // return SUCCESS
    return SUCCESS;
}

int BlockBuffer::getBlockNum(){

    //return corresponding block number.
    return this->blockNum;
}