//
// Created by divmone on 9/25/2026.
//

#pragma once

struct EngineStats {
    float frameTime{0.0f};
    int triangleCount{0};
    int drawcallCount{0};
    float sceneUpdateTime{0.0f};
    float meshDrawTime{0.0f};
};
