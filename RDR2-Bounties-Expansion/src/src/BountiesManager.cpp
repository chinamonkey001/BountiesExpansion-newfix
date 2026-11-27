#include "Main.h";

using namespace std;

const char BOUNTY_FAILED_REPLAY[] = "You may restart failed bounty missions after a few game hours.";

BountiesManager::BountiesManager(ModProgress* progress, MapAreasManager* areasMgr, BountyMissionsFactory* missionsFactory)
{
	this->progress = progress;
	this->areasMgr = areasMgr;
	this->missionsFactory = missionsFactory;

	loadActiveMissions();
}

void BountiesManager::resetMissions(MapAreas areaId)
{
	MapArea* area = areasMgr->getMapArea(areaId);

	string msg("reseting missions in ");
	msg = msg.append(area->name);
	log(msg);

	// A reset is a fresh start: deal a new order for this area.
	area->reshuffleRound();
	progress->setRoundOrder(area->getAreaId(), area->getRoundOrder());

	set<int>::iterator missionsItr = area->getMissionIds()->begin();
	while (missionsItr != area->getMissionIds()->end())
	{
		BaseMissionExecutor* executor = missionsFactory->fromMissionId(*missionsItr);

		string msg("reseting: ");
		msg = msg.append(executor->getMissionData()->targetName);

		progress->allowMission(*missionsItr);
		releaseBounty(*missionsItr);
		executor->setMissionStatus(BountyMissionStatus::Pending);

		if (!isExecutorLoaded(executor))
		{
			missionExecutors.push_back(executor);
			msg = msg.append(", was not loaded");
		}

		log(msg);
		missionsItr++;
	}

	progress->save();
}

void BountiesManager::update()
{
	BaseMissionExecutor* curr;
	BountyMissionStatus currStatus;
	int currId;
	//vector<BaseMissionExecutor*> finishedMissions;
	vector<BaseMissionExecutor*>::iterator it = missionExecutors.begin();

	while (it != missionExecutors.end())
	{
		curr = *it;
		currStatus = curr->getMissionStatus();
		currId = curr->getMissionData()->id;

		if (currStatus == BountyMissionStatus::Completed)
		{
			std::string msg = "completed mission: ";
			msg = msg.append(curr->getMissionData()->targetName);
			log(msg);

			progress->completeMission(currId);
			progress->save();
			releaseBounty(currId);
			it = missionExecutors.erase(it);
			nextMissions[currId] = getGameTime();
			//finishedMissions.push_back(curr);

		}
		else if (currStatus == BountyMissionStatus::Failed)
		{
			releaseBounty(currId);
			it = missionExecutors.erase(it);
			failedMissions[currId] = getGameTime();

			std::string msg = "mission failed: ";
			msg = msg.append(curr->getMissionData()->targetName);
			log(msg);
			setHelpMessage(TF("ui.hint.restart", BOUNTY_FAILED_REPLAY));
		}
		else
		{
			if (currStatus >= BountyMissionStatus::CollectedPoster && currStatus <= BountyMissionStatus::Completed)
			{
				// Once per few seconds at most. This block runs every frame while
				// the bounty is in progress, and an unthrottled save here rewrote
				// the progress file sixty times a second.
				int nowSave = GAMEPLAY::GET_GAME_TIMER();
				if (nowSave - lastProgressSaveAt > 5000)
				{
					lastProgressSaveAt = nowSave;
					progress->collectMission(currId);
					progress->save();
				}
			}

			(*it)->update();
			it++;
		}
	}
		tm gameTime = getGameTime();
		tm* missionNextTime;
		int missionId;
		map<int, tm>::iterator id = nextMissions.begin();
		while (id != nextMissions.end())
		{
			missionId = (*id).first;
			missionNextTime = &(*id).second;
			int diffInSecs = difftime(mktime(&gameTime), mktime(missionNextTime));
			if (diffInSecs >= NEXT_MISSION_TIME_WAITING_SECS)
			{
				curr = missionsFactory->fromMissionId(missionId);
				startNextMission(curr);
				id = nextMissions.erase(id);
			}
			else
			{
				id++;
			}
		}
	//for (it = finishedMissions.begin(); it != finishedMissions.end(); it++)
	//{
	//	startNextMission(*it);
	//}

	updateFailedMissions();
}

void BountiesManager::loadActiveMissions()
{
	vector<MapArea*> areas = areasMgr->getMapAreas();
	vector<MapArea*>::iterator areasItr;
	MapArea* currArea;

	log("loading missions:");

	for (areasItr = areas.begin(); areasItr != areas.end(); areasItr++)
	{
		currArea = *areasItr;
		set<int>* missionIds = currArea->getMissionIds();
		set<int>::iterator missionItr = missionIds->begin();

		// The round order is part of the save: a reload continues the current
		// round. Only a missing order (first run, wiped file) or one that no
		// longer matches this area's mission set gets a fresh shuffle.
		vector<int> savedOrder = progress->getRoundOrder(currArea->getAreaId());
		bool orderValid = savedOrder.size() == missionIds->size();
		if (orderValid)
		{
			for (size_t i = 0; i < savedOrder.size(); i++)
			{
				if (missionIds->find(savedOrder[i]) == missionIds->end())
				{
					orderValid = false;
					break;
				}
			}
		}

		if (orderValid)
		{
			currArea->setRoundOrder(savedOrder);
		}
		else
		{
			currArea->reshuffleRound();
			progress->setRoundOrder(currArea->getAreaId(), currArea->getRoundOrder());
			progress->save();

			string roundLog("new bounty round in ");
			roundLog.append(currArea->name);
			roundLog.append(" (initialised)");
			log(roundLog);
		}

		// A bounty already carried (poster in hand / mission running) keeps its
		// slot. Otherwise the round opens on the first card of the saved order
		// that is still up for grabs — with a wiped file that is simply the
		// first card of the newly dealt round.
		int activeMissionId = -1;
		for (set<int>::iterator probe = missionIds->begin(); probe != missionIds->end(); ++probe)
		{
			if (!progress->hasMission(*probe))
			{
				continue;
			}

			BountyMissionStatus st = progress->getMissionProgress(*probe);
			if (st == BountyMissionStatus::CollectedPoster || st == BountyMissionStatus::InProgress)
			{
				activeMissionId = *probe;
				break;
			}
		}

		if (activeMissionId != -1)
		{
			missionItr = missionIds->find(activeMissionId);
		}
		else
		{
			bool openingFound = false;
			vector<int>& order = currArea->getRoundOrder();
			for (size_t i = 0; i < order.size(); i++)
			{
				int candidate = order[i];
				BountyMissionStatus st = progress->hasMission(candidate)
					? progress->getMissionProgress(candidate)
					: BountyMissionStatus::Pending;

				if (st == BountyMissionStatus::Unavailable || st == BountyMissionStatus::Pending)
				{
					set<int>::iterator opening = missionIds->find(candidate);
					if (opening != missionIds->end())
					{
						missionItr = opening;
						openingFound = true;

						string openLog("opening bounty of the round in ");
						openLog.append(currArea->name);
						openLog.append(": id ");
						openLog.append(to_string(candidate));
						log(openLog);
					}
					break;
				}
			}

			// Every bounty of this area is done: the round is over, so deal a new
			// one. Only this area is reshuffled, the other areas keep their order.
			// restartAreaRound already puts the first card of the new round up, so
			// skip the scan below to avoid loading a second executor here.
			if (!openingFound && !missionIds->empty())
			{
				restartAreaRound(currArea);
				continue;
			}
		}

		int currMissionId;
		BaseMissionExecutor* executor;

		bool doneAreaHandling = false;;
		while (missionItr != missionIds->end() && !doneAreaHandling)
		{
			currMissionId = *missionItr;
			if (missionsFactory->doesMissionExist(currMissionId))
			{
				BountyMissionStatus missionProgress = progress->getMissionProgress(currMissionId);
				if (missionProgress > BountyMissionStatus::Unavailable && missionProgress < BountyMissionStatus::Completed)
				{
					// A save written while two bounties were carried ends up here.
					// The first one keeps the slot, the others go back on their
					// board and can be taken again later.
					if (missionProgress >= BountyMissionStatus::CollectedPoster &&
						!claimBounty(currMissionId))
					{
						progress->allowMission(currMissionId);
						progress->save();
						missionProgress = BountyMissionStatus::Pending;

						string resetLog = string("bounty already active, poster returned to the board: ")
							.append(to_string(currMissionId));
						log(resetLog);

						executor = missionsFactory->fromMissionId(currMissionId);
						executor->setMissionStatus(missionProgress);
						missionExecutors.push_back(executor);
						doneAreaHandling = true;
						missionItr++;
						continue;
					}

					executor = missionsFactory->fromMissionId(currMissionId);
					executor->setMissionStatus(missionProgress);
					missionExecutors.push_back(executor);
					doneAreaHandling = true;
					log(executor->getMissionData()->targetName);
				}
				executor = missionsFactory->fromMissionId(currMissionId);
				if (missionProgress == BountyMissionStatus::Completed)
				{
					loadNextMission(executor);
				}
			}
			else
			{
				string logTxt = string("could not find a registered executor for mission id: ").append(to_string(currMissionId));
				log(logTxt);
			}

			missionItr++;
		}
	}
}

void BountiesManager::loadNextMission(BaseMissionExecutor* after)
{
	MapArea* area = areasMgr->getMapArea(after->getMissionData()->area);
	int nextMissionId = pickNextMissionId(area, after->getMissionData()->id);

	if (nextMissionId == -1)
	{
		restartAreaRound(area);
	}
}

void BountiesManager::updateFailedMissions()
{
	tm gameTime = getGameTime();
	tm* missionFailureTime;
	int missionId;
	BaseMissionExecutor* missionExecutor;

	map<int, tm>::iterator it = failedMissions.begin();

	while (it != failedMissions.end())
	{
		missionId = (*it).first;
		missionFailureTime = &(*it).second;

		int diffInSecs = difftime(mktime(&gameTime), mktime(missionFailureTime));
		if (diffInSecs >= RESTART_FAILED_MISSION_WAITING_SECS)
		{
			// Still carrying another bounty: keep this one waiting instead of
			// spawning a second active case file.
			if (!claimBounty(missionId))
			{
				it++;
				continue;
			}

			missionExecutor = missionsFactory->fromMissionId(missionId);
			missionExecutor->setMissionStatus(BountyMissionStatus::CollectedPoster);
			missionExecutors.push_back(missionExecutor);
			it = failedMissions.erase(it);
		}
		else
		{
			it++;
		}
	}
}

void BountiesManager::startNextMission(BaseMissionExecutor* after)
{
	log((new string("starting next mission after: "))->append(after->getMissionData()->targetName));

	MapArea* area = areasMgr->getMapArea(after->getMissionData()->area);
	int nextMissionId = pickNextMissionId(area, after->getMissionData()->id);

	if (nextMissionId != -1)
	{
		BaseMissionExecutor* nextMission = missionsFactory->fromMissionId(nextMissionId);

		if (progress->getMissionProgress(nextMissionId) > BountyMissionStatus::Unavailable)
		{
			nextMission->setMissionStatus(progress->getMissionProgress(nextMissionId));
		}

		progress->allowMission(nextMissionId);
		missionExecutors.push_back(nextMission);

		std::string msg = "starting new mission: ";
		msg = msg.append(nextMission->getMissionData()->targetName);
		log(msg);
	}
	else
	{
		restartAreaRound(area);
	}
}

int BountiesManager::pickNextMissionId(MapArea* area, int fromId)
{
	if (!area)
	{
		return -1;
	}

	vector<int>& order = area->getRoundOrder();
	size_t count = order.size();
	if (count == 0)
	{
		return -1;
	}

	// Where the bounty that just finished sits in this round. -1 when it is not
	// part of the round at all (a bounty carried over from an earlier round),
	// in which case the walk simply starts at the first card.
	size_t fromPos = count;
	for (size_t i = 0; i < count; i++)
	{
		if (order[i] == fromId)
		{
			fromPos = i;
			break;
		}
	}

	// Everything after it in the round, wrapping around to the start. Wrapping
	// matters for reloads: the bounties the player still owes may be behind him
	// in the freshly shuffled order.
	for (size_t i = 0; i < count; i++)
	{
		size_t idx = (fromPos + 1 + i) % count;
		if (fromPos != count && idx == fromPos)
		{
			continue;
		}

		int id = order[idx];
		BountyMissionStatus st = progress->getMissionProgress(id);
		if (st == BountyMissionStatus::Unavailable || st == BountyMissionStatus::Pending)
		{
			return id;
		}
	}

	return -1;
}

void BountiesManager::restartAreaRound(MapArea* area)
{
	if (!area)
	{
		return;
	}

	// A new round: deal a new order, hand every bounty of the area back to its
	// board, and put the first one of the new order up for the player. The old
	// code reset the status of one executor fixed outside the loop, so after a
	// full round only the first bounty of an area ever came back.
	area->reshuffleRound();
	progress->setRoundOrder(area->getAreaId(), area->getRoundOrder());

	set<int>* ids = area->getMissionIds();
	for (set<int>::iterator it = ids->begin(); it != ids->end(); ++it)
	{
		BaseMissionExecutor* ex = missionsFactory->fromMissionId(*it);
		if (ex)
		{
			ex->setMissionStatus(BountyMissionStatus::Pending);
		}
		progress->allowMission(*it);
	}

	if (!ids->empty())
	{
		BaseMissionExecutor* first = missionsFactory->fromMissionId(area->getRoundOrder().front());
		if (first && !isExecutorLoaded(first))
		{
			missionExecutors.push_back(first);
		}
	}

	progress->save();
	log(string("new bounty round in ").append(area->name));
}

bool BountiesManager::isExecutorLoaded(BaseMissionExecutor* executor)
{
	vector<BaseMissionExecutor*>::iterator executorsItr = missionExecutors.begin();
	while (executorsItr != missionExecutors.end())
	{
		if ((*executorsItr)->getMissionData()->id == executor->getMissionData()->id)
		{
			return true;
		}

		executorsItr++;
	}

	return false;
}