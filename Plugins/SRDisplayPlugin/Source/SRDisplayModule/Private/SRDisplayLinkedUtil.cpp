/*
 * Copyright 2024 Sony Corporation
 */
#pragma once
#include <SRDisplayModule/Public/SRDisplayLinkedUtil.h>

#include <CoreMinimal.h>
#include <vector>
#include <algorithm>

namespace srdisplay::util
{
	FVector GetSRDidplsyManagerPostion(float DisplayHeight, float DisplayWidth, ELinkedMode LinkedMode, int SRDisplayNum)
	{
		constexpr float DefaultBezelWidth_cm = 2.7f;
		constexpr float DefaultBezelHeight_cm = 8.3f;

		if (SRDisplayNum > 4)
		{
			return FVector(0, 0, 0);
		}

		TArray<FVector> SRDisplayManagerPositions = TArray<FVector>();
		switch (LinkedMode)
		{
		case ELinkedMode::VERTICAL_MODE:
			SRDisplayManagerPositions = TArray<FVector>
			{
				FVector(0, 0, 0),
				FVector(0, 0, -DisplayHeight - DefaultBezelHeight_cm),
				FVector(0, 0, DisplayHeight + DefaultBezelHeight_cm),
				FVector(0, 0, -2 * (DisplayHeight + DefaultBezelHeight_cm))
			};
			break;
		case ELinkedMode::HORIZONTAL_MODE:
			SRDisplayManagerPositions = TArray<FVector>
			{
				FVector(0, 0, 0),
				FVector(0, DisplayWidth + DefaultBezelWidth_cm, 0),
				FVector(0, -DisplayWidth - DefaultBezelWidth_cm, 0),
				FVector(0, 0, 0)
			};
			break;
		case ELinkedMode::SINGLE_MODE:
		default:
			return FVector(0, 0, 0);
		}
		return SRDisplayManagerPositions[SRDisplayNum];
	}

	FVector GetSRDidplsyManagerPostionFromModel(ELinkedMode LinkedMode, int SRDisplayNum, bool isSR1)
	{
		constexpr float SR1DisplayHeight_cm = 19.36224f;
		constexpr float SR1DisplayWidth_cm = 34.42176f;

		constexpr float SR2DisplayHeight_cm = 33.5664f;
		constexpr float SR2DisplayWidth_cm = 59.6736f;

		return isSR1
			? GetSRDidplsyManagerPostion(SR1DisplayHeight_cm, SR1DisplayWidth_cm, LinkedMode, SRDisplayNum)
			: GetSRDidplsyManagerPostion(SR2DisplayHeight_cm, SR2DisplayWidth_cm, LinkedMode, SRDisplayNum);
	}

	void SortDeviceListByLinkedMode(SonyOzDeviceInfo* DeviceList, const uint64_t Size, ELinkedMode Mode)
	{
		std::vector<SonyOzDeviceInfo> DeviceListVector;
		for (int i = 0; i < Size; i++)
		{
			DeviceListVector.push_back(DeviceList[i]);
		}
		auto compareX = [](const SonyOzDeviceInfo& a, const SonyOzDeviceInfo& b) {return a.target_monitor_rectangle.left < b.target_monitor_rectangle.left; };
		auto compareY = [](const SonyOzDeviceInfo& a, const SonyOzDeviceInfo& b) {return a.target_monitor_rectangle.top < b.target_monitor_rectangle.top; };

		switch (Mode)
		{
		case ELinkedMode::VERTICAL_MODE:
			std::sort(DeviceListVector.begin(), DeviceListVector.end(), compareY);
			if (DeviceListVector.size() >= 3)
			{
				std::swap(DeviceListVector[0], DeviceListVector[1]);
				std::swap(DeviceListVector[1], DeviceListVector[2]);
			}
			break;
		case ELinkedMode::HORIZONTAL_MODE:
			std::sort(DeviceListVector.begin(), DeviceListVector.end(), compareX);
			if (DeviceListVector.size() >= 3)
			{
				std::swap(DeviceListVector[0], DeviceListVector[1]);
				std::swap(DeviceListVector[1], DeviceListVector[2]);
			}
			break;
		case ELinkedMode::SINGLE_MODE:
		default:
			break;
		}
		std::copy(DeviceListVector.begin(), DeviceListVector.end(), DeviceList);
	}
}
