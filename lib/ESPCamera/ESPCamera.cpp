#include "ESPCamera.h"

#include "camera_pins.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "ESPCamera";

ESPCamera::ESPCamera(void)
{
    initialized = false;
    InitializeConfig();
}

ESPCamera::~ESPCamera()
{
    //
}

void ESPCamera::InitializeConfig(void)
{
    cameraConfig.pin_pwdn  = PWDN_GPIO_NUM;
    cameraConfig.pin_reset = RESET_GPIO_NUM;
    cameraConfig.pin_xclk  = XCLK_GPIO_NUM;
    cameraConfig.pin_sscb_sda = SIOD_GPIO_NUM;
    cameraConfig.pin_sscb_scl = SIOC_GPIO_NUM;

    cameraConfig.pin_d7 = Y9_GPIO_NUM;
    cameraConfig.pin_d6 = Y8_GPIO_NUM;
    cameraConfig.pin_d5 = Y7_GPIO_NUM;
    cameraConfig.pin_d4 = Y6_GPIO_NUM;
    cameraConfig.pin_d3 = Y5_GPIO_NUM;
    cameraConfig.pin_d2 = Y4_GPIO_NUM;
    cameraConfig.pin_d1 = Y3_GPIO_NUM;
    cameraConfig.pin_d0 = Y2_GPIO_NUM;
    cameraConfig.pin_vsync = VSYNC_GPIO_NUM;
    cameraConfig.pin_href  = HREF_GPIO_NUM;
    cameraConfig.pin_pclk  = PCLK_GPIO_NUM;

    // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    // cameraConfig.xclk_freq_hz = 20000000;
    cameraConfig.xclk_freq_hz = 10000000;

    cameraConfig.ledc_timer = LEDC_TIMER_0;
    cameraConfig.ledc_channel = LEDC_CHANNEL_0;
    cameraConfig.pixel_format = PIXFORMAT_JPEG;  // YUV422, GRAYSCALE, RGB565, JPEG
    cameraConfig.frame_size = FRAMESIZE_UXGA;    // QQVGA-UXGA Do not use sizes above QVGA when not JPEG
    cameraConfig.jpeg_quality = 8;               // 0 - 63 lower number means higher quality
    cameraConfig.fb_count = 1;                   // if more than one, i2s runs in continuous mode. Use only with JPEG
    cameraConfig.grab_mode = CAMERA_GRAB_LATEST; // also, CAMERA_GRAB_WHEN_EMPTY
}

esp_err_t ESPCamera::Initialize(bool usePSRAM)
{
    if (initialized) {
        Deinit();
    }

    if (usePSRAM) {
        cameraConfig.frame_size = FRAMESIZE_UXGA;
        cameraConfig.jpeg_quality = 8;
        cameraConfig.fb_count = 2;
    }
    else {
        cameraConfig.frame_size = FRAMESIZE_SVGA;
        cameraConfig.jpeg_quality = 12;
        cameraConfig.fb_count = 1;
    }

    esp_err_t err = esp_camera_init(&cameraConfig);
    initialized = (ESP_OK == err);

    return initialized;
}

bool ESPCamera::Reset(void)
{
    if (cameraConfig.pin_reset < 0) {
        return false;
    }

    ESP_LOGD(TAG, "Resetting camera");
    gpio_config_t conf = { 0 };
    conf.pin_bit_mask = 1LL << cameraConfig.pin_reset;
    conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&conf);

    gpio_set_level((gpio_num_t)cameraConfig.pin_reset, 0);
    vTaskDelay(10 / portTICK_PERIOD_MS);
    gpio_set_level((gpio_num_t)cameraConfig.pin_reset, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    return true;
}

bool ESPCamera::PowerDown(void)
{
    if (cameraConfig.pin_pwdn < 0) {
        return false;
    }

    ESP_LOGD(TAG, "Power down the camera");
    gpio_config_t conf = { 0 };
    conf.pin_bit_mask = 1LL << cameraConfig.pin_pwdn;
    conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&conf);

    gpio_set_level((gpio_num_t)cameraConfig.pin_pwdn, 1);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    return true;
}

bool ESPCamera::KeepPowerDownOnDeepSleep(void)
{
    if (cameraConfig.pin_pwdn < 0) {
        return false;
    }

    gpio_deep_sleep_hold_en();
    gpio_hold_en((gpio_num_t)cameraConfig.pin_pwdn);
    return true;
}

void ESPCamera::Deinit(void)
{
    esp_camera_deinit();
    if (!PowerDown())
        Reset();

    initialized = false;
}

bool ESPCamera::IsInitialized(void)
{
    return initialized;
}

void ESPCamera::PrintCameraInfo(void)
{
    sensor_t *camSensor = esp_camera_sensor_get();
    if (camSensor == NULL) return;

    ESP_LOGI(TAG, "Camera settings: AWB %d, AWB gain %d, AGC %d, AGC gain %d, gain ceiling %d, AEC %d, AEC2 %d",
        camSensor->status.awb,
        camSensor->status.awb_gain,
        camSensor->status.agc,
        camSensor->status.agc_gain,
        camSensor->status.gainceiling,
        camSensor->status.aec,
        camSensor->status.aec2);
}
