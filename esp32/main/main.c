#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "sniffer.h"

#define LEN_MAC_ADDR 20

static const char *TAG = "LOCALIZATION";
const static int CONNECTED_BIT = BIT0;
static EventGroupHandle_t wifi_event_group;

char *RSSI_TOPIC = NULL, *CSI_TOPIC = NULL;
station_info_t *station_info = NULL;
char *rssi_data_json = NULL, *csi_data_json = NULL;
esp_mqtt_client_handle_t client; 

/* Filter out the common ESP32 MAC */
static const uint8_t esp_module_mac[32][3] = {
    {0x54, 0x5A, 0xA6}, {0x24, 0x0A, 0xC4}, {0xD8, 0xA0, 0x1D}, {0xEC, 0xFA, 0xBC},
    {0xA0, 0x20, 0xA6}, {0x90, 0x97, 0xD5}, {0x18, 0xFE, 0x34}, {0x60, 0x01, 0x94},
    {0x2C, 0x3A, 0xE8}, {0xA4, 0x7B, 0x9D}, {0xDC, 0x4F, 0x22}, {0x5C, 0xCF, 0x7F},
    {0xAC, 0xD0, 0x74}, {0x30, 0xAE, 0xA4}, {0x24, 0xB2, 0xDE}, {0x68, 0xC6, 0x3A},
    // my macs
    {0x64, 0xA2, 0xF9}, {0xAC, 0xD1, 0xB8}, {0xC4,0x8E,0x8F},
};

static const uint8_t allowed_macs[4][3] = {
    // my macs
    {0x4C, 0xED, 0xFB}, {0xB8, 0x63, 0x4D},
    //{0xC4,0x8E,0x8F},
};

/* The callback function of sniffer */
void wifi_sniffer_cb(void *recv_buf, wifi_promiscuous_pkt_type_t type)
{
    wifi_promiscuous_pkt_t *sniffer = (wifi_promiscuous_pkt_t *)recv_buf;
    sniffer_payload_t *sniffer_payload = (sniffer_payload_t *)sniffer->payload;

    // /* Check if the packet is Probe Request */
    // if (sniffer_payload->header[0] == 0x40) {
    //     return;
    // }
    // static const uint8_t aniket_mobile[3] = {0x64, 0xA2, 0xF9};
    // if(!memcmp(sniffer_payload->source_mac, aniket_mobile, 3)) return;

    // /* Filter out some useless packet */
    // for (int i = 0; i < 32; ++i) {
    //     if (!memcmp(sniffer_payload->source_mac, esp_module_mac[i], 3)) {
    //         return;
    //     }
    // }

    /* Filter in useful packet */
    bool match = false;
    for (int i = 0; i < 4; ++i) {
        if (!memcmp(sniffer_payload->source_mac, allowed_macs[i], 3)) {
            match = true;
        }
    }
    if(!match)
        return;
    /* Map station information*/
    memcpy(station_info->bssid, sniffer_payload->source_mac, sizeof(station_info->bssid));
    station_info->rssi = sniffer->rx_ctrl.rssi;
    station_info->channel = sniffer->rx_ctrl.channel;

    /* Create Json string for publishing*/
    
    sprintf(rssi_data_json, "{'MAC':'%02X:%02X:%02X:%02X:%02X:%02X','RSSI': %d,'Channel': %d,'Header':'%02X'}\n", 
            station_info->bssid[0], station_info->bssid[1], station_info->bssid[2], station_info->bssid[3], station_info->bssid[4], station_info->bssid[5], station_info->rssi, station_info->channel, sniffer_payload->header[0]);
    printf(rssi_data_json);

    esp_mqtt_client_publish(client, RSSI_TOPIC, rssi_data_json, 0, 1, 0);
}

/* The callback function of CSI */
void wifi_csi_cb(void *ctx, wifi_csi_info_t *data) {
    wifi_csi_info_t received = data[0];
    if(!received.first_word_invalid){

        // char* buf = malloc(10*received.len*sizeof(char));        
        // for(int i=0;i<received.len;i++){
        //     char* x = malloc(10*sizeof(char));
        //     sprintf(x,"%02x",received.buf[i]);
        //     strcat(buf,x);
        // }

        sprintf(csi_data_json, "{'MAC':'%02X:%02X:%02X:%02X:%02X:%02X','Channel':%d,'Secondary Channel':%d,'Signal Mode':%d,'Channel Bandwidth':%d,'STBC':%d,'RSSI':%d,'Antenna':%d,'Length':%d,'Buffer':'%s'}\n", 
            received.mac[0], received.mac[1], received.mac[2], received.mac[3], received.mac[4], received.mac[5],
            received.rx_ctrl.channel,
            received.rx_ctrl.secondary_channel, 
            received.rx_ctrl.sig_mode,
            received.rx_ctrl.cwb,
            received.rx_ctrl.stbc,
            received.rx_ctrl.rssi,
            received.rx_ctrl.ant,
            received.len,
            "buf");

        printf(csi_data_json);  

        esp_mqtt_client_publish(client, CSI_TOPIC, csi_data_json, 0, 1, 0); 
    }
    
}

static void sniffer_and_csi_init(){
    
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_cb));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(1));


    ESP_ERROR_CHECK(esp_wifi_set_csi(1));
    wifi_csi_config_t configuration_csi; // CSI = Channel State Information
    configuration_csi.lltf_en = 1;
    configuration_csi.htltf_en = 1;
    configuration_csi.stbc_htltf2_en = 1;
    configuration_csi.ltf_merge_en = 1;
    configuration_csi.channel_filter_en = 1;
    configuration_csi.manu_scale = 0; // Automatic scalling
    //configuration_csi.shift=15; // 0->15
    
    ESP_ERROR_CHECK(esp_wifi_set_csi_config(&configuration_csi));
    ESP_ERROR_CHECK(esp_wifi_set_csi_rx_cb(&wifi_csi_cb, NULL)); 
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            sniffer_and_csi_init();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}


static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            ESP_LOGI(TAG, "Connected to WIFI SSID:[%s]", CONFIG_WIFI_SSID);
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected from WIFI SSID:[%s], Retrying...", CONFIG_WIFI_SSID);
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void wifi_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.csi_enable = 1; //Enable CSI
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "Connecting to WIFI SSID:[%s]", CONFIG_WIFI_SSID);
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

static void variables_init(void){
    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, mac));

    RSSI_TOPIC = (char*)malloc(50 * sizeof(char));
    CSI_TOPIC = (char*)malloc(50 * sizeof(char));
    sprintf(RSSI_TOPIC,"/rssi/%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    sprintf(CSI_TOPIC,"/csi/%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

    ESP_LOGI(TAG, "RSSI_TOPIC:[%s]", RSSI_TOPIC);
    ESP_LOGI(TAG, "CSI_TOPIC:[%s]", CSI_TOPIC);

    station_info = malloc(sizeof(station_info_t));
    rssi_data_json = (char*)malloc(200 * sizeof(char));
    csi_data_json = (char*)malloc(1000 * sizeof(char));
}

static void mqtt_app_start(void)
{   
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
        .event_handle = mqtt_event_handler,
        // .user_context = (void *)your_context
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void app_main()
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    nvs_flash_init();
    wifi_init();
    variables_init();
    mqtt_app_start();
}
