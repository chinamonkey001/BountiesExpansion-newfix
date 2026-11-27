#pragma once

class ModProgress
{
private:
	bool hasChanges;
	const char* dataFile;
	std::map<int, BountyMissionStatus> cache;
	// Per-area round order (areaId -> mission ids in the order they come up).
	// Written to the same file as "<id> <status>" lines plus "order ..." lines,
	// so a reload continues the current round rather than dealing a new one.
	std::map<int, std::vector<int> > roundOrders;

public:
	ModProgress(const char* dataFile);

	BountyMissionStatus getMissionProgress(int missionId);
	// True when the progress file already carries this mission (no side effect,
	// unlike getMissionProgress which seeds a missing mission as Pending).
	bool hasMission(int missionId);
	void completeMission(int missionId);
	void collectMission(int missionId);
	void allowMission(int missionId);
	void save();

	// Empty vector when this area has no persisted order yet.
	std::vector<int> getRoundOrder(int areaId);
	void setRoundOrder(int areaId, const std::vector<int>& ids);

private:
	void setMissionProgress(int missionId, BountyMissionStatus status);
	void readFromDataFile();
	void parseToCache(std::string inputLine);
};