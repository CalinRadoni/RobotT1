#include "ESPCamera.h"

#include "camera_pins.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "ESPCamera";

ESPCamera::ESPCamera(void)
{
    ceqChecks = 5;
    maxChecks = 99;

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

    cameraConfig.ledc_timer   = LEDC_TIMER_0;
    cameraConfig.ledc_channel = LEDC_CHANNEL_0;

    cameraConfig.pixel_format = PIXFORMAT_JPEG;     // YUV422, GRAYSCALE, RGB565, JPEG
    cameraConfig.frame_size   = FRAMESIZE_UXGA;     // QQVGA-UXGA Do not use sizes above QVGA when not JPEG
    cameraConfig.jpeg_quality = 8;                  // 0 - 63 lower number means higher quality
    cameraConfig.fb_count     = 1;                  // if more than one, i2s runs in continuous mode. Use only with JPEG
    cameraConfig.grab_mode    = CAMERA_GRAB_LATEST; // also, CAMERA_GRAB_WHEN_EMPTY
}

bool ESPCamera::Initialize(bool usePSRAM)
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
    if (ESP_OK != err) {
        ESP_LOGE(TAG, "esp_camera_init error %d: %s", err, esp_err_to_name(err));
    }
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


extern "C" {
    uint8_t SCCB_Read(uint8_t slv_addr, uint8_t reg);
    uint8_t SCCB_Write(uint8_t slv_addr, uint8_t reg, uint8_t data);
};

#define YAVG  0x2F
#define GAIN  0x00
#define AEC   0x10
#define REG04 0x04
#define REG45 0x45

int read_reg_sensor(sensor_t *sensor, uint8_t reg)
{
    if(SCCB_Write(sensor->slv_addr, 0xFF, 1) != 0) {
        return 0;
    }
    return SCCB_Read(sensor->slv_addr, reg);
}

uint8_t get_reg_bits_sensor(sensor_t *sensor, uint8_t reg, uint8_t offset, uint8_t mask)
{
    return (read_reg_sensor(sensor, reg) >> offset) & mask;
}

bool ESPCamera::PrintGains(unsigned long ms) {
    if (!ReadGains()) {
        return false;
    }

    ESP_LOGI(TAG, "%8d : YAVG %04d, AGC %03d, AEC %04d", ms, xYAVG, xAGC, xAEC);
    return true;
}
bool ESPCamera::ReadGains(void)
{
    sensor_t *camSensor = esp_camera_sensor_get();
    if (camSensor == NULL) return false;

    xYAVG = read_reg_sensor(camSensor, YAVG);

    xAGC = read_reg_sensor(camSensor, GAIN);

    xAEC = ((uint16_t)get_reg_bits_sensor(camSensor, REG45, 0, 0x3F) << 10)
         | ((uint16_t)read_reg_sensor(camSensor, AEC) << 2)
         | get_reg_bits_sensor(camSensor, REG04, 0, 3); // range is 0 - 1200

    return true;
}

camera_fb_t* ESPCamera::GetImage()
{
    return esp_camera_fb_get();
}

camera_fb_t* ESPCamera::GetImage_wait(void)
{
    camera_fb_t *pic = NULL;

    pic = esp_camera_fb_get();
    ReadGains();

    unsigned int cnt = 0;
    unsigned int cntAGC = 0;
    unsigned int cntAEC = 0;
    int prevAGC;
    int prevAEC;
    bool done = false;

    prevAGC = xAGC;
    prevAEC = xAEC;

    while ((cnt < maxChecks) && !done) {
        esp_camera_fb_return(pic);
        pic = esp_camera_fb_get();
        ReadGains();

        if (prevAGC == xAGC) { ++cntAGC; }
        else {
            prevAGC = xAGC;
            cntAGC = 0;
        }
        if (prevAEC == xAEC) { ++cntAEC; }
        else {
            prevAEC = xAEC;
            cntAEC = 0;
        }
        ++cnt;

        done = true;
        if (cntAGC < ceqChecks) done = false;
        if (cntAEC < ceqChecks) done = false;
    }

    ESP_LOGI(TAG, "YAVG %d, AGC %d, AEC %d", xYAVG, xAGC, xAEC);
    ESP_LOGI(TAG, "cntAGC %d, cntAEC %d, cnt %d", cntAGC, cntAEC, cnt);

    return pic;
}

void ESPCamera::ReleaseImage(camera_fb_t *pic)
{
    if (pic != NULL) {
        esp_camera_fb_return(pic);
    }
}

void ESPCamera::SetConsecutiveChecks(unsigned int val)
{
    ceqChecks = val;
}

void ESPCamera::SetMaxChecks(unsigned int val)
{
    maxChecks = val;
}
