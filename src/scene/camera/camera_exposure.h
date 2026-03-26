//
// Created by ivans on 20/03/2026.
//

#ifndef SALAMANDER_CAMERA_EXPOSURE_H
#define SALAMANDER_CAMERA_EXPOSURE_H


namespace Salamander::Scene {
    struct CameraExposure {
        float aperture;
        float shutterSpeed;
        float ISO;
        float ev100Override;
    };
}


#endif //SALAMANDER_CAMERA_EXPOSURE_H

