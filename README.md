# Interactive TinyRenderer (with WebGL Frontend)

本项目基于 Dmitry V. Sokolov 的开源渲染器教程 [tinyrenderer](https://github.com/ssloy/tinyrenderer) 进行重构与二次开发。项目采用 C++ 核心逻辑构建光栅化管线，使用 CMake 进行工程管理，并在 Visual Studio 环境下拓展集成了 WebGL 交互式渲染前端，实现了渲染参数的实时调节与可视化多通道效果切换。

---

## 核心特性

- **完整的纯软件光栅化管线**：
  - 顶点坐标变换（MVP 矩阵管线构建）
  - 三角形重心坐标插值（Barycentric Coordinates）与光栅化
  - Z-Buffer 深度测试机制，解决遮挡与深度冲突
- **多重着色模式实时切换**：
  - 线框渲染（Wireframe Mode）
  - 平面着色（Flat Shading）
  - Phong 光照模型着色（Phong Shading）
  - 纹理贴图采样（Texture Mapping）
- **WebGL 实时交互前端**：
  - 使用 Visual Studio 搭建 WebGL 渲染展示与控制交互层
  - 支持鼠标悬浮与拖拽控制视角旋转、滚轮实时缩放（Zoom）
  - 键盘快捷键实时切换展示模型及着色算法

---

## 交互控制说明

| 操作按键 / 手势 | 对应功能 |
| :--- | :--- |
| **Key 1 / 2 / 3** | 切换模型（切换官方自带的 3 组示例模型） |
| **Key 4** | 切换至 **Wireframe**（线框渲染模式） |
| **Key 5** | 切换至 **Flat Shading**（平面着色模式） |
| **Key 6** | 切换至 **Phong Shading**（Phong 光照模式） |
| **Key 7** | 切换至 **Texture Map**（纹理贴图模式） |
| **Mouse Wheel** | 场景视距缩放（Zoom In / Out） |
| **Left Click Drag** | 旋转场景 / 观察视角（Rotate View） |

---

## 技术架构与实现说明

1. **底层渲染管线**：
   - 参考官方教程核心实现，对数据结构与变换矩阵逻辑进行了重构与对齐，保证光栅化与着色阶段的一致性与稳定性。
   - 内置官方提供的 3 组 `.obj` 模型与对应纹理贴图（Diffuse / Normal / Specular），用于算法结果比对与功能演示。

2. **前端交互与跨端展现**：
   - 传统 TinyRenderer 仅支持输出静态 `.tga` 图片；本项目通过 Visual Studio 搭建 WebGL 前端展示容器，把帧缓冲数据与 WebGL 渲染管线进行映射绑定，实现了动态帧渲染与实时键鼠交互。

---

## 构建与运行

### 依赖环境
- **C++17** 或更高版本
- **CMake**（>= 3.15）
- **Visual Studio 2022**（或配置有 WebGL / Emscripten 开发环境的对应版本）
  
## 致谢与参考
ssloy/tinyrenderer：感谢原作者 Dmitry V. Sokolov 提供的经典渲染器教程与模型资产。

## 效果参考
<img width="794" height="781" alt="image" src="https://github.com/user-attachments/assets/73dd7ae4-2f71-4e2a-aef4-ac040537280e" />
<img width="785" height="783" alt="屏幕截图 2026-10-08 223723" src="https://github.com/user-attachments/assets/e01c147c-7652-436c-917f-9c3cc4eab428" />
<img width="988" height="794" alt="屏幕截图 2026-10-08 223707" src="https://github.com/user-attachments/assets/d3aa1f6d-ed75-4ee9-b424-5c040832d80b" />
<img width="1005" height="792" alt="屏幕截图 2026-10-08 223715" src="https://github.com/user-attachments/assets/0966e98e-dd48-4216-86ce-a6b25bb90b7d" />
