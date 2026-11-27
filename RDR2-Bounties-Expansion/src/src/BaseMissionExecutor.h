#pragma once

const int AREA_RADIUS = 90;
const int REQUIRED_DIST_TO_LOCATE = 20;

class BaseMissionExecutor
{
private:
	Prompt* inspectPosterPrompt;
	Prompt* acceptPosterPrompt;
	Prompt* declinePosterPrompt;
	bool posterPanelOpen;
	// Last time this board tried to respawn its sheet. Without it a board whose
	// paper model fails to load retries on every frame, and each retry waits out
	// the model load timeout - six boards in a row froze the game for a minute.
	int posterRespawnAt = 0;
	// Rate limit for rebuilding blips after the mission flag flickers.
	int stoppedFlagAt = 0;
	// Last time this board tried to respawn its sheet. Without it a board whose
	// paper model fails to load retries on every frame, and each retry waits out
	// the model load timeout - six boards in a row froze the game for a minute.
	int posterPanelPage;
	bool posterIsNativeUI;
	int posterRetries;
	int posterRetryTimer;
	// Reward settlement: what the player actually gets when the target
	// is handed over, which can differ from the printed price.
	bool targetWasKilled;
	int deliveredReward;
	// Set once the player has walked away from the board, so the review
	// prompt does not pop up the instant a poster is accepted.
	bool reviewArmed;
	bool posterPanelFromBoard;
	// Set once the "you already carry a bounty" hint was shown at a board.
	bool busyHintShown;
	BountyMissionData* missionData;
	BountyMissionStage stage;
	BountyMissionStatus status;
	MapAreasManager* areasMgr;

	int requiredDistanceToLocate;
	bool mustBeCloseToLocate;
	bool seen;
	bool stopped;
	int targetAreaRadius;
	bool spawnedBountyHunters;
	int spawnchance;
	int nameRefreshCooldown;
	bool spawnchanceComputed;
	bool posterRivalsPlanned;
	bool posterRivalsDone;
	// The ambush is rolled exactly once per bounty, when the poster is
	// accepted. Rolling it while rebuilding blips re-armed it on every
	// mission-flag flicker and piled up riders without end.
	bool posterRivalsRolled;
	bool custodyRivalsDone;

	Object poster;
	Object dummyProp;
	Pickup money;
	Blip posterBlip;
	Blip targetAreaBlip;
	Blip policeLocBlip;
	Blip cellBlip;
	Blip targetBlip;
	Blip moneyBlip;
	vector<Ped> bountyHunters;
	vector<Ped> bountyhorses;
	vector<Ped> randomGuards;
	vector<Blip> guardBlips;

protected:
	Ped target;

public:
	BaseMissionExecutor(BountyMissionData missionData, MapAreasManager* areasMgr);
	BountyMissionData* getMissionData();
	BountyMissionStage getMissionStage();
	BountyMissionStatus getMissionStatus();
	void releaseEntitySafe(Entity* entity);
	void setMissionStatus(BountyMissionStatus status);
	virtual void update();

protected:
	MapArea* getArea();
	Ped getTarget();
	void setTargetAreaRadius(int radius);
	void setRequiredDistanceToLocateTarget(int distance);
	void setMustBeCloseToLocate(bool toggle);
	virtual void prepareSet() = 0;
	virtual Ped spawnTarget() = 0;
	void nextStage();

	void fail(const char* reason = NULL);
	virtual void initialize();
	virtual void onPosterCollected();
	virtual void onArrivalToArea();
	virtual void onTargetLocated();
	virtual void onTargetCaptured();
	virtual void onArrivalToPoliceStation();
	virtual void onTargetHandedOver();
	virtual void onRewardCollected();
	virtual void onFinished(bool shouldCleanup = true);
	virtual void cleanup();

private:
	void applyTargetName();
	void drawPosterPanel();
	void drawTargetMarker();
	void drawPosterOverlay();
	void spawnPosterProp();
	void fillPosterBoard();
	void takePoster();
	void putPosterBack();
	int scaledReward();
	int rewardForDelivery();
	int rewardAmount(bool dead);
	void updatePosterReview(Ped player, const Vector3& playerPos);
	void decorateTarget();
	void spawnBountyHunters();
	void spawnPosterRivals();
	void spawnRandomGuards();
	void releaseGuards();
	void addBountyHunter(Ped horse, bool isDriver);
	// Same bookkeeping as addBountyHunter, without a mount. Used when
	// [Poster] HuntersOnHorse=0: a mounted ped on this build only shouts.
	void addBountyHunterOnFoot(Vector3 pos);
	// Custom relationship group for hunters. The stock bounty hunter ped
	// identity refuses to fight a badge-carrying player, so the group is
	// created here and set to hate the player explicitly.
	Hash getRivalsRelationshipGroup();
	Hash rivalsRelGroup = 0;
	DWORD rivalsFireAt = 0;
	DWORD dropHintAt = 0;
	DWORD rivalsDiagAt = 0;
	// A mounted wave never closes the distance in this build, so after a
	// few seconds of standing still they are put on foot once.
	DWORD rivalsFarSince = 0;
	bool rivalsDismounted = false;
	// Scripted fire orders: the badge mod stops the stock combat AI from
	// engaging, so the weapon is fired directly while a mission is live.
	void enforceRivalsFire();
	Vector3 positionAheadOfPlayer(float distance, float lateralOffset);
	void relocateRivals(const char* reason);
	// A wave that cannot keep up with a galloping player is moved back in
	// front of him; capped so it cannot keep teleporting forever.
	int rivalsRelocateCount = 0;
	void releaseUnnecessaryEntities();
};