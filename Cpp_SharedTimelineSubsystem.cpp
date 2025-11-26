// No Copyright!


#include "Subsystem/Cpp_SharedTimelineSubsystem.h"

void UCpp_SharedTimelineSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	// we iterate by index because tasks may be removed
	for (int32 i = Tasks.Num() - 1; i >= 0; --i) {
		FSharedTimelineTask& Task = Tasks[i];
		if (!Task.IsActive() || Task.IsPaused()) {
			continue;
		}
		ensureAlwaysMsgf(Task.Curve, TEXT("CURVE NOT GIVEN FOR Shared Timeline Subsystem!"));
		Task.Elapsed += (Task.IsReversing()) ? -DeltaTime : DeltaTime;

		// determine duration
		float& UseDuration = Task.Duration;
		if (UseDuration <= 0.f) {
			// safe defensive: fetch range from curve
			float Min, Max;
			Task.Curve->GetTimeRange(Min, Max);
			UseDuration = Max - Min;
			if (UseDuration <= 0.f) {
				UseDuration = 1.f; // base duration
			}
		}

		// clamp or loop
		bool bJustFinished = false;
		if (!Task.IsLooping()) {
			if (Task.Elapsed >= UseDuration) {
				Task.Elapsed = UseDuration;
				bJustFinished = true;
				Task.SetActive(false); // mark finished (will be removed after executing finish callback)
			}
			else if (Task.Elapsed <= 0.f) {
				Task.Elapsed = 0.f;
				bJustFinished = true;
				Task.SetActive(false);
			}
		}
		else {
			// Wrap Elapsed within [0..UseDuration)
			while (Task.Elapsed >= UseDuration) {
				Task.Elapsed -= UseDuration;
			}
			while (Task.Elapsed < 0.f) {
				Task.Elapsed += UseDuration;
			}
		}
		
		// evaluate curve or linear
		const float Value = Task.IsDurationOnly() ? Task.Elapsed : Task.Curve->GetFloatValue(Task.Elapsed);

		// call update callbacks (C++ first then BP)
		if (Task.CPP_Update) {
			Task.CPP_Update(Value);
		}
		else if (Task.BP_Update.IsBound()) {
			Task.BP_Update.Execute(Value);
		}

		// finished callback and removal
		if (bJustFinished) {
			if (Task.CPP_Finished) {
				Task.CPP_Finished();
			}
			else if (Task.BP_Finished.IsBound()) {
				Task.BP_Finished.Execute();
			}
			Tasks.RemoveAtSwap(i, 1, false);
		}
	}
}
TStatId UCpp_SharedTimelineSubsystem::GetStatId() const {
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCpp_SharedTimelineSubsystem, STATGROUP_Tickables);
}
bool UCpp_SharedTimelineSubsystem::IsTickable() const {
	return true;
}

void UCpp_SharedTimelineSubsystem::PlayTimeline(int32& OutHandle, UCurveFloat* Curve, const float InDuration, const bool bLoop, TFunction<void(float)> OnUpdate, TFunction<void()> OnFinished, const float StartTime, const bool bDurationOnly) {
	if (!Curve) {
		OutHandle = INDEX_NONE;
		return;
	}

	FSharedTimelineTask NewTask;
	SetBasePropertiesOfNewTask(NewTask, Curve, InDuration, bLoop, StartTime, bDurationOnly);
	NewTask.CPP_Update = MoveTemp(OnUpdate);
	NewTask.CPP_Finished = MoveTemp(OnFinished);

	Tasks.Add(MoveTemp(NewTask));
	OutHandle = NewTask.Id;
}

void UCpp_SharedTimelineSubsystem::PlayTimeline_BP(int32& OutHandle, UCurveFloat* Curve, const float InDuration, const bool bLoop, const FSharedTimelineBPUpdate UpdateDelegate, const FSharedTimelineBPFinished FinishedDelegate, const float StartTime, const bool bDurationOnly) {
	if (!Curve) {
		OutHandle = INDEX_NONE;
		return;
	}

	FSharedTimelineTask NewTask;
	SetBasePropertiesOfNewTask(NewTask, Curve, InDuration, bLoop, StartTime, bDurationOnly);
	NewTask.BP_Update = UpdateDelegate;
	NewTask.BP_Finished = FinishedDelegate;

	Tasks.Add(MoveTemp(NewTask));
	OutHandle = NewTask.Id;
}
void UCpp_SharedTimelineSubsystem::ReverseTimeline(const int32& InHandle) {
	const int32 Index = FindTaskIndexById(InHandle);
	if (Index != INDEX_NONE) {
		Tasks[Index].SetReverse(true);
	}
}

void UCpp_SharedTimelineSubsystem::PauseTimeline(const int32 Handle) {
	const int32 Index = FindTaskIndexById(Handle);
	if (Index != INDEX_NONE) {
		Tasks[Index].SetPaused(true);
	}
}
void UCpp_SharedTimelineSubsystem::ResumeTimeline(const int32 Handle) {
	const int32 Index = FindTaskIndexById(Handle);
	if (Index != INDEX_NONE) {
		Tasks[Index].SetPaused(false);
	}
}
void UCpp_SharedTimelineSubsystem::StopTimeline(const int32 Handle, const bool bFireFinish) {
	const int32 Index = FindTaskIndexById(Handle);
	if (Index == INDEX_NONE) {
		return;
	}
	const FSharedTimelineTask& Task = Tasks[Index];

	if (bFireFinish) {
		if (Task.CPP_Finished) {
			Task.CPP_Finished();
		}
		else if (Task.BP_Finished.IsBound()) {
			Task.BP_Finished.Execute();
		}
	}

	Tasks.RemoveAtSwap(Index, 1, false);
}

bool UCpp_SharedTimelineSubsystem::IsTimelineActive(const int32 Handle) const {
	const int32 Index = FindTaskIndexById(Handle);
	return Index != INDEX_NONE && Tasks[Index].IsActive() && !Tasks[Index].IsPaused();
}

float UCpp_SharedTimelineSubsystem::GetCurveFloatValueAtTime(const UCurveFloat* Curve, const float Time) {
	ensureAlwaysMsgf(Curve, TEXT("CURVE NOT GIVEN FOR Shared Timeline Subsystem!"));
	return Curve->GetFloatValue(Time);
}

int32 UCpp_SharedTimelineSubsystem::FindTaskIndexById(const int32 Handle) const {
	for (int32 i = 0; i < Tasks.Num(); i++) {
		if (Tasks[i].Id == Handle) {
			return i;
		}
	}
	return INDEX_NONE;
}

void UCpp_SharedTimelineSubsystem::SetBasePropertiesOfNewTask(FSharedTimelineTask& Task, UCurveFloat* Curve, const float InDuration, const bool bLoop, const float StartTime, const bool bValueOnly) {
	Task.Id = NextId++;
	Task.Curve = Curve;
	Task.Duration = InDuration;
	Task.Elapsed = StartTime;
	Task.SetLooping(bLoop);
	Task.SetActive(true);
	Task.SetPaused(false);
	Task.SetDurationOnly(bValueOnly);
}
