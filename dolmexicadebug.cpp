#include "dolmexicadebug.h"

#include <sstream>
#include <prism/stlutil.h>
#include <prism/debug.h>
#include <prism/log.h>
#include <prism/input.h>
#include <prism/math.h>

#include "gamelogic.h"
#include "playerdefinition.h"
#include "characterselectscreen.h"
#include "stage.h"
#include "fightscreen.h"
#include "fightdebug.h"
#include "mugencommandhandler.h"
#include "mugenassignmentevaluator.h"
#include "titlescreen.h"
#include "storymode.h"
#include "randomwatchmode.h"
#include "config.h"
#include "mugenstatehandler.h"

typedef struct {
	int mPreviousValue;
	int* mValuePointer;
} TrackedInteger;

typedef struct {
	std::unordered_map<std::string, TrackedInteger> mMap;
	int mIsOverridingTimeDilatation;
	double mOverridingTimeDilatationSpeed;

	std::unordered_map<std::string, std::set<int>> mStoryCharAnimations;
} DolmexicaDebugData;

static DolmexicaDebugData* gDolmexicaDebugData = nullptr;

static std::vector<std::string> splitCommandString(std::string s) {
	std::vector<std::string> ret;
	int n = 0;
	while (n < (int)s.size()) {
		int next = (int)s.find(' ', n);
		if (next == -1) {
			ret.push_back(s.substr(n));
			break;
		}
		else {
			ret.push_back(s.substr(n, next - n));
			n = next + 1;
		}
	}
	return ret;
}

static void mockFightFinishedCB() {
	setNewScreen(getDreamTitleScreen());
}

static std::string fightCB(void* /*tCaller*/, const std::string& tCommand) {
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 3) return "Too few arguments";
	
	char path[1024];
	getCharacterSelectNamePath(words[1].data(), path);
	setPlayerDefinitionPath(0, path);
	getCharacterSelectNamePath(words[2].data(), path);
	setPlayerDefinitionPath(1, path);

	if (words.size() >= 4) {
		if (words[3] == "watch") {
			setGameModeWatch();
		} 
		else if (words[3] == "superwatch") {
			setGameModeSuperWatch();
		}
		else if (words[3] == "osu") {
			setGameModeOsu();
		}
		else if (words[3] == "versus") {
			setGameModeVersus();
		}
		else if (words[3] == "freeplay") {
			setGameModeFreePlay();
		}
		else {
			setGameModeTraining();
		}
	}
	else {
		setGameModeTraining();
	}

	if (words.size() >= 5) {
		setDreamStageMugenDefinition((getDolmexicaAssetFolder() + "stages/" + words[4]).c_str(), "");
	}
	else {
		setDreamStageMugenDefinition((getDolmexicaAssetFolder() + "stages/kfm.def").c_str(), "");
	}

	startFightScreen(mockFightFinishedCB);

	return "";
}

static std::string stageCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 2) return "Too few arguments";

	setPlayerDefinitionPath(0, (getDolmexicaAssetFolder() + "chars/kfm/kfm.def").c_str());
	setPlayerDefinitionPath(1, (getDolmexicaAssetFolder() + "chars/kfm/kfm.def").c_str());
	setDreamStageMugenDefinition((getDolmexicaAssetFolder() + "stages/" + words[1]).c_str(), "");
	setGameModeTraining();
	startFightScreen(mockFightFinishedCB);

	return "";
}

static std::string fightdebugCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	(void)tCommand;
	switchFightDebugTextActivity();
	return "";
}

static std::string fightcollisionCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	(void)tCommand;
	switchFightCollisionDebugActivity();
	return "";
}

static std::string skipintroCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	(void)tCommand;
	skipFightIntroWithoutFading();
	return "";
}

static std::string rootposCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 3) return "Too few arguments";

	int id = atoi(words[1].data());
	DreamPlayer* p = getRootPlayer(id);
	setPlayerPositionBasedOnScreenCenterX(p, atof(words[2].data()), getPlayerCoordinateP(p));

	if (words.size() >= 4) {
		setPlayerPositionY(p, atof(words[3].data()), getPlayerCoordinateP(p));
	}

	return "";
}

static std::string rootposcloseCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 2) return "Too few arguments";

	double dist = atof(words[1].data());
	setPlayerPositionBasedOnScreenCenterX(getRootPlayer(0), -dist / 2, getPlayerCoordinateP(getRootPlayer(0)));
	setPlayerPositionBasedOnScreenCenterX(getRootPlayer(1), dist / 2, getPlayerCoordinateP(getRootPlayer(1)));

	return "";
}

static std::string rootctrlCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 3) return "Too few arguments";

	int id = atoi(words[1].data());
	DreamPlayer* p = getRootPlayer(id);
	setPlayerControl(p, atoi(words[2].data()));

	return "";
}

static std::string fullpowerCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 2) return "Too few arguments";

	int id = atoi(words[1].data());
	DreamPlayer* p = getRootPlayer(id);
	setPlayerPower(p, getPlayerPowerMax(p));

	return "";
}

static std::string commandCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 3) return "Too few arguments";
	
	int id = atoi(words[1].data());
	DreamPlayer* p = getRootPlayer(id);
	int bufferTime = (words.size() >= 4) ? atoi(words[3].data()) : 2;
	setDreamPlayerCommandActiveForAI(p->mCommandID, words[2].data(), bufferTime);
	return "";
}

static std::string testcommandNumberCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 2) return "Too few arguments";

	int commandNumber = atoi(words[1].data());
	double dist = words.size() >= 3 ? atof(words[2].data()) : 20;

	const int placementDirection = (words.size() >= 4) ? atoi(words[3].data()) : 1;
	setPlayerPositionBasedOnScreenCenterX(getRootPlayer(0), -placementDirection * dist / 2, getPlayerCoordinateP(getRootPlayer(0)));
	setPlayerPositionBasedOnScreenCenterX(getRootPlayer(1), placementDirection * dist / 2, getPlayerCoordinateP(getRootPlayer(1)));

	DreamPlayer* p = getRootPlayer(0);
	const auto result = setDreamPlayerCommandNumberActiveForDebug(p->mCommandID, commandNumber);
	return result ? "" : "Over command amount";
}

static std::string evalCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 3) return "Too few arguments";
	int id = atoi(words[1].data());
	DreamPlayer* p = getRootPlayer(id);

	auto n = tCommand.find(' ', tCommand.find(' ') + 1) + 1;
	std::string assignmentString = tCommand.substr(n);
	char buffer[1024];
	strcpy(buffer, assignmentString.c_str());

	setActiveStateMachineCoordinateP(getPlayerCoordinateP(p));

	auto assignment = parseDreamMugenAssignmentFromString(buffer);
	std::string result;
	evaluateDreamAssignmentAndReturnAsString(result, &assignment, p);
	destroyDreamMugenAssignment(assignment);
	std::string ret = result;
	return ret;
}

static std::string evalhelperCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 4) return "Too few arguments";
	int id = atoi(words[1].data());
	int helper = atoi(words[2].data());
	DreamPlayer* p = getRootPlayer(id);
	p = getPlayerHelperOrNullIfNonexistant(p, helper);
	if (!p) {
		return "No helper with that ID.";
	}

	auto n = tCommand.find(' ', tCommand.find(' ', tCommand.find(' ') + 1) + 1) + 1;
	std::string assignmentString = tCommand.substr(n);
	char buffer[1024];
	strcpy(buffer, assignmentString.c_str());

	setActiveStateMachineCoordinateP(getPlayerCoordinateP(p));

	auto assignment = parseDreamMugenAssignmentFromString(buffer);
	std::string result;
	evaluateDreamAssignmentAndReturnAsString(result, &assignment, p);
	destroyDreamMugenAssignment(assignment);
	std::string ret = result;
	return ret;
}

static std::string trackvarCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 3) return "Too few arguments";
	int id = atoi(words[1].data());
	int varNumber = atoi(words[2].data());

	DreamPlayer* p = getRootPlayer(id);

	TrackedInteger e;
	e.mValuePointer = getPlayerVariableReference(p, varNumber);
	e.mPreviousValue = *e.mValuePointer;
	const auto s = std::string("player ").append(std::to_string(id)).append("; var ").append(std::to_string(varNumber));
	gDolmexicaDebugData->mMap[s] = e;
	
	const auto ret = std::string(" value: ").append(std::to_string(*e.mValuePointer));
	return ret;
}

static std::string untrackvarCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 3) return "Too few arguments";
	int id = atoi(words[1].data());
	int varNumber = atoi(words[2].data());

	const auto s = std::string("player ").append(std::to_string(id)).append("; var ").append(std::to_string(varNumber));
	gDolmexicaDebugData->mMap.erase(s);
	return "";
}

static std::string stateCB(void* tCaller, const std::string& tCommand) {
	(void)tCaller;
	std::vector<std::string> words = splitCommandString(tCommand);
	if (words.size() < 3) return "Too few arguments";
	int id = atoi(words[1].data());
	int stateNumber = atoi(words[2].data());
	DreamPlayer* p = getRootPlayer(id);

	changePlayerState(p, stateNumber);
	return "";
}

static std::string storyCB(void* /*tCaller*/, const std::string& tCommand) {
	const auto words = splitCommandString(tCommand);
	if (words.size() < 2) return "Too few arguments";
	const auto& name = words[1];

	int step = 0;
	if (words.size() >= 3) {
		step = atoi(words[2].c_str());
	}

	startStoryModeWithForcedStartState(name.c_str(), step);
	return "";
}

static std::string randomwatchCB(void* /*tCaller*/, const std::string& /*tCommand*/) {
	startRandomWatchMode();
	return "";
}

static std::string speedCB(void* /*tCaller*/, const std::string& tCommand) {
	const auto words = splitCommandString(tCommand);
	if (words.size() < 2) return "Too few arguments";
	const auto speed = atof(words[1].c_str());
	setWrapperTimeDilatation(speed);
	gDolmexicaDebugData->mIsOverridingTimeDilatation = (speed != 1.0) ? 1 : 0;
	gDolmexicaDebugData->mOverridingTimeDilatationSpeed = speed;
	return "";
}

static std::string roundamountCB(void* /*tCaller*/, const std::string& tCommand) {
	const auto words = splitCommandString(tCommand);
	if (words.size() < 2) return "Too few arguments";
	const auto rounds = atoi(words[1].c_str());
	setRoundsToWin(rounds);
	return "";
}

static std::string readStoryAnimsCB(void* /*tCaller*/, const std::string& /*tCommand*/) {
	auto b = fileToBuffer("debug/anims.txt");
	auto p = getBufferPointer(b);

	while (hasStringFromTextStreamBufferPointer(p)) {
		const auto characterName = readStringFromTextStreamBufferPointer(&p);
	
		while (true) {
			if (!hasStringFromTextStreamBufferPointer(p)) break;
			const auto nextString = readStringFromTextStreamBufferPointer(&p);
			if (nextString == "------------------") break;

			const auto animValue = atoi(nextString.c_str());
			gDolmexicaDebugData->mStoryCharAnimations[characterName].insert(animValue);
		}
	}
	freeBuffer(b);

	return "";
}

static std::string writeStoryAnimsCB(void* /*tCaller*/, const std::string& /*tCommand*/) {
	std::stringstream ss;
	
	if (isFile("debug/anims.txt")) {
		auto b = fileToBuffer("debug/anims.txt");
		bufferToFile("debug/anims_old.txt", b);
		freeBuffer(b);
	}

	for (const auto& it : gDolmexicaDebugData->mStoryCharAnimations) {
		ss << it.first << std::endl << std::endl;
		for (const auto val : it.second) {
			ss << val << std::endl;
		}
		ss << "------------------" << std::endl;

	}

	bufferToFile("debug/anims.txt", makeBuffer((void*)ss.str().c_str(), uint32_t(ss.str().size())));

	return "";
}

static std::string difficultyCB(void* /*tCaller*/, const std::string& tCommand) {
	const auto words = splitCommandString(tCommand);
	if (words.size() < 2) return "Too few arguments";
	const auto difficulty = atoi(words[1].c_str());
	setDifficulty(difficulty);
	return "";
}

#if defined (_WIN32) || defined(VITA)
#include <filesystem>

static bool caseIndifferentManualTestNameSort (const std::string& lhs, const std::string& rhs) {
	return std::lexicographical_compare(lhs.begin(), lhs.end(),
		rhs.begin(), rhs.end(),
		[](char a, char b) {
		return tolower(a) < tolower(b);
	});
}
static struct {
	int mIsActive;
	int mIsAutoMode;
	std::set<std::string, bool(*)(const std::string& lhs, const std::string& rhs)> mCharacters = std::set<std::string, bool(*)(const std::string& lhs, const std::string& rhs)>(caseIndifferentManualTestNameSort);
} gFullCharacterTestData;

static void collectFullCharacterTestCharacters() {
	std::filesystem::path folderMask = "assets/chars" ;
	for (auto const& dir_entry : std::filesystem::directory_iterator{ folderMask })
	{
		gFullCharacterTestData.mCharacters.insert(dir_entry.path().stem().string());
	}
}

static void pruneFullCharacterTestCharacters(const std::string& tStartCharacter) {
	if (tStartCharacter.empty()) return;

	auto it = gFullCharacterTestData.mCharacters.begin();
	while (it != gFullCharacterTestData.mCharacters.end()) {
		auto current = it;
		it++;
		if (*current == tStartCharacter) break;
		gFullCharacterTestData.mCharacters.erase(current);
	}
}

static void processFullCharacterTestCharacters(const std::string& tStartCharacter)
{
	gFullCharacterTestData.mCharacters.clear();
	collectFullCharacterTestCharacters();
	pruneFullCharacterTestCharacters(tStartCharacter);
}

static void fullManualTestFightFinishedCB() {}

static void startNextFullCharacterTestFight() {
	if (gFullCharacterTestData.mCharacters.empty()) {
		abortScreenHandling();
		return;
	}
	const auto characterName = *gFullCharacterTestData.mCharacters.begin();
	gFullCharacterTestData.mCharacters.erase(gFullCharacterTestData.mCharacters.begin());

	char path[1024];
	getCharacterSelectNamePath(characterName.c_str(), path);
	setPlayerDefinitionPath(0, path);
	setPlayerDefinitionPath(1, path);
	setDreamStageMugenDefinition((getDolmexicaAssetFolder() + "stages/kfm.def").c_str(), "");
	setGameModeSuperWatch();
	setRandomSeed(0);
	startFightScreen(fullManualTestFightFinishedCB);
}

static std::string fullCharacterTestCB(void* /*tCaller*/, const std::string& tCommand) {
	const auto words = splitCommandString(tCommand);
	std::string startCharacter;
	if (words.size() >= 2) {
		if (words[1] == "auto") {
			gFullCharacterTestData.mIsAutoMode = 1;
		}
		else {
			startCharacter = words[1];
		}
	}
	if (words.size() >= 3) {
		if (words[2] == "auto") {
			gFullCharacterTestData.mIsAutoMode = 1;
		}
	}
	processFullCharacterTestCharacters(startCharacter);
	gFullCharacterTestData.mIsActive = 1;
	startNextFullCharacterTestFight();
	return "";
}

static void updateFullCharacterTest() {
	if (!gFullCharacterTestData.mIsActive) return;

	static const auto TEST_SECONDS = 120;
	const auto isAutomaticallyOver = gFullCharacterTestData.mIsAutoMode ? (getDreamGameTime() > TEST_SECONDS * 60) : 0;
	if (hasPressedKeyboardKeyFlank(KEYBOARD_F1_PRISM) || isAutomaticallyOver) {
		startNextFullCharacterTestFight();
	}
}

static struct {
	int mIsActive;
	std::set<std::string, bool(*)(const std::string& lhs, const std::string& rhs)> mStages = std::set<std::string, bool(*)(const std::string& lhs, const std::string& rhs)>(caseIndifferentManualTestNameSort);
} gFullStageTestData;

static void collectFullStageTestStagesRecursive(const std::string& tSearchPath) {
	std::filesystem::path folderMask = tSearchPath;
	for (auto const& dir_entry : std::filesystem::directory_iterator{ folderMask })
	{
		if (dir_entry.is_directory())
		{
			collectFullStageTestStagesRecursive(tSearchPath + dir_entry.path().stem().string() + "/");
		}
		else {
			const auto extension = dir_entry.path().extension().string();
			if (extension != ".def") continue;
			auto filePath = tSearchPath + dir_entry.path().filename().string();
			cleanPathSlashes(filePath);
			gFullStageTestData.mStages.insert(filePath);

		}
	}
}

static void pruneFullStageTestStages(const std::string& tStartStage) {
	if (tStartStage.empty()) return;

	auto it = gFullStageTestData.mStages.begin();
	while (it != gFullStageTestData.mStages.end()) {
		auto current = it;
		it++;
		if (*current == tStartStage) break;
		gFullStageTestData.mStages.erase(current);
	}
}

static void processFullStageTestStages(const std::string& tStartStage)
{
	gFullStageTestData.mStages.clear();
	collectFullStageTestStagesRecursive(getDolmexicaAssetFolder() + "stages/");
	pruneFullStageTestStages(tStartStage);
}

static void startNextFullStageTestFight() {
	if (gFullStageTestData.mStages.empty()) {
		abortScreenHandling();
		return;
	}
	const auto stageName = *gFullStageTestData.mStages.begin();
	gFullStageTestData.mStages.erase(gFullStageTestData.mStages.begin());

	setPlayerDefinitionPath(0, (getDolmexicaAssetFolder() + "chars/kfm/kfm.def").c_str());
	setPlayerDefinitionPath(1, (getDolmexicaAssetFolder() + "chars/kfm/kfm.def").c_str());
	setDreamStageMugenDefinition(stageName.c_str(), "");
	setGameModeTraining();
	setRandomSeed(0);
	startFightScreen(fullManualTestFightFinishedCB);
}

static std::string fullStageTestCB(void* /*tCaller*/, const std::string& tCommand) {
	const auto words = splitCommandString(tCommand);
	std::string startStage;
	if (words.size() >= 2) {
		startStage = words[1];
	}

	processFullStageTestStages(startStage);
	gFullStageTestData.mIsActive = 1;
	startNextFullStageTestFight();
	return "";
}

static void updateFullStageTest() {
	if (!gFullStageTestData.mIsActive) return;

	if (hasPressedKeyboardKeyFlank(KEYBOARD_F1_PRISM)) {
		startNextFullStageTestFight();
	}
}
#else
static std::string fullCharacterTestCB(void*, const std::string&) {
	return "";
}

static void updateFullCharacterTest() {}

static std::string fullStageTestCB(void*, const std::string&) {
	return "";
}

static void updateFullStageTest() {}
#endif

static std::string randomSeedCB(void* /*tCaller*/, const std::string& tCommand) {
	const auto words = splitCommandString(tCommand);
	if (words.size() < 2) return "Too few arguments";
	const auto seed = atoi(words[1].c_str());
	setRandomSeed(unsigned(seed));
	return "";
}

static std::string airJumpCB(void* /*tCaller*/, const std::string& tCommand) {
	const auto words = splitCommandString(tCommand);
	if (words.size() < 2) return "Too few arguments";
	const auto airJumpAmount = atoi(words[1].c_str());
	setPlayerMovementAirJumpNum(getRootPlayer(0), airJumpAmount);
	return "";
}

void initDolmexicaDebug()
{
	gDolmexicaDebugData = new DolmexicaDebugData();
	gDolmexicaDebugData->mIsOverridingTimeDilatation = 0;
	gDolmexicaDebugData->mOverridingTimeDilatationSpeed = 1.0;

	addPrismDebugConsoleCommand("fight", fightCB);
	addPrismDebugConsoleCommand("stage", stageCB);
	addPrismDebugConsoleCommand("fightdebug", fightdebugCB);
	addPrismDebugConsoleCommand("fightcollision", fightcollisionCB);
	addPrismDebugConsoleCommand("skipintro", skipintroCB);
	addPrismDebugConsoleCommand("rootpos", rootposCB);
	addPrismDebugConsoleCommand("rootposclose", rootposcloseCB);
	addPrismDebugConsoleCommand("rootctrl", rootctrlCB);
	addPrismDebugConsoleCommand("fullpower", fullpowerCB);
	addPrismDebugConsoleCommand("command", commandCB);
	addPrismDebugConsoleCommand("testcommandn", testcommandNumberCB);
	addPrismDebugConsoleCommand("eval", evalCB);
	addPrismDebugConsoleCommand("evalhelper", evalhelperCB);
	addPrismDebugConsoleCommand("trackvar", trackvarCB);
	addPrismDebugConsoleCommand("untrackvar", untrackvarCB);
	addPrismDebugConsoleCommand("state", stateCB);
	addPrismDebugConsoleCommand("story", storyCB);
	addPrismDebugConsoleCommand("randomwatch", randomwatchCB);
	addPrismDebugConsoleCommand("speed", speedCB);
	addPrismDebugConsoleCommand("roundamount", roundamountCB);
	addPrismDebugConsoleCommand("readstoryanims", readStoryAnimsCB);
	addPrismDebugConsoleCommand("writestoryanims", writeStoryAnimsCB);
	addPrismDebugConsoleCommand("difficulty", difficultyCB);
	addPrismDebugConsoleCommand("fullcharactertest", fullCharacterTestCB);
	addPrismDebugConsoleCommand("fullstagetest", fullStageTestCB);
	addPrismDebugConsoleCommand("randomseed", randomSeedCB);
	addPrismDebugConsoleCommand("airjump", airJumpCB);
}

static void loadDolmexicaDebugHandler(void* tData) {
	(void)tData;
	setProfilingSectionMarkerCurrentFunction();
	gDolmexicaDebugData->mMap.clear();
}

static void updateSingleTrackedInteger(void* tCaller, const std::string& tKey, TrackedInteger& e) {
	(void)tCaller;

	if (*e.mValuePointer != e.mPreviousValue) {
		std::ostringstream ss;
		ss << tKey << " changed to " << *e.mValuePointer;
		submitToPrismDebugConsole(ss.str());
	}

	e.mPreviousValue = *e.mValuePointer;

}

static void updateSpeedOverrideToggle() {
	if (hasPressedKeyboardKeyFlank(KEYBOARD_F9_PRISM)) {
		gDolmexicaDebugData->mIsOverridingTimeDilatation ^= 1;
		if (gDolmexicaDebugData->mIsOverridingTimeDilatation) setWrapperTimeDilatation(gDolmexicaDebugData->mOverridingTimeDilatationSpeed);
		else setWrapperTimeDilatation(1.0);
	}
}

static void updateDolmexicaDebugHandler(void* tData) {
	(void)tData;
	setProfilingSectionMarkerCurrentFunction();
	updateSpeedOverrideToggle();
	updateFullCharacterTest();
	updateFullStageTest();
	stl_string_map_map(gDolmexicaDebugData->mMap, updateSingleTrackedInteger);
}

ActorBlueprint getDolmexicaDebug() {
	return makeActorBlueprint(loadDolmexicaDebugHandler, NULL, updateDolmexicaDebugHandler);
}

int isDebugOverridingTimeDilatation()
{
	if (!gDolmexicaDebugData) return 0;
	return gDolmexicaDebugData->mIsOverridingTimeDilatation;
}

void addDebugDolmexicaStoryCharacterAnimation(const char * tCharacter, int tAnimation)
{
	if (!gDolmexicaDebugData) return;
	std::string name = tCharacter;
	turnStringLowercase(name);
	gDolmexicaDebugData->mStoryCharAnimations[name].insert(tAnimation);
}
