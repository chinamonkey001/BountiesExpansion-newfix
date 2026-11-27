#pragma once

class MapArea
{
private:
	set<int> areaMissionIds;
	// The order this area's bounties appear in, reshuffled at the start of every
	// round. Persisted in the progress file, so a reload continues the round
	// instead of dealing a new hand. Walking the id set in ascending order put
	// the same faces on the board in the same sequence on every playthrough.
	vector<int> roundOrderIds;
	int areaId;

public:
	const char* name;
	Vector3* policeDeptCoords;
	// Where a dead target has to be dropped: the station's backyard.
	Vector3* corpseDropCoords;
	Vector3* bountyPostersCoords;
	Vector3* cellCoords;
	Vector3* moneyCoords;
	int cellnumber;
	float bountyPosterHeading;

	MapArea(const char* name, Vector3 policeDeptCoords, Vector3 corpseDropCoords, Vector3 bountyPostersCoords, Vector3 moneyCoords, Vector3 cellCoords, int cellnumber, float bountyPosterHeading = 0);

	void linkMission(int missionId);
	// New random order for the next round, every bounty exactly once.
	void reshuffleRound();
	// Current round order, shuffled on first use.
	vector<int>& getRoundOrder();
	// Restore an order read back from the progress file.
	void setRoundOrder(const vector<int>& ids);
	void setAreaId(int id);
	int getAreaId();
	set<int>* getMissionIds();
}; 