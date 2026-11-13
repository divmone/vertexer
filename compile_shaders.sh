slangc ./shaders/shader.slang \
  -target spirv \
  -profile spirv_1_4 \
  -emit-spirv-directly \
  -fvk-use-entrypoint-name \
  -entry vertMain \
  -entry fragMain \
  -o ./shaders/shader.spv

  slangc ./shaders/sky.slang \
    -target spirv \
    -profile spirv_1_4 \
    -emit-spirv-directly \
    -fvk-use-entrypoint-name \
    -entry vertMain \
    -entry fragMain \
    -o ./shaders/sky.spv

  slangc ./shaders/shader_2d.slang \
    -target spirv \
    -profile spirv_1_4 \
    -emit-spirv-directly \
    -fvk-use-entrypoint-name \
    -entry vertMain \
    -entry fragMain \
    -o ./shaders/shader_2d.spv
