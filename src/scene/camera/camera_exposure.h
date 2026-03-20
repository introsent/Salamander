#pragma once

namespace Salamander::Scene {
    struct CameraExposure {
        float aperture;
        float shutterSpeed;
        float ISO;
        float ev100Override;
    };
}
