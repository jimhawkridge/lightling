#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"

#include "rig.h"

static const char *TAG = "RIG";
const char *NVS_DATA_PART = "data";
const char *NVS_RIG_NS = "rig";
const char *NVS_RIG_KEY = "rig";

static Rig *the_rig = NULL;

FixtureType fixtureTypeFromString(const char *str)
{
  if (!strcasecmp("Light", str))
  {
    return Light;
  }
  else if (!strcasecmp("Fire", str))
  {
    return Fire;
  }
  else
  {
    return UnknownFixtureType;
  }
}

AutomatorType automatorTypeFromString(const char *str)
{
  if (!strcasecmp("AlwaysOn", str))
  {
    return AlwaysOn;
  }
  else if (!strcasecmp("AlwaysOff", str))
  {
    return AlwaysOff;
  }
  else if (!strcasecmp("OneUpOneDownAutomator", str))
  {
    return OneUpOneDownAutomator;
  }
  else if (!strcasecmp("TownhouseAutomator", str))
  {
    return TownhouseAutomator;
  }
  else
  {
    return UnknownAutomatorType;
  }
}

void rig_init()
{
  esp_err_t ret = nvs_flash_init_partition(NVS_DATA_PART);
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_LOGE(TAG, "Erasing and initing 'data' NVS");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
}

static void rig_free(Rig *rig)
{
  if (rig == NULL)
  {
    return;
  }

  for (int g = 0; g < rig->n_groups; g++)
  {
    FixtureGroup *group = rig->groups[g];
    if (group == NULL)
    {
      break;
    }

    for (int f = 0; f < group->n_fixtures; f++)
    {
      Fixture *fix = group->fixtures[f];
      if (fix == NULL)
      {
        break;
      }

      free(fix->name);
      free(fix->channels);
      free(fix);
    }
    free(group->name);
    free(group);
  }
  free(rig);
}

static Rig *rig_load_from_cjson(cJSON *json, char *error_buf)
{
  cJSON *groups_json = cJSON_GetObjectItemCaseSensitive(json, "groups");
  if (groups_json == NULL)
  {
    sprintf(error_buf, "Didn't find `groups` in top level object");
    return NULL;
  }
  else if (!cJSON_IsArray(groups_json))
  {
    sprintf(error_buf, "`groups` should be an array");
    return NULL;
  }

  Rig *rig = malloc(sizeof(Rig));
  rig->n_groups = cJSON_GetArraySize(groups_json);
  size_t groups_size = sizeof(FixtureGroup *) * rig->n_groups;
  rig->groups = malloc(groups_size);
  memset(rig->groups, 0, groups_size);
  uint8_t i = 0;
  cJSON *group_json;
  cJSON_ArrayForEach(group_json, groups_json)
  {
    rig->groups[i] = malloc(sizeof(FixtureGroup));
    FixtureGroup *group = rig->groups[i];
    group->manual = false;
    if (!cJSON_IsObject(group_json))
    {
      sprintf(error_buf, "`groups` must only contain objects");
      return NULL;
    }

    cJSON *fixtures_json = cJSON_GetObjectItemCaseSensitive(group_json, "fixtures");
    if (fixtures_json == NULL)
    {
      sprintf(error_buf, "Didn't find `fixtures` in `group` object");
      return NULL;
    }
    else if (!cJSON_IsArray(fixtures_json))
    {
      sprintf(error_buf, "`fixtures` should be an array");
      return NULL;
    }
    group->n_fixtures = cJSON_GetArraySize(fixtures_json);
    size_t fixtures_size = sizeof(Fixture *) * group->n_fixtures;
    group->fixtures = malloc(fixtures_size);
    memset(group->fixtures, 0, fixtures_size);

    uint8_t j = 0;
    cJSON *fixture_json;
    cJSON_ArrayForEach(fixture_json, fixtures_json)
    {
      group->fixtures[j] = malloc(sizeof(Fixture));
      Fixture *fixture = group->fixtures[j];
      fixture->on = false;
      if (!cJSON_IsObject(fixture_json))
      {
        sprintf(error_buf, "`fixtures` must only contain objects");
        return NULL;
      }

      cJSON *channels_json = cJSON_GetObjectItemCaseSensitive(fixture_json, "channels");
      if (channels_json == NULL)
      {
        sprintf(error_buf, "Didn't find `channels` in `fixture` object");
        return NULL;
      }
      else if (!cJSON_IsArray(channels_json))
      {
        sprintf(error_buf, "`channels` should be an array");
        return NULL;
      }
      fixture->n_channels = cJSON_GetArraySize(channels_json);
      size_t channels_size = sizeof(uint8_t) * fixture->n_channels;
      fixture->channels = malloc(channels_size);
      memset(fixture->channels, 0, channels_size);

      uint8_t c = 0;
      cJSON *channel_json;
      cJSON_ArrayForEach(channel_json, channels_json)
      {
        if (!cJSON_IsNumber(channel_json))
        {
          sprintf(error_buf, "`channels` should be a numbers");
          return NULL;
        }
        fixture->channels[c] = channel_json->valueint - 1; // 0 based in code, 1 based in JSON / on si
        c++;
      }

      cJSON *name_json = cJSON_GetObjectItemCaseSensitive(fixture_json, "name");
      if (name_json == NULL)
      {
        sprintf(error_buf, "Didn't find `name` in `fixture` object");
        return NULL;
      }
      else if (!cJSON_IsString(name_json))
      {
        sprintf(error_buf, "`name` should be a string");
        return NULL;
      }
      fixture->name = malloc(strlen(cJSON_GetStringValue(name_json)) + 1);
      strcpy(fixture->name, cJSON_GetStringValue(name_json));

      cJSON *type_json = cJSON_GetObjectItemCaseSensitive(fixture_json, "type");
      if (name_json == NULL)
      {
        sprintf(error_buf, "Didn't find `type` in `fixture` object");
        return NULL;
      }
      else if (!cJSON_IsString(type_json))
      {
        sprintf(error_buf, "`type` should be a string");
        return NULL;
      }
      char *type_str = cJSON_GetStringValue(type_json);
      FixtureType ftype = fixtureTypeFromString(type_str);
      if (ftype == UnknownFixtureType)
      {
        snprintf(error_buf, RIG_ERROR_BUF_LEN, "Unknown fixture type `%s`", type_str);
        return NULL;
      }
      fixture->fixture_type = ftype;

      cJSON *level_json = cJSON_GetObjectItemCaseSensitive(fixture_json, "level");
      if (level_json == NULL)
      {
        sprintf(error_buf, "Didn't find `level` in `fixture` object");
        return NULL;
      }
      else if (!cJSON_IsNumber(level_json))
      {
        sprintf(error_buf, "`level` should be a number");
        return NULL;
      }
      fixture->level = level_json->valueint;

      j++;
    }

    cJSON *name_json = cJSON_GetObjectItemCaseSensitive(group_json, "name");
    if (name_json == NULL)
    {
      sprintf(error_buf, "Didn't find `name` in `group` object");
      return NULL;
    }
    else if (!cJSON_IsString(name_json))
    {
      sprintf(error_buf, "`name` should be a string");
      return NULL;
    }
    group->name = malloc(strlen(cJSON_GetStringValue(name_json)) + 1);
    strcpy(group->name, cJSON_GetStringValue(name_json));

    cJSON *auto_json = cJSON_GetObjectItemCaseSensitive(group_json, "automator");
    if (name_json == NULL)
    {
      sprintf(error_buf, "Didn't find `automator` in `group` object");
      return NULL;
    }
    else if (!cJSON_IsString(auto_json))
    {
      sprintf(error_buf, "`automator` should be a string");
      return NULL;
    }
    char *automator_str = cJSON_GetStringValue(auto_json);
    AutomatorType automator = automatorTypeFromString(automator_str);
    if (automator == UnknownAutomatorType)
    {
      snprintf(error_buf, RIG_ERROR_BUF_LEN, "Unknown automator `%s`", automator_str);
      return NULL;
    }
    group->automator = automator;

    i++;
  }

  return rig;
}

static Rig *rig_load_from_json(const char *buf, char *error_buf) // error_buf must be ERROR_BUF_LEN bytes
{
  cJSON *json = cJSON_Parse(buf);
  if (json == NULL)
  {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL)
    {
      for (int i = 0; i < strlen(error_ptr); i++)
      {
        if (error_ptr[i] == '"')
        {
          ((char *)error_ptr)[i] = '`'; // Yeah, this is crummy :-/
        }
      }
      snprintf(error_buf, RIG_ERROR_BUF_LEN, "Error before: %s", error_ptr);
    }
    else
    {
      sprintf(error_buf, "Parse error at unknown location");
    }
    return NULL;
  }

  Rig *rig = rig_load_from_cjson(json, error_buf);
  cJSON_Delete(json);
  return rig;
}

// Omit "json" if you don't want the JSON back
// If you supply it then you own the loaded spec
static void rig_load_from_nvs(char **json)
{
  nvs_handle_t handle;
  esp_err_t err;
  char *spec;

  ESP_ERROR_CHECK(nvs_open(NVS_RIG_NS, NVS_READONLY, &handle));

  size_t required_size = 0;
  err = nvs_get_blob(handle, NVS_RIG_KEY, NULL, &required_size);
  if (err != ESP_OK)
  {
    char *default_spec = "{\"groups\": []}";
    spec = malloc(strlen(default_spec) + 1);
    strcpy(spec, default_spec);
  }
  else
  {
    spec = malloc(required_size);
    ESP_ERROR_CHECK(nvs_get_blob(handle, NVS_RIG_KEY, spec, &required_size));
  }
  nvs_close(handle);

  char error_buf[RIG_ERROR_BUF_LEN];
  error_buf[0] = '\0';
  rig_free(the_rig);
  the_rig = rig_load_from_json(spec, error_buf);
  ESP_LOGI(TAG, "Loaded rig with maybe some errors: %s", error_buf);

  if (json)
  {
    *json = spec;
  }
  else
  {
    free(spec);
  }
}

static bool rig_save_to_nvs(const char *spec)
{
  nvs_handle_t handle;
  esp_err_t err;

  ESP_ERROR_CHECK(nvs_open(NVS_RIG_NS, NVS_READWRITE, &handle));

  err = nvs_set_blob(handle, NVS_RIG_KEY, spec, strlen(spec) + 1);
  if (err != ESP_OK)
    return false;

  err = nvs_commit(handle);
  if (err != ESP_OK)
    return false;

  nvs_close(handle);
  return true;
}

Rig *rig_get()
{
  if (the_rig == NULL)
  {
    rig_load_from_nvs(NULL);
  }
  return the_rig;
}

// Called owns spec
char *rig_get_json()
{
  char *spec;
  rig_load_from_nvs(&spec);
  return spec;
}

void rig_import(const char *buf, char *error_buf)
{
  Rig *new_rig = rig_load_from_json(buf, error_buf);
  if (new_rig == NULL)
  {
    rig_free(new_rig);
    return;
  }

  bool saved = rig_save_to_nvs(buf);
  if (!saved)
  {
    sprintf(error_buf, "Parsed OK, but failed to save to storage");
    return;
  }

  Rig *old_rig = the_rig;
  the_rig = new_rig;
  rig_free(old_rig);
}
