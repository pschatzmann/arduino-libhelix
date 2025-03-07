#pragma once
#include "../ConfigHelix.h"
#include "esp_log.h"

// Logging Implementation
#if HELIX_LOGGING_ACTIVE
    #define TAG_HELIX "libhelix"
    #define LOGD_HELIX(...) ESP_LOGD(TAG_HELIX,__VA_ARGS__);
    #define LOGI_HELIX(...) ESP_LOGI(TAG_HELIX,__VA_ARGS__);
    #define LOGW_HELIX(...) ESP_LOGW(TAG_HELIX,__VA_ARGS__);
    #define LOGE_HELIX(...) ESP_LOGE(TAG_HELIX,__VA_ARGS__);
#else
    // Remove all log statments from the code
    #define LOGD_HELIX(...) 
    #define LOGI_HELIX(...) 
    #define LOGW_HELIX(...) 
    #define LOGE_HELIX(...) 
#endif

