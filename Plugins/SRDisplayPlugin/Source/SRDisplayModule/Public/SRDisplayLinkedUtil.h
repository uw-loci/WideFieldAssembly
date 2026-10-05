#pragma once

#include "xr-runtime-common/xr_api_defs.h"
#include <SRDisplayModule/Classes/Blueprint/SRDisplayProjectSettings.h>

namespace srdisplay::util
{
	extern FVector GetSRDidplsyManagerPostion(float DisplayHeight, float DisplayWidth, ELinkedMode LinkedMode, int SRDisplayNum);
	extern FVector GetSRDidplsyManagerPostionFromModel(ELinkedMode LinkedMode, int SRDisplayNum, bool isSR1);
	extern void SortDeviceListByLinkedMode(SonyOzDeviceInfo* DeviceList, const uint64_t Size, ELinkedMode Mode);
}

