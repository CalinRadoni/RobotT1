#ifndef ESPCamera_H
#define ESPCamera_H

#include "esp_camera.h"

class ESPCamera {
public:
    ESPCamera(void);
    virtual ~ESPCamera();

    void InitializeConfig(void);

    bool Initialize(bool usePSRAM);

    bool Reset(void);
    bool PowerDown(void);
    bool KeepPowerDownOnDeepSleep(void);

    void Deinit(void);

    bool IsInitialized(void);

    void PrintCameraInfo(void);
    bool PrintGains(unsigned long ms);

    camera_fb_t* GetImage(void);
    camera_fb_t* GetImage_wait(void);
    void ReleaseImage(camera_fb_t*);

protected:
    bool initialized;
    camera_config_t cameraConfig;

    int xYAVG;
    int xAGC;
    int xAEC;
    bool ReadGains(void);
};

#endif
