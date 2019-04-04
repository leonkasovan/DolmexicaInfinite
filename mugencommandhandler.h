#pragma once

#include <prism/actorhandler.h>
#include <prism/mugenanimationhandler.h>

#include "mugencommandreader.h"
#include "playerdefinition.h"

int registerDreamMugenCommands(int tControllerID, DreamMugenCommands* tCommands);

int isDreamCommandActive(int tID, const char* tCommandName);
int isDreamCommandActiveByLookupIndex(int tID, int tLookupIndex);
int isDreamCommandForLookup(int tID, const char* tCommandName, int* oLookupIndex);
void setDreamPlayerCommandActiveForAI(int tID, const char* tCommandName, Duration tBufferTime);
void setDreamPlayerCommandNumberActiveForDebug(int tID, int tCommandNumber);
int getDreamPlayerCommandAmount(int tID);

void setDreamMugenCommandFaceDirection(int tID, FaceDirection tDirection);

ActorBlueprint getDreamMugenCommandHandler();