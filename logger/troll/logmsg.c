#include "logmsg.h"
#include "troll.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

static LevelInfo levels_inf[TROLL_MAX_LEVELS];

void init_levels(){
  for (int i = 0; i < TROLL_MAX_LEVELS; i++) {
    levels_inf[i].max_body_len = 128;
    levels_inf[i].name = NULL;
    levels_inf[i].format = NULL;
    levels_inf[i].active = true;
  }
}

LevelInfo* register_level(int level, const char* level_name, const char* level_format){
  if (level>=TROLL_MAX_LEVELS || level_name == NULL){
   return NULL;
  }
  levels_inf[level].name = level_name;
  levels_inf[level].format = level_format;
  return &levels_inf[level];
}


/// @return NULL для незарегестрированных уровней
LevelInfo* level_info(int level){
  if (level>=TROLL_MAX_LEVELS ||  levels_inf[level].name==NULL){
    return NULL;
  } else {
    return &levels_inf[level];
  }
}


/// Generate prefix to buf and return prefix length
int make_prefix(int level, struct timespec sp, char *buf, size_t buf_sz) {
  // TODO  True parsing of level_format
  if (!buf) return 0;
  LevelInfo* li = level_info(level);
  size_t level_name_len = strlen(li->name);
  size_t level_fmt_len = strlen(li->format);
  if (level_fmt_len + level_name_len + 32 < buf_sz) {
    buf[0]='\0';
    snprintf(buf, buf_sz, "[ %s ", li->name);
    strftime(buf+strlen(buf), buf_sz-strlen(buf), "%D %T", gmtime(&sp.tv_sec));
    snprintf(buf+strlen(buf), buf_sz-strlen(buf), ".%ld ] ", sp.tv_nsec/1000000);
    return strlen(buf);
  } else if (buf_sz>15) {
    buf[0]='\0';
    strcpy(buf, "[!format error]");
    return strlen(buf) + 1;
  }
  return 0;
  
}
