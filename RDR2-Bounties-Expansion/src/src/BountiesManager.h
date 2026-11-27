#pragma once

using namespace std;

const int RESTART_FAILED_MISSION_WAITING_HOURS = 12;
const int RESTART_FAILED_MISSION_WAITING_SECS = RESTART_FAILED_MISSION_WAITING_HOURS * 60 * 60;

const int NEXT_MISSION_TIME_WAITING_HOURS = 6;//the 6 hours is for testing purposes and not the final wait time
const int NEXT_MISSION_TIME_WAITING_SECS = NEXT_MISSION_TIME_WAITING_HOURS * 60 * 60;

class BountiesManager
{
private:
	ModProgress* progress;
	MapAreasManager* areasMgr;
	BountyMissionsFactory* missionsFactory;
	std::map<int, BaseMissionExecutor*> executorsCache;
	vector<BaseMissionExecutor*> missionExecutors;
	map<int, tm> failedMissions;
	map<int, tm> nextMissions;

	// Throttles the progress save. A mission sitting in CollectedPoster status
	// used to trigger collectMission + save on EVERY frame, i.e. sixty file
	// writes a second, which saturated the disk and froze the script.
	int lastProgressSaveAt = 0;

public:
	BountiesManager(ModProgress* progress, MapAreasManager* areasMgr, BountyMissionsFactory* missionsFactory);
	void update();
	void resetMissions(MapAreas area);

private:
	void loadActiveMissions();
	void startNextMission(BaseMissionExecutor* after);
	void loadNextMission(BaseMissionExecutor* after);
	void updateFailedMissions();
	bool isExecutorLoaded(BaseMissionExecutor* executor);
	// Next bounty of the current round that the player can actually take, or -1
	// when the round is spent.
	int pickNextMissionId(MapArea* area, int fromId);
	// Reshuffle the area, hand every bounty back, put the first of the new
	// order on the board.
	void restartAreaRound(MapArea* area);
};
