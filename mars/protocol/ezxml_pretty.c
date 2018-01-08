#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "ezxml.h"

/* Counts number of child elements in a container element. 
 * Name is the name of the child element. 
 * Errors if no occurances found. */ 
int ezxml_count_children(ezxml_t Node, const char *Name, int min_count)
{
  ezxml_t Cur, sibling;
  int Count;

  Count = 0;
  Cur = Node->child;
	sibling = NULL;

	if(Cur)
	{
		sibling = Cur->sibling;
	}
	    
    while(Cur)
	{
		if(strcmp(Cur->name, Name) == 0)
		{
		    ++Count;
		}
	    Cur = Cur->next;
	    if(Cur == NULL)
		{
		    Cur = sibling;
			if(Cur != NULL)
			{
				sibling = Cur->sibling;
			}
		}
	}   
	/* Error if no occurances found */ 
	if(Count < min_count)
	{
		printf("Expected node '%s' to have %d "
		    "child elements, but none found.\n", Node->name, min_count);
	    return 0;
	}
   return Count;
}

#define EZXML_INDENT 2

ezxml_t ezxml_pretty(ezxml_t xml)
{
   int i = 0, j, level = 0;
   ezxml_t x;
   char *s;

   if (! xml || strspn(xml->txt, " \t\r\n") != strlen(xml->txt)) return xml;

   if (xml->child) {
       for (x = xml; x; x = x->parent) level += EZXML_INDENT; // how deep?

       for (x = xml->child; x; x = x->ordered) {
           i += level + 1;
           x->off = i;
       }

       s = (char *)malloc(i + level + 2);
       for (j = 0; j <= i; j += level + 1) {
           s[j] = '\n';
           memset(s + j + 1, ' ', level);
       }
       s[j - EZXML_INDENT] = '\0';

       ezxml_set_flag(ezxml_set_txt(xml, s), EZXML_TXTM);
       ezxml_pretty(xml->child);
   }
   else ezxml_set_txt(xml, "");

   return ezxml_pretty(xml->ordered);
}


/*
const char *
      checkElements(ezxml_t x, ...) {
	va_list ap;
	if (!x || !x->child)
	  return 0;
	for (ezxml_t c = x->child; c; c = c->sibling) {
	  va_start(ap, x);
	  char *p;
	  while ((p = va_arg(ap, char*)))
	    if (!strcasecmp(p, c->name))
	      break;
	  va_end(ap);
	  if (!p)
	    return esprintf("Invalid element \"%s\", in a \"%s\" element", c->name, x->name);
	}
	return 0;
      }
*/