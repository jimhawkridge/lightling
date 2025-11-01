#include "cJSON.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "control.h"
#include "fixturetypes.h"
#include "rig.h"

#include "ico/favicon256.h"
#include "ico/favicon32.h"

static const char* TAG = "WEB";

extern const char index_html[] asm("_binary_index_html_start");
extern const char spec_html[] asm("_binary_spec_html_start");
extern const char style_css[] asm("_binary_style_css_start");
extern const char mui_min_js[] asm("_binary_mui_min_js_start");
extern const char mui_min_css[] asm("_binary_mui_min_css_start");

static esp_err_t index_get_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "GET index");
  httpd_resp_send(req, index_html, strlen(index_html));
  return ESP_OK;
}

static const httpd_uri_t index_page = {.uri = "/",
                                       .method = HTTP_GET,
                                       .handler = index_get_handler};

static esp_err_t spec_page_get_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "GET spec page");
  httpd_resp_send(req, spec_html, strlen(spec_html));
  return ESP_OK;
}

static const httpd_uri_t spec_page = {.uri = "/spec.html",
                                      .method = HTTP_GET,
                                      .handler = spec_page_get_handler};

static esp_err_t style_get_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "GET style");
  httpd_resp_set_type(req, "text/css");
  httpd_resp_send(req, style_css, strlen(style_css));
  return ESP_OK;
}

static const httpd_uri_t style_css_get = {.uri = "/style.css",
                                          .method = HTTP_GET,
                                          .handler = style_get_handler};

static esp_err_t js_get_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "GET js");
  httpd_resp_set_type(req, "application/javascript");
  httpd_resp_send(req, mui_min_js, strlen(mui_min_js));
  return ESP_OK;
}

static const httpd_uri_t mui_js = {.uri = "/mui.min.js",
                                   .method = HTTP_GET,
                                   .handler = js_get_handler};

static esp_err_t css_get_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "GET css");
  httpd_resp_set_type(req, "text/css");
  httpd_resp_send(req, mui_min_css, strlen(mui_min_css));
  return ESP_OK;
}

static const httpd_uri_t mui_css = {.uri = "/mui.min.css",
                                    .method = HTTP_GET,
                                    .handler = css_get_handler};

char* get_state() {
  Rig* rig = rig_get();

  cJSON* state_json = cJSON_CreateObject();
  cJSON* groups_json = cJSON_CreateArray();
  for (int i = 0; i < rig->n_groups; i++) {
    FixtureGroup* fixture_group = rig->groups[i];
    cJSON* group_json = cJSON_CreateObject();
    cJSON* gid = cJSON_CreateNumber(i);
    cJSON_AddItemToObject(group_json, "id", gid);
    cJSON* gname = cJSON_CreateString(fixture_group->name);
    cJSON_AddItemToObject(group_json, "name", gname);
    cJSON* gmanual =
        fixture_group->manual ? cJSON_CreateTrue() : cJSON_CreateFalse();
    cJSON_AddItemToObject(group_json, "manual", gmanual);
    cJSON_AddItemToArray(groups_json, group_json);

    cJSON* fixtures_json = cJSON_CreateArray();
    for (int j = 0; j < fixture_group->n_fixtures; j++) {
      Fixture* fixture = fixture_group->fixtures[j];
      cJSON* fixture_json = cJSON_CreateObject();
      cJSON* fid = cJSON_CreateNumber(j);
      cJSON_AddItemToObject(fixture_json, "id", fid);
      cJSON* fname = cJSON_CreateString(fixture->name);
      cJSON_AddItemToObject(fixture_json, "name", fname);
      cJSON* fon = cJSON_CreateBool(fixture->on);
      cJSON_AddItemToObject(fixture_json, "on", fon);
      cJSON_AddItemToArray(fixtures_json, fixture_json);
    }
    cJSON_AddItemToObject(group_json, "fixtures", fixtures_json);
  }
  cJSON_AddItemToObject(state_json, "groups", groups_json);

  // Caller must free returned value
  char* string = cJSON_Print(state_json);
  cJSON_Delete(state_json);
  return string;
}

static esp_err_t state_get_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "GET state");
  char* string = get_state();
  httpd_resp_send(req, string, strlen(string));
  free(string);
  return ESP_OK;
}

static const httpd_uri_t state_json = {.uri = "/state.json",
                                       .method = HTTP_GET,
                                       .handler = state_get_handler};

static esp_err_t favicon32_get_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "GET icon32");
  httpd_resp_send(req, favicon32_png, favicon32_png_len);
  return ESP_OK;
}

static const httpd_uri_t favicon32 = {.uri = "/favicon32.png",
                                      .method = HTTP_GET,
                                      .handler = favicon32_get_handler};

static esp_err_t favicon256_get_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "GET icon256");
  httpd_resp_send(req, favicon256_png, favicon256_png_len);
  return ESP_OK;
}

static const httpd_uri_t favicon256 = {.uri = "/favicon256.png",
                                       .method = HTTP_GET,
                                       .handler = favicon256_get_handler};

static esp_err_t control_post_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "POST control");
  const int BUF_LEN = 100;
  char buf[BUF_LEN];
  int ret;
  if (req->content_len >= BUF_LEN) {
    ESP_LOGE(TAG, "POST body is too long");
    return ESP_FAIL;
  }

  if ((ret = httpd_req_recv(req, buf, req->content_len)) <= 0) {
    ESP_LOGE(TAG, "Read failed");
    return ESP_FAIL;
  }
  buf[ret] = '\0';

  ESP_LOGI(TAG, "Buf is %s", buf);
  cJSON* json = cJSON_Parse(buf);
  cJSON* group_id = cJSON_GetObjectItemCaseSensitive(json, "id");
  bool fail = false;
  if (!cJSON_IsNumber(group_id)) {
    ESP_LOGE(TAG, "JSON group_id wrong");
    fail = true;
  }
  cJSON* manual = cJSON_GetObjectItemCaseSensitive(json, "manual");
  if (!cJSON_IsBool(manual)) {
    ESP_LOGE(TAG, "JSON manual not bool");
    fail = true;
  }
  if (fail) {
    cJSON_Delete(json);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Requesting manual %d for group %d", cJSON_IsTrue(manual),
           group_id->valueint);
  control_set_mode_from_id(group_id->valueint, cJSON_IsTrue(manual));
  cJSON_Delete(json);

  char* string = get_state();
  httpd_resp_send_chunk(req, string, strlen(string));
  free(string);
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

static const httpd_uri_t control_post = {.uri = "/control",
                                         .method = HTTP_POST,
                                         .handler = control_post_handler,
                                         .user_ctx = NULL};

static esp_err_t light_post_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "post light");
  const int BUF_LEN = 100;
  char buf[BUF_LEN];
  int ret;
  if (req->content_len >= BUF_LEN) {
    ESP_LOGE(TAG, "POST body is too long");
    return ESP_FAIL;
  }

  if ((ret = httpd_req_recv(req, buf, req->content_len)) <= 0) {
    ESP_LOGE(TAG, "Read failed");
    return ESP_FAIL;
  }
  buf[ret] = '\0';

  cJSON* json = cJSON_Parse(buf);
  cJSON* group_id = cJSON_GetObjectItemCaseSensitive(json, "group_id");
  cJSON* fixture_id = cJSON_GetObjectItemCaseSensitive(json, "fixture_id");
  bool fail = false;
  if (!cJSON_IsNumber(group_id)) {
    ESP_LOGE(TAG, "JSON group_id wrong");
    fail = true;
  }
  if (!cJSON_IsNumber(fixture_id)) {
    ESP_LOGE(TAG, "JSON fixture_id wrong");
    fail = true;
  }
  cJSON* state = cJSON_GetObjectItemCaseSensitive(json, "state");
  if (!cJSON_IsBool(state)) {
    ESP_LOGE(TAG, "JSON state wrong");
    fail = true;
  }
  if (fail) {
    cJSON_Delete(json);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Requesting state %d for group %d, fixture %d",
           cJSON_IsTrue(state), group_id->valueint, fixture_id->valueint);
  fixture_switch_ids(group_id->valueint, fixture_id->valueint,
                     cJSON_IsTrue(state));
  cJSON_Delete(json);

  char* string = get_state();
  httpd_resp_send_chunk(req, string, strlen(string));
  free(string);
  httpd_resp_send_chunk(req, NULL, 0);
  return ESP_OK;
}

static const httpd_uri_t light_post = {.uri = "/light",
                                       .method = HTTP_POST,
                                       .handler = light_post_handler,
                                       .user_ctx = NULL};

esp_err_t http_404_error_handler(httpd_req_t* req, httpd_err_code_t err) {
  httpd_resp_send_err(
      req, HTTPD_404_NOT_FOUND,
      "You seem lost. <a href=\"/\">Click me to get found again!</a>");
  return ESP_FAIL;
}

static esp_err_t spec_get_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "GET spec");
  char* string = rig_get_json();
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, string, strlen(string));
  free(string);
  return ESP_OK;
}

static const httpd_uri_t spec_get = {.uri = "/spec.json",
                                     .method = HTTP_GET,
                                     .handler = spec_get_handler};

static esp_err_t spec_post_handler(httpd_req_t* req) {
  ESP_LOGI(TAG, "POST spec");
  int ret = 0;
  if (req->content_len > 1 << 16) {
    ESP_LOGE(TAG, "POST body is too long");
    return ESP_FAIL;
  }
  char* buf = malloc(req->content_len + 1);
  int left = req->content_len;
  while (left > 0) {
    ESP_LOGI(TAG, "ret: %d, left: %d, len: %d", ret, left, req->content_len);
    if ((ret = httpd_req_recv(req, buf + ret, left)) <= 0) {
      ESP_LOGE(TAG, "Read failed");
      free(buf);
      return ESP_FAIL;
    }
    left -= ret;
  }
  buf[req->content_len - left] = '\0';

  char error_buf[RIG_ERROR_BUF_LEN] = "";
  ESP_LOGI(TAG, "Expected len %d", req->content_len);
  ESP_LOGI(TAG, "Read %d chars", ret);
  ESP_LOGI(TAG, "Spec is %s", buf);
  rig_import(buf, error_buf);
  free(buf);

  const int len = RIG_ERROR_BUF_LEN + 16;
  char string[len];
  snprintf(string, len, "{\"error\": \"%s\"}", error_buf);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_send(req, string, strlen(string));

  return ESP_OK;
}

static const httpd_uri_t spec_post = {.uri = "/spec.json",
                                      .method = HTTP_POST,
                                      .handler = spec_post_handler};

static httpd_handle_t server = NULL;

void webapp_start() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 15;

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &index_page);
    httpd_register_uri_handler(server, &spec_page);
    httpd_register_uri_handler(server, &style_css_get);
    httpd_register_uri_handler(server, &mui_js);
    httpd_register_uri_handler(server, &mui_css);
    httpd_register_uri_handler(server, &state_json);
    httpd_register_uri_handler(server, &control_post);
    httpd_register_uri_handler(server, &light_post);
    httpd_register_uri_handler(server, &spec_get);
    httpd_register_uri_handler(server, &spec_post);
    httpd_register_uri_handler(server, &favicon32);
    httpd_register_uri_handler(server, &favicon256);
  } else {
    ESP_LOGI(TAG, "Error starting server!");
  }
}

void webapp_stop() {
  httpd_stop(server);
}