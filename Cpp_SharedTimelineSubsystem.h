// No Copyright!

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Cpp_SharedTimelineSubsystem.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FSharedTimelineBPUpdate, float, Value);
DECLARE_DYNAMIC_DELEGATE(FSharedTimelineBPFinished);

struct FSharedTimelineTask {
	FSharedTimelineTask() = default;
	
	// Unique ID handle for this task
	int32 Id = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UCurveFloat> Curve = nullptr;

	// Duration for the timeline (if <= 0 and Curve is present, we will use curve max time)
	float Duration = 0.f;
	float Elapsed = 0.f;
	
	enum EFlags : uint8 {
		Active   = 1 << 0,
		Paused   = 1 << 1,
		Looping  = 1 << 2,
		Reverse  = 1 << 3,
		DurationOnly  = 1 << 4,
		Ratio    = 1 << 5
	};
	uint8 Flags = Active;

	TFunction<void(float)> CPP_Update;
	TFunction<void()> CPP_Finished;

	FSharedTimelineBPUpdate BP_Update;
	FSharedTimelineBPFinished BP_Finished;
	
	// Helpers for flags
	FORCEINLINE bool IsActive() const { return (Flags & Active) != 0; }
	FORCEINLINE bool IsPaused() const { return (Flags & Paused) != 0; }
	FORCEINLINE bool IsLooping() const { return (Flags & Looping) != 0; }
	FORCEINLINE bool IsReversing() const { return (Flags & Reverse) != 0; }
	FORCEINLINE bool IsDurationOnly() const { return (Flags & DurationOnly) != 0; }
	FORCEINLINE bool IsRatio() const { return (Flags & Ratio) != 0; }
	
	FORCEINLINE void SetActive(const bool b) { b ? (Flags |= Active) : (Flags &= ~Active); }
	FORCEINLINE void SetPaused(const bool b) { b ? (Flags |= Paused) : (Flags &= ~Paused); }
	FORCEINLINE void SetLooping(const bool b) { b ? (Flags |= Looping) : (Flags &= ~Looping); }
	FORCEINLINE void SetReverse(const bool b) { b ? (Flags |= Reverse) : (Flags &= ~Reverse); }
	FORCEINLINE void SetDurationOnly(const bool b) { b ? (Flags |= DurationOnly) : (Flags &= ~DurationOnly); }
	FORCEINLINE void SetRatio(const bool b) { b ? (Flags |= Ratio) : (Flags &= ~Ratio); }
};

/**
 * 
 */
UCLASS(DisplayName = "Shared Timeline Subsystem")
class AURA_API UCpp_SharedTimelineSubsystem : public UTickableWorldSubsystem {
	GENERATED_BODY()

public:
	//================================================================================================================
	// FUNCTIONS
	//================================================================================================================
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	
	/**
	 * Play a timeline task. Returns handle ID (>=0) or INDEX_NONE on failure.
	 * @param OutHandle Gives back the handle ID
	 * @param Curve In curve to use for the timeline
	 * @param InDuration 
	 * @param bLoop Whether to loop the timeline, does not call finished delegate if looping
	 * @param UpdateDelegate Delegate called on timeline update
	 * @param FinishedDelegate  Delegate called when timeline finished
	 * @param StartTime 
	 * @param bDurationOnly If true, the timeline will output elapsed time instead of curve value
	 * @param bUseRatio If true, the timeline will output ratio (Elapsed / Duration ) instead of curve value
	 */
	void PlayTimeline(int32& OutHandle, UCurveFloat* Curve, float InDuration, bool bLoop = false, TFunction<void(float)> OnUpdate = nullptr, TFunction<void()> OnFinished = nullptr, float StartTime = 0.f, bool bDurationOnly = false, bool bUseRatio = false);

	
	/**
	 * Blueprint-friendly version accepting dynamic delegates
	 * @param OutHandle Gives back the handle ID
	 * @param Curve In curve to use for the timeline
	 * @param InDuration 
	 * @param bLoop Whether to loop the timeline, does not call finished delegate if looping
	 * @param UpdateDelegate Delegate called on timeline update
	 * @param FinishedDelegate  Delegate called when timeline finished
	 * @param StartTime 
	 * @param bDurationOnly If true, the timeline will output elapsed time instead of curve value
	 * @param bUseRatio If true, the timeline will output ratio (Elapsed / Duration ) instead of curve value
	 */
	UFUNCTION(BlueprintCallable, Category = "SharedTimeline")
	void PlayTimeline_BP(int32& OutHandle, UCurveFloat* Curve, float InDuration,
	                     bool bLoop, FSharedTimelineBPUpdate UpdateDelegate, FSharedTimelineBPFinished FinishedDelegate, 
	                     float StartTime = 0.f, bool bDurationOnly = false, bool bUseRatio = false);
	UFUNCTION(BLueprintCallable, Category = "SharedTimeline")
	void ReverseTimeline(const int32& InHandle);

	UFUNCTION(BlueprintCallable, Category = "SharedTimeline")
	void PauseTimeline(int32 Handle);
	UFUNCTION(BlueprintCallable, Category = "SharedTimeline")
	void ResumeTimeline(int32 Handle);
	UFUNCTION(BlueprintCallable, Category = "SharedTimeline")
	void StopTimeline(int32 Handle, bool bFireFinish = false);

	UFUNCTION(BlueprintCallable, Category = "SharedTimeline")
	bool IsTimelineActive(int32 Handle) const;
	
	UFUNCTION(BlueprintPure, Category = "SharedTimeline")
	static float GetCurveFloatValueAtTime(const UCurveFloat* Curve, const float Time);
	
protected:
	//================================================================================================================
	// FUNCTIONS
	//================================================================================================================
	int32 FindTaskIndexById(int32 Handle) const;

	void SetBasePropertiesOfNewTask(FSharedTimelineTask& Task, UCurveFloat* Curve, const float InDuration, const bool bLoop, const float StartTime, const bool bValueOnly, const bool bUseRatio);

	
	//================================================================================================================
	// PROPERTIES & VARIABLES
	//================================================================================================================
	TArray<FSharedTimelineTask> Tasks;

	// simple handle generator
	int32 NextId = 1;
};
