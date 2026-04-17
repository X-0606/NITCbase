#include "BPlusTree.h"

#include <cstring>

RecId BPlusTree::bPlusSearch(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int op)
{
    IndexId searchIndex;
    AttrCacheTable::getSearchIndex(relId, attrName, &searchIndex);

    AttrCatEntry attrCatEntry;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    int type = attrCatEntry.attrType;

    int block, index;

    if (searchIndex.block == -1 && searchIndex.index == -1)
    {
        block = attrCatEntry.rootBlock;
        index = 0;

        if (block == -1)
        {
            return RecId{-1, -1};
        }
    }
    else
    {
        block = searchIndex.block;
        index = searchIndex.index + 1;
        IndLeaf leaf(block);
        HeadInfo leafHead;
        leaf.getHeader(&leafHead);

        if (index >= leafHead.numEntries)
        {
            block = leafHead.rblock;
            index = 0;

            if (block == -1)
            {
                return RecId{-1, -1};
            }
        }
    }

    while (StaticBuffer::getStaticBlockType(block) == IND_INTERNAL)
    {
        IndInternal internalBlk(block);
        HeadInfo intHead;
        internalBlk.getHeader(&intHead);
        InternalEntry intEntry;

        if (op == NE || op == LT || op == LE)
        {
            // Move left for NE, LT, LE since values in ascending order
            internalBlk.getEntry(&intEntry, 0);
            block = intEntry.lChild;
        }
        else
        {
            // For EQ, GT, GE: move to left child of first entry >= attrVal
            int i = 0;
            bool found = false;
            for (i = 0; i < intHead.numEntries; i++)
            {
                internalBlk.getEntry(&intEntry, i);
                int cmpVal = compareAttrs(intEntry.attrVal, attrVal, type);
                if (
                    (op == EQ && cmpVal >= 0) ||
                    (op == GT && cmpVal > 0) ||
                    (op == GE && cmpVal >= 0))
                {
                    found = true;
                    break;
                }
            }

            if (found)
            {
                block = intEntry.lChild;
            }
            else
            {
                block = intEntry.rChild;
            }
        }
    }

    while (block != -1)
    {
        IndLeaf leafBlk(block);
        HeadInfo leafHead;
        leafBlk.getHeader(&leafHead);
        Index leafEntry;

        while (index < leafHead.numEntries)
        {
            leafBlk.getEntry(&leafEntry, index);
            int cmpVal = compareAttrs(leafEntry.attrVal, attrVal, type);

            if (
                (op == EQ && cmpVal == 0) ||
                (op == LE && cmpVal <= 0) ||
                (op == LT && cmpVal < 0) ||
                (op == GT && cmpVal > 0) ||
                (op == GE && cmpVal >= 0) ||
                (op == NE && cmpVal != 0))
            {
                searchIndex.block = block;
                searchIndex.index = index;
                AttrCacheTable::setSearchIndex(relId, attrName, &searchIndex);
                return RecId{leafEntry.block, leafEntry.slot};
            }
            else if ((op == EQ || op == LE || op == LT) && cmpVal > 0)
            {
                return RecId{-1, -1};
            }

            ++index;
        }

        // Only NE needs to traverse entire linked list
        if (op != NE)
        {
            break;
        }

        block = leafHead.rblock;
        index = 0;
    }

    return RecId{-1, -1};
}

int BPlusTree::bPlusCreate(int relId, char attrName[ATTR_SIZE])
{
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID)
        return E_NOTPERMITTED;

    AttrCatEntry attrcat;
    int status = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrcat);

    if (status != SUCCESS)
    {
        return status;
    }

    if (attrcat.rootBlock != -1)
    {
        return SUCCESS;
    }

    IndLeaf rootBlockBuf;
    int rootBlock = rootBlockBuf.getBlockNum();

    if (rootBlock == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);
    int block = relCatEntry.firstBlk;

    attrcat.rootBlock = rootBlock;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrcat);
    while (block != -1)
    {
        RecBuffer recblk(block);
        unsigned char slotMap[relCatEntry.numSlotsPerBlk];
        recblk.getSlotMap(slotMap);

        for (int i = 0; i < relCatEntry.numSlotsPerBlk; i++)
            if (slotMap[i] == SLOT_OCCUPIED)
            {
                Attribute record[relCatEntry.numAttrs];
                recblk.getRecord(record, i);
                RecId recId{block, i};

                status = bPlusInsert(relId, attrName, record[attrcat.offset], recId);

                if (status == E_DISKFULL)
                {
                    return E_DISKFULL;
                }
            }

        HeadInfo leafhead;
        recblk.getHeader(&leafhead);
        block = leafhead.rblock;
    }

    return SUCCESS;
}

int BPlusTree::bPlusDestroy(int rootBlockNum)
{
    if (rootBlockNum < 0 || rootBlockNum >= 8192)
    {
        return E_OUTOFBOUND;
    }

    int type = StaticBuffer::getStaticBlockType(rootBlockNum);

    if (type == IND_LEAF)
    {
        IndLeaf leafblk(rootBlockNum);
        leafblk.releaseBlock();
        return SUCCESS;
    }
    else if (type == IND_INTERNAL)
    {
        IndInternal indblock(rootBlockNum);
        HeadInfo leafhead;
        indblock.getHeader(&leafhead);

        // Delete lChild of first entry and rChild of all entries to avoid duplicates
        InternalEntry internalEntry;
        indblock.getEntry(&internalEntry, 0);
        BPlusTree::bPlusDestroy(internalEntry.lChild);

        for (int i = 0; i < leafhead.numEntries; i++)
        {
            indblock.getEntry(&internalEntry, i);
            BPlusTree::bPlusDestroy(internalEntry.rChild);
        }

        indblock.releaseBlock();
        return SUCCESS;
    }
    else
    {
        return E_INVALIDBLOCK;
    }
}

int BPlusTree::bPlusInsert(int relId, char attrName[ATTR_SIZE], Attribute attrVal, RecId recId)
{
    AttrCatEntry attrcat;
    int status = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrcat);

    if (status != SUCCESS)
    {
        return status;
    }

    if (attrcat.rootBlock == -1)
    {
        return E_NOINDEX;
    }

    int rootblockNum = attrcat.rootBlock;
    int leafBlkNum = findLeafToInsert(rootblockNum, attrVal, attrcat.attrType);

    Index entry;
    entry.attrVal = attrVal;
    entry.block = recId.block;
    entry.slot = recId.slot;
    insertIntoLeaf(relId, attrName, leafBlkNum, entry);

    if (leafBlkNum == E_DISKFULL)
    {
        bPlusDestroy(rootblockNum);
        return E_DISKFULL;
    }

    return SUCCESS;
}

int BPlusTree::findLeafToInsert(int rootBlock, Attribute attrVal, int attrType)
{
    int blockNum = rootBlock;

    while (StaticBuffer::getStaticBlockType(blockNum) != IND_LEAF)
    {
        IndInternal internalBlk(blockNum);
        HeadInfo header;
        internalBlk.getHeader(&header);

        InternalEntry entry;
        bool found = false;
        int i = 0;
        for (i = 0; i < header.numEntries; i++)
        {
            internalBlk.getEntry(&entry, i);
            int cmpVal = compareAttrs(entry.attrVal, attrVal, attrType);
            if (cmpVal >= 0)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            internalBlk.getEntry(&entry, header.numEntries - 1);
            blockNum = entry.rChild;
        }
        else
        {
            blockNum = entry.lChild;
        }
    }

    return blockNum;
}

int BPlusTree::insertIntoLeaf(int relId, char attrName[ATTR_SIZE], int blockNum, Index indexEntry)
{
    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);

    IndLeaf leafBlock(blockNum);
    HeadInfo leafhead;
    leafBlock.getHeader(&leafhead);

    // Array to hold existing entries + new entry in sorted order
    Index internalEntries[leafhead.numEntries + 1];

    int i = 0;
    int j = 0;
    bool inserted = false;
    for (i = 0; i < leafhead.numEntries; i++)
    {
        Index tempEntry;
        leafBlock.getEntry(&tempEntry, i);

        if (!inserted && compareAttrs(indexEntry.attrVal, tempEntry.attrVal, attrCatBuf.attrType) <= 0)
        {
            internalEntries[j++] = indexEntry;
            inserted = true;
        }
        internalEntries[j++] = tempEntry;
    }
    if (!inserted)
    {
        internalEntries[j] = indexEntry;
    }

    if (leafhead.numEntries != MAX_KEYS_LEAF)
    {
        leafhead.numEntries++;
        leafBlock.setHeader(&leafhead);

        for (i = 0; i < leafhead.numEntries; i++)
        {
            leafBlock.setEntry(&internalEntries[i], i);
        }

        return SUCCESS;
    }

    // Leaf is full, split needed
    int newRightBlk = splitLeaf(blockNum, internalEntries);

    if (newRightBlk == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    int status;
    if (leafhead.pblock != -1)
    {
        // Middle value (at index MIDDLE_INDEX_LEAF) propagates to parent
        InternalEntry internalEntry;
        internalEntry.attrVal = internalEntries[MIDDLE_INDEX_LEAF].attrVal;
        internalEntry.lChild = blockNum;
        internalEntry.rChild = newRightBlk;

        status = insertIntoInternal(relId, attrName, leafhead.pblock, internalEntry);
    }
    else
    {
        // Current block is root, create new root
        status = createNewRoot(relId, attrName, internalEntries[MIDDLE_INDEX_LEAF].attrVal, blockNum, newRightBlk);
    }

    return status;
}

int BPlusTree::splitLeaf(int leafBlockNum, Index internalEntries[])
{
    // Allocate new leaf block for right split
    IndLeaf rightBlk;
    IndLeaf leftBlk(leafBlockNum);

    int rightBlkNum = rightBlk.getBlockNum();
    int leftBlkNum = leafBlockNum;

    if (rightBlkNum == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    leftBlk.getHeader(&leftBlkHeader);
    rightBlk.getHeader(&rightBlkHeader);

    rightBlkHeader.numEntries = (MAX_KEYS_LEAF + 1) / 2;
    rightBlkHeader.pblock = leftBlkHeader.pblock;
    rightBlkHeader.lblock = leftBlkNum;
    rightBlk.setHeader(&rightBlkHeader);

    leftBlkHeader.numEntries = (MAX_KEYS_LEAF + 1) / 2;
    leftBlkHeader.rblock = rightBlkNum;
    leftBlk.setHeader(&leftBlkHeader);

    // Distribute entries: first half to left, second half to right
    for (int i = 0; i < 32; i++)
    {
        leftBlk.setEntry(&internalEntries[i], i);
    }
    for (int i = 0; i < 32; i++)
    {
        rightBlk.setEntry(&internalEntries[i + 32], i);
    }

    return rightBlkNum;
}

int BPlusTree::insertIntoInternal(int relId, char attrName[ATTR_SIZE], int intBlockNum, InternalEntry intEntry)
{
    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);

    IndInternal intBlk(intBlockNum);
    HeadInfo blockHeader;
    intBlk.getHeader(&blockHeader);

    // Array to hold existing entries + new entry in sorted order
    InternalEntry internalEntries[blockHeader.numEntries + 1];

    int i = 0, j = 0;
    bool inserted = false;
    for (i = 0; i < blockHeader.numEntries; i++)
    {
        InternalEntry tempEntry;
        intBlk.getEntry(&tempEntry, i);

        if (!inserted && compareAttrs(intEntry.attrVal, tempEntry.attrVal, attrCatBuf.attrType) <= 0)
        {
            internalEntries[j] = intEntry;
            j++;
            inserted = true;
        }
        internalEntries[j] = tempEntry;
        if (inserted && j > 0 && j <= blockHeader.numEntries)
        {
            // Update lChild of entry following the newly inserted one
            internalEntries[j].lChild = internalEntries[j - 1].rChild;
        }
        j++;
    }

    if (!inserted)
    {
        internalEntries[blockHeader.numEntries] = intEntry;
    }

    if (blockHeader.numEntries != MAX_KEYS_INTERNAL)
    {
        blockHeader.numEntries++;
        intBlk.setHeader(&blockHeader);

        for (i = 0; i < blockHeader.numEntries; i++)
        {
            intBlk.setEntry(&internalEntries[i], i);
        }

        return SUCCESS;
    }

    // Internal block is full, split needed
    int newRightBlk = splitInternal(intBlockNum, internalEntries);

    if (newRightBlk == E_DISKFULL)
    {
        // Destroy right subtree that hasn't been connected yet
        BPlusTree::bPlusDestroy(intEntry.rChild);
        return E_DISKFULL;
    }

    int status;
    if (blockHeader.pblock != -1)
    {
        // Middle value (at index MIDDLE_INDEX_INTERNAL) propagates to parent
        InternalEntry middleEntry;
        middleEntry.lChild = intBlockNum;
        middleEntry.rChild = newRightBlk;
        middleEntry.attrVal = internalEntries[MIDDLE_INDEX_INTERNAL].attrVal;

        status = insertIntoInternal(relId, attrName, blockHeader.pblock, middleEntry);
    }
    else
    {
        // Current block is root, create new root
        status = createNewRoot(relId, attrName, internalEntries[MIDDLE_INDEX_INTERNAL].attrVal, intBlockNum, newRightBlk);
    }

    return status;
}

int BPlusTree::splitInternal(int intBlockNum, InternalEntry internalEntries[])
{
    // Allocate new internal block for right split
    IndInternal rightBlk;
    IndInternal leftBlk(intBlockNum);

    int rightBlkNum = rightBlk.getBlockNum();
    int leftBlkNum = intBlockNum;

    if (rightBlkNum == E_DISKFULL)
    {
        return E_DISKFULL;
    }

    HeadInfo leftBlkHeader, rightBlkHeader;
    leftBlk.getHeader(&leftBlkHeader);
    rightBlk.getHeader(&rightBlkHeader);

    rightBlkHeader.numEntries = MAX_KEYS_INTERNAL / 2;
    rightBlkHeader.pblock = leftBlkHeader.pblock;
    rightBlk.setHeader(&rightBlkHeader);

    leftBlkHeader.numEntries = MAX_KEYS_INTERNAL / 2;
    leftBlk.setHeader(&leftBlkHeader);

    // Distribute entries: first 50 to left, next 50 to right (index 50 goes to parent)
    for (int i = 0; i < 50; i++)
    {
        leftBlk.setEntry(&internalEntries[i], i);
    }
    for (int i = 0; i < 50; i++)
    {
        rightBlk.setEntry(&internalEntries[i + 51], i);
    }

    int type = StaticBuffer::getStaticBlockType(internalEntries[51].lChild);

    // Update parent pointers in child blocks to point to new right block
    for (int i = 0; i < 50; i++)
    {
        BlockBuffer childBlk(internalEntries[i + 51].rChild);
        if (i == 0)
        {
            BlockBuffer firstChildBlk(internalEntries[51].lChild);
            HeadInfo firstChildHeader;
            firstChildBlk.getHeader(&firstChildHeader);
            firstChildHeader.pblock = rightBlkNum;
            firstChildBlk.setHeader(&firstChildHeader);
        }

        HeadInfo childHeader;
        childBlk.getHeader(&childHeader);
        childHeader.pblock = rightBlkNum;
        childBlk.setHeader(&childHeader);
    }

    return rightBlkNum;
}

int BPlusTree::createNewRoot(int relId, char attrName[ATTR_SIZE], Attribute attrVal, int lChild, int rChild)
{
    AttrCatEntry attrCatBuf;
    AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatBuf);

    // Allocate new internal block as root
    IndInternal newRootBlk;
    int newRootBlkNum = newRootBlk.getBlockNum();

    if (newRootBlkNum == E_DISKFULL)
    {
        // Destroy right subtree that hasn't been connected yet
        BPlusTree::bPlusDestroy(rChild);
        return E_DISKFULL;
    }

    HeadInfo head;
    newRootBlk.getHeader(&head);
    head.numEntries = 1;
    newRootBlk.setHeader(&head);

    InternalEntry newEntry;
    newEntry.attrVal = attrVal;
    newEntry.lChild = lChild;
    newEntry.rChild = rChild;
    newRootBlk.setEntry(&newEntry, 0);

    BlockBuffer leftChildBlk(lChild);
    BlockBuffer rightChildBlk(rChild);

    HeadInfo leftHead, rightHead;
    leftChildBlk.getHeader(&leftHead);
    rightChildBlk.getHeader(&rightHead);

    leftHead.pblock = newRootBlkNum;
    rightHead.pblock = newRootBlkNum;

    leftChildBlk.setHeader(&leftHead);
    rightChildBlk.setHeader(&rightHead);

    attrCatBuf.rootBlock = newRootBlkNum;
    AttrCacheTable::setAttrCatEntry(relId, attrName, &attrCatBuf);

    return SUCCESS;
}
