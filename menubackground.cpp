#include "menubackground.h"

#include <assert.h>

#include <prism/log.h>
#include <prism/system.h>
#include <prism/math.h>

#include "mugenstagehandler.h"
#include "stage.h"

static struct {
	Vector3DI mLocalCoordinates;
} gMenuBackgroundData;

static int isMenuBackgroundGroup(MugenDefScriptGroup* tGroup, const char* tBackgroundGroupName) {
	if (tGroup == NULL) return 0;

	char firstW[100];
	int items = sscanf(tGroup->mName.data(), "%s", firstW);
	return items == 1 && !strcmp(tBackgroundGroupName, firstW);
}

static void loadNormalMenuBackgroundGroup(MugenDefScriptGroup* tGroup, int i, MugenSpriteFile* tSprites) {
	Vector3DI spriteNo = getMugenDefVectorIOrDefaultAsGroup(tGroup, "spriteno", makeVector3DI(0, 0, 0));
	int layerNo = getMugenDefIntegerOrDefaultAsGroup(tGroup, "layerno", 0);
	Vector3D start = getMugenDefVectorOrDefaultAsGroup(tGroup, "start", makePosition(0, 0, 0));
	start.z = i;
	Vector3D delta = getMugenDefVectorOrDefaultAsGroup(tGroup, "delta", makePosition(1, 1, 0));
	Vector3DI tile = getMugenDefVectorIOrDefaultAsGroup(tGroup, "tile", makeVector3DI(0, 0, 0));
	Vector3DI tileSpacing = getMugenDefVectorIOrDefaultAsGroup(tGroup, "tilespacing", makeVector3DI(0, 0, 0));
	BlendType blendType = getBackgroundBlendType(tGroup);
	blendType = handleBackgroundMask(tGroup, blendType);
	const auto alpha = getBackgroundAlphaVector(tGroup);
	GeoRectangle constraintRectangle = getMugenDefGeoRectangleOrDefaultAsGroup(tGroup, "window", makeGeoRectangle(-INF / 2, -INF / 2, INF, INF));
	Vector3D velocity = getMugenDefVectorOrDefaultAsGroup(tGroup, "velocity", makePosition(0, 0, 0));
	int id = getMugenDefIntegerOrDefaultAsGroup(tGroup, "id", -1);

	addDreamMugenStageHandlerAnimatedBackgroundElement(start, createOneFrameMugenAnimationForSprite(spriteNo.x, spriteNo.y), 1, tSprites, delta, tile, tileSpacing, blendType, alpha, constraintRectangle, makePosition(0, 0, 0), velocity, makePosition(0, 0, 0), makePosition(0, 0, 0), 1, 0, makePosition(1.0, 1.0, 1.0), makePosition(0.0, 0.0, 0.0), makePosition(1.0, 1.0, 1.0), layerNo, id, 0, makeVector3DI(1, 1, 1), makePosition(1, 1, 1), 1.0, 0, gMenuBackgroundData.mLocalCoordinates);
}

static void loadAnimatedMenuBackgroundGroup(MugenDefScriptGroup* tGroup, int i, MugenSpriteFile* tSprites, MugenAnimations* tAnimations) {
	int animation = getMugenDefIntegerOrDefaultAsGroup(tGroup, "actionno", -1);
	int layerNo = getMugenDefIntegerOrDefaultAsGroup(tGroup, "layerno", 0);
	Vector3D start = getMugenDefVectorOrDefaultAsGroup(tGroup, "start", makePosition(0, 0, 0));
	start.z = i + layerNo*BACKGROUND_UPPER_BASE_Z;
	Vector3D delta = getMugenDefVectorOrDefaultAsGroup(tGroup, "delta", makePosition(1, 1, 0));
	Vector3DI tile = getMugenDefVectorIOrDefaultAsGroup(tGroup, "tile", makeVector3DI(0, 0, 0));
	Vector3DI tileSpacing = getMugenDefVectorIOrDefaultAsGroup(tGroup, "tilespacing", makeVector3DI(0, 0, 0));
	BlendType blendType = getBackgroundBlendType(tGroup);
	const auto alpha = getBackgroundAlphaVector(tGroup);
	GeoRectangle constraintRectangle = getMugenDefGeoRectangleOrDefaultAsGroup(tGroup, "window", makeGeoRectangle(-INF / 2, -INF / 2, INF, INF));
	Vector3D velocity = getMugenDefVectorOrDefaultAsGroup(tGroup, "velocity", makePosition(0, 0, 0));
	int id = getMugenDefIntegerOrDefaultAsGroup(tGroup, "id", -1);
	
	addDreamMugenStageHandlerAnimatedBackgroundElement(start, getMugenAnimation(tAnimations, animation), 0, tSprites, delta, tile, tileSpacing, blendType, alpha, constraintRectangle, makePosition(0, 0, 0), velocity, makePosition(0, 0, 0), makePosition(0, 0, 0), 1, 0, makePosition(1.0, 1.0, 1.0), makePosition(0.0, 0.0, 0.0), makePosition(1.0, 1.0, 1.0), layerNo, id, 0, makeVector3DI(1, 1, 1), makePosition(1, 1, 1), 1.0, 0, gMenuBackgroundData.mLocalCoordinates);
}

static void loadMenuBackgroundGroup(MugenDefScriptGroup* tGroup, int i, MugenSpriteFile* tSprites, MugenAnimations* tAnimations) {
	char* type = getAllocatedMugenDefStringVariableAsGroup(tGroup, "type");
	if (!strcmp("normal", type) || !strcmp("parallax", type)) {
		loadNormalMenuBackgroundGroup(tGroup, i, tSprites);
	} else if (!strcmp("anim", type)) {
		loadAnimatedMenuBackgroundGroup(tGroup, i, tSprites, tAnimations);
	}
	else {
		logWarningFormat("Unimplemented menu background type %s. Try to load normal.", type);
		loadNormalMenuBackgroundGroup(tGroup, i, tSprites);
	}

	freeMemory(type);
}

void loadMenuBackground(MugenDefScript * tScript, MugenSpriteFile * tSprites, MugenAnimations * tAnimations, const char * tDefinitionGroupName, const char * tBackgroundGroupName)
{
	instantiateActor(getDreamMugenStageHandler());
	gMenuBackgroundData.mLocalCoordinates = makeVector3DI(320, 240, 0);
	setDreamStageCoordinates(gMenuBackgroundData.mLocalCoordinates);
	setDreamMugenStageHandlerCameraCoordinates(gMenuBackgroundData.mLocalCoordinates);
	setDreamMugenStageHandlerCameraRange(makeGeoRectangle(0, 0, gMenuBackgroundData.mLocalCoordinates.x, gMenuBackgroundData.mLocalCoordinates.y));
	setDreamMugenStageHandlerCameraPosition(makePosition(0, 0, 0));

	MugenDefScriptGroup* current = &tScript->mGroups[tDefinitionGroupName];
	current = current->mNext;

	int i = 1;
	int isRunning = 1;
	while (isRunning) {
		if (!isMenuBackgroundGroup(current, tBackgroundGroupName)) break;

		loadMenuBackgroundGroup(current, i, tSprites, tAnimations);

		i++;
		current = current->mNext;
	}
}
