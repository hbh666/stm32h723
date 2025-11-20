# 溪地创新功率计驱动

使用示例：

```c
// 在适当的地方初始化功率计（如main.c中）
PowerMeterInstance *power_meter = PowerMeterInit(&hfdcan1); // 使用FDCAN1

// 在任务循环中获取功率数据
void PowerMonitorTask(void)
{
    float voltage = PowerMeterGetVoltage(power_meter);
    float current = PowerMeterGetCurrent(power_meter);
    float power = PowerMeterGetPower(power_meter);
    float energy = PowerMeterGetEnergy(power_meter);
    
    // 打印或使用这些数据
    printf("V: %.1fV, I: %.1fA, P: %.1fW, E: %.1fJ\n", 
           voltage, current, power, energy);
}
```