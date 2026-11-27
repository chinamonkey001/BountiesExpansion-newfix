#pragma execution_character_set("utf-8")
#include "Main.h";

BountyMissionsFactory::BountyMissionsFactory(MapAreasManager* areasMgr)
{
	this->areasMgr = areasMgr;
	initializeCache();
}

bool BountyMissionsFactory::doesMissionExist(int missionId)
{
	return (executorsCache.find(missionId) != executorsCache.end());
}

BaseMissionExecutor* BountyMissionsFactory::fromMissionId(int missionId)
{
	if (executorsCache.find(missionId) != executorsCache.end())
	{
		return executorsCache.find(missionId)->second;
	}

	return NULL;
}

std::vector<int> BountyMissionsFactory::getAllMissionIds()
{
	std::vector<int> missionsData;
	std::map<int, BaseMissionExecutor*>::iterator it = executorsCache.begin();

	while(it != executorsCache.end())
	{
		missionsData.push_back((*it).second->getMissionData()->id);
		++it;
	}

	std::sort(missionsData.begin(), missionsData.end());
	return missionsData;
}

void BountyMissionsFactory::initializeCache()
{
	executorsCache[1] = eliasTraditionExecutor();
	executorsCache[2] = turkishRunnerExecutor();
	executorsCache[3] = gushkalaInataExecutor();
	executorsCache[4] = hankEnepayExecutor();
	executorsCache[5] = nateNashobaExecutor();
	executorsCache[6] = charlesMoonExecutor();
	executorsCache[7] = jacobSeedExecutor();
	executorsCache[8] = janeBullionExecutor();
	executorsCache[9] = bufordDukeExecutor();
	executorsCache[10] = harperConnorExecutor();
	executorsCache[11] = wallaceKnoxExecutor();
	executorsCache[12] = emmettDavisExecutor();
	executorsCache[13] = andrewClarkExecutor();
	executorsCache[14] = chesterBurnettExecutor();
	executorsCache[15] = cullenBakerExecutor();
	executorsCache[16] = johnBullExecutor();
	executorsCache[17] = marquisRussellExecutor();
	executorsCache[18] = peteEatonExecutor();
	executorsCache[19] = jesseHillExecutor();
	executorsCache[20] = quentinChigurhExecutor();
	executorsCache[21] = tucoVillaExecutor();
	executorsCache[22] = billyWilsonExecutor();
	executorsCache[23] = joseChavezExecutor();
	executorsCache[24] = panchoDanielExecutor();
	executorsCache[25] = archStantonExecutor();
	executorsCache[26] = richardMooreExecutor();
	executorsCache[27] = juanCortezExecutor();
	executorsCache[28] = clintonvonHagenExecutor();
	executorsCache[29] = carlMartinExecutor();
	executorsCache[30] = treasuryPeteExecutor();
	executorsCache[31] = robertCorbucciExecutor();
	executorsCache[32] = mikeSandersExecutor();
	executorsCache[33] = chrisJaniceExecutor();
	executorsCache[34] = justusBarnesExecutor();
	executorsCache[35] = williamRogersExecutor();
	executorsCache[36] = bobLaneExecutor();
	executorsCache[37] = georgeCoenExecutor();
	executorsCache[38] = buckHicoxExecutor();
	executorsCache[39] = johnEastwoodExecutor();

	map<int, BaseMissionExecutor*>::iterator it = executorsCache.begin();
	BaseMissionExecutor* curr;
	while (it != executorsCache.end())
	{
		curr = (*it).second;
		areasMgr->getMapArea(curr->getMissionData()->area)->linkMission(curr->getMissionData()->id);
		++it;
	}
}

EliasTraditionExecutor* BountyMissionsFactory::eliasTraditionExecutor()
{
	BountyMissionData data;
	data.id = 1;
	data.area = MapAreas::Blackwater;
	data.missionName = "Elias's Tradition";
	data.crime = "mission.1.crime";
	data.description = "mission.1.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 75;
	data.rewardStr = "$75";
	data.startPosition.x = -2032.61;
	data.startPosition.y = -1909.63;
	data.startPosition.z = 110.051;
	data.isTargetMale = true;
	data.targetName = "mission.1.target";

	return new EliasTraditionExecutor(data, areasMgr);
}

TurkishRunnerExecutor* BountyMissionsFactory::turkishRunnerExecutor()
{
	BountyMissionData data;
	data.id = 2;
	data.area = MapAreas::Blackwater;
	data.missionName = "The Turkish Runner";
	data.crime = "mission.2.crime";
	data.description = "mission.2.desc";
	data.requiredTargetCondition = TargetCondition::Alive;
	data.reward = 50;
	data.rewardStr = "$50";
	data.startPosition = toVector3(-2815.12, -2613.38, 92.7243);
	data.isTargetMale = true;
	data.targetName = "mission.2.target";

	return new TurkishRunnerExecutor(data, areasMgr);
}

GushkalaInataExecutor* BountyMissionsFactory::gushkalaInataExecutor()
{
	BountyMissionData data;
	data.id = 3;
	data.area = MapAreas::Blackwater;
	data.missionName = "GushkalaInata";
	data.crime = "mission.3.crime";
	data.description = "mission.3.desc";
	data.requiredTargetCondition = TargetCondition::Alive;
	data.reward = 80;
	data.rewardStr = "$80";
	data.startPosition = toVector3(-2324.694, -1440.603, 151.5746);
	data.isTargetMale = true;
	data.targetName = "mission.3.target";

	return new GushkalaInataExecutor(data, areasMgr);
}

HankEnepayExecutor* BountyMissionsFactory::hankEnepayExecutor()
{
	BountyMissionData data;
	data.id = 4;
	data.area = MapAreas::Blackwater;
	data.missionName = "HankEnepay";
	data.crime = "mission.4.crime";
	data.description = "mission.4.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 90;
	data.rewardStr = "$90";
	data.startPosition = toVector3(-2357.136, -948.3532, 165.9597);
	data.isTargetMale = true;
	data.targetName = "mission.4.target";

	return new HankEnepayExecutor(data, areasMgr);
}

NateNashobaExecutor* BountyMissionsFactory::nateNashobaExecutor()
{
	BountyMissionData data;
	data.id = 5;
	data.area = MapAreas::Blackwater;
	data.missionName = "NateNashoba";
	data.crime = "mission.5.crime";
	data.description = "mission.5.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 75;
	data.rewardStr = "$75";
	data.startPosition = toVector3(-2575.68, -1371.892, 148.1127);
	data.isTargetMale = true;
	data.targetName = "mission.5.target";

	return new NateNashobaExecutor(data, areasMgr);
}

CharlesMoonExecutor* BountyMissionsFactory::charlesMoonExecutor()
{
	BountyMissionData data;
	data.id = 6;
	data.area = MapAreas::Blackwater;
	data.missionName = "CharlesMoon";
	data.crime = "mission.6.crime";
	data.description = "mission.6.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 80;
	data.rewardStr = "$80";
	data.startPosition = toVector3(-1957.099, -2712.923, 67.1411);
	data.isTargetMale = true;
	data.targetName = "mission.6.target";

	return new CharlesMoonExecutor(data, areasMgr);
}

JacobSeedExecutor* BountyMissionsFactory::jacobSeedExecutor()
{
	BountyMissionData data;
	data.id = 7;
	data.area = MapAreas::Rhodes;
	data.missionName = "Jacob Seed";
	data.crime = "mission.7.crime";
	data.description = "mission.7.desc";
	data.requiredTargetCondition = TargetCondition::Alive;
	data.reward = 60;
	data.rewardStr = "$60";
	data.startPosition = toVector3(1551.14, -372.55, 80.5242);
	data.isTargetMale = true;
	data.targetName = "mission.7.target";

	return new JacobSeedExecutor(data, areasMgr);
}

JaneBullionExecutor* BountyMissionsFactory::janeBullionExecutor()
{
	BountyMissionData data;
	data.id = 8;
	data.area = MapAreas::Rhodes;
	data.missionName = "JaneBullion";
	data.crime = "mission.8.crime";
	data.description = "mission.8.desc";
	data.requiredTargetCondition = TargetCondition::Alive;
	data.reward = 30;
	data.rewardStr = "$30";
	data.startPosition = toVector3(2114.806, -1277.321, 41.13449);
	data.isTargetMale = false;
	data.targetName = "mission.8.target";

	return new JaneBullionExecutor(data, areasMgr);
}

BufordDukeExecutor* BountyMissionsFactory::bufordDukeExecutor()
{
	BountyMissionData data;
	data.id = 9;
	data.area = MapAreas::Rhodes;
	data.missionName = "BufordDuke";
	data.crime = "mission.9.crime";
	data.description = "mission.9.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 55;
	data.rewardStr = "$55";
	data.startPosition = toVector3(1585.817, -1851.348, 51.3534);
	data.isTargetMale = true;
	data.targetName = "mission.9.target";

	return new BufordDukeExecutor(data, areasMgr);
}

HarperConnorExecutor* BountyMissionsFactory::harperConnorExecutor()
{
	BountyMissionData data;
	data.id = 10;
	data.area = MapAreas::Rhodes;
	data.missionName = "HarperConnor";
	data.crime = "mission.10.crime";
	data.description = "mission.10.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 65;
	data.rewardStr = "$65";
	data.startPosition = toVector3(768.0861, -852.4528, 54.38295);
	data.isTargetMale = true;
	data.targetName = "mission.10.target";

	return new HarperConnorExecutor(data, areasMgr);
}

WallaceKnoxExecutor* BountyMissionsFactory::wallaceKnoxExecutor()
{
	BountyMissionData data;
	data.id = 11;
	data.area = MapAreas::Rhodes;
	data.missionName = "WallaceKnox";
	data.crime = "mission.11.crime";
	data.description = "mission.11.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 70;
	data.rewardStr = "$70";
	data.startPosition = toVector3(1382.377, -2081.04, 50.99044);
	data.isTargetMale = true;
	data.targetName = "mission.11.target";

	return new WallaceKnoxExecutor(data, areasMgr);
}

EmmettDavisExecutor* BountyMissionsFactory::emmettDavisExecutor()
{
	BountyMissionData data;
	data.id = 12;
	data.area = MapAreas::Rhodes;
	data.missionName = "EmmettDavis";
	data.crime = "mission.12.crime";
	data.description = "mission.12.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 55;
	data.rewardStr = "$55";
	data.startPosition = toVector3(718.9932, -464.4996, 78.51635);
	data.isTargetMale = true;
	data.targetName = "mission.12.target";

	return new EmmettDavisExecutor(data, areasMgr);
}

AndrewClarkExecutor* BountyMissionsFactory::andrewClarkExecutor()
{
	BountyMissionData data;
	data.id = 13;
	data.area = MapAreas::Valentine;
	data.missionName = "安德鲁·克拉克";
	data.crime = "mission.13.crime";
	data.description = "mission.13.desc";
	data.requiredTargetCondition = TargetCondition::Alive;
	data.reward = 40;
	data.rewardStr = "$40";
	data.startPosition = toVector3(-311.059, 1355.05, 158.291);
	data.isTargetMale = true;
	data.targetName = "mission.13.target";

	return new AndrewClarkExecutor(data, areasMgr);
}

ChesterBurnettExecutor* BountyMissionsFactory::chesterBurnettExecutor()
{
	BountyMissionData data;
	data.id = 14;
	data.area = MapAreas::Valentine;
	data.missionName = "ChesterBurnett";
	data.crime = "mission.14.crime";
	data.description = "mission.14.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 65;
	data.rewardStr = "$65";
	data.startPosition = toVector3(-1353.869, 2440.07, 308.4282);
	data.isTargetMale = true;
	data.targetName = "mission.14.target";

	return new ChesterBurnettExecutor(data, areasMgr);
}

CullenBakerExecutor* BountyMissionsFactory::cullenBakerExecutor()
{
	BountyMissionData data;
	data.id = 15;
	data.area = MapAreas::Valentine;
	data.missionName = "CullenBaker";
	data.crime = "mission.15.crime";
	data.description = "mission.15.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 70;
	data.rewardStr = "$70";
	data.startPosition = toVector3(378.7642, -10.15373, 107.0928);
	data.isTargetMale = true;
	data.targetName = "mission.15.target";

	return new CullenBakerExecutor(data, areasMgr);
}

JohnBullExecutor* BountyMissionsFactory::johnBullExecutor()
{
	BountyMissionData data;
	data.id = 16;
	data.area = MapAreas::Valentine;
	data.missionName = "JohnBull";
	data.crime = "mission.16.crime";
	data.description = "mission.16.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 70;
	data.rewardStr = "$70";
	data.startPosition = toVector3(1256.149, 1161.379, 149.3029);
	data.isTargetMale = true;
	data.targetName = "mission.16.target";

	return new JohnBullExecutor(data, areasMgr);
}

MarquisRussellExecutor* BountyMissionsFactory::marquisRussellExecutor()
{
	BountyMissionData data;
	data.id = 17;
	data.area = MapAreas::Valentine;
	data.missionName = "MarquisRussell";
	data.crime = "mission.17.crime";
	data.description = "mission.17.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 75;
	data.rewardStr = "$75";
	data.startPosition = toVector3(-1963.104, 2158.636, 326.5899);
	data.isTargetMale = true;
	data.targetName = "mission.17.target";

	return new MarquisRussellExecutor(data, areasMgr);
}

PeteEatonExecutor* BountyMissionsFactory::peteEatonExecutor()
{
	BountyMissionData data;
	data.id = 18;
	data.area = MapAreas::Valentine;
	data.missionName = "PeteEaton";
	data.crime = "mission.18.crime";
	data.description = "mission.18.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 65;
	data.rewardStr = "$65";
	data.startPosition = toVector3(1079.162, 115.801, 93.26965);
	data.isTargetMale = true;
	data.targetName = "mission.18.target";

	return new PeteEatonExecutor(data, areasMgr);
}

JesseHillExecutor* BountyMissionsFactory::jesseHillExecutor()
{
	BountyMissionData data;
	data.id = 19;
	data.area = MapAreas::Tumbleweed;
	data.missionName = "JesseHill";
	data.crime = "mission.19.crime";
	data.description = "mission.19.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 60;
	data.rewardStr = "$60";
	data.startPosition = toVector3(-5705.996, -2390.512, 4.240256);
	data.isTargetMale = true;
	data.targetName = "mission.19.target";

	return new JesseHillExecutor(data, areasMgr);
}

QuentinChigurhExecutor* BountyMissionsFactory::quentinChigurhExecutor()
{
	BountyMissionData data;
	data.id = 20;
	data.area = MapAreas::Tumbleweed;
	data.missionName = "QuentinChigurh";
	data.crime = "mission.20.crime";
	data.description = "mission.20.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 60;
	data.rewardStr = "$60";
	data.startPosition = toVector3(-6451.273, -3538.335, -25.54377);
	data.isTargetMale = true;
	data.targetName = "mission.20.target";

	return new QuentinChigurhExecutor(data, areasMgr);
}

TucoVillaExecutor* BountyMissionsFactory::tucoVillaExecutor()
{
	BountyMissionData data;
	data.id = 21;
	data.area = MapAreas::Tumbleweed;
	data.missionName = "TucoVilla";
	data.crime = "mission.21.crime";
	data.description = "mission.21.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 65;
	data.rewardStr = "$65";
	data.startPosition = toVector3(-5053.802, -3629.418, -4.846035);
	data.isTargetMale = true;
	data.targetName = "mission.21.target";

	return new TucoVillaExecutor(data, areasMgr);
}

BillyWilsonExecutor* BountyMissionsFactory::billyWilsonExecutor()
{
	BountyMissionData data;
	data.id = 22;
	data.area = MapAreas::Tumbleweed;
	data.missionName = "BillyWilson";
	data.crime = "mission.22.crime";
	data.description = "mission.22.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 65;
	data.rewardStr = "$65";
	data.startPosition = toVector3(-4458.25, -3654.89, 57.7783);
	data.isTargetMale = true;
	data.targetName = "mission.22.target";

	return new BillyWilsonExecutor(data, areasMgr);
}

JoseChavezExecutor* BountyMissionsFactory::joseChavezExecutor()
{
	BountyMissionData data;
	data.id = 23;
	data.area = MapAreas::Tumbleweed;
	data.missionName = "JoseChavez";
	data.crime = "mission.23.crime";
	data.description = "mission.23.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 55;
	data.rewardStr = "$55";
	data.startPosition = toVector3(-4484.039, -2865.338, -12.28567);
	data.isTargetMale = true;
	data.targetName = "mission.23.target";

	return new JoseChavezExecutor(data, areasMgr);
}

PanchoDanielExecutor* BountyMissionsFactory::panchoDanielExecutor()
{
	BountyMissionData data;
	data.id = 24;
	data.area = MapAreas::Tumbleweed;
	data.missionName = "PanchoDaniel";
	data.crime = "mission.24.crime";
	data.description = "mission.24.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 60;
	data.rewardStr = "$60";
	data.startPosition = toVector3(-3836.463, -3014.846, -8.249296);
	data.isTargetMale = true;
	data.targetName = "mission.24.target";

	return new PanchoDanielExecutor(data, areasMgr);
}

ArchStantonExecutor* BountyMissionsFactory::archStantonExecutor()
{
	BountyMissionData data;
	data.id = 25;
	data.area = MapAreas::Armadillo;
	data.missionName = "ArchStanton";
	data.crime = "mission.25.crime";
	data.description = "mission.25.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 30;
	data.rewardStr = "$30";
	data.startPosition = toVector3(-3508.315, -1967.514, 4.799573);
	data.isTargetMale = true;
	data.targetName = "mission.25.target";

	return new ArchStantonExecutor(data, areasMgr);
}

RichardMooreExecutor* BountyMissionsFactory::richardMooreExecutor()
{
	BountyMissionData data;
	data.id = 26;
	data.area = MapAreas::Armadillo;
	data.missionName = "RichardMoore";
	data.crime = "mission.26.crime";
	data.description = "mission.26.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 35;
	data.rewardStr = "$35";
	data.startPosition = toVector3(-4388.753, -2161.34, 49.57555);
	data.isTargetMale = true;
	data.targetName = "mission.26.target";

	return new RichardMooreExecutor(data, areasMgr);
}

JuanCortezExecutor* BountyMissionsFactory::juanCortezExecutor()
{
	BountyMissionData data;
	data.id = 27;
	data.area = MapAreas::Armadillo;
	data.missionName = "JuanCortez";
	data.crime = "mission.27.crime";
	data.description = "mission.27.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 25;
	data.rewardStr = "$25";
	data.startPosition = toVector3(-3484.982, -3468.916, 0.1763178);
	data.isTargetMale = true;
	data.targetName = "mission.27.target";

	return new JuanCortezExecutor(data, areasMgr);
}

ClintonvonHagenExecutor* BountyMissionsFactory::clintonvonHagenExecutor()
{
	BountyMissionData data;
	data.id = 28;
	data.area = MapAreas::SaintDenis;
	data.missionName = "ClintonvonHagen";
	data.crime = "mission.28.crime";
	data.description = "mission.28.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 105;
	data.rewardStr = "$105";
	data.startPosition = toVector3(1874.233, -540.7623, 44.48211);
	data.isTargetMale = true;
	data.targetName = "mission.28.target";

	return new ClintonvonHagenExecutor(data, areasMgr);
}

CarlMartinExecutor* BountyMissionsFactory::carlMartinExecutor()
{
	BountyMissionData data;
	data.id = 29;
	data.area = MapAreas::SaintDenis;
	data.missionName = "CarlMartin";
	data.crime = "mission.29.crime";
	data.description = "mission.29.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 100;
	data.rewardStr = "$100";
	data.startPosition = toVector3(1522.948, -18.84619, 97.08103);
	data.isTargetMale = true;
	data.targetName = "mission.29.target";

	return new CarlMartinExecutor(data, areasMgr);
}

TreasuryPeteExecutor* BountyMissionsFactory::treasuryPeteExecutor()
{
	BountyMissionData data;
	data.id = 30;
	data.area = MapAreas::SaintDenis;
	data.missionName = "TreasuryPete";
	data.crime = "mission.30.crime";
	data.description = "mission.30.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 100;
	data.rewardStr = "$100";
	data.startPosition = toVector3(2884.414, -244.7007, 41.77635);
	data.isTargetMale = true;
	data.targetName = "mission.30.target";

	return new TreasuryPeteExecutor(data, areasMgr);
}

RobertCorbucciExecutor* BountyMissionsFactory::robertCorbucciExecutor()
{
	BountyMissionData data;
	data.id = 31;
	data.area = MapAreas::SaintDenis;
	data.missionName = "RobertCorbucci";
	data.crime = "mission.31.crime";
	data.description = "mission.31.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 105;
	data.rewardStr = "$105";
	data.startPosition = toVector3(1261.292, -419.4841, 95.66013);
	data.isTargetMale = true;
	data.targetName = "mission.31.target";

	return new RobertCorbucciExecutor(data, areasMgr);
}

MikeSandersExecutor* BountyMissionsFactory::mikeSandersExecutor()
{
	BountyMissionData data;
	data.id = 32;
	data.area = MapAreas::SaintDenis;
	data.missionName = "LucienVinrouge";
	data.crime = "mission.32.crime";
	data.description = "mission.32.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 110;
	data.rewardStr = "$110";
	data.startPosition = toVector3(2315.076, -343.0998, 42.65502);
	data.isTargetMale = true;
	data.targetName = "mission.32.target";

	return new MikeSandersExecutor(data, areasMgr);
}

ChrisJaniceExecutor* BountyMissionsFactory::chrisJaniceExecutor()
{
	BountyMissionData data;
	data.id = 33;
	data.area = MapAreas::SaintDenis;
	data.missionName = "ChrisJanice";
	data.crime = "mission.33.crime";
	data.description = "mission.33.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 110;
	data.rewardStr = "$110";
	data.startPosition = toVector3(1888.254, 294.222, 76.73746);
	data.isTargetMale = true;
	data.targetName = "mission.33.target";

	return new ChrisJaniceExecutor(data, areasMgr);
}

JustusBarnesExecutor* BountyMissionsFactory::justusBarnesExecutor()
{
	BountyMissionData data;
	data.id = 34;
	data.area = MapAreas::Strawberry;
	data.missionName = "JustusBarnes";
	data.crime = "mission.34.crime";
	data.description = "mission.34.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 60;
	data.rewardStr = "$60";
	data.startPosition = toVector3(-853.949, -750.213, 59.23435);
	data.isTargetMale = true;
	data.targetName = "mission.34.target";

	return new JustusBarnesExecutor(data, areasMgr);
}

WilliamRogersExecutor* BountyMissionsFactory::williamRogersExecutor()
{
	BountyMissionData data;
	data.id = 35;
	data.area = MapAreas::Strawberry;
	data.missionName = "WilliamRogers";
	data.crime = "mission.35.crime";
	data.description = "mission.35.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 55;
	data.rewardStr = "$55";
	data.startPosition = toVector3(-2480.93, -171.0881, 178.6074);
	data.isTargetMale = true;
	data.targetName = "mission.35.target";

	return new WilliamRogersExecutor(data, areasMgr);
}

BobLaneExecutor* BountyMissionsFactory::bobLaneExecutor()
{
	BountyMissionData data;
	data.id = 36;
	data.area = MapAreas::Strawberry;
	data.missionName = "BobLane";
	data.crime = "mission.36.crime";
	data.description = "mission.36.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 55;
	data.rewardStr = "$55";
	data.startPosition = toVector3(-1873.52, -1112.42, 83.35722);
	data.isTargetMale = true;
	data.targetName = "mission.36.target";

	return new BobLaneExecutor(data, areasMgr);
}

GeorgeCoenExecutor* BountyMissionsFactory::georgeCoenExecutor()
{
	BountyMissionData data;
	data.id = 37;
	data.area = MapAreas::Strawberry;
	data.missionName = "GeorgeCoen";
	data.crime = "mission.37.crime";
	data.description = "mission.37.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 65;
	data.rewardStr = "$65";
	data.startPosition = toVector3(-1578.95, 253.227, 111.212);
	data.isTargetMale = true;
	data.targetName = "mission.37.target";

	return new GeorgeCoenExecutor(data, areasMgr);
}

BuckHicoxExecutor* BountyMissionsFactory::buckHicoxExecutor()
{
	BountyMissionData data;
	data.id = 38;
	data.area = MapAreas::Strawberry;
	data.missionName = "BuckHicox";
	data.crime = "mission.38.crime";
	data.description = "mission.38.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 60;
	data.rewardStr = "$60";
	data.startPosition = toVector3(-2125.775, 361.3478, 136.1135);
	data.isTargetMale = true;
	data.targetName = "mission.38.target";

	return new BuckHicoxExecutor(data, areasMgr);
}

JohnEastwoodExecutor* BountyMissionsFactory::johnEastwoodExecutor()
{
	BountyMissionData data;
	data.id = 39;
	data.area = MapAreas::Strawberry;
	data.missionName = "JohnEastwood";
	data.crime = "mission.39.crime";
	data.description = "mission.39.desc";
	data.requiredTargetCondition = TargetCondition::DeadOrAlive;
	data.reward = 55;
	data.rewardStr = "$55";
	data.startPosition = toVector3(-1515.187, 717.3212, 125.5926);
	data.isTargetMale = true;
	data.targetName = "mission.39.target";

	return new JohnEastwoodExecutor(data, areasMgr);
}
