#include <nds.h>
#include "BatteryIpcService.h"
#include "sharedMemory.h"
#include <libtwl/spi/spiPmic.h>
#include <nds/system.h>

static int sPollCounter = 0;

void BatteryIpcService::Start()
{
    SHARED_IS_CHARGING = 0;
}

void BatteryIpcService::Update()
{
    if (++sPollCounter < 60)
        return;

    sPollCounter = 0;

    const u32 battery = getBatteryLevel();

    SHARED_BATTERY_STATE = battery;

    const u32 dsBattlevel = battery & BATTERY_LEVEL_MASK;

    SHARED_IS_CHARGING =
        (battery & BATTERY_CHARGER_CONNECTED) ? 1 : 0;
}