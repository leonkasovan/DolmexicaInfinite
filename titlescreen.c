#include "titlescreen.h"

#include <assert.h>

#include <prism/animation.h>
#include <prism/screeneffect.h>
#include <prism/input.h>
#include <prism/mugendefreader.h>
#include <prism/file.h>
#include <prism/mugenanimationhandler.h>
#include <prism/math.h>
#include <prism/mugensoundfilereader.h>
#include <prism/mugentexthandler.h>
#include <prism/tweening.h>
#include <prism/clipboardhandler.h>
#include <prism/screeneffect.h>
#include <prism/timer.h>

#include <prism/log.h>

#include "mugensound.h"
#include "menubackground.h"
#include "characterselectscreen.h"
#include "arcademode.h"
#include "versusmode.h"
#include "trainingmode.h"
#include "fightscreen.h"
#include "storymode.h"
#include "creditsmode.h"
#include "optionsscreen.h"
#include "boxcursorhandler.h"
#include "survivalmode.h"
#include "watchmode.h"
#include "superwatchmode.h"
#include "randomwatchmode.h"
#include "exhibitmode.h"
#include "freeplaymode.h"
#include "intro.h"

typedef struct {
	void(*mCB)();
	Position mOffset;

	int mTextID;
} MenuText;

typedef struct {
	int mFadeInTime;
	int mFadeOutTime;

	Position mMenuPosition;
	Vector3DI mItemFont;
	Vector3DI mItemActiveFont;

	Vector3D mItemSpacing;
	Vector3D mWindowMarginsY;
	GeoRectangle mMenuRectangle;

	int mIsBoxCursorVisible;
	GeoRectangle mBoxCursorCoordinates;

	int mVisibleItemAmount;

	Vector3DI mCursorMoveSound;
	Vector3DI mCursorDoneSound;
	Vector3DI mCancelSound;

} MenuHeader;

typedef struct {
	int mNow;
	int mWaitTime;
	int mIsEnabled;

} DemoHeader;

static struct {
	MugenDefScript mScript;
	MugenSpriteFile mSprites;
	MugenAnimations mAnimations;
	MugenSounds mSounds;

	MenuHeader mHeader;
	DemoHeader mDemo;
	Vector mMenus;

	Position mMenuBasePosition;
	Position mMenuTargetPosition;

	int mBoxCursorID;

	TextureData mWhiteTexture;
	int mCreditBGAnimationID;
	int mLeftCreditTextID;
	int mRightCreditTextID;

	int mTopOption;
	int mSelected;
} gData;

static void gotoArcadeMode(void* tCaller) {
	(void)tCaller;
	startArcadeMode();
}

static void arcadeCB() {
	addFadeOut(gData.mHeader.mFadeOutTime, gotoArcadeMode, NULL);
}

static void gotoVersusMode(void* tCaller) {
	(void)tCaller;
	startVersusMode();
}

static void versusCB() {
	addFadeOut(gData.mHeader.mFadeOutTime, gotoVersusMode, NULL);
}

static void gotoTrainingMode(void* tCaller) {
	(void)tCaller;
	startTrainingMode();
}

static void trainingCB() {
	addFadeOut(gData.mHeader.mFadeOutTime, gotoTrainingMode, NULL);
}

static void gotoStoryMode(void* tCaller) {
	(void)tCaller;
	startStoryMode();
}

static void storyCB() {
	addFadeOut(gData.mHeader.mFadeOutTime, gotoStoryMode, NULL);
}

static void gotoCreditsMode(void* tCaller) {
	(void)tCaller;
	startCreditsMode();
}

static void creditsCB() {
	addFadeOut(gData.mHeader.mFadeOutTime, gotoCreditsMode, NULL);
}

static void gotoOptionsScreen(void* tCaller) {
	(void)tCaller;
	startOptionsScreen();
}

static void optionsCB() {
	addFadeOut(gData.mHeader.mFadeOutTime, gotoOptionsScreen, NULL);
}

static void gotoSurvivalMode(void* tCaller) {
	(void)tCaller;
	startSurvivalMode();
}

static void survivalCB() {
	addFadeOut(gData.mHeader.mFadeOutTime, gotoSurvivalMode, NULL);
}

static void gotoWatchMode(void* tCaller) {
	(void)tCaller;
	startWatchMode();
}

static void watchCB() {
	addFadeOut(gData.mHeader.mFadeOutTime, gotoWatchMode, NULL);
}

static void gotoSuperWatchMode(void* tCaller) {
	(void)tCaller;
	startSuperWatchMode();
}

static void superWatchCB() {
	addFadeOut(gData.mHeader.mFadeOutTime, gotoSuperWatchMode, NULL);
}

static void gotoRandomWatchMode(void* tCaller) {
	(void)tCaller;
	startRandomWatchMode();
}

static void randomWatchCB() {
	addFadeOut(gData.mHeader.mFadeOutTime, gotoRandomWatchMode, NULL);
}

static void gotoFreePlayMode(void* tCaller) {
	(void)tCaller;
	startFreePlayMode();
}

static void freePlayCB() {
	addFadeOut(gData.mHeader.mFadeOutTime, gotoFreePlayMode, NULL);
}

static void gotoExhibitMode(void* tCaller) {
	(void)tCaller;
	if (hasIntroStoryboard() && hasFinishedIntroWaitCycle()) {
		playIntroStoryboard();
	}
	else {
		increaseIntroWaitCycle();
		startExhibitMode();
	}	
}

static void exhibitCB(void* tCaller) {
	(void)tCaller;
	addFadeOut(gData.mHeader.mFadeOutTime, gotoExhibitMode, NULL);
}

static void exitCB() {
	abortScreenHandling();
}

static void loadMenuHeader() {
	gData.mHeader.mFadeInTime = getMugenDefIntegerOrDefault(&gData.mScript, "Title Info", "fadein.time", 30);
	gData.mHeader.mFadeOutTime = getMugenDefIntegerOrDefault(&gData.mScript, "Title Info", "fadeout.time", 30);

	gData.mHeader.mMenuPosition = getMugenDefVectorOrDefault(&gData.mScript, "Title Info", "menu.pos", makePosition(0, 0, 0));
	gData.mHeader.mItemFont = getMugenDefVectorIOrDefault(&gData.mScript, "Title Info", "menu.item.font", makeVector3DI(1, 0, 0));
	gData.mHeader.mItemActiveFont = getMugenDefVectorIOrDefault(&gData.mScript, "Title Info", "menu.item.active.font", makeVector3DI(1, 0, 0));
	gData.mHeader.mItemSpacing = getMugenDefVectorOrDefault(&gData.mScript, "Title Info", "menu.item.spacing", makePosition(0, 0, 0));

	gData.mHeader.mWindowMarginsY = getMugenDefVectorOrDefault(&gData.mScript, "Title Info", "menu.window.margins.y", makePosition(0, 0, 0));

	gData.mHeader.mVisibleItemAmount = getMugenDefIntegerOrDefault(&gData.mScript, "Title Info", "menu.window.visibleitems", 10);
	gData.mHeader.mIsBoxCursorVisible = getMugenDefIntegerOrDefault(&gData.mScript, "Title Info", "menu.boxcursor.visible", 1);

	gData.mHeader.mMenuRectangle = makeGeoRectangle(-INF / 2, gData.mHeader.mMenuPosition.y - gData.mHeader.mWindowMarginsY.x, INF, gData.mHeader.mWindowMarginsY.x + gData.mHeader.mWindowMarginsY.y + gData.mHeader.mVisibleItemAmount*gData.mHeader.mItemSpacing.y);

	MugenStringVector boxCoordinateVector = getMugenDefStringVectorVariable(&gData.mScript, "Title Info", "menu.boxcursor.coords");
	assert(boxCoordinateVector.mSize >= 4);
	gData.mHeader.mBoxCursorCoordinates.mTopLeft.x = atof(boxCoordinateVector.mElement[0]);
	gData.mHeader.mBoxCursorCoordinates.mTopLeft.y = atof(boxCoordinateVector.mElement[1]);
	gData.mHeader.mBoxCursorCoordinates.mBottomRight.x = atof(boxCoordinateVector.mElement[2]);
	gData.mHeader.mBoxCursorCoordinates.mBottomRight.y = atof(boxCoordinateVector.mElement[3]);

	gData.mHeader.mCursorMoveSound = getMugenDefVectorIOrDefault(&gData.mScript, "Title Info", "cursor.move.snd", makeVector3DI(0, 0, 0));
	gData.mHeader.mCursorDoneSound = getMugenDefVectorIOrDefault(&gData.mScript, "Title Info", "cursor.done.snd", makeVector3DI(0, 0, 0));
	gData.mHeader.mCancelSound = getMugenDefVectorIOrDefault(&gData.mScript, "Title Info", "cancel.snd", makeVector3DI(0, 0, 0));
}

static void loadDemoHeader() {
	gData.mDemo.mIsEnabled = getMugenDefIntegerOrDefault(&gData.mScript, "Demo Mode", "enabled", 1);
	gData.mDemo.mWaitTime = getMugenDefIntegerOrDefault(&gData.mScript, "Demo Mode", "title.waittime", 600);
	gData.mDemo.mNow = 0;
}

static void addMenuPoint(char* tVariableName, void(*tCB)()) {
	if (!isMugenDefStringVariable(&gData.mScript, "Title Info", tVariableName)) return;
	char* text = getAllocatedMugenDefStringVariable(&gData.mScript, "Title Info", tVariableName);
	
	if (!strcmp("", text)) {
		freeMemory(text);
		return;
	}

	int index = vector_size(&gData.mMenus);
	MenuText* e = allocMemory(sizeof(MenuText));
	e->mCB = tCB;
	e->mOffset = makePosition(gData.mHeader.mItemSpacing.x*index, gData.mHeader.mItemSpacing.y*index, 50);
	Position position = vecAdd(gData.mHeader.mMenuPosition, e->mOffset);
	e->mTextID = addMugenText(text, position, gData.mHeader.mItemFont.x);
	setMugenTextColor(e->mTextID, getMugenTextColorFromMugenTextColorIndex(gData.mHeader.mItemFont.y));
	setMugenTextAlignment(e->mTextID, getMugenTextAlignmentFromMugenAlignmentIndex(gData.mHeader.mItemFont.z));
	setMugenTextRectangle(e->mTextID, gData.mHeader.mMenuRectangle);
	vector_push_back_owned(&gData.mMenus, e);

	freeMemory(text);
}

static void setLowerOptionAsBase() {
	gData.mTopOption = gData.mSelected - (gData.mHeader.mVisibleItemAmount - 1);
	gData.mMenuTargetPosition = makePosition(gData.mHeader.mMenuPosition.x - gData.mHeader.mItemSpacing.x*gData.mTopOption, gData.mHeader.mMenuPosition.y - gData.mHeader.mItemSpacing.y*gData.mTopOption, 0);
}

static void setUpperOptionAsBase() {
	gData.mTopOption = gData.mSelected;	
	gData.mMenuTargetPosition = makePosition(gData.mHeader.mMenuPosition.x - gData.mHeader.mItemSpacing.x*gData.mTopOption, gData.mHeader.mMenuPosition.y - gData.mHeader.mItemSpacing.y*gData.mTopOption, 0);
}

static void setSelectedMenuElementActive() {
	MenuText* e = vector_get(&gData.mMenus, gData.mSelected);
	setMugenTextFont(e->mTextID, gData.mHeader.mItemActiveFont.x);
	setMugenTextColor(e->mTextID, getMugenTextColorFromMugenTextColorIndex(gData.mHeader.mItemActiveFont.y));
	setMugenTextAlignment(e->mTextID, getMugenTextAlignmentFromMugenAlignmentIndex(gData.mHeader.mItemActiveFont.z));

	if (gData.mSelected >= gData.mTopOption + gData.mHeader.mVisibleItemAmount) {
		setLowerOptionAsBase();
	}
	else if (gData.mSelected < gData.mTopOption) {
		setUpperOptionAsBase();
	}
}

static void setSelectedMenuElementInactive() {
	MenuText* e = vector_get(&gData.mMenus, gData.mSelected);
	setMugenTextFont(e->mTextID, gData.mHeader.mItemFont.x);
	setMugenTextColor(e->mTextID, getMugenTextColorFromMugenTextColorIndex(gData.mHeader.mItemFont.y));
	setMugenTextAlignment(e->mTextID, getMugenTextAlignmentFromMugenAlignmentIndex(gData.mHeader.mItemFont.z));
}

static void loadCredits() {
	gData.mCreditBGAnimationID = playOneFrameAnimationLoop(makePosition(0, 230, 50), &gData.mWhiteTexture);
	setAnimationSize(gData.mCreditBGAnimationID, makePosition(320, 20, 1), makePosition(0, 0, 0));
	setAnimationColor(gData.mCreditBGAnimationID, 0, 0, 0.5);

	gData.mLeftCreditTextID = addMugenText("Dolmexica Infinite Demo 5", makePosition(0, 240, 51), 1);
	
	gData.mRightCreditTextID = addMugenText("11/02/18 Presented by Dogma", makePosition(320, 240, 51), 1);
	setMugenTextAlignment(gData.mRightCreditTextID, MUGEN_TEXT_ALIGNMENT_RIGHT);
}


static void loadBoxCursor() {
	if (gData.mHeader.mIsBoxCursorVisible) {
		gData.mBoxCursorID = addBoxCursor(makePosition(0, 0, 0), makePosition(0, 0, 49), gData.mHeader.mBoxCursorCoordinates); 
	}
}

static void loadTitleMusic() {
	char* path = getAllocatedMugenDefStringOrDefault(&gData.mScript, "Music", "title.bgm", " ");
	int isLooping = getMugenDefIntegerOrDefault(&gData.mScript, "Music", "title.bgm.loop", 1);

	if (isMugenBGMMusicPath(path)) {
		playMugenBGMMusicPath(path, isLooping);
	}

	freeMemory(path);
}

static void loadTitleScreen() {
	setWrapperTitleScreen(&DreamTitleScreen);
	
	instantiateActor(BoxCursorHandler);

	gData.mWhiteTexture = getEmptyWhiteTexture();

	char folder[1024];
	gData.mScript = loadMugenDefScript("assets/data/system.def");
	getPathToFile(folder, "assets/data/system.def");
	setWorkingDirectory(folder);

	char* text = getAllocatedMugenDefStringVariable(&gData.mScript, "Files", "spr");
	gData.mSprites = loadMugenSpriteFileWithoutPalette(text);
	gData.mAnimations = loadMugenAnimationFile("system.def");
	freeMemory(text);

	text = getAllocatedMugenDefStringVariable(&gData.mScript, "Files", "snd");
	gData.mSounds = loadMugenSoundFile(text);
	freeMemory(text);
	
	loadMenuHeader();
	loadDemoHeader();
	loadMenuBackground(&gData.mScript, &gData.mSprites, &gData.mAnimations, "TitleBGdef", "TitleBG");

	gData.mMenus = new_vector();
	addMenuPoint("menu.itemname.story", storyCB);
	addMenuPoint("menu.itemname.arcade", arcadeCB);
	addMenuPoint("menu.itemname.freeplay", freePlayCB);
	addMenuPoint("menu.itemname.versus", versusCB);
	addMenuPoint("menu.itemname.teamarcade", arcadeCB);
	addMenuPoint("menu.itemname.teamversus", arcadeCB);
	addMenuPoint("menu.itemname.teamcoop", arcadeCB);
	addMenuPoint("menu.itemname.survival", survivalCB);
	addMenuPoint("menu.itemname.survivalcoop", arcadeCB);
	addMenuPoint("menu.itemname.training", trainingCB);
	addMenuPoint("menu.itemname.watch", watchCB);
	addMenuPoint("menu.itemname.superwatch", superWatchCB);
	addMenuPoint("menu.itemname.randomwatch", randomWatchCB);
	addMenuPoint("menu.itemname.options", arcadeCB); // TODO: readd
	addMenuPoint("menu.itemname.credits", creditsCB);
	addMenuPoint("menu.itemname.exit", exitCB);

	loadBoxCursor();

	gData.mMenuBasePosition = gData.mHeader.mMenuPosition;
	gData.mMenuTargetPosition = gData.mMenuBasePosition;
	gData.mTopOption = 0;
	setSelectedMenuElementActive();

	loadCredits();

	setWorkingDirectory("/");
	loadTitleMusic();

	addFadeIn(gData.mHeader.mFadeInTime, NULL, NULL);

	logTextureMemoryState();
	logMemoryState();
}

static void unloadTitleScreen() {
	unloadMugenDefScript(gData.mScript);
	unloadMugenSpriteFile(&gData.mSprites);
	unloadMugenAnimationFile(&gData.mAnimations);
	unloadMugenSoundFile(&gData.mSounds);

	delete_vector(&gData.mMenus);
}

static void updateItemSelection() {
	
	if (hasPressedDownFlank()) {
		setSelectedMenuElementInactive();
		gData.mSelected = (gData.mSelected + 1) % vector_size(&gData.mMenus);
		setSelectedMenuElementActive();
		tryPlayMugenSound(&gData.mSounds, gData.mHeader.mCursorMoveSound.x, gData.mHeader.mCursorMoveSound.y);
	}
	else if (hasPressedUpFlank()) {
		setSelectedMenuElementInactive();
		gData.mSelected--;
		if (gData.mSelected < 0) gData.mSelected += vector_size(&gData.mMenus);
		setSelectedMenuElementActive();
		tryPlayMugenSound(&gData.mSounds, gData.mHeader.mCursorMoveSound.x, gData.mHeader.mCursorMoveSound.y);
	}

}

static void updateMenuBasePosition() {
	gData.mMenuBasePosition = vecAdd(gData.mMenuBasePosition, vecScale(vecSub(gData.mMenuTargetPosition, gData.mMenuBasePosition), 0.1));
}

static void updateMenuElementPositions() {
	int i;
	int l = vector_size(&gData.mMenus);
	for (i = 0; i < l; i++) {
		MenuText* e = vector_get(&gData.mMenus, i);
		setMugenTextPosition(e->mTextID, vecAdd(gData.mMenuBasePosition, e->mOffset));
	}
}

static void updateItemSelectionConfirmation() {
	if (hasPressedAFlank() || hasPressedStartFlank()) {
		MenuText* e = vector_get(&gData.mMenus, gData.mSelected);
		tryPlayMugenSound(&gData.mSounds, gData.mHeader.mCursorDoneSound.x, gData.mHeader.mCursorDoneSound.y);
		e->mCB();
	}
}

static void updateSelectionBoxPosition() {
	MenuText* e = vector_get(&gData.mMenus, gData.mSelected);

	Position pos = getMugenTextPosition(e->mTextID);
	pos.z = 0;
	setBoxCursorPosition(gData.mBoxCursorID, pos);
}

static void updateSelectionBox() {
	if (!gData.mHeader.mIsBoxCursorVisible) return;

	updateSelectionBoxPosition();
}

static void updateDemoMode() {
	if (!gData.mDemo.mIsEnabled) return;

	if (hasPressedAnyButton()) {
		gData.mDemo.mNow = 0;
	}

	if (gData.mDemo.mNow >= gData.mDemo.mWaitTime) {
		exhibitCB(NULL);
		gData.mDemo.mNow = 0;
	}

	gData.mDemo.mNow++;
}

static void updateTitleScreen() {
	// startFightScreen(); // TODO
	
	updateItemSelection();
	updateMenuBasePosition();
	updateMenuElementPositions();
	updateSelectionBox();
	updateItemSelectionConfirmation();
	updateDemoMode();
}

Screen DreamTitleScreen = {
	.mLoad = loadTitleScreen,
	.mUnload = unloadTitleScreen,
	.mUpdate = updateTitleScreen,
};
