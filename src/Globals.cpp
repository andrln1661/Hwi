#include "Globals.h"

ModbusHandler *modbusHandler = nullptr;
DeviceManager *deviceManager = nullptr;
Motor **motors = nullptr; // Changed from Motor* to Motor**