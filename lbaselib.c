/*
** $Id: $
** Basic library
** See Copyright Notice in lua.h
*/



#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"



/*
** If your system does not support `stderr', redefine this function, or
** redefine _ERRORMESSAGE so that it won't need _ALERT.
*/
static int luaB__ALERT (lua_State *L) {
  fputs(luaL_check_string(L, 1), stderr);
  return 0;
}


/*
** Basic implementation of _ERRORMESSAGE.
** The library `liolib' redefines _ERRORMESSAGE for better error information.
*/
static int luaB__ERRORMESSAGE (lua_State *L) {
  lua_settop(L, 1);
  lua_getglobals(L);
  lua_pushstring(L, LUA_ALERT);
  lua_rawget(L, -2);
  if (lua_isfunction(L, -1)) {  /* avoid error loop if _ALERT is not defined */
    luaL_checktype(L, 1, "string");
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushstring(L, "error: ");
    lua_pushvalue(L, 1);
    lua_pushstring(L, "\n");
    lua_concat(L, 3);
    lua_call(L, 1, 0);
  }
  return 0;
}


/*
** If your system does not support `stdout', you can just remove this function.
** If you need, you can define your own `print' function, following this
** model but changing `fputs' to put the strings at a proper place
** (a console window or a log file, for instance).
*/
static int luaB_print (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    if (lua_call(L, 1, 1) != 0)
      lua_error(L, NULL);
    s = lua_tostring(L, -1);  /* get result */
    if (s == NULL)
      lua_error(L, "`tostring' must return a string to `print'");
    if (i>1) fputs("\t", stdout);
    fputs(s, stdout);
    lua_pop(L, 1);  /* pop result */
  }
  fputs("\n", stdout);
  return 0;
}


static int luaB_tonumber (lua_State *L) {
  int base = luaL_opt_int(L, 2, 10);
  if (base == 10) {  /* standard conversion */
    luaL_checktype(L, 1, "any");
    if (lua_isnumber(L, 1)) {
      lua_pushnumber(L, lua_tonumber(L, 1));
      return 1;
    }
  }
  else {
    const char *s1 = luaL_check_string(L, 1);
    char *s2;
    unsigned long n;
    luaL_arg_check(L, 2 <= base && base <= 36, 2, "base out of range");
    n = strtoul(s1, &s2, base);
    if (s1 != s2) {  /* at least one valid digit? */
      while (isspace((unsigned char)*s2)) s2++;  /* skip trailing spaces */
      if (*s2 == '\0') {  /* no invalid trailing characters? */
        lua_pushnumber(L, n);
        return 1;
      }
    }
  }
  lua_pushnil(L);  /* else not a number */
  return 1;
}


static int luaB_error (lua_State *L) {
  lua_error(L, luaL_opt_string(L, 1, NULL));
  return 0;  /* to avoid warnings */
}

static int luaB_setglobal (lua_State *L) {
  luaL_checktype(L, 2, "any");
  lua_setglobal(L, luaL_check_string(L, 1));
  return 0;
}

static int luaB_getglobal (lua_State *L) {
  lua_getglobal(L, luaL_check_string(L, 1));
  return 1;
}

static int luaB_tag (lua_State *L) {
  luaL_checktype(L, 1, "any");
  lua_pushnumber(L, lua_tag(L, 1));
  return 1;
}

static int luaB_settag (lua_State *L) {
  luaL_checktype(L, 1, "table");
  lua_pushvalue(L, 1);  /* push table */
  lua_settag(L, luaL_check_int(L, 2));
  lua_pop(L, 1);  /* remove second argument */
  return 1;  /* return first argument */
}

static int luaB_newtag (lua_State *L) {
  lua_pushnumber(L, lua_newtag(L));
  return 1;
}

static int luaB_copytagmethods (lua_State *L) {
  lua_pushnumber(L, lua_copytagmethods(L, luaL_check_int(L, 1),
                                          luaL_check_int(L, 2)));
  return 1;
}

static int luaB_globals (lua_State *L) {
  lua_getglobals(L);  /* value to be returned */
  if (!lua_isnull(L, 1)) {
    luaL_checktype(L, 1, "table");
    lua_pushvalue(L, 1);  /* new table of globals */
    lua_setglobals(L);
  }
  return 1;
}

static int luaB_rawget (lua_State *L) {
  luaL_checktype(L, 1, "table");
  luaL_checktype(L, 2, "any");
  lua_rawget(L, -2);
  return 1;
}

static int luaB_rawset (lua_State *L) {
  luaL_checktype(L, 1, "table");
  luaL_checktype(L, 2, "any");
  luaL_checktype(L, 3, "any");
  lua_rawset(L, -3);
  return 1;
}

static int luaB_settagmethod (lua_State *L) {
  int tag = (int)luaL_check_int(L, 1);
  const char *event = luaL_check_string(L, 2);
  luaL_arg_check(L, lua_isfunction(L, 3) || lua_isnil(L, 3), 3,
                 "function or nil expected");
  lua_pushnil(L);  /* to get its tag */
  if (strcmp(event, "gc") == 0 && tag != lua_tag(L, -1))
    lua_error(L, "deprecated use: cannot set the `gc' tag method from Lua");
  lua_pop(L, 1);  /* remove the nil */
  lua_settagmethod(L, tag, event);
  return 1;
}

static int luaB_gettagmethod (lua_State *L) {
  lua_gettagmethod(L, luaL_check_int(L, 1), luaL_check_string(L, 2));
  return 1;
}


static int luaB_collectgarbage (lua_State *L) {
  lua_pushnumber(L, lua_collectgarbage(L, luaL_opt_int(L, 1, 0)));
  return 1;
}


static int luaB_type (lua_State *L) {
  luaL_checktype(L, 1, "any");
  lua_pushstring(L, lua_type(L, 1));
  return 1;
}


static int luaB_next (lua_State *L) {
  luaL_checktype(L, 1, "table");
  lua_settop(L, 2);  /* create a 2nd argument if there isn't one */
  if (lua_next(L, 1))
    return 2;
  else {
    lua_pushnil(L);
    return 1;
  }
}


static int passresults (lua_State *L, int status, int oldtop) {
  if (status == 0) {
    int nresults = lua_gettop(L) - oldtop;
    if (nresults > 0)
      return nresults;  /* results are already on the stack */
    else {
      lua_pushuserdata(L, NULL);  /* at least one result to signal no errors */
      return 1;
    }
  }
  else {  /* error */
    lua_pushnil(L);
    lua_pushnumber(L, status);  /* error code */
    return 2;
  }
}

static int luaB_dostring (lua_State *L) {
  int oldtop = lua_gettop(L);
  size_t l;
  const char *s = luaL_check_lstr(L, 1, &l);
  if (*s == '\27')  /* binary files start with ESC... */
    lua_error(L, "`dostring' cannot run pre-compiled code");
  return passresults(L, lua_dobuffer(L, s, l, luaL_opt_string(L, 2, s)), oldtop);
}


static int luaB_dofile (lua_State *L) {
  int oldtop = lua_gettop(L);
  const char *fname = luaL_opt_string(L, 1, NULL);
  return passresults(L, lua_dofile(L, fname), oldtop);
}


static int luaB_call (lua_State *L) {
  int oldtop;
  const char *options = luaL_opt_string(L, 3, "");
  int err = 0;  /* index of old error method */
  int i, status;
  int n;
  luaL_checktype(L, 2, "table");
  n = lua_getn(L, 2);
  if (!lua_isnull(L, 4)) {  /* set new error method */
    lua_getglobal(L, LUA_ERRORMESSAGE);
    err = lua_gettop(L);  /* get index */
    lua_pushvalue(L, 4);
    lua_setglobal(L, LUA_ERRORMESSAGE);
  }
  oldtop = lua_gettop(L);  /* top before function-call preparation */
  /* push function */
  lua_pushvalue(L, 1);
  luaL_checkstack(L, n, "too many arguments");
  for (i=0; i<n; i++)  /* push arg[1...n] */
    lua_rawgeti(L, 2, i+1);
  status = lua_call(L, n, LUA_MULTRET);
  if (err != 0) {  /* restore old error method */
    lua_pushvalue(L, err);
    lua_setglobal(L, LUA_ERRORMESSAGE);
  }
  if (status != 0) {  /* error in call? */
    if (strchr(options, 'x'))
      lua_pushnil(L);  /* return nil to signal the error */
    else
      lua_error(L, NULL);  /* propagate error without additional messages */
    return 1;
  }
  if (strchr(options, 'p'))  /* pack results? */
    lua_error(L, "deprecated option `p' in `call'");
  return lua_gettop(L) - oldtop;  /* results are already on the stack */
}


static int luaB_tostring (lua_State *L) {
  char buff[64];
  switch (lua_type(L, 1)[2]) {
    case 'm':  /* nuMber */
      lua_pushstring(L, lua_tostring(L, 1));
      return 1;
    case 'r':  /* stRing */
      lua_pushvalue(L, 1);
      return 1;
    case 'b':  /* taBle */
      sprintf(buff, "table: %p", lua_topointer(L, 1));
      break;
    case 'n':  /* fuNction */
      sprintf(buff, "function: %p", lua_topointer(L, 1));
      break;
    case 'e':  /* usErdata */
      sprintf(buff, "userdata(%d): %p", lua_tag(L, 1), lua_touserdata(L, 1));
      break;
    case 'l':  /* niL */
      lua_pushstring(L, "nil");
      return 1;
    default:
      luaL_argerror(L, 1, "value expected");
  }
  lua_pushstring(L, buff);
  return 1;
}


static int luaB_foreachi (lua_State *L) {
  int n, i;
  luaL_checktype(L, 1, "table");
  luaL_checktype(L, 2, "function");
  n = lua_getn(L, 1);
  for (i=1; i<=n; i++) {
    lua_pushvalue(L, 2);  /* function */
    lua_pushnumber(L, i);  /* 1st argument */
    lua_rawgeti(L, 1, i);  /* 2nd argument */
    if (lua_call(L, 2, 1) != 0)
      lua_error(L, NULL);
    if (!lua_isnil(L, -1))
      return 1;
    lua_pop(L, 1);  /* remove nil result */
  }
  return 0;
}


static int luaB_foreach (lua_State *L) {
  luaL_checktype(L, 1, "table");
  luaL_checktype(L, 2, "function");
  lua_pushnil(L);  /* first index */
  for (;;) {
    if (lua_next(L, 1) == 0)
      return 0;
    lua_pushvalue(L, 2);  /* function */
    lua_pushvalue(L, -3);  /* key */
    lua_pushvalue(L, -3);  /* value */
    if (lua_call(L, 2, 1) != 0) lua_error(L, NULL);
    if (!lua_isnil(L, -1))
      return 1;
    lua_pop(L, 2);  /* remove value and result */
  }
}


static int luaB_assert (lua_State *L) {
  luaL_checktype(L, 1, "any");
  if (lua_isnil(L, 1))
    luaL_verror(L, "assertion failed!  %.90s", luaL_opt_string(L, 2, ""));
  return 0;
}


static int luaB_getn (lua_State *L) {
  luaL_checktype(L, 1, "table");
  lua_pushnumber(L, lua_getn(L, 1));
  return 1;
}


static int luaB_tinsert (lua_State *L) {
  int v = lua_gettop(L);  /* last argument: to be inserted */
  int n, pos;
  luaL_checktype(L, 1, "table");
  n = lua_getn(L, 1);
  if (v == 2)  /* called with only 2 arguments */
    pos = n+1;
  else
    pos = luaL_check_int(L, 2);  /* 2nd argument is the position */
  lua_pushstring(L, "n");
  lua_pushnumber(L, n+1);
  lua_rawset(L, 1);  /* t.n = n+1 */
  for (; n>=pos; n--) {
    lua_rawgeti(L, 1, n);
    lua_rawseti(L, 1, n+1);  /* t[n+1] = t[n] */
  }
  lua_pushvalue(L, v);
  lua_rawseti(L, 1, pos);  /* t[pos] = v */
  return 0;
}


static int luaB_tremove (lua_State *L) {
  int pos, n;
  luaL_checktype(L, 1, "table");
  n = lua_getn(L, 1);
  pos = luaL_opt_int(L, 2, n);
  if (n <= 0) return 0;  /* table is "empty" */
  lua_rawgeti(L, 1, pos);  /* result = t[pos] */
  for ( ;pos<n; pos++) {
    lua_rawgeti(L, 1, pos+1);
    lua_rawseti(L, 1, pos);  /* a[pos] = a[pos+1] */
  }
  lua_pushstring(L, "n");
  lua_pushnumber(L, n-1);
  lua_rawset(L, 1);  /* t.n = n-1 */
  lua_pushnil(L);
  lua_rawseti(L, 1, n);  /* t[n] = nil */
  return 1;
}




/*
** {======================================================
** Quicksort
** (based on `Algorithms in MODULA-3', Robert Sedgewick;
**  Addison-Wesley, 1993.)
*/


static void swap (lua_State *L, int i, int j) {
  lua_rawgeti(L, 1, i);
  lua_rawgeti(L, 1, j);
  lua_rawseti(L, 1, i);
  lua_rawseti(L, 1, j);
}

static int sort_comp (lua_State *L, int n, int r) {
  /* WARNING: the caller (auxsort) must ensure stack space */
  int res;
  if (!lua_isnil(L, 2)) {  /* function? */
    lua_pushvalue(L, 2);
    if (r) {
      lua_rawgeti(L, 1, n);   /* a[n] */
      lua_pushvalue(L, -3);  /* pivot */
    }
    else {
      lua_pushvalue(L, -2);  /* pivot */
      lua_rawgeti(L, 1, n);   /* a[n] */
    }
    if (lua_call(L, 2, 1) != 0) lua_error(L, NULL);
    res = !lua_isnil(L, -1);
  }
  else {  /* a < b? */
    lua_rawgeti(L, 1, n);   /* a[n] */
    if (r)  
      res = lua_lessthan(L, -1, -2);
    else
      res = lua_lessthan(L, -2, -1);
  }
  lua_pop(L, 1);
  return res;
}

static void auxsort (lua_State *L, int l, int u) {
  while (l < u) {  /* for tail recursion */
    int i, j;
    luaL_checkstack(L, 4, "array too large");
    /* sort elements a[l], a[(l+u)/2] and a[u] */
    lua_rawgeti(L, 1, u);
    if (sort_comp(L, l, 0))  /* a[u] < a[l]? */
      swap(L, l, u);
    lua_pop(L, 1);
    if (u-l == 1) break;  /* only 2 elements */
    i = (l+u)/2;
    lua_rawgeti(L, 1, i);  /* Pivot = a[i] */
    if (sort_comp(L, l, 0))  /* a[i]<a[l]? */
      swap(L, l, i);
    else {
      if (sort_comp(L, u, 1))  /* a[u]<a[i]? */
        swap(L, i, u);
    }
    lua_pop(L, 1);  /* pop old a[i] */
    if (u-l == 2) break;  /* only 3 elements */
    lua_rawgeti(L, 1, i);  /* Pivot */
    swap(L, i, u-1);  /* put median element as pivot (a[u-1]) */
    /* a[l] <= P == a[u-1] <= a[u], only needs to sort from l+1 to u-2 */
    i = l; j = u-1;
    for (;;) {  /* invariant: a[l..i] <= P <= a[j..u] */
      /* repeat i++ until a[i] >= P */
      while (sort_comp(L, ++i, 1))
        if (i>u) lua_error(L, "invalid order function for sorting");
      /* repeat j-- until a[j] <= P */
      while (sort_comp(L, --j, 0))
        if (j<l) lua_error(L, "invalid order function for sorting");
      if (j<i) break;
      swap(L, i, j);
    }
    swap(L, u-1, i);  /* swap pivot (a[u-1]) with a[i] */
    /* a[l..i-1] <= a[i] == P <= a[i+1..u] */
    /* adjust so that smaller "half" is in [j..i] and larger one in [l..u] */
    if (i-l < u-i) {
      j=l; i=i-1; l=i+2;
    }
    else {
      j=i+1; i=u; u=j-2;
    }
    lua_pop(L, 1);  /* remove pivot from stack */
    auxsort(L, j, i);  /* call recursively the smaller one */
  }  /* repeat the routine for the larger one */
}

static int luaB_sort (lua_State *L) {
  int n;
  luaL_checktype(L, 1, "table");
  n = lua_getn(L, 1);
  if (!lua_isnull(L, 2))  /* is there a 2nd argument? */
    luaL_checktype(L, 2, "function");
  lua_settop(L, 2);  /* make sure there is two arguments */
  auxsort(L, 1, n);
  return 0;
}

/* }====================================================== */



/*
** {======================================================
** Deprecated functions to manipulate global environment.
** =======================================================
*/


/*
** gives an explicit error in any attempt to call a deprecated function
*/
static int deprecated_func (lua_State *L) {
  luaL_verror(L, "function `%.20s' is deprecated", luaL_check_string(L, -1));
  return 0;  /* to avoid warnings */
}


#define num_deprecated	4

static const char *const deprecated_names [num_deprecated] = {
  "foreachvar", "nextvar", "rawgetglobal", "rawsetglobal"
};


static void deprecated_funcs (lua_State *L) {
  int i;
  for (i=0; i<num_deprecated; i++) {
    lua_pushstring(L, deprecated_names[i]);
    lua_pushcclosure(L, deprecated_func, 1);
    lua_setglobal(L, deprecated_names[i]);
  }
}


/* }====================================================== */

static const struct luaL_reg base_funcs[] = {
  {LUA_ALERT, luaB__ALERT},
  {LUA_ERRORMESSAGE, luaB__ERRORMESSAGE},
  {"call", luaB_call},
  {"collectgarbage", luaB_collectgarbage},
  {"copytagmethods", luaB_copytagmethods},
  {"dofile", luaB_dofile},
  {"dostring", luaB_dostring},
  {"error", luaB_error},
  {"foreach", luaB_foreach},
  {"foreachi", luaB_foreachi},
  {"getglobal", luaB_getglobal},
  {"gettagmethod", luaB_gettagmethod},
  {"globals", luaB_globals},
  {"newtag", luaB_newtag},
  {"next", luaB_next},
  {"print", luaB_print},
  {"rawget", luaB_rawget},
  {"rawset", luaB_rawset},
  {"rawgettable", luaB_rawget},  /* for compatibility */
  {"rawsettable", luaB_rawset},  /* for compatibility */
  {"setglobal", luaB_setglobal},
  {"settag", luaB_settag},
  {"settagmethod", luaB_settagmethod},
  {"tag", luaB_tag},
  {"tonumber", luaB_tonumber},
  {"tostring", luaB_tostring},
  {"type", luaB_type},
  {"assert", luaB_assert},
  {"getn", luaB_getn},
  {"sort", luaB_sort},
  {"tinsert", luaB_tinsert},
  {"tremove", luaB_tremove}
};



void lua_baselibopen (lua_State *L) {
  luaL_openl(L, base_funcs);
  lua_pushstring(L, LUA_VERSION);
  lua_setglobal(L, "_VERSION");
  deprecated_funcs(L);
}

