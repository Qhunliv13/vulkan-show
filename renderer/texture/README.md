# 纹理加载和采样功能

这个模块实现了从文件加载纹理并上传到GPU进行采样的功能，参考了Godot 4.5.1的实现方式。

## 功能特性

- 从PNG文件加载纹理
- 自动创建VkImage、VkImageView和VkSampler
- 自动处理图像布局转换
- 支持在shader中进行纹理采样

## 使用方法

### 1. 加载纹理

```cpp
#include "texture/texture.h"

using namespace renderer::texture;

// 获取Vulkan对象
VkDevice device = renderer->GetDevice();
VkPhysicalDevice physicalDevice = renderer->GetPhysicalDevice();
VkCommandPool commandPool = renderer->GetCommandPool();
VkQueue graphicsQueue = renderer->GetGraphicsQueue();

// 创建纹理对象
Texture texture;

// 从文件加载纹理
if (!texture.LoadFromFile(device, physicalDevice, commandPool, graphicsQueue, 
                          "assets/test.png")) {
    // 加载失败
    return;
}
```

### 2. 在Shader中使用纹理

在fragment shader中声明纹理采样器：

```glsl
layout(set = 0, binding = 0) uniform sampler2D texSampler;

void main() {
    vec4 texColor = texture(texSampler, uv);
    outColor = texColor;
}
```

### 3. 创建Descriptor Set

需要在创建descriptor set layout时添加纹理绑定：

```cpp
VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
samplerLayoutBinding.binding = 0;
samplerLayoutBinding.descriptorCount = 1;
samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
samplerLayoutBinding.pImmutableSamplers = nullptr;
samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
```

更新descriptor set：

```cpp
VkDescriptorImageInfo imageInfo = texture.GetDescriptorInfo();

VkWriteDescriptorSet descriptorWrite = {};
descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
descriptorWrite.dstSet = descriptorSet;
descriptorWrite.dstBinding = 0;
descriptorWrite.dstArrayElement = 0;
descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
descriptorWrite.descriptorCount = 1;
descriptorWrite.pImageInfo = &imageInfo;
vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
```

### 4. 清理资源

当不再需要纹理时，记得清理：

```cpp
texture.Cleanup(device);
```

## 实现细节

- 使用staging buffer将CPU端的图像数据上传到GPU
- 自动处理图像布局转换（UNDEFINED -> TRANSFER_DST -> SHADER_READ_ONLY）
- 使用线性过滤和重复寻址模式创建采样器
- 支持RGBA格式的纹理

## 参考

参考了Godot 4.5.1中`servers/rendering/renderer_rd/storage_rd/texture_storage.cpp`的实现方式。

