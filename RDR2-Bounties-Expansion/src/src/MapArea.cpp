#include "Main.h"

MapArea::MapArea(const char* name, Vector3 policeDeptCoords, Vector3 corpseDropCoords, Vector3 bountyPostersCoords, Vector3 moneyCoords, Vector3 cellCoords, int cellnumber, float bountyPosterHeading)
{
	this->name = name;
	this->policeDeptCoords = new Vector3(policeDeptCoords);
	this->corpseDropCoords = new Vector3(corpseDropCoords);
	this->bountyPostersCoords = new Vector3(bountyPostersCoords);
	this->moneyCoords = new Vector3(moneyCoords);
	this->cellCoords = new Vector3(cellCoords);
	this->cellnumber = cellnumber;
	this->bountyPosterHeading = bountyPosterHeading;
	this->areaId = 0;
}

void MapArea::linkMission(int missionId)
{
	areaMissionIds.insert(missionId);
}

void MapArea::reshuffleRound()
{
	roundOrderIds.clear();
	for (set<int>::iterator it = areaMissionIds.begin(); it != areaMissionIds.end(); ++it)
	{
		roundOrderIds.push_back(*it);
	}

	// Fisher-Yates with plain rand(): <random> is not pulled in anywhere else in
	// the mod, and this way the build settings stay untouched.
	for (size_t i = roundOrderIds.size(); i > 1; --i)
	{
		size_t j = (size_t)(rand() % (int)i);
		int tmp = roundOrderIds[i - 1];
		roundOrderIds[i - 1] = roundOrderIds[j];
		roundOrderIds[j] = tmp;
	}
}

vector<int>& MapArea::getRoundOrder()
{
	// First look at this area in a session: deal a hand.
	if (roundOrderIds.empty())
	{
		reshuffleRound();
	}

	return roundOrderIds;
}

void MapArea::setRoundOrder(const vector<int>& ids)
{
	roundOrderIds = ids;
}

void MapArea::setAreaId(int id)
{
	areaId = id;
}

int MapArea::getAreaId()
{
	return areaId;
}

set<int>* MapArea::getMissionIds()
{
	return &(this->areaMissionIds);
}