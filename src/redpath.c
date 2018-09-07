//----------------------------------------------------------------------------
// Copyright Notice:
// Copyright 2017 DMTF. All rights reserved.
// License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/libredfish/blob/master/LICENSE.md
//----------------------------------------------------------------------------
#include <stdlib.h>
#include <string.h>

#include <redpath.h>

static char* getVersion(const char* path, char** end);
static void parseNode(const char* path, redPathNode* node, redPathNode** end);

static char* getStringTill(const char* string, const char* terminator, char** retEnd);

redPathNode* parseRedPath(const char* path)
{
    redPathNode* node;
    redPathNode* endNode;
    char* curPath;
    char* end;

    if(!path || strlen(path) == 0)
    {
        return NULL;
    }

    node = (redPathNode*)calloc(1, sizeof(redPathNode));
    if(!node)
    {
        return NULL;
    }
    if(path[0] == '/')
    {
        node->isRoot = true;
        if(path[1] == 'v')
        {
            node->version = getVersion(path+1, &curPath);
            if(curPath == NULL)
            {
                return node;
            }
            if(curPath[0] == '/')
            {
                curPath++;
            }
            node->next = parseRedPath(curPath);
        }
        else
        {
           node->next = parseRedPath(path+1);
        }
        return node;
    }
    node->isRoot = false;
    curPath = getStringTill(path, "/", &end);
    endNode = node;
    parseNode(curPath, node, &endNode);
    free(curPath);
    if(end != NULL)
    {
        endNode->next = parseRedPath(end+1);
    }
    return node;
}

void cleanupRedPath(redPathNode* node)
{
    if(!node)
    {
        return;
    }
    cleanupRedPath(node->next);
    node->next = NULL;
    if(node->version)
    {
        free(node->version);
    }
    if(node->nodeName)
    {
        free(node->nodeName);
    }
    if(node->op)
    {
        free(node->op);
    }
    if(node->propName)
    {
        free(node->propName);
    }
    if(node->value)
    {
        free(node->value);
    }
    free(node);
}

static char* getVersion(const char* path, char** end)
{
    return getStringTill(path, "/", end);
}

static void parseNode(const char* path, redPathNode* node, redPathNode** end)
{
    char* indexStart;
    char* index;
    char* indexEnd;
    char* nodeName = getStringTill(path, "[", &indexStart);
    size_t tmpIndex;
    char* opChars;

    node->nodeName = nodeName;
    if(indexStart == NULL)
    {
        *end = node;
        return;
    }
    node->next = (redPathNode*)calloc(1, sizeof(redPathNode));
    if(!node->next)
    {
        return;
    }
    //Skip past [
    indexStart++;
    *end = node->next;
    index = getStringTill(indexStart, "]", NULL);
    tmpIndex = (size_t)strtoull(index, &indexEnd, 0);
    if(indexEnd != index)
    {
        free(index);
        node->next->index = tmpIndex;
        node->next->isIndex = true;
        return;
    }
    opChars = strpbrk(index, "<>=");
    if(opChars == NULL)
    {
        //TODO handle last() and position()
#ifdef _MSC_VER
		node->next->op = _strdup("exists");
#else
        node->next->op = strdup("exists");
#endif
        node->next->propName = index;
        return;
    }
    node->next->propName = (char*)malloc((opChars - index)+1);
    memcpy(node->next->propName, index, (opChars - index));
    node->next->propName[(opChars - index)] = 0;

    tmpIndex = 1;
    while(1)
    {
        if(opChars[tmpIndex] == '=' || opChars[tmpIndex] == '<' || opChars[tmpIndex] == '>')
        {
            tmpIndex++;
            continue;
        }
        break;
    }

    node->next->op = (char*)malloc(tmpIndex+1);
    memcpy(node->next->op, opChars, tmpIndex);
    node->next->op[tmpIndex] = 0;

#ifdef _MSC_VER
	node->next->value = _strdup(opChars + tmpIndex);
#else
    node->next->value = strdup(opChars+tmpIndex);
#endif
    free(index);
}

static char* getStringTill(const char* string, const char* terminator, char** retEnd)
{
    char* ret;
    char* end;
    end = strstr((char*)string, terminator);
    if(retEnd)
    {
        *retEnd = end;
    }
    if(end == NULL)
    {
        //No terminator
#ifdef _MSC_VER
		return _strdup(string);
#else
        return strdup(string);
#endif
    }
    ret = (char*)malloc((end-string)+1);
    memcpy(ret, string, (end-string));
    ret[(end-string)] = 0;
    return ret;
}
