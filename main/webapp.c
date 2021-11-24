#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"

#include "rig.h"

#include "ico/favicon32.h"
#include "ico/favicon256.h"

static const char *TAG = "WEB";

extern const char index_html[] asm("_binary_index_html_start");
extern const char spec_html[] asm("_binary_spec_html_start");
extern const char style_css[] asm("_binary_style_css_start");
extern const char mui_min_js[] asm("_binary_mui_min_js_start");
extern const char mui_min_css[] asm("_binary_mui_min_css_start");

static esp_err_t index_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET index");
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

static const httpd_uri_t index_page = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_get_handler};

static esp_err_t spec_page_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET spec page");
    httpd_resp_send(req, spec_html, strlen(spec_html));
    return ESP_OK;
}

static const httpd_uri_t spec_page = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = spec_page_get_handler};

static esp_err_t style_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET style");
    httpd_resp_send(req, style_css, strlen(style_css));
    return ESP_OK;
}

static const httpd_uri_t style_css_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = style_get_handler};

static esp_err_t js_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET js");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, mui_min_js, strlen(mui_min_js));
    return ESP_OK;
}

static const httpd_uri_t mui_js = {
    .uri = "/mui.min.js",
    .method = HTTP_GET,
    .handler = js_get_handler};

static esp_err_t css_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET css");
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, mui_min_css, strlen(mui_min_css));
    return ESP_OK;
}

static const httpd_uri_t mui_css = {
    .uri = "/mui.min.css",
    .method = HTTP_GET,
    .handler = css_get_handler};

char *get_state()
{
    //     board_type_t board_type = chain_type();

    //     group_info_t *group_infos;
    //     int group_count;
    //     if (board_type == LEGO_LEADER)
    //     {
    //         group_count = defs_lego(&group_infos);
    //     }
    //     else
    //     {
    //         group_count = defs_railway(&group_infos);
    //     }
    //     // ESP_LOGI(TAG, "Group count is %d", group_count);

    //     cJSON *state = cJSON_CreateObject();

    //     cJSON *groups = cJSON_CreateArray();
    //     for (int i = 0; i < group_count; i++)
    //     {
    //         // ESP_LOGI(TAG, "Group: %s", group_infos[i].name);
    //         cJSON *group = cJSON_CreateObject();
    //         cJSON *id = cJSON_CreateNumber(i);
    //         cJSON_AddItemToObject(group, "id", id);
    //         cJSON *name = cJSON_CreateString(group_infos[i].name);
    //         cJSON_AddItemToObject(group, "name", name);
    //         cJSON *mode = cJSON_CreateString(sched_get_control(i) ? "auto" : "manual");
    //         cJSON_AddItemToObject(group, "mode", mode);
    //         cJSON_AddItemToArray(groups, group);

    //         cJSON *leds = cJSON_CreateArray();
    //         for (int j = 0; j < group_infos[i].n_leds; j++)
    //         {
    //             cJSON *led = cJSON_CreateObject();
    //             cJSON *id = cJSON_CreateNumber(group_infos[i].leds[j].id);
    //             cJSON_AddItemToObject(led, "id", id);
    //             cJSON *name = cJSON_CreateString(group_infos[i].leds[j].name);
    //             cJSON_AddItemToObject(led, "name", name);
    //             cJSON *on = cJSON_CreateBool(sched_get_state(group_infos[i].leds[j].id));
    //             cJSON_AddItemToObject(led, "on", on);
    //             cJSON_AddItemToArray(leds, led);
    //         }
    //         cJSON_AddItemToObject(group, "leds", leds);
    //     }
    //     cJSON_AddItemToObject(state, "groups", groups);

    //     // Caller must free returned value
    //     char *string = cJSON_Print(state);
    //     cJSON_Delete(state);
    //     return string;
    char *string = malloc(3);
    strcpy(string, "{}");
    return string;
}

static esp_err_t state_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET state");
    char *string = get_state();
    httpd_resp_send(req, string, strlen(string));
    free(string);
    return ESP_OK;
}

static const httpd_uri_t state_json = {
    .uri = "/state.json",
    .method = HTTP_GET,
    .handler = state_get_handler};

static esp_err_t favicon32_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET icon32");
    httpd_resp_send(req, favicon32_png, favicon32_png_len);
    return ESP_OK;
}

static const httpd_uri_t favicon32 = {
    .uri = "/favicon32.png",
    .method = HTTP_GET,
    .handler = favicon32_get_handler};

static esp_err_t favicon256_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET icon256");
    httpd_resp_send(req, favicon256_png, favicon256_png_len);
    return ESP_OK;
}

static const httpd_uri_t favicon256 = {
    .uri = "/favicon256.png",
    .method = HTTP_GET,
    .handler = favicon256_get_handler};

static esp_err_t control_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST control");
    const int BUF_LEN = 100;
    char buf[BUF_LEN];
    int ret;
    if (req->content_len >= BUF_LEN)
    {
        ESP_LOGE(TAG, "POST body is too long");
        return ESP_FAIL;
    }

    if ((ret = httpd_req_recv(req, buf, req->content_len)) <= 0)
    {
        ESP_LOGE(TAG, "Read failed");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    ESP_LOGI(TAG, "Buf is %s", buf);
    cJSON *json = cJSON_Parse(buf);
    cJSON *group_id = cJSON_GetObjectItemCaseSensitive(json, "id");
    bool fail = false;
    if (!cJSON_IsNumber(group_id))
    {
        ESP_LOGE(TAG, "JSON group_id wrong");
        fail = true;
    }
    cJSON *mode = cJSON_GetObjectItemCaseSensitive(json, "mode");
    if (!cJSON_IsString(mode) || (mode->valuestring == NULL))
    {
        ESP_LOGE(TAG, "JSON mode wrong");
        fail = true;
    }
    if (fail)
    {
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Requesting mode %s for group %d", mode->valuestring, group_id->valueint);
    // sched_control(group_id->valueint, strcmp("auto", mode->valuestring) == 0);
    cJSON_Delete(json);

    char *string = get_state();
    httpd_resp_send_chunk(req, string, strlen(string));
    free(string);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t control_post = {
    .uri = "/control",
    .method = HTTP_POST,
    .handler = control_post_handler,
    .user_ctx = NULL};

static esp_err_t light_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "post light");
    const int BUF_LEN = 100;
    char buf[BUF_LEN];
    int ret;
    if (req->content_len >= BUF_LEN)
    {
        ESP_LOGE(TAG, "POST body is too long");
        return ESP_FAIL;
    }

    if ((ret = httpd_req_recv(req, buf, req->content_len)) <= 0)
    {
        ESP_LOGE(TAG, "Read failed");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    cJSON *json = cJSON_Parse(buf);
    cJSON *led_id = cJSON_GetObjectItemCaseSensitive(json, "id");
    bool fail = false;
    if (!cJSON_IsNumber(led_id))
    {
        ESP_LOGE(TAG, "JSON led_id wrong");
        fail = true;
    }
    cJSON *state = cJSON_GetObjectItemCaseSensitive(json, "state");
    if (!cJSON_IsBool(state))
    {
        ESP_LOGE(TAG, "JSON state wrong");
        fail = true;
    }
    if (fail)
    {
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Requesting state %d for led %d", cJSON_IsTrue(state), led_id->valueint);
    // sched_set_state(led_id->valueint, cJSON_IsTrue(state));
    cJSON_Delete(json);

    char *string = get_state();
    httpd_resp_send_chunk(req, string, strlen(string));
    free(string);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t light_post = {
    .uri = "/light",
    .method = HTTP_POST,
    .handler = light_post_handler,
    .user_ctx = NULL};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "You seem lost. <a href=\"/\">Click me to get found again!</a>");
    return ESP_FAIL;
}

static esp_err_t spec_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET spec");
    char *string = "{}"; //get_state();
    httpd_resp_send(req, string, strlen(string));
    // free(string);
    return ESP_OK;
}

static const httpd_uri_t spec_get = {
    .uri = "/spec.json",
    .method = HTTP_GET,
    .handler = spec_get_handler};

static esp_err_t spec_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST spec");
    int ret;
    if (req->content_len > 1 << 16)
    {
        ESP_LOGE(TAG, "POST body is too long");
        return ESP_FAIL;
    }
    char *buf = malloc(req->content_len) + 1;
    if ((ret = httpd_req_recv(req, buf, req->content_len)) <= 0)
    {
        ESP_LOGE(TAG, "Read failed");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    char error_buf[RIG_ERROR_BUF_LEN] = "";
    rig_import(buf, error_buf);
    free(buf);

    int len = RIG_ERROR_BUF_LEN + 16;
    char string[len];
    snprintf(string, len, "{\"error\": \"%s\"}", error_buf);
    httpd_resp_send(req, string, strlen(string));

    return ESP_OK;
}

static const httpd_uri_t spec_post = {
    .uri = "/spec.json",
    .method = HTTP_POST,
    .handler = spec_post_handler};

static httpd_handle_t server = NULL;

void webapp_start()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
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
    }
    else
    {
        ESP_LOGI(TAG, "Error starting server!");
    }
}

void webapp_stop()
{
    httpd_stop(server);
}