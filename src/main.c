/*
 *  Copyright 2014 The Luvit Authors. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#define LUA_LIB
#include "luvi.h"
#include "luv.h"
#include "lenv.c"
#include "luvi.c"
#ifndef MINIZ_NO_STDIO
#define MINIZ_NO_STDIO
#endif
#include "lminiz.c"
#include "snapshot.c"
#ifdef WITH_PCRE
int luaopen_rex_pcre(lua_State* L);
#endif
#ifdef WITH_PLAIN_LUA
#include "../deps/bit.c"
#endif

#ifdef WITH_CUSTOM
int luvi_custom(lua_State* L);
#endif

static int luvi_traceback(lua_State *L) {
  if (!lua_isstring(L, 1))  /* 'message' not a string? */
    return 1;  /* keep it intact */
  lua_pushglobaltable(L);
  lua_getfield(L, -1, "debug");
  lua_remove(L, -2);
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}

static lua_State* vm_acquire(){
  lua_State*L = luaL_newstate();
  if (L == NULL)
    return L;

  // Add in the lua standard and compat libraries
  luvi_openlibs(L);

  // Get package.loaded so that we can load modules
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "loaded");

  // load luv into uv in advance so that the metatables for async work.
  lua_pushcfunction(L, luaopen_luv);
  lua_call(L, 0, 1);
  lua_setfield(L, -2, "uv");

  // remove package.loaded
  lua_remove(L, -1);

  // Get package.preload so we can store builtins in it.
  lua_getfield(L, -1, "preload");
  lua_remove(L, -2); // Remove package

  lua_pushcfunction(L, luaopen_env);
  lua_setfield(L, -2, "env");

  lua_pushcfunction(L, luaopen_miniz);
  lua_setfield(L, -2, "miniz");

  lua_pushcfunction(L, luaopen_snapshot);
  lua_setfield(L, -2, "snapshot");

#ifdef WITH_LPEG
  lua_pushcfunction(L, luaopen_lpeg);
  lua_setfield(L, -2, "lpeg");
#endif

#ifdef WITH_PCRE
  lua_pushcfunction(L, luaopen_rex_pcre);
  lua_pushvalue(L, -1);
  lua_setfield(L, -3, "rex_pcre");
  lua_setfield(L, -2, "rex");
#endif

  // Store luvi module definition at preload.luvi
  lua_pushcfunction(L, luaopen_luvi);
  lua_setfield(L, -2, "luvi");

#ifdef WITH_OPENSSL
  // Store openssl module definition at preload.openssl
  lua_pushcfunction(L, luaopen_openssl);
  lua_setfield(L, -2, "openssl");
#endif

#ifdef WITH_ZLIB
  // Store zlib module definition at preload.zlib
  lua_pushcfunction(L, luaopen_zlib);
  lua_setfield(L, -2, "zlib");
#endif

#ifdef WITH_PLAIN_LUA
  {
      LUALIB_API int luaopen_init(lua_State *L);
      LUALIB_API int luaopen_luvibundle(lua_State *L);
      LUALIB_API int luaopen_luvipath(lua_State *L);
      lua_pushcfunction(L, luaopen_init);
      lua_setfield(L, -2, "init");
      lua_pushcfunction(L, luaopen_luvibundle);
      lua_setfield(L, -2, "luvibundle");
      lua_pushcfunction(L, luaopen_luvipath);
      lua_setfield(L, -2, "luvipath");
      luaL_requiref(L, "bit", luaopen_bit, 1);
      lua_pop(L, 1);
  }
#endif

#ifdef WITH_CUSTOM
  luvi_custom(L);
#endif
  return L;
}


static void vm_release(lua_State*L) {
  lua_close(L);
}

LUALIB_API int luaopen_luviruntime(lua_State *L){

  int errfunc;

  char* argv[] = {"love"};
  char **argv2 = uv_setup_args(1, argv);

  luv_set_thread_cb(vm_acquire, vm_release);

  // Get package.loaded so that we can load modules
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "loaded");

  // load luv into uv in advance so that the metatables for async work.
  lua_pushcfunction(L, luaopen_luv);
  lua_call(L, 0, 1);
  lua_setfield(L, -2, "uv");

  // remove package.loaded
  lua_remove(L, -1);

  // Get package.preload so we can store builtins in it.
  lua_getfield(L, -1, "preload");
  lua_remove(L, -2); // Remove package

  lua_pushcfunction(L, luaopen_env);
  lua_setfield(L, -2, "env");

  lua_pushcfunction(L, luaopen_miniz);
  lua_setfield(L, -2, "miniz");

  lua_pushcfunction(L, luaopen_snapshot);
  lua_setfield(L, -2, "snapshot");

  #ifdef WITH_LPEG
    lua_pushcfunction(L, luaopen_lpeg);
    lua_setfield(L, -2, "lpeg");
  #endif

  #ifdef WITH_PCRE
    lua_pushcfunction(L, luaopen_rex_pcre);
    lua_pushvalue(L, -1);
    lua_setfield(L, -3, "rex_pcre");
    lua_setfield(L, -2, "rex");
  #endif

    // Store luvi module definition at preload.luvi
    lua_pushcfunction(L, luaopen_luvi);
    lua_setfield(L, -2, "luvi");

  #ifdef WITH_OPENSSL
    // Store openssl module definition at preload.openssl
    lua_pushcfunction(L, luaopen_openssl);
    lua_setfield(L, -2, "openssl");
  #endif

  #ifdef WITH_ZLIB
    // Store zlib module definition at preload.zlib
    lua_pushcfunction(L, luaopen_zlib);
    lua_setfield(L, -2, "zlib");
  #endif

  #ifdef WITH_PLAIN_LUA
    {
        LUALIB_API int luaopen_init(lua_State *L);
        LUALIB_API int luaopen_luvibundle(lua_State *L);
        LUALIB_API int luaopen_luvipath(lua_State *L);
        lua_pushcfunction(L, luaopen_init);
        lua_setfield(L, -2, "init");
        lua_pushcfunction(L, luaopen_luvibundle);
        lua_setfield(L, -2, "luvibundle");
        lua_pushcfunction(L, luaopen_luvipath);
        lua_setfield(L, -2, "luvipath");
        luaL_requiref(L, "bit", luaopen_bit, 1);
        lua_pop(L, 1);
    }
  #endif

  #ifdef WITH_CUSTOM
    luvi_custom(L);
  #endif


#ifdef WITH_WINSVC
  // Store luvi module definition at preload.openssl
  lua_pushcfunction(L, luaopen_winsvc);
  lua_setfield(L, -2, "winsvc");
  lua_pushcfunction(L, luaopen_winsvcaux);
  lua_setfield(L, -2, "winsvcaux");
#endif


  /* push debug function */
  lua_pushcfunction(L, luvi_traceback);
  errfunc = lua_gettop(L);

  return 0;
}