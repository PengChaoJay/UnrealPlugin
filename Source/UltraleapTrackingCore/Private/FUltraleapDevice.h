/******************************************************************************
 * Copyright (C) Ultraleap, Inc. 2011-2021.                                   *
 *                                                                            *
 * Use subject to the terms of the Apache License 2.0 available at            *
 * http://www.apache.org/licenses/LICENSE-2.0, or another agreement           *
 * between Ultraleap and you, your company or other organization.             *
 ******************************************************************************/

#pragma once

#include "BodyStateDeviceConfig.h"
#include "BodyStateHMDSnapshot.h"
#include "BodyStateInputInterface.h"
#include "IInputDevice.h"
#include "IXRTrackingSystem.h"
#include "LeapC.h"
#include "LeapComponent.h"
#include "LeapImage.h"
#include "LeapLiveLink.h"
#include "LeapUtility.h"
#include "LeapWrapper.h"
#include "OpenXRToLeapWrapper.h"
#include "SceneViewExtension.h"
#include "UltraleapTrackingData.h"
#include "IUltraleapTrackingPlugin.h"
/**
 * Stores raw controller data and custom toggles
 */


class FUltraleapDevice : public LeapWrapperCallbackInterface, public IBodyStateInputRawInterface, public IHandTrackingDevice
{
public:
	FUltraleapDevice(IHandTrackingWrapper* LeapDeviceWrapperIn, ITrackingDeviceWrapper* TrackingDeviceWrapperIn);
	virtual ~FUltraleapDevice();

	

	/** Main input capture and event parsing 'tick' */
	void CaptureAndEvaluateInput();
	void ParseEvents();

	// IHandTrackingDevice implementation
	virtual void AddEventDelegate(const ULeapComponent* EventDelegate) override;
	virtual void RemoveEventDelegate(const ULeapComponent* EventDelegate) override;
	/** Tick the interface (e.g. check for new controllers) */
	virtual void Tick(float DeltaTime) override;
	/** Poll for controller state and send events if needed */
	virtual void SendControllerEvents() override;
	// end of IHandTrackingDevice implementation

	void ShutdownLeap();
	void AreHandsVisible(bool& LeftHandIsVisible, bool& RightHandIsVisible);
	void LatestFrame(FLeapFrameData& OutFrame);
	void SetSwizzles(ELeapQuatSwizzleAxisB ToX, ELeapQuatSwizzleAxisB ToY, ELeapQuatSwizzleAxisB ToZ, ELeapQuatSwizzleAxisB ToW);
	// Policy and toggles
	void SetLeapPolicy(ELeapPolicyFlag Flag, bool Enable);
	void SetTrackingMode(ELeapMode Flag);
	// BodyState
	virtual void UpdateInput(int32 DeviceID, class UBodyStateSkeleton* Skeleton) override;
	virtual void OnDeviceDetach();

	FCriticalSection LeapSection;

	void SetOptions(const FLeapOptions& Options);
	FLeapOptions GetOptions();
	FLeapStats GetStats();
	
	
private:
	bool UseTimeBasedVisibilityCheck = false;
	bool UseTimeBasedGestureCheck = false;
	// Time Based Variables
	// Pinch And Grab Thresholds
	float StartGrabThreshold = .8f;
	float EndGrabThreshold = .5f;
	float StartPinchThreshold = .8f;
	float EndPinchThreshold = .5f;
	double TimeSinceLastLeftPinch = 0;
	double TimeSinceLastRightPinch = 0;
	double TimeSinceLastLeftGrab = 0;
	double TimeSinceLastRightGrab = 0;
	double PinchTimeout = 100000;	 //.1 Second
	double GrabTimeout = 100000;	 //.1 Second
	bool IsLeftPinching = false;
	bool IsRightPinching = false;
	bool IsLeftGrabbing = false;
	bool IsRightGrabbing = false;
	// Visibility Tracking and Thresholds
	bool IsLeftVisible = false;
	bool IsRightVisible = false;
	int64_t TimeSinceLastLeftVisible = 10000;
	int64_t TimeSinceLastRightVisible = 10000;
	int64_t VisibilityTimeout = 1000000;	// 1 Second
	int64_t LastLeapTime = 0;
	FLeapHandData LastLeftHand;
	FLeapHandData LastRightHand;

	// Private UProperties
	
	TArray<ULeapComponent*> EventDelegates;	   // delegate storage

	// Private utility methods
	void CallFunctionOnComponents(TFunction<void(ULeapComponent*)> InFunction);	   // lambda multi-cast convenience wrapper
	bool EmitKeyUpEventForKey(FKey Key, int32 User, bool Repeat);
	bool EmitKeyDownEventForKey(FKey Key, int32 User, bool Repeat);
	bool EmitAnalogInputEventForKey(FKey Key, float Value, int32 User, bool Repeat);
	bool HandClosed(float Strength);
	bool HandPinched(float Strength);
	void CheckHandVisibility();
	void CheckPinchGesture();
	void CheckGrabGesture();

	int64 GetInterpolatedNow();

	// Internal states
	FLeapOptions Options;
	FLeapStats Stats;

	// Interpolation time offsets
	int64 HandInterpolationTimeOffset;		// in microseconds
	int64 FingerInterpolationTimeOffset;	// in microseconds

	// HMD
	FName HMDType;

	// Timewarp
	float TimewarpTween;
	int64 TimeWarpTimeStamp;
	float SlerpTween;

	// Frame timing
	LeapUtilityTimer FrameTimer;
	double GameTimeInSec;
	int64 FrameTimeInMicros;

	// Game thread Data
	FLeapFrameData CurrentFrame;
	FLeapFrameData PastFrame;

	TArray<int32> PastVisibleHands;

	// Time warp support
	BSHMDSnapshotHandler SnapshotHandler;

	// Image handling
	TSharedPtr<FLeapImage> LeapImageHandler;
	void OnImageCallback(UTexture2D* LeftCapturedTexture, UTexture2D* RightCapturedTexture);

	// v5 Tracking mode API
	static bool bUseNewTrackingModeAPI;
	// Wrapper link
	TSharedPtr<IHandTrackingWrapper> Leap;
	ILeapConnector* Connector;
	ITrackingDeviceWrapper* TrackingDeviceWrapper;

	// LeapWrapper Callbacks
	// Per device
	virtual void OnFrame(const LEAP_TRACKING_EVENT* frame) override;
	virtual void OnImage(const LEAP_IMAGE_EVENT* image_event) override;
	virtual void OnPolicy(const uint32_t current_policies) override;
	virtual void OnTrackingMode(const eLeapTrackingMode current_tracking_mode) override;
	virtual void OnLog(const eLeapLogSeverity severity, const int64_t timestamp, const char* message) override;

	// Bodystate link
	int32 BodyStateDeviceId;
	FBodyStateDeviceConfig Config;

#if WITH_EDITOR
	// LiveLink
	TSharedPtr<FLeapLiveLinkProducer> LiveLink;
#endif

	// Convenience Converters - Todo: wrap into separate class?
	void SetBSFingerFromLeapDigit(class UBodyStateFinger* Finger, const FLeapDigitData& LeapDigit);
	void SetBSThumbFromLeapThumb(class UBodyStateFinger* Finger, const FLeapDigitData& LeapDigit);
	void SetBSHandFromLeapHand(class UBodyStateHand* Hand, const FLeapHandData& LeapHand);

	void SwitchTrackingSource(const bool UseOpenXRAsSource);

	void Init();
	void InitOptions();
};