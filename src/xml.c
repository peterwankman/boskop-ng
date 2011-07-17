/* 
 * xml.c -- XML utilities 
 * 
 * Copyright (C) 2009-2010  Martin Wolters et al.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to 
 * the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA
 * 
 */


#include <string.h>

#include "xml.h"

xmlNode *find_node(xmlNode *parent, const char *name, xmlElementType type)
{
   xmlNode *n;

   if (!parent)
      return NULL;

   for (n = parent->children; n; n = n->next) {
      if (n->type == type && (!name || (n->name
            && !strcasecmp((const char*)n->name, name))))
         return n;
   }
   return NULL;
}

xmlNode *next_node(xmlNode *cur)
{
   xmlNode *n;

   if (!cur || !cur->name)
      return NULL;

   for (n = cur->next; n; n = n->next) {
      if (n->type == cur->type &&  n->name && !strcasecmp((const char*)n->name, (const char*)cur->name))
         return n;
   }
   return NULL;
}

xmlNode *nth_node(xmlNode *cur, int n)
{
   while(n > 0) {
      cur = next_node(cur);
      --n;
   }
   return cur;
}

int count_nodes(xmlNode *cur)
{
   int c = 1;

   if (!cur)
      return 0;
   
   while ((cur = next_node(cur)))
      ++c;
   return c;
}

char *text_node(xmlNode *parent)
{
   xmlNode *n;

   n = find_node(parent, NULL, XML_TEXT_NODE);
   if (n && n->content)
      return (char*)n->content;

   n = find_node(parent, NULL, XML_CDATA_SECTION_NODE);
   if (n && n->content)
      return (char*)n->content;

   return NULL;
}

char *node_prop(xmlNode *parent, const char *attr)
{
   if (parent)
      return (char*)xmlGetProp(parent, (const xmlChar *)attr);
   return NULL;
}

