
#include "parser.h"
#include "assert.h"
#include "json.h"

#include <stdio.h>

static const char *find_area(JsonNode *node, char errmsg[ERR_LEN]) {
  assert(node);
  JsonNode *ar = json_find_member(node, "nearest_area");
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 'nearest_area' \n");
    return NULL;
  }
  ar = json_find_element(ar, 0);
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 1st obj in 'neares_area' \n");
    return NULL;
  }
  ar = json_find_member(ar, "areaName");
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 'areaName' \n");
    return NULL;
  }
  ar = json_find_element(ar, 0);
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 1st obj in 'areaName' \n");
    return NULL;
  }
  ar = json_find_member(ar, "value");
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't 'value' in 1st obj in 'areaName' \n");
    return NULL;
  }
  if (ar->tag != JSON_STRING) {
    snprintf(errmsg, ERR_LEN, "invalid type of 'value' in 'areaName' \n");
    return NULL;
  }
  return ar->string_;
}


static const char *find_descr(JsonNode *node, const char* lang, char errmsg[ERR_LEN]) {
  assert(node);
  JsonNode *ar = json_find_member(node, "current_condition");
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 'current_condition' \n");
    return NULL;
  }
  ar = json_find_element(ar, 0);
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 1st obj in 'current_condition' \n");
    return NULL;
  }
  
  const char *name = lang? lang : "weatherDesc";
  ar = json_find_member(ar, name);
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find '%s' \n", name);
    return NULL;
  }
  ar = json_find_element(ar, 0);
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 1st obj in 'weatherDesc' \n");
    return NULL;
  }
  ar = json_find_member(ar, "value");
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't 'value' in 1st obj in 'weatherDesc' \n");
    return NULL;
  }
  if (ar->tag != JSON_STRING) {
    snprintf(errmsg, ERR_LEN, "invalid type of 'value' in 'weatherDesc' \n");
    return NULL;
  }
  return ar->string_;
}


static const char *find_wind_degree(JsonNode *node, char errmsg[ERR_LEN]) {
  assert(node);
  JsonNode *ar = json_find_member(node, "current_condition");
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 'current_condition' \n");
    return NULL;
  }
  ar = json_find_element(ar, 0);
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 1st obj in 'current_condition' \n");
    return NULL;
  }
  ar = json_find_member(ar, "winddirDegree");
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 'winddirDegree' \n");
    return NULL;
  }
  if (ar->tag != JSON_STRING) {
    snprintf(errmsg, ERR_LEN, "invalid type of 'winddirDegree'\n");
    return NULL;
  }
  return ar->string_;
}


static const char *find_wind_speed(JsonNode *node, char errmsg[ERR_LEN]) {
  assert(node);
  JsonNode *ar = json_find_member(node, "current_condition");
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 'current_condition' \n");
    return NULL;
  }
  ar = json_find_element(ar, 0);
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 1st obj in 'current_condition' \n");
    return NULL;
  }
  ar = json_find_member(ar, "windspeedKmph");
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 'windspeedKmph' \n");
    return NULL;
  }
  if (ar->tag != JSON_STRING) {
    snprintf(errmsg, ERR_LEN, "invalid type of 'windspeedKmph'\n");
    return NULL;
  }
  return ar->string_;
}


static const char *find_temp_c(JsonNode *node, char errmsg[ERR_LEN]) {
  assert(node);
  JsonNode *ar = json_find_member(node, "current_condition");
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 'current_condition' \n");
    return NULL;
  }
  ar = json_find_element(ar, 0);
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 1st obj in 'current_condition' \n");
    return NULL;
  }
  ar = json_find_member(ar, "temp_C");
  if (ar == NULL) {
    snprintf(errmsg, ERR_LEN, "can't find 'temp_C' \n");
    return NULL;
  }
  if (ar->tag != JSON_STRING) {
    snprintf(errmsg, ERR_LEN, "invalid type of 'temp_C'\n");
    return NULL;
  }
  return ar->string_;
}


bool parse_wrrt(const char *json_str, char *out, size_t out_sz, char errmsg[ERR_LEN]) {
  assert(out);
  JsonNode *node = json_decode(json_str);
  if (node == NULL) {
    sprintf(errmsg, "can't parse JSON content \n");
    return false;
  }
  if (json_check(node, errmsg) == false) {
    json_delete(node);
    return false;
  };
  const char *area = find_area(node, errmsg);
  if (!area) {
    json_delete(node);
    return false;
  }
  const char *descr = find_descr(node, "lang_ru", errmsg);
  if (!descr) {
    json_delete(node);
    return false;
  }
  const char *wind_degree = find_wind_degree(node, errmsg);
  if (!wind_degree) {
    json_delete(node);
    return false;
  }
  const char *wind_speed = find_wind_speed(node, errmsg);
  if (!wind_speed) {
    json_delete(node);
    return false;
  }
  const char *temp_c = find_temp_c(node, errmsg);
  if (!temp_c) {
    json_delete(node);
    return false;
  }
  snprintf(out, out_sz, "%s: %s, ветер в направлении %s° со скростью %s км/ч, температура %s℃", area,
           descr, wind_degree, wind_speed, temp_c);
  json_delete(node);
  return true;
}