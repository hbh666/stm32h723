# TronOneH7_Scaffold

# 创一达妙开发板脚手架

介绍

创一工作室基于达妙H7(MC02)
开发板编写的c/cpp框架，使用Clion作为IDE，CMake+Ninja构建，arm-none-eabi-gcc（首选）或Starm-Clang工具链（根据系统及处理器架构，环境情况自动检测），Ozone/Clion+OpenOCD/MDK进行调试，可以下载STM32CubeCLT进行环境安装，一键编译。

## 功能介绍和展示

### 起源

### 优势

- 轻量级框架，易于上手
- 现代开发工具链，代码静态检查

---

### 效果展示

实战展示：

### 可用功能

#### bsp封装

#### 模块封装

#### 应用封装

---

## 架构

总览。

### 软件栈

### 设计思想

## 执行顺序与数据流

### 初始化

### 数据流

---

## 开发工具

介绍完整的工作流。

### 工具链及环境配置

### CMake

### 调试和性能分析

---

## 如何使用本框架

仓库中有各种各样的说明文档和使用帮助。

### 编译烧录

### 基本文档

### 阅读代码

### 运行单个bsp/module测试

### Clion集成工具

---

## 后续计划

---

## 致谢

本项目高度参考以下项目的代码/库文件或仅参考部分设计思想，它们是：

湖南大学-跃鹿 [Basic_framework](https://gitee.com/hnuyuelurm/basic_framework)

大连民族大学-C.ONE [One_framework](https://gitee.com/dlmu-cone/one_framework)

辽宁科技大学-COD [COD_H7_Template_CLion](https://github.com/GrassFanWang/COD_H7_Template_CLion)

深圳北理莫斯科大学-北极熊 [StandardRobot++](https://gitee.com/SMBU-POLARBEAR/StandardRobotpp)

中国科学技术大学-RoboWalker [麻神的PPT及舵轮例程，以及damiao_mc02_bsp库的内容](https://gitee.com/yssickjgd/RoboWalker2023_sentry)

大连理工大学-凌Bug LingRoboMX(暂未开源)

西南交通大学-Helios Helios_powerful_framework(暂未开源)

青岛大学 [XRobot](https://xrobot-org.github.io/)

吉林大学-吉甲大师 [UIML](https://github.com/wuchuang2001/UIML)

哈工大深圳-南工骁鹰 [EC2022](https://rmbbs.robomaster.com/article/19321)

---

使用了以下开源库：

大连民族大学-C.ONE [rpl](https://gitee.com/dlmu-cone/rpl)

大连民族大学-C.ONE [ems](https://gitee.com/dlmu-cone/ems)

哈工程-创梦之翼(不过玺佬现在应该是在南科大叭) [IMU驱动控制程序-四元数及欧拉角解算](https://github.com/WangHongxi2001/RoboMaster-C-Board-INS-Example)

西交利物浦大学-GMaster [电机功率模型](https://github.com/MaxwellDemonLin/Motor-modeling-and-power-control)

香港科技大学-ENTERPRIZE [功率控制算法](https://github.com/hkustenterprize/RM2024-PowerModule)

华南理工大学-华南虎 [电控SRML库](https://github.com/scutrobotlab/RM2021_Hero/tree/master/Chassis/SRML)

华师佛山-VANGUARD [电控弹道解算例程](https://github.com/CodeAlanqian/SolveTrajectory)

达妙科技H7(MC02)开发板说明书及例程 ：[达妙H7(MC02)开发板官方资料](https://gitee.com/kit-miao/dm-mc02)

---

还要非常感谢愿意与我队电控一起交流，并无私提供帮助的这些队伍，他们是（以下排名不分先后）：

首都师范大学 W.PIE

河北工业大学 山海机甲

四川大学 火锅

重庆大学 千里

成都信息工程大学 风信子

华北理工大学 HORIZON

厦门理工大学 PFA

大连交通大学 TOE

大连大学 智能引擎

哈尔滨工程大学 创梦之翼

北京信息科技大学 星辰

沈阳航空航天大学 TUP

西北工业大学 WMJ

电子科技大学中山学院 RoboBraver

南方科技大学 ARTINX

厦门大学 RCS

天津理工大学 LIF

天津工业大学 304

华东理工大学 Origin

谢谢大家。