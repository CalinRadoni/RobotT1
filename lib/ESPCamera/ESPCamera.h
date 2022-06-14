#ifndef ESPCamera_H
#define ESPCamera_H

#include "esp_camera.h"

class ESPCamera {
public:
    ESPCamera(void);
    virtual ~ESPCamera();

    void InitializeConfig(void);

    esp_err_t Initialize(bool usePSRAM);

    bool Reset(void);
    bool PowerDown(void);
    bool KeepPowerDownOnDeepSleep(void);

    void Deinit(void);

    bool IsInitialized(void);

    void PrintCameraInfo(void);

protected:
    bool initialized;
    camera_config_t cameraConfig;
};

#endif
