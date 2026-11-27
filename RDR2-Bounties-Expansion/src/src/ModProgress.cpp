#include "Main.h";

using namespace std;

ModProgress::ModProgress(const char* dataFile)
{
	hasChanges = false;
	this->dataFile = dataFile;
	readFromDataFile();
}

void ModProgress::completeMission(int missionId)
{
	setMissionProgress(missionId, BountyMissionStatus::Completed);
}

void ModProgress::collectMission(int missionId)
{
	setMissionProgress(missionId, BountyMissionStatus::CollectedPoster);
}

void ModProgress::allowMission(int missionId)
{
	setMissionProgress(missionId, BountyMissionStatus::Pending);
}

void ModProgress::save()
{
	std::map<int, BountyMissionStatus>::iterator it;
	std::ofstream file;


	if (!hasChanges)
	{
		return;
	}

	log("saving mod progress");

	file.open(dataFile);;
	if (file.is_open())
	{
		bool first = true;

		it = cache.begin();
		while (it != cache.end())
		{
			if (!first)
			{
				file << "\n";
			}

			std::stringstream output;
			; output << it->first << " " << static_cast<int>(it->second);
			file << output.str();

			first = false;
			it++;
		}

		// "<areaId> <missionId> <missionId> ..." keeps the current round alive
		// across reloads. Old builds read these lines as garbage and skip them
		// (parseToCache cannot parse them as "<id> <status>").
		std::map<int, std::vector<int> >::iterator orderIt = roundOrders.begin();
		while (orderIt != roundOrders.end())
		{
			if (!first)
			{
				file << "\n";
			}

			file << "order " << orderIt->first;
			for (size_t i = 0; i < orderIt->second.size(); i++)
			{
				file << " " << orderIt->second[i];
			}

			first = false;
			orderIt++;
		}

		hasChanges = false;
		file.close();
	}
}

std::vector<int> ModProgress::getRoundOrder(int areaId)
{
	std::map<int, std::vector<int> >::iterator it = roundOrders.find(areaId);

	if (it == roundOrders.end())
	{
		return std::vector<int>();
	}

	return it->second;
}

void ModProgress::setRoundOrder(int areaId, const std::vector<int>& ids)
{
	roundOrders[areaId] = ids;
	hasChanges = true;
}

BountyMissionStatus ModProgress::getMissionProgress(int missionId)
{
	std::map<int, BountyMissionStatus>::iterator it = cache.find(missionId);

	if (it == cache.end())
	{
		setMissionProgress(missionId, BountyMissionStatus::Pending);
		save();
		return BountyMissionStatus::Pending;
	}

	return it->second;;
}

bool ModProgress::hasMission(int missionId)
{
	return cache.find(missionId) != cache.end();
}

void ModProgress::setMissionProgress(int missionId, BountyMissionStatus status)
{
	std::map<int, BountyMissionStatus>::iterator it = cache.find(missionId);

	if (it == cache.end())
	{
		cache[missionId] = status;
		hasChanges = true;
	}
	else
	{
		if (it->second != status)
		{
			hasChanges = true;
			it->second = status;
		}
	}

}

void ModProgress::readFromDataFile()
{
	std::ifstream file;
	std::string input;
	int count = 0;

	file.open(dataFile);;
	if (file.is_open())
	{
		log("loading mod progress from data file");

		while (!file.eof())
		{
			std::getline(file, input);
			parseToCache(input);
			count++;
		}

		file.close();
		string text = string("mod progress loaded: ").append(to_string(count)).append(" missions");
		log(text);
	}
	else
	{
		log("data file not found, starting with an empty progress map");
	}
}

void ModProgress::parseToCache(std::string inputLine)
{
	std::string idStr, statusStr;
	int id, status;

	// "order <areaId> <id> <id> ..." — the round order of one area.
	if (inputLine.compare(0, 6, "order ") == 0)
	{
		std::istringstream stream(inputLine);
		std::string tag;
		int areaId;

		if (!(stream >> tag >> areaId))
		{
			return;
		}

		std::vector<int> ids;
		int missionId;
		while (stream >> missionId)
		{
			ids.push_back(missionId);
		}

		roundOrders[areaId] = ids;
		return;
	}

	int splitPos = inputLine.find_first_of(' ');

	if (splitPos == std::string::npos)
	{
		return;
	}

	idStr = inputLine.substr(0, splitPos);
	statusStr = inputLine.substr(splitPos + 1);

	try
	{
		id = std::stoi(idStr);
		status = std::stoi(statusStr);
	}
	catch (...)
	{
		return;
	}

	cache[id] = static_cast<BountyMissionStatus>(status);
}