#include "mugencommandhandler.h"

#include <assert.h>
#include <algorithm>
#include <vector>

#include <prism/profiling.h>
#include <prism/datastructures.h>
#include <prism/log.h>
#include <prism/system.h>
#include <prism/input.h>
#include <prism/math.h>
#include <prism/stlutil.h>

using namespace std;

typedef struct {
	char mName[100];
	int mIsActive;
	Duration mNow;
	Duration mBufferTime;
} MugenCommandState;

typedef struct {
	map<string, MugenCommandState> mStates;
} MugenCommandStates;

typedef struct {
	char mName[100];
	DreamMugenCommandInput* mInput;
	int mStep;
	Duration mNow;
} ActiveMugenCommand;

typedef struct {
	int mIsBeingProcessed;

} InternalMugenCommandState;

typedef struct {
	DreamMugenCommands* tCommands;
	MugenCommandStates tStates;
	StringMap mInternalStates;

	List mActiveCommands;

	int mControllerID;
	int mIsFacingRight;
} RegisteredMugenCommand;

static struct {
	vector<RegisteredMugenCommand> mRegisteredCommands;

	uint32_t mHeldMask[2];
	uint32_t mPreviousHeldMask[2];
} gMugenCommandHandler;

static void loadMugenCommandHandler(void* tData) {
	(void)tData;
	stl_new_vector(gMugenCommandHandler.mRegisteredCommands);
}



static void unloadSingleRegisteredCommand(void* tCaller, RegisteredMugenCommand& tData) {
	(void)tCaller;
	RegisteredMugenCommand* e = &tData;
	delete_list(&e->mActiveCommands);
	stl_delete_map(e->tStates.mStates);
	delete_string_map(&e->mInternalStates);
}

static void unloadMugenCommandHandler(void* tData) {
	(void)tData;
	stl_vector_map(gMugenCommandHandler.mRegisteredCommands, unloadSingleRegisteredCommand);
	stl_delete_vector(gMugenCommandHandler.mRegisteredCommands);
}

static void addSingleMugenCommandState(RegisteredMugenCommand* tCaller, const string &tKey, DreamMugenCommand& tData) {
	(void)tData;
	RegisteredMugenCommand* s = (RegisteredMugenCommand*)tCaller;

	MugenCommandState e;
	strcpy(e.mName, tKey.data());
	e.mIsActive = 0;
	s->tStates.mStates[tKey] = e;
	
	InternalMugenCommandState* internalState = (InternalMugenCommandState*)allocMemory(sizeof(InternalMugenCommandState));
	internalState->mIsBeingProcessed = 0;
	string_map_push_owned(&s->mInternalStates, e.mName, internalState);
}

static void setupMugenCommandStates(RegisteredMugenCommand* e) {
	stl_new_map(e->tStates.mStates);
	stl_string_map_map(e->tCommands->mCommands, addSingleMugenCommandState, e);
}

int registerDreamMugenCommands(int tControllerID, DreamMugenCommands * tCommands)
{
	
	RegisteredMugenCommand e;
	e.mActiveCommands = new_list();
	e.tCommands = tCommands;
	e.mInternalStates = new_string_map();
	e.mControllerID = tControllerID;
	e.mIsFacingRight = 1;
	setupMugenCommandStates(&e);

	gMugenCommandHandler.mRegisteredCommands.push_back(e);
	return gMugenCommandHandler.mRegisteredCommands.size() - 1;
}

int isDreamCommandActive(int tID, const char * tCommandName)
{
	RegisteredMugenCommand* e = &gMugenCommandHandler.mRegisteredCommands[tID];
	string key(tCommandName);
	if (!stl_map_contains(e->tStates.mStates, key)) {
		logWarningFormat("Querying nonexistant command name %s.", tCommandName);
		return 0;
	}
	MugenCommandState* state = &e->tStates.mStates[key];
	
	return state->mIsActive;
}

static void setCommandStateActive(RegisteredMugenCommand* tRegisteredCommand, const char* tName, Duration tBufferTime);

void setDreamPlayerCommandActiveForAI(int tID, const char * tCommandName, Duration tBufferTime)
{
	RegisteredMugenCommand* e = &gMugenCommandHandler.mRegisteredCommands[tID];
	setCommandStateActive(e, tCommandName, tBufferTime);
}

void setDreamMugenCommandFaceDirection(int tID, FaceDirection tDirection)
{
	RegisteredMugenCommand* e = &gMugenCommandHandler.mRegisteredCommands[tID];
	e->mIsFacingRight = tDirection == FACE_DIRECTION_RIGHT;
}

static int handleSingleCommandInputStepAndReturnIfActive(DreamMugenCommandInputStep* tStep, int* oIsStepOver, int tControllerID, int tIsFacingRight);

typedef struct {
	int isActiveAmount;
	int isStepOverAmount;
	int mControllerID;
	int mIsFacingRight;
} MultipleCommandInputStepCaller;

static void handleSingleMultipleCommandInputStep(void* tCaller, void* tData) {
	MultipleCommandInputStepCaller* caller = (MultipleCommandInputStepCaller*)tCaller;
	DreamMugenCommandInputStep* step = (DreamMugenCommandInputStep*)tData;

	int isStepOver = 0;
	int isActive = handleSingleCommandInputStepAndReturnIfActive(step, &isStepOver, caller->mControllerID, caller->mIsFacingRight);

	caller->isActiveAmount += isActive;
	caller->isStepOverAmount += isStepOver;
}

static int handleMultipleCommandInputStep(DreamMugenCommandInputStep* tStep, int* oIsStepOver, int tControllerID, int tIsFacingRight) {
	DreamMugenCommandInputStepMultipleTargetData* data = (DreamMugenCommandInputStepMultipleTargetData*)tStep->mData;
	assert(tStep->mTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_MULTIPLE);

	MultipleCommandInputStepCaller caller;
	caller.isActiveAmount = 0;
	caller.isStepOverAmount = 0;
	caller.mControllerID = tControllerID;
	caller.mIsFacingRight = tIsFacingRight;
	vector_map(&data->mSubSteps, handleSingleMultipleCommandInputStep, &caller);

	if (caller.isActiveAmount < vector_size(&data->mSubSteps)) return 0;

	assert(caller.isStepOverAmount == 0 || caller.isStepOverAmount == vector_size(&data->mSubSteps));

	if (caller.isStepOverAmount) *oIsStepOver = 1;
	else *oIsStepOver = 0;

	return 1;
}

#define MASK_A (1 << 0)
#define MASK_B (1 << 1)
#define MASK_C (1 << 2)
#define MASK_X (1 << 3)
#define MASK_Y (1 << 4)
#define MASK_Z (1 << 5)

#define MASK_START (1 << 6)

#define MASK_LEFT (1 << 7)
#define MASK_RIGHT (1 << 8)
#define MASK_UP (1 << 9)
#define MASK_DOWN (1 << 10)

#define MASK_DOWN_LEFT (MASK_DOWN | MASK_LEFT)
#define MASK_DOWN_RIGHT (MASK_DOWN | MASK_RIGHT)
#define MASK_UP_LEFT (MASK_UP | MASK_LEFT)
#define MASK_UP_RIGHT (MASK_UP | MASK_RIGHT)

static int isButtonCommandActive(DreamMugenCommandInputStepTarget tTarget, uint32_t tMask, int isFacingRight) {
	int directionMask = 0;
	directionMask |= (tMask & (MASK_UP | MASK_DOWN | MASK_LEFT | MASK_RIGHT));

	if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_A) return (tMask & MASK_A) == MASK_A;
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_B) return (tMask & MASK_B) == MASK_B;
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_C) return (tMask & MASK_C) == MASK_C;
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_X) return (tMask & MASK_X) == MASK_X;
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_Y) return (tMask & MASK_Y) == MASK_Y;
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_Z) return (tMask & MASK_Z) == MASK_Z;

	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_START) return (tMask & MASK_START) == MASK_START;

	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_UP) return directionMask == MASK_UP;
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_DOWN) return directionMask == MASK_DOWN;
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_FORWARD) {
		if (isFacingRight) return  directionMask == MASK_RIGHT;
		else return directionMask == MASK_LEFT;
	}
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_BACKWARD) {
		if (isFacingRight) return  directionMask == MASK_LEFT;
		else return directionMask == MASK_RIGHT;
	}

	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_DOWN_FORWARD) {
		if (isFacingRight) return  directionMask == MASK_DOWN_RIGHT;
		else return directionMask == MASK_DOWN_LEFT;
	}
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_DOWN_BACKWARD) {
		if (isFacingRight) return directionMask == MASK_DOWN_LEFT;
		else return directionMask == MASK_DOWN_RIGHT;
	}
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_UP_FORWARD) {
		if (isFacingRight) return  directionMask == MASK_UP_RIGHT;
		else return directionMask == MASK_UP_LEFT;
	}
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_UP_BACKWARD) {
		if (isFacingRight) return  directionMask == MASK_UP_LEFT;
		else return directionMask == MASK_UP_RIGHT;
	}

	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_MULTI_FORWARD) {
		return isButtonCommandActive(MUGEN_COMMAND_INPUT_STEP_TARGET_FORWARD, tMask, isFacingRight)
			|| isButtonCommandActive(MUGEN_COMMAND_INPUT_STEP_TARGET_UP_FORWARD, tMask, isFacingRight)
			|| isButtonCommandActive(MUGEN_COMMAND_INPUT_STEP_TARGET_DOWN_FORWARD, tMask, isFacingRight);
	}
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_MULTI_BACKWARD) {
		return isButtonCommandActive(MUGEN_COMMAND_INPUT_STEP_TARGET_BACKWARD, tMask, isFacingRight)
			|| isButtonCommandActive(MUGEN_COMMAND_INPUT_STEP_TARGET_UP_BACKWARD, tMask, isFacingRight)
			|| isButtonCommandActive(MUGEN_COMMAND_INPUT_STEP_TARGET_DOWN_BACKWARD, tMask, isFacingRight);
	}
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_MULTI_UP) {
		return isButtonCommandActive(MUGEN_COMMAND_INPUT_STEP_TARGET_UP, tMask, isFacingRight)
			|| isButtonCommandActive(MUGEN_COMMAND_INPUT_STEP_TARGET_UP_FORWARD, tMask, isFacingRight)
			|| isButtonCommandActive(MUGEN_COMMAND_INPUT_STEP_TARGET_UP_BACKWARD, tMask, isFacingRight);
	}
	else if (tTarget == MUGEN_COMMAND_INPUT_STEP_TARGET_MULTI_DOWN) {
		return isButtonCommandActive(MUGEN_COMMAND_INPUT_STEP_TARGET_DOWN, tMask, isFacingRight)
			|| isButtonCommandActive(MUGEN_COMMAND_INPUT_STEP_TARGET_DOWN_FORWARD, tMask, isFacingRight)
			|| isButtonCommandActive(MUGEN_COMMAND_INPUT_STEP_TARGET_DOWN_BACKWARD, tMask, isFacingRight);
	}
	else {
		return 0;
	}
}

static int isTargetHeld(DreamMugenCommandInputStepTarget tTarget, int tControllerID, int tIsFacingRight) {
	return isButtonCommandActive(tTarget, gMugenCommandHandler.mHeldMask[tControllerID], tIsFacingRight);
}

static int isTargetPressed(DreamMugenCommandInputStepTarget tTarget, int tControllerID, int tIsFacingRight) {
	return isButtonCommandActive(tTarget, gMugenCommandHandler.mHeldMask[tControllerID], tIsFacingRight) && !isButtonCommandActive(tTarget, gMugenCommandHandler.mPreviousHeldMask[tControllerID], tIsFacingRight);
}

static int isTargetReleased(DreamMugenCommandInputStepTarget tTarget, int tControllerID, int tIsFacingRight) {
	return !isButtonCommandActive(tTarget, gMugenCommandHandler.mHeldMask[tControllerID], tIsFacingRight) && isButtonCommandActive(tTarget, gMugenCommandHandler.mPreviousHeldMask[tControllerID], tIsFacingRight);
}

static int handleHoldingCommandInputStep(DreamMugenCommandInputStep* tStep, int* oIsStepOver, int tControllerID, int tIsFacingRight) {
	int ret = isTargetHeld(tStep->mTarget, tControllerID, tIsFacingRight);
	*oIsStepOver = 1; // TODO add to active steps for command and always check

	return ret;
}

static int handlePressingCommandInputStep(DreamMugenCommandInputStep* tStep, int* oIsStepOver, int tControllerID, int tIsFacingRight) {
	int ret = isTargetPressed(tStep->mTarget, tControllerID, tIsFacingRight);
	*oIsStepOver = 1;

	return ret;
}

static int handleReleasingCommandInputStep(DreamMugenCommandInputStep* tStep, int* oIsStepOver, int tControllerID, int tIsFacingRight) {
	int ret = isTargetReleased(tStep->mTarget, tControllerID, tIsFacingRight);
	*oIsStepOver = 1; 

	return ret;
}



static int handleSingleCommandInputStepAndReturnIfActive(DreamMugenCommandInputStep* tStep, int* oIsStepOver, int tControllerID, int tIsFacingRight) {

	if (tStep->mType == MUGEN_COMMAND_INPUT_STEP_TYPE_MULTIPLE) {
		return handleMultipleCommandInputStep(tStep, oIsStepOver, tControllerID, tIsFacingRight);
	}
	else if (tStep->mType == MUGEN_COMMAND_INPUT_STEP_TYPE_HOLDING) {
		return handleHoldingCommandInputStep(tStep, oIsStepOver, tControllerID, tIsFacingRight);
	}
	else if (tStep->mType == MUGEN_COMMAND_INPUT_STEP_TYPE_PRESS) {
		return handlePressingCommandInputStep(tStep, oIsStepOver, tControllerID, tIsFacingRight);
	}
	else if (tStep->mType == MUGEN_COMMAND_INPUT_STEP_TYPE_RELEASE) {
		return handleReleasingCommandInputStep(tStep, oIsStepOver, tControllerID, tIsFacingRight);
	}
	else {
		return 0;
	}
}

static void removeActiveCommand(ActiveMugenCommand* tCommand, RegisteredMugenCommand* tRegisteredCommand) {
	InternalMugenCommandState* state = (InternalMugenCommandState*)string_map_get(&tRegisteredCommand->mInternalStates, tCommand->mName);
	state->mIsBeingProcessed = 0;
}

static void setCommandStateActive(RegisteredMugenCommand* tRegisteredCommand, const char* tName, Duration tBufferTime) {
	MugenCommandState* state = &tRegisteredCommand->tStates.mStates[tName];
	state->mIsActive = 1;
	state->mNow = 0;
	state->mBufferTime = tBufferTime;
}

static void setCommandStateInactive(MugenCommandState* tState) {
	tState->mIsActive = 0;
}

static int isSameStepAsBefore(ActiveMugenCommand* tCommand) {
	assert(tCommand->mStep > 0);
	assert(tCommand->mStep < vector_size(&tCommand->mInput->mInputSteps));

	DreamMugenCommandInputStep* mPreviousStep = (DreamMugenCommandInputStep*)vector_get(&tCommand->mInput->mInputSteps, tCommand->mStep - 1);
	DreamMugenCommandInputStep* mStep = (DreamMugenCommandInputStep*)vector_get(&tCommand->mInput->mInputSteps, tCommand->mStep);

	int isSameType = mPreviousStep->mType == mStep->mType;
	if (!isSameType) return 0;

	int isPressType = mPreviousStep->mType == MUGEN_COMMAND_INPUT_STEP_TYPE_PRESS;
	if (!isPressType) return 0;

	int haveSameTarget = mPreviousStep->mTarget == mStep->mTarget;
	return haveSameTarget;
}

static int updateSingleActiveMugenCommand(void* tCaller, void* tData) {
	RegisteredMugenCommand* registeredCommand = (RegisteredMugenCommand*)tCaller;
	ActiveMugenCommand* command = (ActiveMugenCommand*)tData;

	if (handleDurationAndCheckIfOver(&command->mNow, command->mInput->mTime)) {
		removeActiveCommand(command, registeredCommand);
		return 1;
	}

	int isRunning = 1;
	while (isRunning) {
		DreamMugenCommandInputStep* step = (DreamMugenCommandInputStep*)vector_get(&command->mInput->mInputSteps, command->mStep);

		int isStepOver = 0;
		int isActive = handleSingleCommandInputStepAndReturnIfActive(step, &isStepOver, registeredCommand->mControllerID, registeredCommand->mIsFacingRight);
		if (!isActive) return 0;

		if (isStepOver) {
			command->mStep++;
			if (command->mStep == vector_size(&command->mInput->mInputSteps)) {
				setCommandStateActive(registeredCommand, command->mName, command->mInput->mBufferTime);
				removeActiveCommand(command, registeredCommand);
				return 1;
			}

			if (isSameStepAsBefore(command)) break;
		}
	}

	return 0;
}

static void updateActiveMugenCommands(RegisteredMugenCommand* tCommand) {
	list_remove_predicate(&tCommand->mActiveCommands, updateSingleActiveMugenCommand, tCommand);
}

typedef struct {
	RegisteredMugenCommand* mRegisteredCommand;
	const string& mName;
} StaticMugenCommandInputCaller;

static void addNewActiveMugenCommand(DreamMugenCommandInput* tInput, RegisteredMugenCommand* tRegisteredCommand, const char* tName, int mIsStepOver) {
	
	if (vector_size(&tInput->mInputSteps) == 1) {
		setCommandStateActive(tRegisteredCommand, tName, tInput->mBufferTime);
		return;
	}

	ActiveMugenCommand* e = (ActiveMugenCommand*)allocMemory(sizeof(ActiveMugenCommand));
	e->mInput = tInput;
	strcpy(e->mName, tName);
	e->mNow = 0;
	e->mStep = min(1, mIsStepOver);

	int isAlreadyOver = 0;
	if (!isSameStepAsBefore(e)) {
		isAlreadyOver = updateSingleActiveMugenCommand(tRegisteredCommand, e);
	}
	if(!isAlreadyOver) list_push_back_owned(&tRegisteredCommand->mActiveCommands, e);
	else freeMemory(e);
}

static void updateSingleStaticMugenCommandInput(void* tCaller, void* tData) {
	DreamMugenCommandInput* input = (DreamMugenCommandInput*)tData;
	StaticMugenCommandInputCaller* caller = (StaticMugenCommandInputCaller*)tCaller;

	DreamMugenCommandInputStep* firstStep = (DreamMugenCommandInputStep*)vector_get(&input->mInputSteps, 0);

	int mIsStepOver = 0;
	int mIsActive = handleSingleCommandInputStepAndReturnIfActive(firstStep, &mIsStepOver, caller->mRegisteredCommand->mControllerID, caller->mRegisteredCommand->mIsFacingRight);

	if (!mIsActive) return;

	addNewActiveMugenCommand(input, caller->mRegisteredCommand, caller->mName.data(), mIsStepOver);
}

static void updateSingleStaticMugenCommand(RegisteredMugenCommand* tCaller, const string& tKey, DreamMugenCommand& tData) {
	RegisteredMugenCommand* registeredCommand = (RegisteredMugenCommand*)tCaller;
	DreamMugenCommand* command = &tData;
	
	InternalMugenCommandState* internalState = (InternalMugenCommandState*)string_map_get(&registeredCommand->mInternalStates, tKey.data());
	if (internalState->mIsBeingProcessed) return;

	StaticMugenCommandInputCaller caller = {
		registeredCommand,
		tKey
	};
	vector_map(&command->mInputs, updateSingleStaticMugenCommandInput, &caller);
}

static void updateStaticMugenCommands(RegisteredMugenCommand* tCommand) {
	stl_string_map_map(tCommand->tCommands->mCommands, updateSingleStaticMugenCommand, tCommand);
}

static void updateSingleCommandState(void* tCaller, const string& tKey, MugenCommandState& tData) {
	(void)tCaller;
	(void)tKey;
	MugenCommandState* state = &tData;
	if (!state->mIsActive) return;

	if (handleDurationAndCheckIfOver(&state->mNow, state->mBufferTime)) {
		setCommandStateInactive(state);
	}
}

static void updateCommandStates(RegisteredMugenCommand* tCommand) {
	stl_string_map_map(tCommand->tStates.mStates, updateSingleCommandState);
}

static void updateSingleInputMaskEntry(int i, uint32_t tMask, int tHoldValue) {
	gMugenCommandHandler.mHeldMask[i] |= (tMask * min(tHoldValue, 1));
}

static void updateInputMask(int i) {
	gMugenCommandHandler.mPreviousHeldMask[i] = gMugenCommandHandler.mHeldMask[i];
	gMugenCommandHandler.mHeldMask[i] = 0;

	updateSingleInputMaskEntry(i, MASK_A, hasPressedASingle(i));
	updateSingleInputMaskEntry(i, MASK_B, hasPressedBSingle(i));
	updateSingleInputMaskEntry(i, MASK_C, hasPressedRSingle(i));
	updateSingleInputMaskEntry(i, MASK_X, hasPressedXSingle(i));
	updateSingleInputMaskEntry(i, MASK_Y, hasPressedYSingle(i));
	updateSingleInputMaskEntry(i, MASK_Z, hasPressedLSingle(i));

	updateSingleInputMaskEntry(i, MASK_START, hasPressedStartSingle(i));

	updateSingleInputMaskEntry(i, MASK_LEFT, hasPressedLeftSingle(i));
	updateSingleInputMaskEntry(i, MASK_RIGHT, hasPressedRightSingle(i));
	updateSingleInputMaskEntry(i, MASK_UP, hasPressedUpSingle(i));
	updateSingleInputMaskEntry(i, MASK_DOWN, hasPressedDownSingle(i));

	
}

static void updateInputMasks() {
	int i;
	for (i = 0; i < 2; i++) {
		updateInputMask(i);
	}
}

static void updateSingleRegisteredCommand(void* tCaller, RegisteredMugenCommand& tData) {
	(void)tCaller;
	RegisteredMugenCommand* command = &tData;

	updateCommandStates(command);
	updateActiveMugenCommands(command);
	updateStaticMugenCommands(command);
}

static void updateMugenCommandHandler(void* tData) {
	(void)tData;
	updateInputMasks();

	stl_vector_map(gMugenCommandHandler.mRegisteredCommands, updateSingleRegisteredCommand);
}

ActorBlueprint getDreamMugenCommandHandler() {
	return makeActorBlueprint(loadMugenCommandHandler, unloadMugenCommandHandler, updateMugenCommandHandler);
};
