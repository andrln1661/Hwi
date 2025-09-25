#pragma once

#include "ModbusHandler.h"
#include "DeviceManager.h"
#include "Motor.h"

extern ModbusHandler *modbusHandler;
extern DeviceManager *deviceManager;
extern Motor **motors; // Changed from Motor* to Motor**