/* 
 * plugins.c -- Handling for plugins
 * 
 * Copyright (C) 2009  Martin Wolters et al.
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

#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "io.h"
#include "plugins.h"
#include "conf.h"
#include "util.h"

#ifndef PATH_MAX
#define PATH_MAX _POSIX_PATH_MAX
#endif

#define MAX_DEP 25

LIST_HEAD(plugins);
int plugins_init = 0;

const char *plugins_nonly(const char *name)
{
   const char *p;

   if (!name)
      return NULL;

   if ((p = strrchr(name, '/'))) {
      ++p;
   } else {
      p = name;
   }
   return p;
}

plugin_t *plugins_find(const char *name)
{
   plugin_t *pl;
   int len;
   const char *p = plugins_nonly(name);

   if (!p || list_empty(&plugins))
      return NULL;

   len = strlen(p);
   if (len > 3 && !strcmp(p+len-3, ".so"))
     len -= 3;

   list_for_each_entry(pl, &plugins, elm) {
      if (pl->name && strlen(pl->name) == len && !strncmp(pl->name, p, len))
         return pl;
   }
   return NULL;
}

int plugins_depends(const char *name)
{
   int i, ret = 0;
   plugin_t *pl;

   list_for_each_entry(pl, &plugins, elm) {
      for (i = 0; pl->depends[i]; ++i) {
         if (!strcasecmp(name, pl->depends[i]))
            ++ret;
      }
   }
   return ret;
}

int plugins_open(const char *name)
{
   plugin_t *pl;
   void *v;
   char *dlerr;
   int i, len;
   char buf[PATH_MAX];

   if (plugins_find(name))
      return 0;

   len = strlen(name);
   if (len >= 3 && !strcmp(name+len-3, ".so")) {
#ifdef MODDIR
      snprintf(buf, sizeof(buf), MODDIR "/%s", name);
#else
      snprintf(buf, sizeof(buf), "%s", name);
#endif
   } else {
#ifdef MODDIR
      snprintf(buf, sizeof(buf), MODDIR "/%s.so", name);
#else
      snprintf(buf, sizeof(buf), "%s.so", name);
#endif
   }
   
   v = dlopen(buf, RTLD_NOW);
   if (!v) {
      warn("Plugin %s: Module is missing: %s.\n", name, dlerror());
      return -1;
   }
   
   pl = dlsym(v, "plugin_info");
   dlerr = dlerror();
   if (dlerr) {
      warn("Plugin %s: Unable to load module: %s.\n", name, dlerr);
      dlclose(v);
      return -1;
   }
   
   pl->self = v;
   name = plugins_nonly(name);

   len = strlen(name);
   if (len >= 3 && !strcmp(name+len-3, ".so"))
     len -= 3;

   pl->name = malloc(len-3+1);
   if (pl->name) {
      memcpy(pl->name, (const void*)name, len);
      pl->name[len] = '\0';
   }

   for (i = 0; pl->depends[i]; ++i) {
      if (plugins_require(pl->depends[i]) == -1) {
         warn("Plugin %s: Missing dependency %s.\n", pl->name, pl->depends[i]);
         dlclose(v);
         return -1;
      }
   }

   if (pl->timer && io_settimer(pl->timer) == -1) {
      warn("Plugin %s: Unable to set module timer.\n", pl->name);
      dlclose(v);
      return -1;
   }

   if (pl->init && pl->init() < 0){
      io_unsettimer(pl->timer);
      warn("Plugin %s: Unable to init module.\n", pl->name);
      dlclose(v);
      return -1;
   }
   
   list_add(&pl->elm, &plugins);
   printc("Plugin %s: Loaded module.\n", pl->name);
   return 0;
}

int plugins_require(const char *name)
{
   int i, n, ret;
   char *f;
   static int cnt;

   if (!name || plugins_find(name))
      return 0;

   if (++cnt > MAX_DEP) {
      warn("Plugin %s: Too many dependencies\n", name);
      --cnt;
      return -1; 
   }

   n = config_getcnt("core", "module");
   for (i = 0; i < n; ++i) {
      f = config_getn("core", "module", i);
      if (f && !strcmp(plugins_nonly(f), name)) {
         ret =  plugins_open(f);
         --cnt;
         return ret;
      }
   }
   --cnt;
   return -1;
}

int plugins_close(const char *name)
{
   plugin_t *pl;
   pl = plugins_find(name);
   if (!pl)
      return -1;

   if(plugins_depends(name) > 0)
      return -2;

   printc("Unload module %s\n", pl->name);
   if (pl->timer)
      io_unsettimer(pl->timer);
   if (pl->destroy)
      pl->destroy();
   if (pl->name)
     free(pl->name);
   list_del(&pl->elm);
   dlclose(pl->self);
   return 0;
}

void plugins_unload(void)
{
   plugin_t *pl, *n;
   
   if (list_empty(&plugins))
      return;

   list_for_each_entry_safe(pl, n, &plugins, elm) {
      printc("Unload module %s\n", pl->name); 
      if (pl->timer)
         io_unsettimer(pl->timer);
      if (pl->destroy)
         pl->destroy();
      list_del(&pl->elm);
      dlclose(pl->self);
   }
}

void plugins_load(void)
{
   int i, n;
   
   if (!plugins_init) {
      if (!atexit(plugins_unload))
         plugins_init = 1;
   }

   n = config_getcnt("core", "module");
   printc("Trying to load %i modules.\n", n);
   for (i = 0; i < n; ++i)
      plugins_open(config_getn("core", "module", i));
}

int plugins_reload(const char *name)
{
   plugin_t *pl;

   pl = plugins_find(name);
   if (!pl) {
      return -1;
   }

   if (pl->destroy) {
      pl->destroy();
   }

   if (pl->init) {
      if (pl->init() < 0) {
         warn("Plugin %s: Unable to init module.\n", pl->name);
         list_del(&pl->elm);
         dlclose(pl->self);
         return -3;
      }
   }
   return 0;
}

