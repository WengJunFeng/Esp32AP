#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_event_loop.h>
#include <esp_http_server.h>
#include <sys/param.h>

#include <WIFIAP.h>

#define EXAMPLE_ESP_WIFI_SSID "MaixAP"
#define EXAMPLE_ESP_WIFI_PASS "MaixPassword"

static const char *TAG = "MAIN";

/* An HTTP GET handler */
esp_err_t indexGetHandler(httpd_req_t *req) {
  char *buf;
  size_t buf_len;

  /* Get header value string length and allocate memory for length + 1, extra byte for null termination */
  buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
  if (buf_len > 1) {
    buf = new char[buf_len];
    /* Copy null terminated value string into buffer */
    if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK) {
      ESP_LOGI(TAG, "Found header => Host: %s", buf);
    }
    delete buf;
  }

  /* Read URL query string length and allocate memory for length + 1, extra byte for null termination */
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = new char[buf_len];
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      ESP_LOGI(TAG, "Found URL query => %s", buf);
      char param[32];
      httpd_resp_set_hdr(req, "Content-Type", HTTPD_TYPE_JSON);
      httpd_resp_sendstr_chunk(req, "{");
      /* Get command */
      if (httpd_query_key_value(buf, "cmd", param, sizeof(param)) == ESP_OK) {
        if (strcmp(param, "hello") == 0) {
          /* Hello message */
          httpd_resp_sendstr_chunk(req, "\"msg\":\"Hello World!\"");
        } else if (strcmp(param, "v") == 0) {
          /* Application version */
          httpd_resp_sendstr_chunk(req, "\"version\":\"1.0.0\"");
        } else {
          /* Invalid command */
          httpd_resp_sendstr_chunk(req, "\"err\":\"invalid cmd\",\"cmd\":\"");
          httpd_resp_sendstr_chunk(req, param);
          httpd_resp_sendstr_chunk(req, "\"");
        }
      } else {
        httpd_resp_sendstr_chunk(req, "\"err\":\"unknown param\"");
      }
      httpd_resp_sendstr_chunk(req, "}");
      httpd_resp_sendstr_chunk(req, NULL);
    }
    delete buf;
  }

  /* After sending the HTTP response the old HTTP request headers are lost. Check if HTTP request headers can be read now. */
  if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
    ESP_LOGI(TAG, "Request headers lost");
  }
  return ESP_OK;
}

httpd_uri_t indexUri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = &indexGetHandler,
    .user_ctx = nullptr};

httpd_handle_t start_webserver(void) {
  esp_err_t espErr;
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  /* Start the httpd server */
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  espErr = httpd_start(&server, &config);
  if (espErr == ESP_OK) {
    /* Set URI handlers */
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &indexUri);
    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}

void stop_webserver(httpd_handle_t server) {
  httpd_stop(server);
}

static esp_err_t event_handler(void *ctx, system_event_t *event) {
  httpd_handle_t *server = (httpd_handle_t *)ctx;

  switch (event->event_id) {
  case SYSTEM_EVENT_STA_START:
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
    ESP_ERROR_CHECK(esp_wifi_connect());
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
    ESP_LOGI(TAG, "Got IP: '%s'", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

    if (*server == NULL) {
      *server = start_webserver();
    }
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
    ESP_ERROR_CHECK(esp_wifi_connect());

    if (*server) {
      stop_webserver(*server);
      *server = NULL;
    }
    break;
  default:
    break;
  }
  return ESP_OK;
}

static void initialise_wifi(void *arg) {
  wifi_config_t wifi_config = {0};
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, arg));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  strcpy(reinterpret_cast<char *>(wifi_config.sta.ssid), "internet");
  strcpy(reinterpret_cast<char *>(wifi_config.sta.password), "internet_wifi");
  ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

extern "C" void app_main() {
  // static WIFIAP wifiAp;
  static httpd_handle_t server = NULL;
  // ip4_addr_t ip;
  // bool status;
  /* Initialize NVS */
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
  }

  initialise_wifi(&server);

  // status = wifiAp.softAP(EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
  // status = wifiAp.begin("internet", "internet_wifi");
  // if (status) {
  //   ip = wifiAp.getSoftAPIP();
  //   ESP_LOGI(TAG, "IP: " IPSTR "", IP2STR(&ip));
  //   server = start_webserver();
  // }
}
