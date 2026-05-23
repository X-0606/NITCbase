#include "Algebra.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include "../Buffer/BlockBuffer.h"

bool isNumber(char *str)
{
    int len;
    float ignore;
    int ret = sscanf(str, "%f %n", &ignore, &len);
    return ret == 1 && len == strlen(str);
}


int Algebra::select(char srcRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE], char *targetRel)
{
    int srcRelid = OpenRelTable::getRelId(srcRel);
    if (srcRelid < 0 || srcRelid >= MAX_OPEN)
    {
        srcRelid = OpenRelTable::openRel(srcRel);
        if (srcRelid == E_RELNOTEXIST)
        {
            return E_RELNOTEXIST;
        }
    }

    AttrCatEntry attrCatEntry;
    int status = AttrCacheTable::getAttrCatEntry(srcRelid, attr, &attrCatEntry);
    if (status != SUCCESS)
    {
        return E_ATTRNOTEXIST;
    }

    Attribute attrVal;
    int type = attrCatEntry.attrType;
    if (type == NUMBER)
    {
        if (isNumber(strVal))
        {
            attrVal.nVal = atof(strVal);
        }
        else
        {
            return E_ATTRTYPEMISMATCH;
        }
    }
    else if (type == STRING)
    {
        strcpy(attrVal.sVal, strVal);
    }

    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelid, &srcRelCatEntry);
    int src_nAttrs = srcRelCatEntry.numAttrs;

    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];
    for (int i = 0; i < src_nAttrs; i++)
    {
        AttrCatEntry entry;
        AttrCacheTable::getAttrCatEntry(srcRelid, i, &entry);
        strcpy(attr_names[i], entry.attrName);
        attr_types[i] = entry.attrType;
    }

    // Check if we need to create target relation
    int targetRelId = -1;
    if (targetRel != nullptr)
    {
        // Create and open target relation
        status = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);
        if (status != SUCCESS)
        {
            return status;
        }

        targetRelId = OpenRelTable::openRel(targetRel);
        if (targetRelId < 0)
        {
            Schema::deleteRel(targetRel);
            return targetRelId;
        }
    }
    else
    {
        // Display header for console output
        for (int i = 0; i < src_nAttrs; i++)
        {
            printf("%-15s |", attr_names[i]);
        }
    }

    Attribute record[src_nAttrs];
    RelCacheTable::resetSearchIndex(srcRelid);
    AttrCacheTable::resetSearchIndex(srcRelid, attr);

    count = 0;
    while (BlockAccess::search(srcRelid, record, attr, attrVal, op) == SUCCESS)
    {
        if (targetRelId == -1)
        {
            // Display mode
            printf("\n");
            for (int i = 0; i < src_nAttrs; i++)
            {
                if (attr_types[i] == NUMBER)
                    printf("%-15.2f |", record[i].nVal);
                else
                    printf("%-15s |", record[i].sVal);
            }
        }
        else
        {
            // Insert mode
            int ret = BlockAccess::insert(targetRelId, record);
            if (ret != SUCCESS)
            {
                Schema::closeRel(targetRel);
                Schema::deleteRel(targetRel);
                return ret;
            }
        }
        count++;
    }


    if (targetRelId != -1)
    {
        printf("The comparisions are %d\n", count);
        Schema::closeRel(targetRel);
    }

    return SUCCESS;
}

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE])
{
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;

    int relId = OpenRelTable::getRelId(relName);
    if (relId == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    RelCatEntry relcatentry;
    RelCacheTable::getRelCatEntry(relId, &relcatentry);
    if (relcatentry.numAttrs != nAttrs)
    {
        return E_NATTRMISMATCH;
    }

    Attribute recordValues[nAttrs];
    for (int i = 0; i < nAttrs; i++)
    {
        AttrCatEntry attrcatentry;
        AttrCacheTable::getAttrCatEntry(relId, i, &attrcatentry);
        int type = attrcatentry.attrType;

        if (type == NUMBER)
        {
            if (isNumber(record[i]))
            {
                recordValues[i].nVal = atof(record[i]);
            }
            else
            {
                return E_ATTRTYPEMISMATCH;
            }
        }
        else if (type == STRING)
        {
            strcpy(recordValues[i].sVal, record[i]);
        }
    }

    return BlockAccess::insert(relId, recordValues);
}

int Algebra::project(char srcRel[ATTR_SIZE], char *targetRel)
{
    int srcRelid = OpenRelTable::getRelId(srcRel);
    if (srcRelid < 0 || srcRelid >= MAX_OPEN)
    {
        srcRelid = OpenRelTable::openRel(srcRel);
        if (srcRelid == E_RELNOTEXIST)
        {
            return E_RELNOTEXIST;
        }
    }

    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelid, &srcRelCatEntry);
    int numAttrs = srcRelCatEntry.numAttrs;

    char attrNames[numAttrs][ATTR_SIZE];
    int attrTypes[numAttrs];
    for (int i = 0; i < numAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelid, i, &attrCatEntry);
        strcpy(attrNames[i], attrCatEntry.attrName);
        attrTypes[i] = attrCatEntry.attrType;
    }
    int targetRelId=-1;
    if(targetRel==nullptr){
        for(int i=0;i<numAttrs;i++){
            printf("%-15s |",attrNames[i]);
        }
    }
    else{
      int ret = Schema::createRel(targetRel, numAttrs, attrNames, attrTypes);
    if (ret != SUCCESS)
    {
        return ret;
    }

     targetRelId = OpenRelTable::openRel(targetRel);
    if (targetRelId < 0)
    {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }
    }
    

    RelCacheTable::resetSearchIndex(srcRelid);
    Attribute record[numAttrs];
    while (BlockAccess::project(srcRelid, record) == SUCCESS)
    {
        if(targetRelId==-1){
           printf("\n");
            for (int i = 0; i < numAttrs; i++)
            {
                if (attrTypes[i] == NUMBER)
                    printf("%-15.2f |", record[i].nVal);
                else
                    printf("%-15s |", record[i].sVal);
            }
        }
        else{

          int ret = BlockAccess::insert(targetRelId, record);
        if (ret != SUCCESS)
        {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
        
        }
       
    }

    if(targetRel!=nullptr)
    Schema::closeRel(targetRel);
    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE],int tar_nAttrs, char tar_Attrs[][ATTR_SIZE] ,char *targetRel)
{
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN)
    {
        srcRelId = OpenRelTable::openRel(srcRel);
        if (srcRelId == E_RELNOTEXIST)
        {
            return E_RELNOTEXIST;
        }
    }

    RelCatEntry srcRelCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &srcRelCatEntry);
    int src_nAttrs = srcRelCatEntry.numAttrs;

    int attr_offset[tar_nAttrs];
    int attr_types[tar_nAttrs];

    for (int i = 0; i < tar_nAttrs; i++)
    {
        AttrCatEntry attrCatEntry;
        int ret = AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &attrCatEntry);
        if (ret != SUCCESS)
        {
            return E_ATTRNOTEXIST;
        }
        attr_offset[i] = attrCatEntry.offset;
        attr_types[i] = attrCatEntry.attrType;
    }

    int targetRelId = -1;

    if (targetRel == nullptr)
    {
        for (int i = 0; i < tar_nAttrs; i++)
        {
            AttrCatEntry attrCatEntry;
            int ret = AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &attrCatEntry);
            printf("%-15s |", tar_Attrs[i]);
        }
    }
    else
    {
        int ret = Schema::createRel(targetRel, tar_nAttrs, tar_Attrs, attr_types);
        if (ret != SUCCESS)
        {
            return ret;
        }

        targetRelId = OpenRelTable::openRel(targetRel);
        if (targetRelId < 0)
        {
            Schema::deleteRel(targetRel);
            return targetRelId;
        }
    }

    RelCacheTable::resetSearchIndex(srcRelId);
    Attribute record[src_nAttrs];

    if (targetRel == nullptr)
    {
        printf("\n");
    }

    while (BlockAccess::project(srcRelId, record) == SUCCESS)
    {
        Attribute proj_record[tar_nAttrs];
        for (int i = 0; i < tar_nAttrs; i++)
        {
            proj_record[i] = record[attr_offset[i]];
        }
        if (targetRelId != -1)
        {
            int insertRet = BlockAccess::insert(targetRelId, proj_record);
            if (insertRet != SUCCESS)
            {
                Schema::closeRel(targetRel);
                Schema::deleteRel(targetRel);
                return insertRet;
            }
        }
        else
        {
            printf("\n");
            for (int i = 0; i < tar_nAttrs; i++)
            {
                if (attr_types[i] == NUMBER)
                {
                    printf("%-15.2f |", proj_record[i].nVal);
                }
                else
                {
                    printf("%-15s |", proj_record[i].sVal);
                }
            }
            
        }
    }

    if (targetRel != nullptr)
    {
        Schema::closeRel(targetRel);
    }
    return SUCCESS;
}

int Algebra::join(char srcRelation1[ATTR_SIZE], char srcRelation2[ATTR_SIZE], char targetRelation[ATTR_SIZE], char attribute1[ATTR_SIZE], char attribute2[ATTR_SIZE])
{

    int srcrelid1 = OpenRelTable::getRelId(srcRelation1);
    int srcrelid2 = OpenRelTable::getRelId(srcRelation2);

    if (srcrelid2 == E_RELNOTOPEN || srcrelid1 == E_RELNOTOPEN)
    {
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry1, attrCatEntry2;
    int status = AttrCacheTable::getAttrCatEntry(srcrelid1, attribute1, &attrCatEntry1);
    if (status == E_ATTRNOTEXIST)
    {
        return status;
    }
    status = AttrCacheTable::getAttrCatEntry(srcrelid2, attribute2, &attrCatEntry2);
    if (status == E_ATTRNOTEXIST)
    {
        return status;
    }

    if (attrCatEntry1.attrType != attrCatEntry2.attrType)
    {
        return E_ATTRTYPEMISMATCH;
    }

    RelCatEntry relCatEntry1, relCatEntry2;
    RelCacheTable::getRelCatEntry(srcrelid1, &relCatEntry1);
    RelCacheTable::getRelCatEntry(srcrelid2, &relCatEntry2);

    int numOfAttributes1 = relCatEntry1.numAttrs;
    int numOfAttributes2 = relCatEntry2.numAttrs;

    for (int i = 0; i < numOfAttributes1; i++)
    {
        AttrCatEntry attr1;
        AttrCacheTable::getAttrCatEntry(srcrelid1, i, &attr1);

        for (int j = 0; j < numOfAttributes2; j++)
        {
            AttrCatEntry attr2;
            AttrCacheTable::getAttrCatEntry(srcrelid2, j, &attr2);

            if (strcmp(attr2.attrName, attribute2) == 0 && strcmp(attr1.attrName, attribute1) == 0)
            {
                continue;
            }

            if (strcmp(attr1.attrName, attr2.attrName) == 0)
            {
                return E_DUPLICATEATTR;
            }
        }
    }

    if (attrCatEntry2.rootBlock == -1)
    {
        BPlusTree::bPlusCreate(srcrelid2, attribute2);
    }

    int targetAttrCount = numOfAttributes1 + numOfAttributes2 - 1;

    char targetRelAttrNames[targetAttrCount][ATTR_SIZE];
    int targetRelAttrTypes[targetAttrCount];

    int targetIdx = 0;
    for (int i = 0; i < numOfAttributes1; i++)
    {
        AttrCatEntry attr1;
        AttrCacheTable::getAttrCatEntry(srcrelid1, i, &attr1);
        strcpy(targetRelAttrNames[targetIdx], attr1.attrName);
        targetRelAttrTypes[targetIdx] = attr1.attrType;
        targetIdx++;
    }

    for (int i = 0; i < numOfAttributes2; i++)
    {
        AttrCatEntry attr1;
        AttrCacheTable::getAttrCatEntry(srcrelid2, i, &attr1);
        if (strcmp(attribute2, attr1.attrName) == 0)
        {
            continue;
        }
        strcpy(targetRelAttrNames[targetIdx], attr1.attrName);
        targetRelAttrTypes[targetIdx] = attr1.attrType;
        targetIdx++;
    }

    status = Schema::createRel(targetRelation, targetAttrCount, targetRelAttrNames, targetRelAttrTypes);
    if (status != SUCCESS)
    {
        return status;
    }

    int targetRelId = OpenRelTable::openRel(targetRelation);

    if (targetRelId == E_CACHEFULL)
    {
        Schema::deleteRel(targetRelation);
        return E_CACHEFULL;
    }

    Attribute record1[numOfAttributes1];
    Attribute record2[numOfAttributes2];
    Attribute targetRecord[targetAttrCount];

    RelCacheTable::resetSearchIndex(srcrelid1);

    while (BlockAccess::project(srcrelid1, record1) == SUCCESS)
    {
        RelCacheTable::resetSearchIndex(srcrelid2);
        AttrCacheTable::resetSearchIndex(srcrelid2, attribute2);

        while (BlockAccess::search(
                   srcrelid2, record2, attribute2, record1[attrCatEntry1.offset], EQ) == SUCCESS)
        {
            int k = 0;
            for (int i = 0; i < numOfAttributes1; i++)
            {
                targetRecord[k++] = record1[i];
            }
            AttrCatEntry tempAttr;
            for (int i = 0; i < numOfAttributes2; i++)
            {
                AttrCacheTable::getAttrCatEntry(srcrelid2, i, &tempAttr);
                if (strcmp(attribute2, tempAttr.attrName) != 0)
                {
                    targetRecord[k++] = record2[i];
                }
            }

            status = BlockAccess::insert(targetRelId, targetRecord);

            if (status != SUCCESS)
            {
                OpenRelTable::closeRel(targetRelId);
                Schema::deleteRel(targetRelation);
                return E_DISKFULL;
            }
        }
    }

    OpenRelTable::closeRel(targetRelId);
    return SUCCESS;
}
