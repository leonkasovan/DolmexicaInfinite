#pragma once

#include <unordered_map>
#include <string>

#include <prism/datastructures.h>
#include <prism/animation.h>

using namespace prism;

typedef enum {
	MUGEN_COMMAND_INPUT_STEP_TYPE_PRESS,
	MUGEN_COMMAND_INPUT_STEP_TYPE_MULTIPLE,
	MUGEN_COMMAND_INPUT_STEP_TYPE_HOLDING,
	MUGEN_COMMAND_INPUT_STEP_TYPE_RELEASE,

} DreamMugenCommandInputStepType;

typedef enum {
	MUGEN_COMMAND_INPUT_STEP_TARGET_MULTIPLE = 0,

	MUGEN_COMMAND_INPUT_STEP_TARGET_DOWN,
	MUGEN_COMMAND_INPUT_STEP_TARGET_UP,
	MUGEN_COMMAND_INPUT_STEP_TARGET_FORWARD,
	MUGEN_COMMAND_INPUT_STEP_TARGET_BACKWARD,

	MUGEN_COMMAND_INPUT_STEP_TARGET_A,
	MUGEN_COMMAND_INPUT_STEP_TARGET_B,
	MUGEN_COMMAND_INPUT_STEP_TARGET_C,
	MUGEN_COMMAND_INPUT_STEP_TARGET_X,
	MUGEN_COMMAND_INPUT_STEP_TARGET_Y,
	MUGEN_COMMAND_INPUT_STEP_TARGET_Z,

	MUGEN_COMMAND_INPUT_STEP_TARGET_START,

	MUGEN_COMMAND_INPUT_STEP_TARGET_DOWN_FORWARD,
	MUGEN_COMMAND_INPUT_STEP_TARGET_DOWN_BACKWARD,
	MUGEN_COMMAND_INPUT_STEP_TARGET_UP_FORWARD,
	MUGEN_COMMAND_INPUT_STEP_TARGET_UP_BACKWARD,

	MUGEN_COMMAND_INPUT_STEP_TARGET_MULTI_DOWN,
	MUGEN_COMMAND_INPUT_STEP_TARGET_MULTI_UP,
	MUGEN_COMMAND_INPUT_STEP_TARGET_MULTI_FORWARD,
	MUGEN_COMMAND_INPUT_STEP_TARGET_MULTI_BACKWARD,
} DreamMugenCommandInputStepTarget;

typedef struct {
	Vector mSubSteps;
} DreamMugenCommandInputStepMultipleTargetData;

typedef struct {
	int mDuration;
} DreamMugenCommandInputStepReleaseData;

typedef struct{
	DreamMugenCommandInputStepType mType;
	DreamMugenCommandInputStepTarget mTarget;
	int mDoesNotAllowOtherInputBetween;
	void* mData;
} DreamMugenCommandInputStep;

typedef struct {
	Vector mInputSteps;
	int mTime;
	int mBufferTime;
} DreamMugenCommandInput;

typedef struct {
	Vector mInputs;
	int mMinimumDuration;
} DreamMugenCommand;

typedef struct {
	std::unordered_map<std::string, DreamMugenCommand> mCommands;
} DreamMugenCommands;

DreamMugenCommands loadDreamMugenCommandFile(char* tPath);
void unloadDreamMugenCommandFile(DreamMugenCommands* tCommands);