#pragma execution_character_set("utf-8")
#include <cmath>
#include "Main.h"

#include "ModConfig.h"

namespace {

// ---------------------------------------------------------------------------
// Portrait casting.
//
// Faces are grouped by what the wanted notice says about the criminal and by
// the kind of person they are, not by which town hands the notice out. The ini
// owns the groups, so new artwork only needs a new number in the list.
// ---------------------------------------------------------------------------
int parsePortraitList(const char* list, int* out, int maxOut)
{
	if (list == NULL || out == NULL)
	{
		return 0;
	}

	int count = 0;
	const char* p = list;
	while (*p != '\0' && count < maxOut)
	{
		while (*p == ' ' || *p == '\t' || *p == ',')
		{
			p++;
		}
		if (*p == '\0')
		{
			break;
		}

		// Accept "portrait_7", "portrait7" or a bare number and keep the digits
		// that follow the last letter/underscore.
		int value = 0;
		bool digits = false;
		while (*p != '\0' && *p != ',')
		{
			if (*p >= '0' && *p <= '9')
			{
				value = value * 10 + (*p - '0');
				digits = true;
			}
			else if (*p == ' ')
			{
				break;
			}
			p++;
		}
		if (digits && value > 0)
		{
			out[count++] = value;
		}
		while (*p != '\0' && *p != ',')
		{
			p++;
		}
	}
	return count;
}

int pickFromPortraitGroup(const char* group, int missionId)
{
	if (group == NULL || group[0] == '\0')
	{
		return 0;
	}

	char raw[512];
	raw[0] = '\0';
	modIniString("Portraits", group, "", raw, sizeof(raw));

	int list[32];
	int n = parsePortraitList(raw, list, 32);
	if (n <= 0)
	{
		return 0;
	}

	// Stable choice: the same criminal always gets the same face, and two
	// bounties in the same group do not all land on the first entry.
	int pick = (int)(((unsigned int)missionId * 2654435761u) >> 13) % n;
	return list[pick];
}

// The ped model each mission spawns, taken from its executor. The name itself
// encodes sex and age (G_M_M_ = male middle-aged, G_M_O_ = male old,
// A_F_ = female) plus the crowd it belongs to, which is the most reliable
// casting hint available - better than guessing from the crime text.
// Keep in sync with <Mission>Executor::spawnTarget() if a model changes.
const char* const kTargetModel[40] =
{
	"",
	"g_m_m_unimountainmen_01",   // 1
	"G_M_M_UniBanditos_01",   // 2
	"g_m_m_unimountainmen_01",   // 3
	"g_m_m_unimountainmen_01",   // 4
	"g_m_m_unimountainmen_01",   // 5
	"g_m_m_unimountainmen_01",   // 6
	"g_m_o_uniexconfeds_01",   // 7
	"A_F_M_LowerSDTownfolk_01",   // 8
	"G_M_O_UniExConfeds_01",   // 9
	"G_M_O_UniExConfeds_01",   // 10
	"G_M_O_UniExConfeds_01",   // 11
	"G_M_O_UniExConfeds_01",   // 12
	"A_M_M_GriFancyTravellers_01",   // 13
	"G_M_M_UNIMICAHGOONS_01",   // 14
	"G_M_M_UniDuster_01",   // 15
	"G_M_M_UniDuster_02",   // 16
	"G_M_M_UNIMICAHGOONS_01",   // 17
	"G_M_M_UniDuster_02",   // 18
	"G_M_M_UniCriminals_02",   // 19
	"G_M_M_UniCriminals_02",   // 20
	"G_M_M_UniBanditos_01",   // 21
	"G_M_M_UniCriminals_02",   // 22
	"G_M_M_UniBanditos_01",   // 23
	"G_M_M_UniBanditos_01",   // 24
	"G_M_M_UniCriminals_02",   // 25
	"G_M_M_UniCriminals_02",   // 26
	"G_M_M_UniBanditos_01",   // 27
	"A_M_M_SDFANCYTRAVELLERS_01",   // 28
	"G_M_M_UniCriminals_01",   // 29
	"A_M_O_SDUPPERCLASS_01",   // 30
	"G_M_M_UniBronteGoons_01",   // 31
	"A_M_M_SDFANCYTRAVELLERS_01",   // 32
	"G_M_M_UniCriminals_01",   // 33
	"G_M_M_UniRanchers_01",   // 34
	"G_M_M_UniRanchers_01",   // 35
	"G_M_M_UniRanchers_01",   // 36
	"G_M_M_UniRanchers_01",   // 37
	"G_M_M_UniRanchers_01",   // 38
	"G_M_M_UniRanchers_01",   // 39
};

int pickFromModelList(const char* model, int missionId)
{
	if (model == NULL || model[0] == '\0')
	{
		return 0;
	}

	// Ini keys are case insensitive; normalise anyway so the file reads the same
	// way the model does in OpenIV (lower case for ped models).
	char key[64];
	int n = 0;
	for (; model[n] != '\0' && n < (int)sizeof(key) - 1; n++)
	{
		char c = model[n];
		key[n] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
	}
	key[n] = '\0';

	char raw[512];
	raw[0] = '\0';
	modIniString("PortraitModel", key, "", raw, sizeof(raw));

	int list[32];
	int count = parsePortraitList(raw, list, 32);
	if (count <= 0)
	{
		return 0;
	}

	// Round robin inside the model: count how many earlier missions spawn the
	// same ped and take that as the index. Hashing looked random but often sent
	// four of five bounties to the same face; this always alternates and is
	// still deterministic, so a save reload never reshuffles the portraits.
	int nth = 0;
	for (int id = 1; id < missionId && id < 40; id++)
	{
		if (_stricmp(kTargetModel[id], model) == 0)
		{
			nth++;
		}
	}
	return list[nth % count];
}

const char* groupFromKeywords(const char* crime, const char* description)
{
	struct Rule
	{
		const char* key;
		const char* group;
	};
	static const Rule rules[] =
	{
		// Order matters: the more specific reading wins.
		{ "弃尸", "violent_white_50" },
		{ "军火", "bandit_30" },
		{ "盗枪", "bandit_30" },
		{ "抢劫", "bandit_30" },
		{ "劫掠", "bandit_30" },
		{ "驿站", "bandit_30" },
		{ "盗马", "bandit_30" },
		{ "欺诈", "white_collar_30" },
		{ "盗窃", "white_collar_30" },
		{ "盗用", "white_collar_30" },
		{ "谋杀", "violent_white_30" },
		{ "杀人", "violent_white_30" },
		{ "行凶", "violent_white_30" },
	};

	const int count = (int)(sizeof(rules) / sizeof(rules[0]));
	if (crime != NULL)
	{
		for (int i = 0; i < count; i++)
		{
			if (strstr(crime, rules[i].key) != NULL)
			{
				return rules[i].group;
			}
		}
	}
	if (description != NULL)
	{
		for (int i = 0; i < count; i++)
		{
			if (strstr(description, rules[i].key) != NULL)
			{
				return rules[i].group;
			}
		}
	}
	return "default";
}

const char* groupFromModel(const char* model)
{
	if (model == NULL || model[0] == '\0')
	{
		return NULL;
	}

	char name[64];
	int n = 0;
	for (; model[n] != '\0' && n < (int)sizeof(name) - 1; n++)
	{
		char c = model[n];
		name[n] = (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
	}
	name[n] = '\0';

	// "<prefix>_<sex>_<age>_<crowd>_<variant>"
	const bool female = (strstr(name, "_f_") != NULL);
	const bool old = (strstr(name, "_o_") != NULL);

	if (female)
	{
		return old ? "female_50" : "female_30";
	}
	if (strstr(name, "banditos") != NULL)
	{
		return "latino_30";              // the game's Mexican outlaws
	}
	if (strstr(name, "townfolk") != NULL)
	{
		return old ? "black_50" : "black_30";
	}
	if (strstr(name, "fancytravellers") != NULL || strstr(name, "upperclass") != NULL)
	{
		return "white_collar_30";        // travellers and city money
	}
	if (strstr(name, "criminals") != NULL)
	{
		return old ? "violent_white_50" : "violent_white_30";
	}
	if (strstr(name, "exconfeds") != NULL || strstr(name, "mountainmen") != NULL ||
	    strstr(name, "ranchers") != NULL || strstr(name, "duster") != NULL ||
	    strstr(name, "micahgoons") != NULL || strstr(name, "brontegoons") != NULL)
	{
		return old ? "violent_white_50" : "bandit_30";
	}
	return NULL;
}

int resolvePortraitIndex(const BountyMissionData* data)
{
	if (data == NULL)
	{
		return 1;
	}

	char key[16];
	snprintf(key, sizeof(key), "%d", data->id);

	// 1. A direct texture number always wins, for one-off corrections.
	int direct = modIniInt("PortraitMap", key, 0);
	if (direct > 0)
	{
		return direct;
	}

	// 2. A face pinned to the ped model itself. This is the precise route: the
	//    model name is exactly what OpenIV shows, and every mission that spawns
	//    that model inherits the face. A list is allowed - a model toting five
	//    bounties then yields several different men.
	if (data->id > 0 && data->id < 40)
	{
		int fromModel = pickFromModelList(kTargetModel[data->id], data->id);
		if (fromModel > 0)
		{
			return fromModel;
		}
	}

	// 3. An explicit group for this mission.
	char group[64];
	group[0] = '\0';
	modIniString("PortraitClass", key, "", group, sizeof(group));

	// 4. Otherwise cast by the model's crowd, then by the wording on the notice.
	//    A woman never gets a beard, so sex is checked before the crime keywords.
	if (group[0] == '\0')
	{
		// The model is the strongest hint: it is the man the player actually
		// meets, and its name carries sex, age and crowd. Crime wording is only
		// the fallback for the rare id without an entry.
		const char* cast = NULL;
		if (data->id > 0 && data->id < 40)
		{
			cast = groupFromModel(kTargetModel[data->id]);
		}
		if (cast == NULL)
		{
			cast = data->isTargetMale ? groupFromKeywords(data->crime, data->description) : "female_30";
		}
		snprintf(group, sizeof(group), "%s", cast);
	}

	int idx = pickFromPortraitGroup(group, data->id);
	if (idx > 0)
	{
		return idx;
	}

	// 5. Nothing configured for that group: fall back to the mixed pool, then to
	//    the old modulo so a portrait always resolves to something.
	idx = pickFromPortraitGroup("default", data->id);
	if (idx > 0)
	{
		return idx;
	}
	return ((data->id - 1) % 8) + 1;
}

} // namespace



namespace
{
	// Crew size per mission id. A firearms theft, a train job or a bank job is
	// a gang outfit, so the target travels with several men. A lone murderer
	// or a fraudster usually has none, or one lookout. Index 0 is unused.
	const unsigned char kGuardMin[40] = {
		0,
		0, 1, 2, 1, 1, 1, 3, 1, 3, 3,
		3, 3, 0, 0, 2, 2, 2, 2, 0, 2,
		1, 2, 2, 2, 2, 1, 0, 0, 0, 3,
		0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	const unsigned char kGuardMax[40] = {
		0,
		1, 2, 3, 2, 2, 2, 5, 2, 5, 5,
		5, 5, 0, 1, 4, 4, 4, 4, 1, 4,
		2, 4, 4, 4, 3, 3, 1, 1, 1, 5,
		1, 1, 1, 1, 1, 1, 1, 1, 1
	};

	void guardRangeForMission(int id, int* outMin, int* outMax)
	{
		if (id < 0 || id >= 40)
		{
			*outMin = 0;
			*outMax = 1;
			return;
		}
		*outMin = kGuardMin[id];
		*outMax = kGuardMax[id];
		if (*outMax < *outMin)
		{
			*outMax = *outMin;
		}
	}
}

namespace
{
	// Review hotkey: a Windows virtual-key code, F7 by default, overridable
	// through [UI] ReviewKey in the ini. Cached because it is polled every
	// frame and GetPrivateProfileIntA is a file read.
	int reviewKeyCode()
	{
		static int s_key = 0;
		if (s_key == 0)
		{
			s_key = modIniInt("UI", "ReviewKey", 0x76); // VK_F7
			if (s_key < 1 || s_key > 254)
			{
				s_key = 0x76;
			}
		}
		return s_key;
	}

	// RewardScale is polled from the poster panel every frame, so it is
	// re-read at most once every two seconds. Editing the ini still takes
	// effect without restarting the game.
	int rewardScaleCached()
	{
		static int s_scale = 100;
		static DWORD s_last = 0;

		DWORD now = GetTickCount();
		if (s_last == 0 || (now - s_last) > 2000)
		{
			s_last = now;
			s_scale = modIniInt("Reward", "RewardScale", 100);
			if (s_scale < 0)    { s_scale = 0; }
			if (s_scale > 1000) { s_scale = 1000; }
		}
		return s_scale;
	}
}

const char BOUNTY_COMPLETED[] = "任务完成!";

BaseMissionExecutor::BaseMissionExecutor(BountyMissionData missionData, MapAreasManager* areasMgr)
{
	this->areasMgr = areasMgr;
	this->missionData = new BountyMissionData(missionData);

	// �ı����ã�factory ����� lang key����������ɵ�ǰ���ԣ��Ҳ�������ԭ��
	this->missionData->targetName = TF(missionData.targetName, missionData.targetName);
	this->missionData->crime = TF(missionData.crime, missionData.crime);
	this->missionData->description = TF(missionData.description, missionData.description);
	this->stage = BountyMissionStage::MissionInitialization;
	this->status = BountyMissionStatus::Unavailable;
	this->posterPanelOpen = false;
	this->posterPanelPage = 0;
	this->posterIsNativeUI = false;
	this->posterRetries = 0;
	this->posterRetryTimer = 0;
	this->targetWasKilled = false;
	this->deliveredReward = 0;
	this->reviewArmed = false;
	this->posterPanelFromBoard = false;
	this->busyHintShown = false;

	inspectPosterPrompt = new Prompt(TF("ui.prompt.inspect", "�鿴ͨ����"), GAMEPLAY::GET_HASH_KEY("INPUT_MERCY_KILL"), Standard);
	inspectPosterPrompt->hide();
	acceptPosterPrompt = new Prompt(TF("ui.prompt.accept", "��������"), GAMEPLAY::GET_HASH_KEY("INPUT_FRONTEND_ACCEPT"), Standard);
	acceptPosterPrompt->hide();
	declinePosterPrompt = new Prompt(TF("ui.prompt.decline", "�뿪"), GAMEPLAY::GET_HASH_KEY("INPUT_FRONTEND_CANCEL"), Standard);
	declinePosterPrompt->hide();
	posterPanelOpen = false;
	posterPanelPage = 0;
	setTargetAreaRadius(AREA_RADIUS);
	setRequiredDistanceToLocateTarget(REQUIRED_DIST_TO_LOCATE);
	setMustBeCloseToLocate(false);

	poster = NULL;
	posterBlip = NULL;
	targetAreaBlip = NULL;
	target = NULL;
	policeLocBlip = NULL;
	nameRefreshCooldown = 0;
	spawnchanceComputed = false;
	posterRivalsPlanned = false;
	posterRivalsDone = false;
	posterRivalsRolled = false;
	custodyRivalsDone = false;
}

BountyMissionData* BaseMissionExecutor::getMissionData()
{
	return missionData;
}

BountyMissionStage BaseMissionExecutor::getMissionStage()
{
	return stage;
}

BountyMissionStatus BaseMissionExecutor::getMissionStatus()
{
	return status;
}

void BaseMissionExecutor::setMissionStatus(BountyMissionStatus status)
{
	this->status = status;

	switch (status)
	{
	case BountyMissionStatus::Pending:
		stage = BountyMissionStage::MissionInitialization;
		break;

	case BountyMissionStatus::CollectedPoster:
		stage = BountyMissionStage::GoToArea;
		onPosterCollected();
		break;

	case BountyMissionStatus::Completed:
		stage = BountyMissionStage::Finished;
		onFinished(false);
		break;
	}
}

void BaseMissionExecutor::update()
{
	if (GAMEPLAY::GET_MISSION_FLAG())
	{
		if (RADAR::DOES_BLIP_EXIST(posterBlip) || RADAR::DOES_BLIP_EXIST(targetAreaBlip) || stage == BountyMissionStage::CaptureTarget)
		{
			if (stage == BountyMissionStage::CaptureTarget)
			{
				cleanup();
				stage = BountyMissionStage::GoToArea;
			}
			deleteBlipSafe(&posterBlip);
			deleteBlipSafe(&targetAreaBlip);
			stopped = true;
		}
	}
	else
	{
		// The mission flag can flicker; every flicker rebuilt all the blips, and
		// the churn exhausted the blip pool after a few minutes of play. Rebuild
		// at most once every five seconds.
		if (stopped == true && GAMEPLAY::GET_GAME_TIMER() - stoppedFlagAt > 5000)
		{
			if (stage == BountyMissionStage::CollectPoster)
			{
				if (!RADAR::DOES_BLIP_EXIST(posterBlip))
				{
					posterBlip = createBlip(poster, 0xB04092F8, -1636832113);
					setBlipLabel(posterBlip, TF("ui.blip.poster", "通缉令"));
				}
			}
			else if (stage == BountyMissionStage::GoToArea)
			{
				// The mission flag flickers on cutscenes, menus and random events.
				// Every flicker used to re-roll the roadside ambush and clear its
				// "already spawned" guard, so a fresh wave of riders was created
				// again and again while the player rode to the area - and nothing
				// ever released them. That pile-up is what froze the game after a
				// couple of bounties. The roll now happens once, on accept.
				// Replace, never stack: this runs again whenever the flag flickers.
				deleteBlipSafe(&targetAreaBlip);
				targetAreaBlip = createBlip(missionData->startPosition, targetAreaRadius, 0xB04092F8, -907204276 /* Bounty sprite */);
				const char* condition = missionData->requiredTargetCondition == TargetCondition::Alive
					? TF("ui.cond.alive", "活捉") : TF("ui.cond.deadoralive", "死活不论");
				std::stringstream label;
				label << TF("ui.blip.hunting", "正在追捕") << ": "
					<< missionData->targetName << ", "
					<< scaledReward() << "$, "
					<< condition;
				setBlipLabel(targetAreaBlip, label.str().c_str());
			}

			stopped = false;
			stoppedFlagAt = GAMEPLAY::GET_GAME_TIMER();
		}
		Ped player = PLAYER::PLAYER_PED_ID();
		Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(player, true, true);
		float distanceToPolice = distanceBetween(ENTITY::GET_ENTITY_COORDS(player, 1, 1), *(getArea()->policeDeptCoords));
		// target is 0 until the target spawns, and asking an invalid entity for
		// its coordinates can park the script thread for good on this build -
		// the picture freezes while the audio keeps going.
		Vector3 targetPos;
		if (target != 0 && ENTITY::DOES_ENTITY_EXIST(target))
		{
			targetPos = ENTITY::GET_ENTITY_COORDS(target, true, false);
		}
		releaseUnnecessaryEntities();

		//if (IsKeyJustUp(VK_KEY_N))
		//{
		//	nextStage();
		//	if (stage == BountyMissionStage::CaptureTarget)
		//	{
		//		if (missionData->requiredTargetCondition == TargetCondition::Alive)
		//		{
		//			//ENTITY::SET_ENTITY_COORDS(target, getArea()->cellCoords->x, getArea()->cellCoords->y, getArea()->cellCoords->z, 0, 0, 0, 0);
		//			ENTITY::SET_ENTITY_COORDS(target, playerPos.x, playerPos.y, playerPos.z, 0, 0, 0, 0);
		//		}
		//		else
		//		{
		//			ENTITY::SET_ENTITY_HEALTH(target, 0, 0);
		//		}
		//	}
		//}

		enforceRivalsFire();

		switch (stage)
		{
		case BountyMissionStage::MissionInitialization:
			initialize();
			nextStage();
			break;

		case BountyMissionStage::CollectPoster:
		{
			// The paper is spawned as the vanilla prop when the mission is
			// created, because the player is somewhere else on the map and the
			// board UI must not be touched from there. Once he actually walks up
			// to the board the native paper gets a few tries, at a slow rate.
			if (!posterIsNativeUI && !posterPanelOpen && BountyBoardUI::configEnabled())
			{
				if (posterRetryTimer > 0)
				{
					posterRetryTimer--;
				}
				else if (posterRetries < 3 &&
				         distanceBetween(playerPos, *getArea()->bountyPostersCoords) <= BountyBoardUI::configDistance())
				{
					posterRetries++;
					posterRetryTimer = 300;
					log("poster: trying native paper at the board");

					if (poster && ENTITY::DOES_ENTITY_EXIST(poster))
					{
						ENTITY::DELETE_ENTITY(&poster);
					}
					poster = NULL;
					spawnPosterProp();
				}
			}

			bool posterOnBoard = (poster != NULL && ENTITY::DOES_ENTITY_EXIST(poster));

			// Only keep the paper refreshed when it can actually be seen. With
			// five idle boards this ran two mask natives plus a databinding pass
			// per board on every single frame, from anywhere on the map - pure
			// churn that adds up over a long session.
			float distToBoard = distanceBetween(playerPos, *getArea()->bountyPostersCoords);
			bool boardInSight = (distToBoard <= 60.0f);

			if (posterOnBoard && boardInSight)
			{
				BountyBoardUI::tick(poster);
				fillPosterBoard();

				// The sheet keeps the vanilla print. Screen-space glyphs cannot follow
				// the paper, so the overlay stays off by default; the localised case
				// file opens when the poster is taken down.
				if (!posterIsNativeUI && modIniInt("Poster", "FaceOnPaper", 0) != 0)
				{
					drawPosterOverlay();
				}

				// The poster is nailed to the board; walking up and taking it
				// down opens the localised case file.
				if (distToBoard <= 3.0f && PED::IS_PED_ON_FOOT(player))
				{
					// One case file at a time: the sheet can still be read, but a
					// second poster is not handed out while one is carried.
					if (isBountyBusy(missionData->id))
					{
						inspectPosterPrompt->hide();
						if (!busyHintShown)
						{
							busyHintShown = true;
							setHelpMessage(TF("ui.hint.busy", "�������Ѿ���һ��ͨ����Ȱ����˽ᡣ"));
						}
						break;
					}

					busyHintShown = false;
					inspectPosterPrompt->setText(TF("ui.prompt.take", "ȡ��ͨ����"));
					inspectPosterPrompt->show();

					if (inspectPosterPrompt->isActivatedByPlayer())
					{
						inspectPosterPrompt->hide();
						takePoster();
					}
				}
				else
				{
					busyHintShown = false;
					inspectPosterPrompt->hide();
				}
				break;
			}

			// The poster left the board: accepting and putting it back are both
			// still possible, nothing is committed yet.
			if (distanceBetween(playerPos, *getArea()->bountyPostersCoords) > 8.0f)
			{
				// Throttled: a failed respawn must not retry every frame, or the
				// board spends the whole session waiting on a model that is not
				// there.
				int nowResp = GAMEPLAY::GET_GAME_TIMER();
				if (nowResp - posterRespawnAt > 5000)
				{
					posterRespawnAt = nowResp;
					putPosterBack();
				}
				break;
			}

			if (posterPanelOpen)
			{
				// Only reached when the native board UI is not available, so the
				// case file still has to be drawn by the script.
				drawPosterPanel();
				inspectPosterPrompt->setText(posterPanelPage == 0
					? TF("ui.prompt.read", "�Ķ�����")
					: TF("ui.prompt.back", "����ժҪ"));
				inspectPosterPrompt->show();

				if (inspectPosterPrompt->isActivatedByPlayer())
				{
					posterPanelPage = (posterPanelPage == 0) ? 1 : 0;
				}
			}
			else
			{
				inspectPosterPrompt->hide();
			}
			acceptPosterPrompt->setText(TF("ui.prompt.accept", "��������"));
			acceptPosterPrompt->show();
			declinePosterPrompt->setText(TF("ui.prompt.putback", "�Żغ���"));
			declinePosterPrompt->show();

			if (acceptPosterPrompt->isActivatedByPlayer())
			{
				// Safety net: taking the paper normally claimed the slot already.
				if (!claimBounty(missionData->id))
				{
					setHelpMessage(TF("ui.hint.busy", "�������Ѿ���һ��ͨ����Ȱ����˽ᡣ"));
					putPosterBack();
					break;
				}

				acceptPosterPrompt->hide();
				declinePosterPrompt->hide();
				posterPanelOpen = false;
				posterPanelPage = 0;
				nextStage();
			}
			else if (declinePosterPrompt->isActivatedByPlayer())
			{
				putPosterBack();
			}
			break;
		}

		case BountyMissionStage::GoToArea:


			// ��ͨ����뿪�����һ�ξ������������ʱ����������
			if (posterRivalsPlanned && !posterRivalsDone &&
				distanceBetween(playerPos, *getArea()->policeDeptCoords) > 120.0f)
			{
				posterRivalsDone = true;
				spawnPosterRivals();
			}
			if (distanceBetween(playerPos, missionData->startPosition) <= targetAreaRadius)
			{
				nextStage();
			}
			break;

		case BountyMissionStage::LocateTarget:
			// Without a live target the sight check below would query an invalid
			// handle, which is what froze the game as the player closed in.
			if (target == 0 || !ENTITY::DOES_ENTITY_EXIST(target))
			{
				break;
			}
			if (!spawnchanceComputed)
			{
				// 25% ����Ѻ��;�б�����ͽ����˽��ˣ�ԭд�� rand() >= 0.75 ���棩
				// Roll once per mission. The percentage is an ini value so the
				// escort ambush can be forced to 100 for testing without a
				// rebuild, or switched to 0 to rule it out entirely.
				int chance = modIniInt("Poster", "CustodyChance", 25);
				if (chance < 0)
				{
					chance = 0;
				}
				if (chance > 100)
				{
					chance = 100;
				}
				int roll = rand() % 100;
				spawnchance = roll < chance;
				spawnchanceComputed = true;

				std::stringstream sc;
				sc << "custody rivals: chance=" << chance << " roll=" << roll
				   << " planned=" << (spawnchance ? 1 : 0);
				log(sc.str());
			}
			if (mustBeCloseToLocate)
			{
				if (distanceBetweenEntities(player, target) <= requiredDistanceToLocate)
				{
					nextStage();
				}
			}
			else if (distanceBetweenEntities(player, target) <= 40.0f)
			{
				// Plain distance instead of a line-of-sight raycast. The LOS call
				// needs the target's collision to be streamed in, and firing it
				// every frame while the player closes in is what parked the script.
				nextStage();
			}
			if (distanceBetween(playerPos, missionData->startPosition) > targetAreaRadius)
			{
				cleanup();
				stage = BountyMissionStage::GoToArea;
			}
			break;

		case BountyMissionStage::CaptureTarget:
			{
				static int beatCap = 0;
				int nowCap = GAMEPLAY::GET_GAME_TIMER();
				if (nowCap - beatCap > 5000)
				{
					beatCap = nowCap;
					log("stage: CaptureTarget alive");
				}
			}
			// The corpse can be handed in for "dead or alive" bounties. For a
			// live-only bounty this only opens up when the ini allows a corpse
			// payout, which is what targetWasKilled records.
			if (isPedHogtied(target) ||
				(ENTITY::IS_ENTITY_DEAD(target) &&
				 (missionData->requiredTargetCondition == TargetCondition::DeadOrAlive || targetWasKilled)))
			{
				nextStage();
			}
			// Distance instead of a two-way line-of-sight raycast. The LOS call
			// needs the target's collision streamed in and runs every frame while
			// the player carries the body back - that is what froze the game on
			// the way to the station.
			if (seen == false && distanceBetweenEntities(player, target) <= 40.0f)
			{
				deleteBlipSafe(&targetBlip);
				targetBlip = createBlip(target, BLIP_TYPE_BOUNTY_TARGET, BLIP_SPRITE_BOUNTY_TARGET);
				seen = true;
			}
			break;

		case BountyMissionStage::ArriveToPoliceStation:
			{
				static int beatArr = 0;
				int nowArr = GAMEPLAY::GET_GAME_TIMER();
				if (nowArr - beatArr > 2000)
				{
					beatArr = nowArr;
					log("stage: ArriveToPoliceStation alive");
				}
			}
			if (target != 0 && ENTITY::DOES_ENTITY_EXIST(target) &&
			    ENTITY::IS_ENTITY_DEAD(target) && !RADAR::DOES_BLIP_EXIST(targetBlip))
			{
				deleteBlipSafe(&targetBlip);
				targetBlip = createBlip(target, BLIP_TYPE_BOUNTY_TARGET, BLIP_SPRITE_BOUNTY_TARGET);
			}
			if (distanceBetween(playerPos, missionData->startPosition) > 250 && spawnchance && !custodyRivalsDone && missionData->area != MapAreas::Armadillo)
			{
				custodyRivalsDone = true;
				log("custody rivals: spawning hunters");
				spawnBountyHunters();
			}
			if (distanceBetween(targetPos, *getArea()->policeDeptCoords) < 20)
			{
				nextStage();
			}
			break;

		case BountyMissionStage::HandOverTarget:
		{
			// A corpse cannot be locked in a cell: dead-or-alive bounties are
			// dropped in the station's backyard, live prisoners still go behind
			// the cell door. The backyard coordinates were not part of the data
			// set until they were measured per station.
			bool corpse = targetWasKilled ||
				(target != 0 && ENTITY::DOES_ENTITY_EXIST(target) && ENTITY::IS_ENTITY_DEAD(target));
			Vector3* dropAt = corpse ? getArea()->corpseDropCoords : getArea()->cellCoords;

			float bodyHeight = ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(target);
			bool playerMounted = PED::IS_PED_ON_MOUNT(PLAYER::PLAYER_PED_ID()) ? true : false;

			// A body is not handed over from the saddle: it has to be unloaded
			// and the player has to stand on his own feet in the backyard.
			bool readyToHandOver;
			if (corpse)
			{
				readyToHandOver = !playerMounted &&
					distanceBetween(targetPos, *dropAt) < 4.0f &&
					bodyHeight <= 1.1f;
			}
			else
			{
				// A live prisoner has to be carried inside the cell: standing at
				// the door on horseback does not count, and the cell trigger is
				// only a metre or two wide.
				readyToHandOver = !playerMounted &&
					distanceBetween(targetPos, *dropAt) < 2.0f &&
					bodyHeight <= 1.1f;
			}

			if (readyToHandOver)
			{
				nextStage();
			}
			else if (distanceBetween(targetPos, *dropAt) < (corpse ? 4.0f : 3.0f))
			{
				DWORD hintNow = GAMEPLAY::GET_GAME_TIMER();
				if (hintNow - dropHintAt > 6000)
				{
					dropHintAt = hintNow;
					if (corpse)
					{
						showSubtitle(TF("ui.sub.drop_body", "把尸体从马上卸下，下马后再交给警局。"));
					}
					else
					{
						showSubtitle(TF("ui.sub.cell_body", "把人扛进牢房，放下后再离开。"));
					}
				}
			}
			else if (distanceBetween(targetPos, *getArea()->policeDeptCoords) >= 20)
			{
				OBJECT::DELETE_OBJECT(&dummyProp);
				deleteBlipSafe(&cellBlip);
				onTargetCaptured();
				stage = BountyMissionStage::ArriveToPoliceStation;
			}
			break;
		}

		case BountyMissionStage::CollectReward:
			if (OBJECT::HAS_PICKUP_BEEN_COLLECTED(money))
			{
				nextStage();
			}
			break;

		}

		updatePosterReview(player, playerPos);
		drawTargetMarker();

		if (stage >= BountyMissionStage::LocateTarget && stage <= BountyMissionStage::HandOverTarget)
		{
			// �۽���ʾ�ؽ�����ϵ������ʱ����� prompt �����û� "Stranger" / ��������
			// ���԰�Լ 1 ��Ľ����ظ�д��ͨ�����ϵ����֡�
			if (nameRefreshCooldown <= 0)
			{
				applyTargetName();
				nameRefreshCooldown = 60;
			}
			else
			{
				nameRefreshCooldown--;
			}
		}

		if (stage >= BountyMissionStage::LocateTarget && stage <= BountyMissionStage::CollectReward)
		{
			if (missionData->requiredTargetCondition == TargetCondition::Alive &&
				ENTITY::IS_ENTITY_DEAD(target) && !targetWasKilled)
			{
				// Live-only bounty and the target is dead. Default is a plain
				// failure, exactly like the vanilla rules. Setting
				// RequiredAliveKilledPercent in the ini to 1-100 turns it into a
				// discounted payout instead, and then the body has to be hauled
				// to the station like any other corpse.
				int discount = modIniInt("Reward", "RequiredAliveKilledPercent", 0);
				if (discount <= 0)
				{
					fail(TF("ui.fail.must_alive", "����ʧ�ܡ�Ŀ������׽��"));
				}
				else
				{
					targetWasKilled = true;
					{
						// "Take the body to the <town> station backyard, then collect."
						std::string areaKey = std::string("ui.area.") + getArea()->name;
						const char* areaName = TF(areaKey.c_str(), getArea()->name);
						const char* bodyTpl = TF("ui.sub.killed_alive",
							"把%s的尸体送到%s警局后院，再回来领取赏金。");
						char bodyMsg[512];
						snprintf(bodyMsg, sizeof(bodyMsg), bodyTpl,
							missionData ? missionData->targetName : TF("ui.poster.unknown", "目标"),
							areaName);
						showSubtitle(bodyMsg);
					}
				}
			}
			else if (ENTITY::IS_ENTITY_DEAD(player) || !ENTITY::DOES_ENTITY_EXIST(target))
			{
				fail();
			}
		}

		if (stage > BountyMissionStage::CaptureTarget && stage < BountyMissionStage::CollectReward)
		{
			if (!ENTITY::IS_ENTITY_DEAD(target) && !isPedHogtied(target))
			{
				deleteBlipSafe(&policeLocBlip);
				deleteBlipSafe(&cellBlip);

				stage = BountyMissionStage::CaptureTarget;

				std::stringstream text;
				text << "抓获 ~COLOR_RED~" << missionData->targetName;
				showSubtitle(text.str().c_str());
			}
		}
	}
}

MapArea* BaseMissionExecutor::getArea()
{
	return areasMgr->getMapArea(missionData->area);
}

Ped BaseMissionExecutor::getTarget()
{
	return target;
}

void BaseMissionExecutor::setTargetAreaRadius(int radius)
{
	this->targetAreaRadius = radius;
}

void BaseMissionExecutor::setRequiredDistanceToLocateTarget(int distance)
{
	requiredDistanceToLocate = distance;
}

void BaseMissionExecutor::setMustBeCloseToLocate(bool toggle)
{
	mustBeCloseToLocate = toggle;
}

void BaseMissionExecutor::nextStage()
{
	switch (stage)
	{
	case BountyMissionStage::MissionInitialization:
		stage = BountyMissionStage::CollectPoster;
		break;

	case BountyMissionStage::CollectPoster:
		onPosterCollected();
		stage = BountyMissionStage::GoToArea;
		break;

	case BountyMissionStage::GoToArea:
		onArrivalToArea();
		stage = BountyMissionStage::LocateTarget;
		break;

	case BountyMissionStage::LocateTarget:
		onTargetLocated();
		stage = BountyMissionStage::CaptureTarget;
		break;

	case BountyMissionStage::CaptureTarget:
		onTargetCaptured();
		stage = BountyMissionStage::ArriveToPoliceStation;
		break;

	case BountyMissionStage::ArriveToPoliceStation:
		onArrivalToPoliceStation();
		stage = BountyMissionStage::HandOverTarget;
		break;

	case BountyMissionStage::HandOverTarget:
		onTargetHandedOver();
		stage = BountyMissionStage::CollectReward;
		break;

	case BountyMissionStage::CollectReward:
		onRewardCollected();
		onFinished(true);
		stage = BountyMissionStage::Finished;
		break;
	}
}

void BaseMissionExecutor::fail(const char* reason)
{
	status = BountyMissionStatus::Failed;
	releaseBounty(missionData->id);
	seen = false;

	if (reason)
	{
		showSubtitle(reason);
	}

	cleanup();
}

// Pushes this mission's contents into the global bounty board databinding
// tree. The engine only has one such tree, so the poster the player is
// standing in front of claims it while he is close enough to read it.
void BaseMissionExecutor::fillPosterBoard()
{
	if (!poster || !ENTITY::DOES_ENTITY_EXIST(poster) || !missionData || !getArea())
	{
		return;
	}
	// All eight mission executors sit in this stage at once, so without a range
	// check every one of them rewrites the single shared board tree every frame
	// and the sheet flickers between eight names (it also flooded the log).
	// Only the board in front of the player claims it.
	{
		Ped playerPed = PLAYER::PLAYER_PED_ID();
		if (!playerPed || !ENTITY::DOES_ENTITY_EXIST(playerPed))
		{
			return;
		}
		Vector3 playerCoords = ENTITY::GET_ENTITY_COORDS(playerPed, true, false);
		if (distanceBetween(playerCoords, *getArea()->bountyPostersCoords) > 45.0f)
		{
			return;
		}
	}

	UINT64 owner = (UINT64)(uintptr_t)missionData;
	if (!BountyBoardUI::needsFill(owner))
	{
		return;
	}

	const char* header = missionData->requiredTargetCondition == TargetCondition::Alive
		? TF("ui.board.header.alive", "WANTED - DEAD OR ALIVE")
		: TF("ui.board.header.dead", "WANTED - DEAD OR ALIVE");

	const char* areaName = getArea()->name ? getArea()->name : "";
	std::string areaKey = std::string("ui.area.") + areaName;
	// The sheet carries only the headline, the name and the reward.
	// Everything else lives in the read panel that opens on the poster.
	// The template prints this line under the name and the vanilla poster puts
	// the issuing station there ("Wanted by the following police department:
	// Valentine"), so the line is the station label followed by the area.
	// The station line reads "Wanted by the following police department:
	// Valentine". The suffix only belongs there when the label itself does not
	// already carry the word, otherwise the line ends up saying it twice.
	std::string stationLabel = TF("ui.poster.body.police_dept", "被以下警察局通缉：");
	std::string body = stationLabel;
	body += "  ";
	body += TF(areaKey.c_str(), areaName);
	if (stationLabel.find("警察局") == std::string::npos)
	{
		body += TF("ui.poster.detail.station_suffix", "警察局");
	}

	std::string price = "$" + std::to_string(scaledReward());

	// The portrait belongs to the criminal, not to the town. resolvePortraitIndex()
	// casts by the notice wording ([PortraitClass] can pin a mission, [PortraitMap]
	// a single texture number) and returns the final portrait_<n> index.
	BountyBoardUI::fillFor(owner, missionData->targetName, header, body.c_str(),
	                       price.c_str(), 0, resolvePortraitIndex(missionData));
}

void BaseMissionExecutor::spawnPosterProp()
{
	Vector3* posterPos = getArea()->bountyPostersCoords;
	float heading = getArea()->bountyPosterHeading;
	bool wantsDead = missionData->requiredTargetCondition != TargetCondition::Alive;

	posterIsNativeUI = false;

	// A sheet that is already nailed up stays: re-entering this function (a
	// mission being re-armed after a reset) used to create a second prop and
	// overwrite the handle, leaving the old paper in the world for good.
	if (poster != NULL && ENTITY::DOES_ENTITY_EXIST(poster))
	{
		return;
	}

	// Radar icon first, anchored to the board coordinates. The sheet may not
	// exist yet - or may have streamed out after a teleport - and a blip tied
	// to a missing entity never shows, which is how boards lost their icon.
	if (!RADAR::DOES_BLIP_EXIST(posterBlip))
	{
		posterBlip = createBlip(*posterPos, 25.0f, 0xB04092F8, -1636832113);
		setBlipLabel(posterBlip, TF("ui.blip.poster", "通缉令"));
	}

	// Only build the paper when the player can actually see it. Rebuilding it
	// for every board right after a teleport cost one model-load wait per board
	// inside a single frame, and remote props unload again immediately, so the
	// retry repeated every frame - that is the stall that showed up on the way
	// to the third bounty.
	{
		Ped nearPed = PLAYER::PLAYER_PED_ID();
		if (nearPed == 0 || !ENTITY::DOES_ENTITY_EXIST(nearPed))
		{
			return;
		}
		Vector3 nearPos = ENTITY::GET_ENTITY_COORDS(nearPed, true, false);
		if (distanceBetween(nearPos, *posterPos) > 80.0f)
		{
			return;
		}
	}

	// The native paper is only ever prepared for the board the player is
	// standing in front of. Doing it for all eight boards during
	// initialization is what stalled the loading screen.
	//
	// _UIAPP_RUNNING is NOT a usable gate: on this build it reads 0 for the
	// whole session, even while the game's own board is open. That check is
	// what kept every board on the printed paper. ZEEPosterFix proves the
	// flowblock plus a state machine is enough on its own - it never looks at
	// the app and still gets the letters onto the sheet.
	bool wantNative = false;
	{
		Ped dbgPed = PLAYER::PLAYER_PED_ID();
		float dbgDist = (dbgPed != 0 && ENTITY::DOES_ENTITY_EXIST(dbgPed))
			? distanceBetween(ENTITY::GET_ENTITY_COORDS(dbgPed, true, false), *posterPos) : -1.0f;
		std::stringstream sdbg;
		sdbg << "poster: spawn attempt area=" << getArea()->name << " dist=" << (int)dbgDist;
		log(sdbg.str());
	}
	if (BountyBoardUI::configEnabled())
	{
		Ped playerPed = PLAYER::PLAYER_PED_ID();
		if (playerPed && ENTITY::DOES_ENTITY_EXIST(playerPed))
		{
			Vector3 playerCoords = ENTITY::GET_ENTITY_COORDS(playerPed, true, false);
			wantNative = distanceBetween(playerCoords, *posterPos) <= BountyBoardUI::configDistance();
		}
	}

	// 1.4.4 nailed the game's own bounty paper to the board and let the
	// "bounty_board" flowblock print the localised text on it, so the letters
	// sit on the paper with correct perspective. Two details decide whether
	// that works: the paper must be a blank sheet (mp005) or at least a model
	// the mask can override, and it has to be slotted into an inspect-poster
	// scenario point BEFORE the entity mask is applied. Masking a free
	// standing prop is what left the paper blank in earlier builds.
	if (wantNative && BountyBoardUI::open())
	{
		const char* paper = BountyBoardUI::pickPosterModel(wantsDead);

		// The flowblock paints onto the paper's forward face. This prop model's
		// forward axis is the one facing the wall, so the sheet has to be turned
		// around or the letters end up on the back. FlipHeading=0 restores the
		// old orientation for boards that already looked right.
		float paperHeading = heading;
		if (modIniInt("Poster", "FlipHeading", 1) != 0)
		{
			paperHeading = heading + 180.0f;
		}

		// A dynamic prop falls off the board the moment the player touches it or
		// the scenario slot releases it - that is the sheet seen lying in the
		// dirt. 1.4.4's paper is pinned, so it is created frozen.
		log("poster: creating native sheet");
		poster = createProp((char*)paper, *posterPos, paperHeading, true, true);
		log(poster ? "poster: native sheet created" : "poster: native sheet FAILED");

		if (poster && ENTITY::DOES_ENTITY_EXIST(poster))
		{
			ENTITY::SET_ENTITY_HEADING(poster, paperHeading);

			BountyBoardUI::createPosterScenario(posterPos->x, posterPos->y, posterPos->z,
			                                    heading, wantsDead);
			BountyBoardUI::associate(poster);
			BountyBoardUI::attach(poster);
			fillPosterBoard();
			// Contents are in the tree now; resetting the machine here makes the
			// engine rebuild it and re-read those contents. Doing this before the
			// fill (as every earlier build did) binds an empty tree and the sheet
			// stays blank.
			BountyBoardUI::rebind();
			posterIsNativeUI = true;

			// Re-anchor the icon onto the sheet. The radius blip used for distant
			// boards is an area marker: it draws on the world map but hides on the
			// minimap once the player is inside its radius, which is exactly where
			// the icon matters.
			deleteBlipSafe(&posterBlip);
			posterBlip = createBlip(poster, 0xB04092F8, -1636832113);
			setBlipLabel(posterBlip, TF("ui.blip.poster", "通缉令"));
			return;
		}

		BountyBoardUI::deleteScenario();
		BountyBoardUI::close();

		if (poster && ENTITY::DOES_ENTITY_EXIST(poster))
		{
			ENTITY::DELETE_ENTITY(&poster);
		}
		poster = NULL;
	}

	// Fallback: the vanilla printed poster. Its artwork is baked into the model,
	// so it stays English, but the board is never empty and the retry loop in
	// update() can still replace it once the board UI becomes resident.
	log("poster: creating fallback sheet");
	if (wantsDead)
	{
		poster = createProp("p_pos_wanteddead01x", *posterPos, heading, false, true);
	}
	else
	{
		poster = createProp("p_pos_wantedalive01x", *posterPos, heading, false, true);
	}
	log(poster ? "poster: fallback sheet created" : "poster: fallback sheet FAILED");

	if (poster && ENTITY::DOES_ENTITY_EXIST(poster))
	{
		ENTITY::SET_ENTITY_HEADING(poster, heading);

		// Same reason as above: a blip bound to the sheet shows on the minimap.
		deleteBlipSafe(&posterBlip);
		posterBlip = createBlip(poster, 0xB04092F8, -1636832113);
		setBlipLabel(posterBlip, TF("ui.blip.poster", "通缉令"));
	}

}

void BaseMissionExecutor::takePoster()
{
	// The slot is claimed here: from now on this case file is carried and every
	// other board refuses to hand out a second one.
	if (!claimBounty(missionData->id))
	{
		setHelpMessage(TF("ui.hint.busy", "�������Ѿ���һ��ͨ����Ȱ����˽ᡣ"));
		return;
	}

	if (poster && ENTITY::DOES_ENTITY_EXIST(poster))
	{
		ENTITY::DELETE_ENTITY(&poster);
	}
	poster = NULL;

	posterPanelPage = 0;
	posterIsNativeUI = false;
	// Always show the readable case file: it is the only place the player can
	// see the localised name, reward, department and case description.
	posterPanelOpen = true;

	BountyBoardUI::deleteScenario();
	BountyBoardUI::close();

	inspectPosterPrompt->hide();
	acceptPosterPrompt->setText(TF("ui.prompt.accept", "��������"));
	acceptPosterPrompt->show();
	declinePosterPrompt->setText(TF("ui.prompt.putback", "�Żغ���"));
	declinePosterPrompt->show();
}

void BaseMissionExecutor::putPosterBack()
{
	posterPanelOpen = false;
	posterPanelPage = 0;
	posterIsNativeUI = false;
	acceptPosterPrompt->hide();
	declinePosterPrompt->hide();
	inspectPosterPrompt->hide();

	if (poster && ENTITY::DOES_ENTITY_EXIST(poster))
	{
		ENTITY::DELETE_ENTITY(&poster);
	}
	poster = NULL;
	releaseBounty(missionData->id);

	spawnPosterProp();
}

void BaseMissionExecutor::initialize()
{
	status = BountyMissionStatus::Pending;
	spawnPosterProp();
}

void BaseMissionExecutor::onPosterCollected()
{
	status = BountyMissionStatus::CollectedPoster;

	// Exactly one roll per bounty, and it happens here. update() rebuilds
	// the area blip every time the mission flag flickers; re-rolling there
	// handed out a brand new ambush on every flicker, with no upper bound.
	if (!posterRivalsRolled)
	{
		posterRivalsRolled = true;
		int rivalsChance = modIniInt("Poster", "RivalsChance", 25);
		if (rivalsChance < 0) { rivalsChance = 0; }
		if (rivalsChance > 100) { rivalsChance = 100; }
		int rivalsRoll = rand() % 100;
		posterRivalsPlanned = modIniInt("Poster", "Rivals", 0) != 0 && rivalsRoll < rivalsChance;
		posterRivalsDone = false;
		{
			std::stringstream pr;
			pr << "poster rivals: chance=" << rivalsChance << " roll=" << rivalsRoll
			   << " planned=" << (posterRivalsPlanned ? 1 : 0);
			log(pr.str());
		}
	}

	{
		Ped player = PLAYER::PLAYER_PED_ID();
		Vector3 posterCoords = *getArea()->bountyPostersCoords;

		// ��ȡ��ת�����򺣱��壬����ͣ�٣�������д�ʼǱ�������
		AI::TASK_TURN_PED_TO_FACE_COORD(player, posterCoords.x, posterCoords.y, posterCoords.z, 1000);
		inspectPosterPrompt->hide();
		WAIT(900);
	}

	if (poster && ENTITY::DOES_ENTITY_EXIST(poster))
	{
		ENTITY::DELETE_ENTITY(&poster);
	}
	poster = NULL;

	targetAreaBlip = createBlip(missionData->startPosition, targetAreaRadius, 0xB04092F8, -907204276 /* Bounty sprite */);

	const char* condition = missionData->requiredTargetCondition == TargetCondition::Alive
		? TF("ui.cond.alive", "��׽") : TF("ui.cond.deadoralive", "�����");
	std::stringstream label;
	label << TF("ui.blip.hunting", "����׷��") << ": "
		<< missionData->targetName << ", "
		<< scaledReward() << "$, "
		<< condition;
	setBlipLabel(targetAreaBlip, label.str().c_str());

	showSubtitle(TF("ui.sub.accepted", "�ѽ���ͨ���Ŀ�������ѱ�ע�ڵ�ͼ�ϡ�"));
	setHelpMessage(TF("ui.hint.review", "�����а� F7 ����ʱ�鿴ͨ���"));

	log(string("poster collected: ").append(missionData->targetName).append(" at ").append(getArea()->name));
}

void BaseMissionExecutor::onArrivalToArea()
{
	status = BountyMissionStatus::InProgress;
	Ped player = PLAYER::PLAYER_PED_ID();
	Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(player, true, false);
	setBlipLabel(targetAreaBlip, "抵达区域");

	target = spawnTarget();
	prepareSet();
	applyTargetName(); // ˢ����д��ͨ�������֣�������ҿ���

	spawnchanceComputed = false;
	custodyRivalsDone = false;
	releaseGuards();
	spawnRandomGuards();

	const char* gender = missionData->isTargetMale ? "男" : "女";
	const char* condition = missionData->requiredTargetCondition == TargetCondition::Alive
					? TF("ui.cond.alive", "活捉") : TF("ui.cond.deadoralive", "死活不论");
	std::stringstream text;
	text << "缉拿 ~COLOR_RED~" << missionData->targetName << "~COLOR_WHITE~（" << gender << "），" << condition;
	showSubtitle(text.str().c_str());

	log(string("mission started: ").append(getMissionData()->targetName).append(" in ").append(getArea()->name));
}

void BaseMissionExecutor::onTargetLocated()
{
	decorateTarget();
	deleteBlipSafe(&targetAreaBlip);
	seen = false;

	/*const char* gender = missionData->isTargetMale ? "��" : "��";
	const char* condition = missionData->requiredTargetCondition == TargetCondition::Alive
					? TF("ui.cond.alive", "��׽") : TF("ui.cond.deadoralive", "�����");
	std::stringstream text;
	text << "Capture ~COLOR_RED~" << missionData->targetName << "~COLOR_WHITE~. " << gender << " is wanted " << condition << ".";
	showSubtitle(text.str().c_str());*/
}

void BaseMissionExecutor::onTargetCaptured()
{
	Vector3* posterPos = getArea()->policeDeptCoords;
	dummyProp = createProp("p_shotGlass01x", *posterPos, true, false, false);
	policeLocBlip = createBlip(dummyProp, 0x1857A152);
	RADAR::_0x662D364ABF16DE2F(policeLocBlip, 0x900A4D0A);
	// A dead target is cargo, not a prisoner; saying "escort" while the
	// body is over the saddle reads wrong.
	bool corpse = targetWasKilled || (target != NULL && ENTITY::DOES_ENTITY_EXIST(target) && ENTITY::IS_ENTITY_DEAD(target));
	std::stringstream text;
	if (corpse)
	{
		text << "押送 ~COLOR_RED~" << missionData->targetName << "~COLOR_WHITE~ 把尸体送到 ~COLOR_YELLOW~警局";
	}
	else
	{
		text << "押送 ~COLOR_RED~" << missionData->targetName << "~COLOR_WHITE~ 押送到 ~COLOR_YELLOW~警局";
	}
	showSubtitle(text.str().c_str());
}

void BaseMissionExecutor::onArrivalToPoliceStation()
{
	OBJECT::DELETE_OBJECT(&dummyProp);
	deleteBlipSafe(&policeLocBlip);
	bool corpse = targetWasKilled ||
		(target != NULL && ENTITY::DOES_ENTITY_EXIST(target) && ENTITY::IS_ENTITY_DEAD(target));

	// Live prisoners are walked to the cell door (and the door is unlocked so
	// they can be put inside). A body is dropped off at the station instead -
	// no door, no cell, just the hand-over.
	Vector3* dropPos = corpse ? getArea()->corpseDropCoords : getArea()->cellCoords;
	if (!corpse)
	{
		OBJECT::_SET_DOOR_ACCELERATION_LIMIT(getArea()->cellnumber, 0);
	}
	dummyProp = createProp("p_shotGlass01x", *dropPos, true, false, false);
	cellBlip = createBlip(dummyProp, 0x1857A152);//0xC19DA63
	releaseGuards();

	vector<Ped>::iterator pedItr;
	for (pedItr = bountyHunters.begin(); pedItr != bountyHunters.end(); pedItr++)
	{
		releaseEntitySafe(&(*pedItr));
	}
	for (pedItr = bountyhorses.begin(); pedItr != bountyhorses.end(); pedItr++)
	{
		releaseEntitySafe(&(*pedItr));
	}

	std::stringstream text;
	if (corpse)
	{
		text << "押送 ~COLOR_RED~" << missionData->targetName << "~COLOR_WHITE~ 把尸体交给 ~COLOR_YELLOW~警局";
	}
	else
	{
		text << "押送 ~COLOR_RED~" << missionData->targetName << "~COLOR_WHITE~ 押进 ~COLOR_YELLOW~牢房";
	}
	showSubtitle(text.str().c_str());
}

void BaseMissionExecutor::onTargetHandedOver()
{
	OBJECT::DELETE_OBJECT(&dummyProp);
	Vector3* posterPos = getArea()->moneyCoords;
	dummyProp = createProp("p_shotGlass01x", *posterPos, true, false, false);
	moneyBlip = createBlip(dummyProp, 0x1857A152);//0xC19DA63
	deliveredReward = rewardForDelivery();
	money = OBJECT::CREATE_PICKUP(GAMEPLAY::GET_HASH_KEY("PICKUP_MONEY_VARIABLE"), (*getArea()->moneyCoords).x, (*getArea()->moneyCoords).y, (*getArea()->moneyCoords).z, 0, deliveredReward * 100, true, GAMEPLAY::GET_HASH_KEY("s_inv_moneyclip01x"), 1, 1);
	//OBJECT::_0xB8F5062070BB6DBD(money, true);
	std::stringstream text;
	text << "~COLOR_WHITE~ 领取 ~COLOR_YELLOW~赏金";
	showSubtitle(text.str().c_str());
	deleteBlipSafe(&targetBlip);
	deleteBlipSafe(&cellBlip);
}

void BaseMissionExecutor::onRewardCollected()
{
	OBJECT::DELETE_OBJECT(&dummyProp);
	deleteBlipSafe(&moneyBlip);
	std::stringstream text;
	text << "通缉完成 $" << deliveredReward << ".00 赏金已到账";
	showSubtitle(text.str().c_str());
}

// ---------------------------------------------------------------------------
// Payout. The printed price is the reference amount; the ini scales it and
// applies the dead/alive ratio that actually applies to this delivery.
// ---------------------------------------------------------------------------
// The printed price. RewardScale applies here, so the poster, the blip and
// the panel all show the same number the player will actually be paid for
// catching the target alive.
int BaseMissionExecutor::scaledReward()
{
	int base = missionData ? missionData->reward : 0;
	if (base <= 0)
	{
		return 0;
	}

	long long v = ((long long)base * rewardScaleCached()) / 100;
	if (v < 1)
	{
		v = 1;
	}
	return (int)v;
}

int BaseMissionExecutor::rewardForDelivery()
{
	int base = scaledReward();
	if (base <= 0)
	{
		return 0;
	}

	bool dead = targetWasKilled;
	if (!dead && target != NULL && ENTITY::DOES_ENTITY_EXIST(target))
	{
		dead = ENTITY::IS_ENTITY_DEAD(target) != 0;
	}

	return rewardAmount(dead);
}

// Payout for one delivery style. Two different deals are in play:
//   dead-or-alive poster -> the body is the baseline deal (100%), bringing
//   them in breathing pays a premium (120%);
//   live-only poster     -> alive pays CapturedAlivePercent and a corpse pays
//   nothing unless RequiredAliveKilledPercent opens a discounted rate.
int BaseMissionExecutor::rewardAmount(bool dead)
{
	int base = scaledReward();
	if (base <= 0)
	{
		return 0;
	}

	bool requiredAlive = (missionData && missionData->requiredTargetCondition == TargetCondition::Alive);

	int percent;
	if (dead)
	{
		percent = requiredAlive
			? modIniInt("Reward", "RequiredAliveKilledPercent", 0)
			: modIniInt("Reward", "DeadOrAliveKilledPercent", 100);
	}
	else
	{
		percent = requiredAlive
			? modIniInt("Reward", "CapturedAlivePercent", 100)
			: modIniInt("Reward", "DeadOrAliveAlivePercent", 120);
	}
	if (percent < 0)    { percent = 0; }
	if (percent > 1000) { percent = 1000; }

	long long amount = ((long long)base * percent) / 100;
	if (amount < 1)
	{
		amount = 1;
	}
	return (int)amount;
}

// ---------------------------------------------------------------------------
// Reading the case file again once the bounty is running. The paper itself is
// gone from the board by then, so this only draws the same panel the take-down
// step uses, and it never touches the mission stage.
// ---------------------------------------------------------------------------
void BaseMissionExecutor::updatePosterReview(Ped player, const Vector3& playerPos)
{
	if (!missionData)
	{
		return;
	}

	// Still at the board: there the panel belongs to update(), which draws the
	// case file together with the accept / put-back prompts. Touching it here
	// would close the freshly taken poster before it could be read.
	if (stage == BountyMissionStage::CollectPoster)
	{
		return;
	}

	// Bounty over or not started yet: the case file is not available.
	if (stage < BountyMissionStage::CollectPoster || stage >= BountyMissionStage::Finished)
	{
		if (posterPanelOpen)
		{
			posterPanelOpen = false;
			posterPanelPage = 0;
			posterPanelFromBoard = false;
			inspectPosterPrompt->hide();
			declinePosterPrompt->hide();
		}
		return;
	}

	// Hotkey (F7 by default): open or close the case file from anywhere.
	if (IsKeyJustUp(reviewKeyCode()))
	{
		posterPanelOpen = !posterPanelOpen;
		posterPanelPage = 0;
		posterPanelFromBoard = false;

		if (posterPanelOpen)
		{
			log("poster review opened by hotkey");
		}
	}

	float distToBoard = distanceBetween(playerPos, *getArea()->bountyPostersCoords);
	if (distToBoard > 8.0f)
	{
		reviewArmed = true;
	}

	if (!posterPanelOpen)
	{
		// The poster was taken off the board, so there is nothing to look at
		// on site any more. The case file stays reachable through the review
		// hotkey only; walking back to the empty board does nothing.
		inspectPosterPrompt->hide();
		declinePosterPrompt->hide();
		return;
	}

	// Only the board-opened panel follows the player away from the board.
	if (posterPanelFromBoard && distToBoard > 6.0f)
	{
		posterPanelOpen = false;
		posterPanelPage = 0;
		posterPanelFromBoard = false;
		inspectPosterPrompt->hide();
		declinePosterPrompt->hide();
		return;
	}

	drawPosterPanel();

	inspectPosterPrompt->setText(posterPanelPage == 0
		? TF("ui.prompt.read", "�Ķ�����")
		: TF("ui.prompt.back", "����ժҪ"));
	inspectPosterPrompt->show();
	if (inspectPosterPrompt->isActivatedByPlayer())
	{
		posterPanelPage = (posterPanelPage == 0) ? 1 : 0;
	}

	declinePosterPrompt->setText(TF("ui.prompt.close", "����"));
	declinePosterPrompt->show();
	if (declinePosterPrompt->isActivatedByPlayer())
	{
		posterPanelOpen = false;
		posterPanelPage = 0;
		posterPanelFromBoard = false;
		declinePosterPrompt->hide();
		inspectPosterPrompt->hide();
	}
}

void BaseMissionExecutor::onFinished(bool shouldCleanup)
{
	status = BountyMissionStatus::Completed;
	releaseBounty(missionData->id);

	if (shouldCleanup)
	{
		cleanup();
	}
}

void BaseMissionExecutor::spawnBountyHunters()
{
	// Horses and riders appear on the way back with a body. This is the most
	// fragile path in the mod, so it stays behind an ini switch.
	if (modIniInt("Poster", "CustodyRivals", 0) == 0)
	{
		return;
	}

	Ped player = PLAYER::PLAYER_PED_ID();
	Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(player, true, 0);

	log("spawnBountyHunters: begin");

	const char* horseModel = "A_C_Horse_TennesseeWalker_DappleBay";
	HorseSaddleHashes saddles[3] =
	{
		HorseSaddleHashes::MCCLELLAN_01_STOCK_NEW_SADDLE_004,
		HorseSaddleHashes::MCCLELLAN_01_STOCK_NEW_SADDLE_002,
		HorseSaddleHashes::MCCLELLAN_01_STOCK_NEW_SADDLE_002
	};

	int horsesOk = 0;
	bool huntersOnHorse = modIniInt("Poster", "HuntersOnHorse", 0) != 0;
	log(huntersOnHorse ? "custody rivals: spawning mounted" : "custody rivals: spawning on foot");

	for (int i = 0; i < 3; i++)
	{
		// Spawn to the side, never straight ahead: a rider materialising in
		// front of a galloping player is a collision waiting to happen.
		float sideSign = (rand() % 2 == 0) ? -1.0f : 1.0f;
		float sideOffset = sideSign * (8.0f + (float)(rand() % 5));
		float fwdOffset = (float)(rand() % 11) - 5.0f;
		Vector3 spawnPos = positionAheadOfPlayer(fwdOffset, sideOffset);

		if (!huntersOnHorse)
		{
			addBountyHunterOnFoot(spawnPos);
			continue;
		}

		// 40 m instead of 60: at 60 the riders often spawned behind a ridge or
		// inside cover, so only one of the three was ever seen.
		Ped horse = createPed((char*)horseModel, spawnPos);
		if (horse == 0 || !ENTITY::DOES_ENTITY_EXIST(horse))
		{
			log("spawnBountyHunters: horse failed, skipped");
			continue;
		}

		horsesOk++;
		bountyhorses.push_back(horse);
		giveSaddleToHorse(horse, saddles[i]);
		addBountyHunter(horse, true);
	}

	WAIT(1000);

	// Only live riders get a combat order: TASK_COMBAT_PED on a zero handle is
	// exactly what parked the script when a rider failed to spawn.
	int ordered = 0;
	for (vector<Ped>::iterator pedItr = bountyHunters.begin(); pedItr != bountyHunters.end(); pedItr++)
	{
		Ped curr = *pedItr;
		if (curr == 0 || !ENTITY::DOES_ENTITY_EXIST(curr))
		{
			continue;
		}
		AI::TASK_COMBAT_PED(curr, player, 0, 16);
		ordered++;
	}

	showSubtitle(TF("ui.sub.rivals_custody", "匪帮同伙追上来了，想在半路把人抢回去。"));
	spawnedBountyHunters = true;

	std::stringstream sb;
	sb << "spawnBountyHunters: horses=" << horsesOk << " riders=" << ordered;
	log(sb.str());
}

void BaseMissionExecutor::enforceRivalsFire()
{
	if (bountyHunters.empty())
	{
		return;
	}

	DWORD now = GAMEPLAY::GET_GAME_TIMER();
	if (now - rivalsFireAt < 5000)
	{
		return;
	}
	rivalsFireAt = now;

	Ped player = PLAYER::PLAYER_PED_ID();
	if (player == 0 || !ENTITY::DOES_ENTITY_EXIST(player))
	{
		return;
	}

	float nearest = 99999.0f;

	for (size_t i = 0; i < bountyHunters.size(); i++)
	{
		Ped hunter = bountyHunters[i];
		if (hunter == 0 || !ENTITY::DOES_ENTITY_EXIST(hunter) || ENTITY::IS_ENTITY_DEAD(hunter))
		{
			continue;
		}

		// Re-assert every tick: if another mod rewrites the group or the
		// relationship, one of our writes still lands between its writes.
		Hash rivalsGroup = getRivalsRelationshipGroup();
		if (PED::GET_PED_RELATIONSHIP_GROUP_HASH(hunter) != rivalsGroup)
		{
			PED::SET_PED_RELATIONSHIP_GROUP_HASH(hunter, rivalsGroup);
		}
		{
			Hash playerGroup = PED::GET_PED_RELATIONSHIP_GROUP_HASH(player);
			PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, rivalsGroup, playerGroup);
			PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, playerGroup, rivalsGroup);
		}

		float d = distanceBetweenEntitiesHor(hunter, player);
		Ped mount = PED::GET_MOUNT(hunter);
		bool mounted = (mount != 0 && ENTITY::DOES_ENTITY_EXIST(mount));

		if (mounted && d > 8.0f)
		{
			// A mounted ped ignores walk orders, so the horse is driven
			// directly. Move rate keeps it level with a galloping player.
			AI::TASK_GO_TO_ENTITY(mount, player, -1, 6.0f, 3.0f, 0.0f, 0);
			PED::SET_PED_MOVE_RATE_OVERRIDE(mount, 1.55f);
			// No shoot order while closing: it parks the rider and the horse
			// stops with him.
		}
		else
		{
			AI::TASK_COMBAT_PED(hunter, player, 0, 16);
		}

		if (d < nearest)
		{
			nearest = d;
		}
	}

	if (now - rivalsDiagAt > 30000)
	{
		rivalsDiagAt = now;
		for (size_t i = 0; i < bountyHunters.size(); i++)
		{
			Ped hunter = bountyHunters[i];
			if (hunter == 0 || !ENTITY::DOES_ENTITY_EXIST(hunter) || ENTITY::IS_ENTITY_DEAD(hunter))
			{
				continue;
			}

			Hash g = PED::GET_PED_RELATIONSHIP_GROUP_HASH(hunter);
			Hash gd = PED::GET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH(hunter);
			Hash pg = PED::GET_PED_RELATIONSHIP_GROUP_HASH(player);
			int rel = PED::GET_RELATIONSHIP_BETWEEN_PEDS(hunter, player);
			Hash wHash = 0;
			WEAPON::GET_CURRENT_PED_WEAPON(hunter, &wHash, true, 0, false);
			int ammo = (wHash != 0) ? WEAPON::GET_AMMO_IN_PED_WEAPON(hunter, wHash) : -1;
			int incombat = PED::IS_PED_IN_COMBAT(hunter, player) ? 1 : 0;
			std::stringstream dg;
			dg << "rivals diag: group=" << (UINT32)g
			   << " default=" << (UINT32)gd
			   << " playerGroup=" << (UINT32)pg
			   << " rel=" << rel
			   << " weapon=" << (UINT32)wHash
			   << " ammo=" << ammo
			   << " incombat=" << incombat
			   << " wanted=" << (UINT32)rivalsRelGroup
			   << " dist=" << (int)distanceBetweenEntitiesHor(hunter, player);
			log(dg.str());
			break;
		}
	}

	// The player rides faster than any spawned hunter. Instead of giving up on
	// the wave, put it back on the road ahead: twice per wave, never more.
	if (nearest > 120.0f)
	{
		if (rivalsFarSince == 0)
		{
			rivalsFarSince = now;
		}
		else if (now - rivalsFarSince > 6000 && rivalsRelocateCount < 2)
		{
			rivalsRelocateCount++;
			rivalsFarSince = 0;
			relocateRivals("fell behind");
		}
	}
	else
	{
		rivalsFarSince = 0;
	}

	// Standing still and shooting looks wrong; when the wave stays far away the
	// riders are put on foot once, which is the path the area guards use.
	if (modIniInt("Poster", "HuntersDismountIfStuck", 0) != 0 && !rivalsDismounted && nearest > 30.0f)
	{
		if (rivalsFarSince == 0)
		{
			rivalsFarSince = now;
		}
		else if (now - rivalsFarSince > 6000)
		{
			rivalsDismounted = true;
			log("rivals: dismount to engage");

			for (size_t i = 0; i < bountyHunters.size(); i++)
			{
				Ped hunter = bountyHunters[i];
				if (hunter == 0 || !ENTITY::DOES_ENTITY_EXIST(hunter) || ENTITY::IS_ENTITY_DEAD(hunter))
				{
					continue;
				}

				invoke<Void>(0x48E92D3DDE23C23A, hunter, 0, 0, 0, 0, 0);
				AI::TASK_COMBAT_PED(hunter, player, 0, 16);
			}
		}
	}
	else
	{
		rivalsFarSince = 0;
	}
}

Vector3 BaseMissionExecutor::positionAheadOfPlayer(float distance, float lateralOffset)
{
	Ped player = PLAYER::PLAYER_PED_ID();
	Vector3 base = ENTITY::GET_ENTITY_COORDS(player, true, 0);
	Vector3 fwd = ENTITY::GET_ENTITY_FORWARD_VECTOR(player);
	fwd.z = 0.0f;

	float len = sqrtf(fwd.x * fwd.x + fwd.y * fwd.y);
	if (len < 0.001f)
	{
		fwd.x = 0.0f;
		fwd.y = 1.0f;
	}
	else
	{
		fwd.x /= len;
		fwd.y /= len;
	}

	Vector3 pos;
	pos.x = base.x + fwd.x * distance - fwd.y * lateralOffset;
	pos.y = base.y + fwd.y * distance + fwd.x * lateralOffset;
	pos.z = base.z;

	Vector3 safe;
	if (PATHFIND::GET_SAFE_COORD_FOR_PED(pos.x, pos.y, pos.z, true, &safe, 16))
	{
		return safe;
	}
	return pos;
}

void BaseMissionExecutor::relocateRivals(const char* reason)
{
	Ped player = PLAYER::PLAYER_PED_ID();
	if (player == 0 || !ENTITY::DOES_ENTITY_EXIST(player))
	{
		return;
	}

	float heading = ENTITY::GET_ENTITY_HEADING(player);
	int moved = 0;
	int slot = 0;

	for (size_t i = 0; i < bountyHunters.size(); i++)
	{
		Ped hunter = bountyHunters[i];
		if (hunter == 0 || !ENTITY::DOES_ENTITY_EXIST(hunter) || ENTITY::IS_ENTITY_DEAD(hunter))
		{
			continue;
		}

		Vector3 pos = positionAheadOfPlayer(55.0f, (float)((slot % 3) - 1) * 8.0f);
		Ped mount = PED::GET_MOUNT(hunter);
		if (mount != 0 && ENTITY::DOES_ENTITY_EXIST(mount))
		{
			ENTITY::SET_ENTITY_COORDS(mount, pos.x, pos.y, pos.z, 1, 1, 1, 0);
			ENTITY::SET_ENTITY_HEADING(mount, heading);
		}
		ENTITY::SET_ENTITY_COORDS(hunter, pos.x, pos.y, pos.z + 0.2f, 1, 1, 1, 0);
		ENTITY::SET_ENTITY_HEADING(hunter, heading);
		AI::TASK_COMBAT_PED(hunter, player, 0, 16);

		slot++;
		moved++;
	}

	if (moved > 0)
	{
		std::stringstream rs;
		rs << "rivals: relocated (" << reason << ", retry " << rivalsRelocateCount
		   << ", moved " << moved << ")";
		log(rs.str());
	}
}

Hash BaseMissionExecutor::getRivalsRelationshipGroup()
{
	if (rivalsRelGroup != 0)
	{
		return rivalsRelGroup;
	}

	Hash group = 0;
	PED::ADD_RELATIONSHIP_GROUP((char*)"BE_HUNTERS", &group);
	if (group == 0)
	{
		log("rivals: ADD_RELATIONSHIP_GROUP failed, using REL_CRIMINALS");
		rivalsRelGroup = GAMEPLAY::GET_HASH_KEY("REL_CRIMINALS");
		return rivalsRelGroup;
	}

	Ped player = PLAYER::PLAYER_PED_ID();
	Hash playerGroup = (player != 0 && ENTITY::DOES_ENTITY_EXIST(player))
		? PED::GET_PED_RELATIONSHIP_GROUP_HASH(player)
		: GAMEPLAY::GET_HASH_KEY("PLAYER");

	// 5 = hate in the classic relationship enum (0 companion .. 5 hate).
	PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, group, playerGroup);
	PED::SET_RELATIONSHIP_BETWEEN_GROUPS(5, playerGroup, group);

	{
		std::stringstream sb;
		sb << "rivals: relationship group ready hash=" << (UINT32)group
		   << " playerGroup=" << (UINT32)playerGroup;
		log(sb.str());
	}

	rivalsRelGroup = group;
	return rivalsRelGroup;
}

void BaseMissionExecutor::addBountyHunter(Ped horse, bool isDriver)
{
	// A failed spawn returns 0. Every native below (relationship group, blip,
	// weapon) and the follow-up TASK_COMBAT_PED would then be called on an
	// invalid handle, which parks the script thread - this is what froze the
	// game once the hunters showed up on the way back.
	if (horse == 0 || !ENTITY::DOES_ENTITY_EXIST(horse))
	{
		log("addBountyHunter: horse invalid, rider skipped");
		return;
	}

	char hunterModel[64];
	modIniString("Poster", "HunterModel", "G_M_M_UniRanchers_01", hunterModel, sizeof(hunterModel));

	Ped bh = createPedOnHorse(hunterModel, horse, isDriver ? -1 : 0);
	if (bh == 0 || !ENTITY::DOES_ENTITY_EXIST(bh))
	{
		log("addBountyHunter: rider failed to spawn, skipped");
		return;
	}

	Hash rivalsGroup = getRivalsRelationshipGroup();
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(bh, rivalsGroup);
	// Same line the area guards use: without it the ambient behaviour
	// overrides the scripted combat order and the ped just shouts.
	PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(bh, 1);
	// The badge mod leaves these peds unable to enter combat at all
	// (GET_IS_PED_IN_COMBAT stayed 0 while we stood next to them), so the
	// combat profile is raised explicitly.
	PED::SET_PED_COMBAT_ABILITY(bh, 2);
	PED::SET_PED_COMBAT_ATTRIBUTES(bh, 46, true);
	PED::SET_PED_COMBAT_MOVEMENT(bh, 2);
	PED::SET_PED_COMBAT_RANGE(bh, 2);
	PED::SET_PED_TARGET_LOSS_RESPONSE(bh, 1);
	PED::SET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH(bh, rivalsGroup);
	PED::SET_PED_COMBAT_ATTRIBUTES(bh, 46, true);
	DECORATOR::DECOR_SET_INT(bh, "honor_override", 0);
	bountyHunters.push_back(bh);
	createBlip(bh, BLIP_STYLE_ENEMY);
	// Give the weapon outright: pedEquipBestWeapon only equips what the ped
	// already carries, and a stripped loadout leaves the ambush unarmed.
	giveWeaponToPed(bh, RevolverSchofield, 0x64356159, true);
	pedEquipBestWeapon(bh);
	PED::SET_PED_ACCURACY(bh, 35);

	// One line per rider: the count in spawnBountyHunters only proves creation,
	// not that the man is anywhere the player can see him.
	{
		Ped pl = PLAYER::PLAYER_PED_ID();
		float d = (pl != 0 && ENTITY::DOES_ENTITY_EXIST(pl)) ? distanceBetweenEntitiesHor(bh, pl) : -1.0f;
		std::stringstream sb;
		sb << "bounty hunter spawned handle=" << (UINT32)bh << " horse=" << (UINT32)horse
		   << " dist=" << (int)d;
		log(sb.str());
	}
}

void BaseMissionExecutor::addBountyHunterOnFoot(Vector3 pos)
{
	char hunterModel[64];
	modIniString("Poster", "HunterModel", "G_M_M_UniRanchers_01", hunterModel, sizeof(hunterModel));

	Hash model = GAMEPLAY::GET_HASH_KEY(hunterModel);
	STREAMING::REQUEST_MODEL(model, false);

	int loadGuard = 0;
	while (!STREAMING::HAS_MODEL_LOADED(model) && loadGuard < 120)
	{
		WAIT(0);
		loadGuard++;
	}
	if (!STREAMING::HAS_MODEL_LOADED(model))
	{
		log("addBountyHunterOnFoot: model failed to load");
		return;
	}

	Ped bh = createPed(hunterModel, pos);
	if (bh == 0 || !ENTITY::DOES_ENTITY_EXIST(bh))
	{
		log("addBountyHunterOnFoot: spawn failed");
		return;
	}

	Hash rivalsGroup = getRivalsRelationshipGroup();
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(bh, rivalsGroup);
	// Same line the area guards use: without it the ambient behaviour
	// overrides the scripted combat order and the ped just shouts.
	PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(bh, 1);
	// The badge mod leaves these peds unable to enter combat at all
	// (GET_IS_PED_IN_COMBAT stayed 0 while we stood next to them), so the
	// combat profile is raised explicitly.
	PED::SET_PED_COMBAT_ABILITY(bh, 2);
	PED::SET_PED_COMBAT_ATTRIBUTES(bh, 46, true);
	PED::SET_PED_COMBAT_MOVEMENT(bh, 2);
	PED::SET_PED_COMBAT_RANGE(bh, 2);
	PED::SET_PED_TARGET_LOSS_RESPONSE(bh, 1);
	PED::SET_PED_RELATIONSHIP_GROUP_DEFAULT_HASH(bh, rivalsGroup);
	PED::SET_PED_COMBAT_ATTRIBUTES(bh, 46, true);
	DECORATOR::DECOR_SET_INT(bh, "honor_override", 0);
	bountyHunters.push_back(bh);
	createBlip(bh, BLIP_STYLE_ENEMY);
	// Give the weapon outright: pedEquipBestWeapon only equips what the ped
	// already carries, and a stripped loadout leaves the ambush unarmed.
	giveWeaponToPed(bh, RevolverSchofield, 0x64356159, true);
	pedEquipBestWeapon(bh);
	PED::SET_PED_ACCURACY(bh, 35);

	{
		Ped pl = PLAYER::PLAYER_PED_ID();
		float d = (pl != 0 && ENTITY::DOES_ENTITY_EXIST(pl)) ? distanceBetweenEntitiesHor(bh, pl) : -1.0f;
		std::stringstream sb;
		sb << "hunter on foot spawned handle=" << (UINT32)bh << " dist=" << (int)d;
		log(sb.str());
	}
}

void BaseMissionExecutor::releaseUnnecessaryEntities()
{
	Ped player = PLAYER::PLAYER_PED_ID();
	if (player == 0 || !ENTITY::DOES_ENTITY_EXIST(player))
	{
		return;
	}

	// From the ride out to the area onwards, not just the escort stage. The
	// roadside ambush is spawned while the player travels to the target and
	// used to be unreachable for this sweep, so a wave that fell behind
	// stayed in the world for the rest of the mission.
	if (getMissionStage() < BountyMissionStage::GoToArea)
	{
		return;
	}

	// Distance only. Releasing a gang member the moment it died meant
	// SET_ENTITY_AS_NO_LONGER_NEEDED ran against a corpse that was still being
	// ragdolled - once per frame, for every horse and rider. That is where the
	// script parked right after the escort hunters were killed.
	std::vector<Ped>* lists[3] = { &bountyhorses, &bountyHunters, &randomGuards };
	const char* labels[3] = { "horse", "hunter", "guard" };

	for (int l = 0; l < 3; l++)
	{
		for (size_t i = 0; i < lists[l]->size(); i++)
		{
			Ped ped = (*lists[l])[i];
			if (ped == 0 || !ENTITY::DOES_ENTITY_EXIST(ped))
			{
				continue;
			}
			if (distanceBetweenEntitiesHor(ped, player) > 250.0f)
			{
				log(std::string("release far ").append(labels[l]));
				releaseEntitySafe(&(*lists[l])[i]);
			}
		}
	}
}

void BaseMissionExecutor::releaseEntitySafe(Entity* entity)
{
	Vector3 playerPos = ENTITY::GET_ENTITY_COORDS((PLAYER::PLAYER_PED_ID()), true, false);
	if ((*entity) && ENTITY::DOES_ENTITY_EXIST(*entity))
	{
		if (stage == BountyMissionStage::LocateTarget && distanceBetween(playerPos, missionData->startPosition) > targetAreaRadius)
		{
			ENTITY::DELETE_ENTITY(entity);
		}
		else
		{
			ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(entity);
		}
	}
}

// Guards, their blips and their handles all have to go together. The
// arrival path used to clear the vector without releasing anything: the
// crew stayed in the world with no handle left to clean it up, so every
// return to the target area leaked another set of shooters.
void BaseMissionExecutor::releaseGuards()
{
	vector<Ped>::iterator it;
	for (it = randomGuards.begin(); it != randomGuards.end(); it++)
	{
		releaseEntitySafe(&(*it));
	}
	randomGuards.clear();

	for (size_t bi = 0; bi < guardBlips.size(); bi++)
	{
		deleteBlipSafe(&guardBlips[bi]);
	}
	guardBlips.clear();
}

void BaseMissionExecutor::cleanup()
{
	// Bracket the teardown: the freeze after a completed mission happened while
	// this ran, so the last of these two lines tells whether the thread died
	// inside cleanup or somewhere after it.
	log(std::string("cleanup: begin ").append(missionData ? missionData->targetName : "?"));
	posterIsNativeUI = false;
	posterRetryTimer = 0;
	posterPanelOpen = false;
	posterPanelPage = 0;
	targetWasKilled = false;
	reviewArmed = false;
	// The next bounty rolls its own ambush.
	posterRivalsRolled = false;
	posterPanelFromBoard = false;
	inspectPosterPrompt->hide();
	acceptPosterPrompt->hide();
	declinePosterPrompt->hide();
	BountyBoardUI::deleteScenario();
	BountyBoardUI::close();
	if (status == BountyMissionStatus::Completed || status == BountyMissionStatus::Failed)
	{
		OBJECT::_SET_DOOR_ACCELERATION_LIMIT(getArea()->cellnumber, 1);
		deleteBlipSafe(&targetBlip);
		deleteBlipSafe(&targetAreaBlip);
		deleteBlipSafe(&policeLocBlip);
		deleteBlipSafe(&cellBlip);
		releaseEntitySafe(&target);
		releaseEntitySafe(&money);
	}

	// These leaked on every failed or re-armed mission: a blip handle belongs to
	// the radar, not to the entity manager, so releasing it as an entity did
	// nothing; the shot glass prop and the guards were never touched at all.
	deleteBlipSafe(&moneyBlip);
	OBJECT::DELETE_OBJECT(&dummyProp);

	releaseGuards();

	vector<Ped>::iterator pedItr;
	for (pedItr = bountyHunters.begin(); pedItr != bountyHunters.end(); pedItr++)
	{
		releaseEntitySafe(&(*pedItr));
	}
	bountyHunters.clear();

	for (pedItr = bountyhorses.begin(); pedItr != bountyhorses.end(); pedItr++)
	{
		releaseEntitySafe(&(*pedItr));
	}
	bountyhorses.clear();

	log(std::string("cleanup: done ").append(missionData ? missionData->targetName : "?"));
}

// UTF-8 ��֪���Զ����л��ƣ�ȫ���ַ��� 2 ��λ�ƣ�������ʵ��ռ������
static int drawPosterLines(const char* text, float x, float y, float lineHeight, int maxLines,
	int maxUnits = 60, int cr = 235, int cg = 235, int cb = 235)
{
	if (!text || !text[0] || maxLines <= 0)
	{
		return 0;
	}

	// maxUnits �ǰ� scale 0.28 ���Ŀ��ÿ��ȣ�ȫ���ַ��� 2 ��λ��

	size_t total = 0;
	while (text[total])
	{
		total++;
	}

	std::string cur;
	int curUnits = 0;
	int line = 0;
	size_t i = 0;

	while (i < total && line < maxLines)
	{
		unsigned char c = (unsigned char)text[i];
		size_t len = 1;
		if (c >= 0xF0)
		{
			len = 4;
		}
		else if (c >= 0xE0)
		{
			len = 3;
		}
		else if (c >= 0xC0)
		{
			len = 2;
		}
		if (i + len > total)
		{
			len = 1;
		}

		// ��������ʽд��� \n ��Ȼ��Ч
		if (len == 1 && text[i] == '\n')
		{
			if (!cur.empty())
			{
				drawText((char*)cur.c_str(), x, y + line * lineHeight, cr, cg, cb, 255, false, 0.28f, 0.28f);
				line++;
			}
			cur.clear();
			curUnits = 0;
			i += 1;
			continue;
		}

		// ȫ�ǣ����ֽ� UTF-8���� 2 ����λ������� 1 ��
		int w = (len > 1) ? 2 : 1;
		if (curUnits + w > maxUnits && !cur.empty())
		{
			drawText((char*)cur.c_str(), x, y + line * lineHeight, cr, cg, cb, 255, false, 0.28f, 0.28f);
			line++;
			cur.clear();
			curUnits = 0;
			if (line >= maxLines)
			{
				break;
			}
		}

		cur.append(text + i, len);
		curUnits += w;
		i += len;
	}

	if (!cur.empty() && line < maxLines)
	{
		drawText((char*)cur.c_str(), x, y + line * lineHeight, cr, cg, cb, 255, false, 0.28f, 0.28f);
		line++;
	}

	return line;
}

// ���������ػ���ui.area.<Name>��ȱʧʱ����Ӣ��ԭ��
static const char* posterAreaName(const char* area)
{
	if (!area || !area[0])
	{
		return "";
	}

	std::string key = std::string("ui.area.") + area;
	return TF(key.c_str(), area);
}

void BaseMissionExecutor::drawPosterPanel()
{
	if (!missionData)
	{
		return;
	}

	const char* condition = missionData->requiredTargetCondition == TargetCondition::Alive
		? TF("ui.cond.alive", "��׽") : TF("ui.cond.deadoralive", "�����");
	const char* crime = (missionData->crime && missionData->crime[0])
		? missionData->crime : TF("ui.poster.unknown", "���ɲ���");

	// ֽ�߰�ҳ����Ӧ��ժҪҳҪ����ͷ������ҳֻҪ���� + ������Ϣ
	const float W = 0.212f;
	const float H = (posterPanelPage == 0) ? 0.556f : 0.492f;

	const float px = 0.5f - W * 0.5f;
	const float py = 0.5f - H * 0.5f;
	const float pb = py + H;
	const float midX = px + W * 0.5f;

	// RDR2 ͨ����ľ�ֽɫ
	const int paperR = 212, paperG = 193, paperB = 156;
	const int lnR = 132, lnG = 104, lnB = 70;
	const int inkR = 42, inkG = 30, inkB = 18;

	// ---- ���ֽ�棺����ƫ�Ƶĵ�ֽɫ��ģ�ⲻ���ֽ�� ----
	drawRect(px - 0.010f, py - 0.008f, W + 0.020f, H + 0.017f, 0, 0, 0, 165);
	drawRect(px - 0.0055f, py - 0.0045f, W + 0.011f, H + 0.010f, 58, 40, 26, 255);
	drawRect(px - 0.0018f, py - 0.0016f, W + 0.0037f, H + 0.0034f, paperR - 14, paperG - 14, paperB - 12, 255);
	drawRect(px, py, W, H, paperR, paperG, paperB, 255);

	// ֽ�����ɣ������������ײ��԰����Ӽ������������ά��������߻��Ե�����ֽ��
	drawRect(px, py, W, H * 0.18f, paperR + 6, paperG + 6, paperB + 6, 90);
	drawRect(px, pb - H * 0.22f, W, H * 0.22f, paperR - 10, paperG - 12, paperB - 14, 70);
	drawRect(px + W * 0.07f, py + H * 0.152f, W * 0.42f, 0.0006f, 176, 154, 118, 55);
	drawRect(px + W * 0.30f, py + H * 0.286f, W * 0.55f, 0.0006f, 176, 154, 118, 45);
	drawRect(px + W * 0.12f, py + H * 0.520f, W * 0.38f, 0.0006f, 176, 154, 118, 45);
	drawRect(px + W * 0.44f, py + H * 0.760f, W * 0.44f, 0.0006f, 176, 154, 118, 40);
	// ֽ�����ɣ������������ײ��԰����Ӽ������������ά��������߻��Ե�����ֽ��
	drawRect(midX, py + H * 0.02f, 0.0006f, H * 0.96f, 196, 176, 140, 26);

	// ֽ�����ɣ������������ײ��԰����Ӽ������������ά��������߻��Ե�����ֽ��
	drawRect(px + W * 0.06f, py + H * 0.032f, W * 0.88f, 0.0012f, lnR, lnG, lnB, 200);
	drawRect(px + W * 0.06f, pb - H * 0.032f, W * 0.88f, 0.0012f, lnR, lnG, lnB, 200);

	if (posterPanelPage == 0)
	{
		// ---------- ժҪҳ ----------
		// ̧ͷ��ͨ�� + ����̺��ߣ�ģ��ӡˢͨ����İ�ʽ
		drawText((char*)TF("ui.poster.wanted", "ͨ��"), midX, py + H * 0.070f, 96, 30, 18, 255, true, 0.50f, 0.50f);
		drawRect(px + W * 0.08f, py + H * 0.088f, W * 0.20f, 0.0010f, lnR, lnG, lnB, 170);
		drawRect(px + W * 0.72f, py + H * 0.088f, W * 0.20f, 0.0010f, lnR, lnG, lnB, 170);

		drawText((char*)condition, midX, py + H * 0.132f, 118, 30, 22, 255, true, 0.235f, 0.235f);

		std::stringstream reward;
		reward << "$" << scaledReward();
		drawText((char*)reward.str().c_str(), midX, py + H * 0.212f, inkR, inkG, inkB, 255, true, 0.86f, 0.86f);

		drawRect(midX - W * 0.24f, py + H * 0.262f, W * 0.48f, 0.0012f, lnR, lnG, lnB, 190);

		// A dead-or-alive poster pays differently for a body and for a live
		// capture, so both numbers are printed under the printed price.
		if (missionData->requiredTargetCondition == TargetCondition::DeadOrAlive)
		{
			std::stringstream split;
			split << TF("ui.poster.alive", "��׽") << " $" << rewardAmount(false)
				<< "    " << TF("ui.poster.dead", "����") << " $" << rewardAmount(true);
			drawText((char*)split.str().c_str(), midX, py + H * 0.278f, 96, 74, 52, 255, true, 0.205f, 0.205f);
		}

		// ͷ���
		float avW = W * 0.56f;
		float avH = H * 0.245f;
		float avX = midX - avW * 0.5f;
		float avY = py + H * 0.292f;

		// Paper backdrop, then the photo itself. The photo comes from the same
		// dictionary the board uses, keyed by map area, so the panel and the
		// sheet always show the same face.
		drawRect(avX, avY, avW, avH, paperR + 12, paperG + 12, paperB + 14, 255);

		bool drewPhoto = false;
		{
			// Ask the board wrapper: it checks with the correct native hash and
			// never issues a request.
			const char* dictName = BountyBoardUI::portraitDictForDraw();
			char* dict = (dictName != NULL) ? (char*)dictName : NULL;

			// Both settings are read ONCE. Calling GetPrivateProfileInt from the
			// draw loop reads the ini file every frame, and this ini lives on a
			// VMware shared folder - that much disk IO stalls the game the moment
			// the panel opens, which is exactly how the freeze after collecting a
			// poster started.
			static float s_photoScale = -1.0f;
			static int s_photoOn = -1;
			if (s_photoScale < 0.0f)
			{
				s_photoScale = (float)modIniInt("Poster", "PanelPortraitScale", 130) / 100.0f;
			}
			if (s_photoOn < 0)
			{
				s_photoOn = modIniInt("Poster", "PanelPortrait", 1);
			}

			if (dict != NULL && s_photoOn != 0)
			{
				std::string tex = std::string("portrait_")
					+ std::to_string(resolvePortraitIndex(missionData));
				float photoH = avH * s_photoScale;

				drawSprite(dict, (char*)tex.c_str(), avX, avY, avW, photoH, 0, 255, 255, 255, 255);
				drewPhoto = true;
			}
		}

		// Placeholder silhouette only when the portrait dictionary is missing, so
		// a missing ytd never leaves an empty hole in the case file.
		if (!drewPhoto)
		{
			float headW = avW * 0.21f;
			float headH = avH * 0.36f;
			drawRect(midX - headW * 0.5f, avY + avH * 0.15f, headW, headH, 112, 92, 68, 255);

			float shW = avW * 0.58f;
			float shH = avH * 0.30f;
			drawRect(midX - shW * 0.5f, avY + avH * 0.58f, shW, shH, 112, 92, 68, 255);
		}

		// No frame lines around the portrait: the photo is drawn taller than the
		// slot, so a border ends up slicing across the face. The paper backdrop
		// already gives the photo an edge.
		(void)avH;

		// ����
		drawText((char*)missionData->targetName, midX, py + H * 0.640f, inkR, inkG, inkB, 255, true, 0.58f, 0.58f);

		// ����
		std::stringstream crimeLine;
		crimeLine << crime;
		drawPosterLines(crimeLine.str().c_str(), px + W * 0.085f, py + H * 0.716f, H * 0.052f, 2, 42, 96, 76, 52);

		// �ײ�����
		drawRect(px + W * 0.10f, pb - H * 0.155f, W * 0.80f, 0.0012f, lnR, lnG, lnB, 190);
		drawText((char*)TF("ui.poster.issuedby", "�����¾����ͨ��"), midX, pb - H * 0.122f, 96, 74, 52, 255, true, 0.215f, 0.215f);
		drawText((char*)posterAreaName(getArea()->name), midX, pb - H * 0.072f, inkR, inkG, inkB, 255, true, 0.34f, 0.34f);
	}
	else
	{
		// ---------- ����ҳ ----------
		drawText((char*)TF("ui.poster.detail.title", "��������"), midX, py + H * 0.108f, inkR, inkG, inkB, 255, true, 0.40f, 0.40f);
		drawRect(px + W * 0.10f, py + H * 0.152f, W * 0.80f, 0.0012f, lnR, lnG, lnB, 190);

		const char* desc = (missionData->description && missionData->description[0])
			? missionData->description : TF("ui.poster.detail.nodesc", "no case file");
		int descLines = drawPosterLines(desc, px + W * 0.085f, py + H * 0.205f, H * 0.066f, 5, 42, inkR + 40, inkG + 36, inkB + 30);
		if (descLines < 1)
		{
			descLines = 1;
		}

		float infoY = py + H * 0.205f + descLines * H * 0.066f + H * 0.052f;
		if (infoY > pb - H * 0.245f)
		{
			infoY = pb - H * 0.245f;
		}

		drawRect(px + W * 0.10f, infoY - H * 0.030f, W * 0.80f, 0.0012f, lnR, lnG, lnB, 175);

		float labelX = px + W * 0.085f;
		float valueX = px + W * 0.38f;

		drawText((char*)TF("ui.poster.detail.station", "��������"), labelX, infoY, 96, 74, 52, 255, false, 0.235f, 0.235f);
		{ std::string s = std::string(posterAreaName(getArea()->name)) + TF("ui.poster.detail.station_suffix", ""); drawText((char*)s.c_str(), valueX, infoY, inkR, inkG, inkB, 255, false, 0.255f, 0.255f); }

		infoY += H * 0.070f;

		drawText((char*)TF("ui.poster.detail.lastseen", "��û����"), labelX, infoY, 96, 74, 52, 255, false, 0.235f, 0.235f);
		{ std::string s = std::string(posterAreaName(getArea()->name)) + TF("ui.poster.detail.lastseen_suffix", ""); drawText((char*)s.c_str(), valueX, infoY, inkR, inkG, inkB, 255, false, 0.255f, 0.255f); }
	}
}

// ---------------------------------------------------------------------------
// �������ͨ�����ԭ�� prop ����ʵʱ��һ������ֽ�档
// ԭ��ģ�� p_pos_wantedalive01x / p_pos_wanteddead01x �� WANTED �����決��
// ytd ��ͼ��ű��Ĳ��ˣ�ֻ�ܸ�һ�㡣
// ---------------------------------------------------------------------------
// Maps a point on the sheet (u: 0 left .. 1 right, v: 0 top .. 1 bottom) to
// the screen through the four projected corners, so anything drawn with it
// follows the paper instead of sitting in a fixed screen rectangle.
static void posterMapPoint(float ax, float ay, float bx, float by,
                           float cx, float cy, float dx, float dy,
                           float u, float v, float* ox, float* oy)
{
	float topX = ax + (bx - ax) * u;
	float topY = ay + (by - ay) * u;
	float botX = cx + (dx - cx) * u;
	float botY = cy + (dy - cy) * u;
	*ox = topX + (botX - topX) * v;
	*oy = topY + (botY - topY) * v;
}

void BaseMissionExecutor::drawPosterOverlay()
{
	if (!poster || !ENTITY::DOES_ENTITY_EXIST(poster) || !missionData)
	{
		return;
	}

	Vector3 center = ENTITY::GET_ENTITY_COORDS(poster, true, true);
	Ped player = PLAYER::PLAYER_PED_ID();
	Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(player, true, true);

		// Only while the player is close enough to read the sheet.
	if (distanceBetween(playerPos, center) > 7.0f)
	{
		return;
	}

	Hash model = ENTITY::GET_ENTITY_MODEL(poster);
	Vector3 dimMin, dimMax;
	GAMEPLAY::GET_MODEL_DIMENSIONS(model, &dimMin, &dimMax);

	float heading = ENTITY::GET_ENTITY_HEADING(poster);
	float rad = heading * 0.0174532925f;
	float cs = cosf(rad);
	float sn = sinf(rad);

		// The paper may face either axis: the thinner one is its depth.
	bool yIsDepth = (dimMax.y - dimMin.y) <= (dimMax.x - dimMin.x);

		// Player position in the sheet's own frame, to pick the near face.
	float px = playerPos.x - center.x;
	float py = playerPos.y - center.y;
	float localX = px * cs + py * sn;
	float localY = -px * sn + py * cs;

	float u0, u1, v0, v1, faceDepth;
	if (yIsDepth)
	{
		u0 = dimMin.x;
		u1 = dimMax.x;
		v0 = dimMin.z;
		v1 = dimMax.z;
		faceDepth = (localY >= 0.0f) ? dimMax.y : dimMin.y;
	}
	else
	{
		u0 = dimMin.y;
		u1 = dimMax.y;
		v0 = dimMin.z;
		v1 = dimMax.z;
		faceDepth = (localX >= 0.0f) ? dimMax.x : dimMin.x;
	}

		// Keep the type clear of the paper edge.
	float insetU = (u1 - u0) * 0.03f;
	float insetV = (v1 - v0) * 0.03f;
	u0 += insetU;
	u1 -= insetU;
	v0 += insetV;
	v1 -= insetV;

	float uu[4] = { u0, u1, u0, u1 };
	float vv[4] = { v1, v1, v0, v0 };
	float sx[4] = { 0 };
	float sy[4] = { 0 };

	int scrW = 1920;
	int scrH = 1080;
	GRAPHICS::GET_SCREEN_RESOLUTION(&scrW, &scrH);
	float invW = (scrW > 0) ? (1.0f / (float)scrW) : (1.0f / 1920.0f);
	float invH = (scrH > 0) ? (1.0f / (float)scrH) : (1.0f / 1080.0f);

	for (int i = 0; i < 4; i++)
	{
		float lx = yIsDepth ? uu[i] : faceDepth;
		float ly = yIsDepth ? faceDepth : uu[i];

		float wx = center.x + lx * cs - ly * sn;
		float wy = center.y + lx * sn + ly * cs;
		float wz = center.z + vv[i];

		if (!GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(wx, wy, wz, &sx[i], &sy[i]))
		{
			return;
		}

		// �еİ汾�������أ��еķ��� 0~1����������Ӧ
		if (fabsf(sx[i]) > 1.5f || fabsf(sy[i]) > 1.5f)
		{
			sx[i] *= invW;
			sy[i] *= invH;
		}
	}

	float ax = sx[0], ay = sy[0];   // ����
	float bx = sx[1], by = sy[1];   // ����
	float cx = sx[2], cy = sy[2];   // ����
	float dx = sx[3], dy = sy[3];   // ����

	if (fabsf(ax - bx) < 0.0004f || fabsf(ay - cy) < 0.0004f)
	{
		return;
	}


	// Nothing is painted behind the words: the model already carries the
	// printed sheet and its frame, so only the localised glyphs go on top.
	float left = ax, right = ax, top = ay, bottom = ay;
	if (bx < left) left = bx;
	if (cx < left) left = cx;
	if (dx < left) left = dx;
	if (bx > right) right = bx;
	if (cx > right) right = cx;
	if (dx > right) right = dx;
	if (by < top) top = by;
	if (cy < top) top = cy;
	if (dy < top) top = dy;
	if (by > bottom) bottom = by;
	if (cy > bottom) bottom = cy;
	if (dy > bottom) bottom = dy;

	float w = right - left;
	float h = bottom - top;
	if (w <= 0.0f || h <= 0.0f)
	{
		return;
	}

	// The glyphs are screen-space, so on a strongly foreshortened sheet they
	// read as floating in front of the board instead of printed on it. Compare
	// the two horizontal edges: on a paper seen head-on they are about equal.
	float topEdge = sqrtf((bx - ax) * (bx - ax) + (by - ay) * (by - ay));
	float botEdge = sqrtf((dx - cx) * (dx - cx) + (dy - cy) * (dy - cy));
	float shortEdge = (topEdge < botEdge) ? topEdge : botEdge;
	float longEdge = (topEdge < botEdge) ? botEdge : topEdge;
	if (longEdge <= 0.0001f || (shortEdge / longEdge) < 0.78f)
	{
		return;
	}

	// Height of the sheet along the middle column: the type shrinks with the
	// paper when it is seen at an angle, which is what keeps it from floating.
	float localH = fabsf((cy + (dy - cy) * 0.5f) - (ay + (by - ay) * 0.5f));
	if (localH < 0.01f)
	{
		return;
	}

	// Placement lives in the ini because the vanilla print sits at fixed spots
	// and covering it is a matter of nudging a number once on the real board.
	// Values are thousandths of the paper height: 0 is the top edge, 1000 the
	// bottom. Re-read now and then so an edit can be tested without rebuilding.
	static int s_tick = 0;
	static int s_titleY = 45, s_nameY = 545, s_rewardY = 640, s_condY = 730, s_hintY = 850;
	static int s_shiftY = 0, s_scalePct = 100;
	if ((s_tick++ % 120) == 0)
	{
		s_titleY = modIniInt("Poster", "FaceTitleY", 45);
		s_nameY = modIniInt("Poster", "FaceNameY", 545);
		s_rewardY = modIniInt("Poster", "FaceRewardY", 640);
		s_condY = modIniInt("Poster", "FaceCondY", 730);
		s_hintY = modIniInt("Poster", "FaceHintY", 850);
		s_shiftY = modIniInt("Poster", "FaceShiftY", 0);
		s_scalePct = modIniInt("Poster", "FaceScalePct", 100);
	}

	float posTitle = (float)(s_titleY + s_shiftY) / 1000.0f;
	float posName = (float)(s_nameY + s_shiftY) / 1000.0f;
	float posReward = (float)(s_rewardY + s_shiftY) / 1000.0f;
	float posCond = (float)(s_condY + s_shiftY) / 1000.0f;
	float posHint = (float)(s_hintY + s_shiftY) / 1000.0f;
	float typeScale = (s_scalePct > 0) ? ((float)s_scalePct / 100.0f) : 1.0f;

	float sTitle = localH * 2.30f * typeScale;
	float sName = localH * 1.35f * typeScale;
	float sSmall = localH * 0.95f * typeScale;
	if (sTitle > 1.05f) sTitle = 1.05f;
	if (sName > 0.72f) sName = 0.72f;
	if (sSmall > 0.50f) sSmall = 0.50f;
	if (sTitle < 0.16f) sTitle = 0.16f;
	if (sName < 0.12f) sName = 0.12f;
	if (sSmall < 0.10f) sSmall = 0.10f;

	float tx = 0.0f, ty = 0.0f;
	posterMapPoint(ax, ay, bx, by, cx, cy, dx, dy, 0.5f, posTitle, &tx, &ty);
	drawText((char*)TF("ui.poster.header", "ͨ����"), tx, ty, 62, 34, 24, 255, true, sTitle, sTitle);


	// ---- ���� / �ͽ� / ���� ----
	posterMapPoint(ax, ay, bx, by, cx, cy, dx, dy, 0.5f, posName, &tx, &ty);
	drawText((char*)missionData->targetName, tx, ty, 48, 32, 22, 255, true, sName, sName);

	std::stringstream reward;
	reward << TF("ui.poster.reward", "�ͽ�") << " $" << scaledReward();
	posterMapPoint(ax, ay, bx, by, cx, cy, dx, dy, 0.5f, posReward, &tx, &ty);
	drawText((char*)reward.str().c_str(), tx, ty, 92, 52, 20, 255, true, sSmall, sSmall);

	const char* condition = missionData->requiredTargetCondition == TargetCondition::Alive
		? TF("ui.cond.alive", "��׽") : TF("ui.cond.deadoralive", "�����");
	posterMapPoint(ax, ay, bx, by, cx, cy, dx, dy, 0.5f, posCond, &tx, &ty);
	drawText((char*)condition, tx, ty, 56, 40, 28, 255, true, sSmall, sSmall);

	posterMapPoint(ax, ay, bx, by, cx, cy, dx, dy, 0.5f, posHint, &tx, &ty);
	drawText((char*)TF("ui.poster.hint", "�߽��鿴ͨ����"), tx, ty, 110, 86, 54, 255, true, sSmall * 0.78f, sSmall * 0.78f);
}

void BaseMissionExecutor::spawnPosterRivals()
{
	// This is the only path that can freeze the game right after a poster is
	// accepted: it spawns horses and riders, and something in that chain parks
	// the script for good. Off by default so the mission is playable; set
	// [Poster] Rivals=1 to bring the ambush back once the cause is fixed.
	if (modIniInt("Poster", "Rivals", 0) == 0)
	{
		return;
	}

	// Hard cap: a second wave must never stack on top of a live one. Nothing
	// used to stop update() from calling this again on every flag flicker.
	{
		int aliveHunters = 0;
		for (size_t i = 0; i < bountyHunters.size(); i++)
		{
			Ped h = bountyHunters[i];
			if (h != 0 && ENTITY::DOES_ENTITY_EXIST(h) && !ENTITY::IS_ENTITY_DEAD(h))
			{
				aliveHunters++;
			}
		}
		if (aliveHunters >= 4)
		{
			log("spawnPosterRivals: skip, a wave is already alive");
			return;
		}
	}

	log("spawnPosterRivals: begin");
	Ped player = PLAYER::PLAYER_PED_ID();
	Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(player, true, 0);

	int count = 2 + (rand() % 2);
	int spawned = 0;
	bool huntersOnHorse = modIniInt("Poster", "HuntersOnHorse", 0) != 0;
	log(huntersOnHorse ? "poster rivals: spawning mounted" : "poster rivals: spawning on foot");

	for (int i = 0; i < count; i++)
	{
		float sideSign = (rand() % 2 == 0) ? -1.0f : 1.0f;
		float sideOffset = sideSign * (8.0f + (float)(rand() % 5));
		float fwdOffset = (float)(rand() % 11) - 5.0f;
		Vector3 pos = positionAheadOfPlayer(fwdOffset, sideOffset);

		if (!huntersOnHorse)
		{
			addBountyHunterOnFoot(pos);
			spawned++;
			continue;
		}
		log("spawnPosterRivals: creating horse");
		Ped horse = createPed((char*)"A_C_Horse_TennesseeWalker_DappleBay", pos);
		if (!horse || !ENTITY::DOES_ENTITY_EXIST(horse))
		{
			log("spawnPosterRivals: horse failed");
			continue;
		}
		log("spawnPosterRivals: horse ok, saddling");

		bountyhorses.push_back(horse);
		giveSaddleToHorse(horse, HorseSaddleHashes::MCCLELLAN_01_STOCK_NEW_SADDLE_002);
		log("spawnPosterRivals: adding rider");
		addBountyHunter(horse, true);
		log("spawnPosterRivals: rider added");
		spawned++;
	}

	if (spawned <= 0)
	{
		return;
	}

	for (vector<Ped>::iterator it = bountyHunters.begin(); it != bountyHunters.end(); ++it)
	{
		if (ENTITY::DOES_ENTITY_EXIST(*it) && !ENTITY::IS_ENTITY_DEAD(*it))
		{
			AI::TASK_COMBAT_PED(*it, player, 0, 16);
		}
	}

	showSubtitle(TF("ui.sub.rivals_poster", "匪帮已经得到消息，正赶来拦你——他们不会让你把人带走。"));
	log("poster rivals spawned");
}

void BaseMissionExecutor::spawnRandomGuards()
{
	if (!target || !ENTITY::DOES_ENTITY_EXIST(target))
	{
		return;
	}

	// Crew size follows the crime: a gang job gets several men, a lone
	// killer gets none or one lookout.
	int guardMin = 0;
	int guardMax = 1;
	guardRangeForMission(missionData ? missionData->id : 0, &guardMin, &guardMax);

	int count = guardMin;
	if (guardMax > guardMin)
	{
		count += rand() % (guardMax - guardMin + 1);
	}
	if (count <= 0)
	{
		return;
	}

	static char* guardModels[] = {
		(char*)"G_M_M_UniBanditos_01",
		(char*)"G_M_M_UniRanchers_01",
		(char*)"G_M_M_UniCriminals_01"
	};

	// Same guard: rivals can be planned while the target is still unspawned.
	Vector3 targetPos = (target != 0 && ENTITY::DOES_ENTITY_EXIST(target))
		? ENTITY::GET_ENTITY_COORDS(target, true, true)
		: ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), true, true);

	for (int i = 0; i < count; i++)
	{
		float ang = (float)(rand() % 360) * 0.0174532925f;
		float radius = 4.0f + (float)(rand() % 9);

		Vector3 pos = targetPos;
		pos.x += cosf(ang) * radius;
		pos.y += sinf(ang) * radius;

		Ped guard = createPed(guardModels[rand() % 3], pos, (float)(rand() % 360));
		if (!guard || !ENTITY::DOES_ENTITY_EXIST(guard))
		{
			continue;
		}

		PED::SET_PED_RELATIONSHIP_GROUP_HASH(guard, GAMEPLAY::GET_HASH_KEY("REL_CRIMINALS"));
		PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(guard, 1);
		pedEquipBestWeapon(guard);
		AI::TASK_COMBAT_PED(guard, PLAYER::PLAYER_PED_ID(), 0, 16);
		// Keep the handle: a guard blip left on the radar after the guards
		// are gone drains the blip pool over a long session.
		guardBlips.push_back(createBlip(guard, BLIP_STYLE_ENEMY));
		randomGuards.push_back(guard);
	}

	if (!randomGuards.empty())
	{
		char guardBuf[96];
	sprintf(guardBuf, "%s%d%s",
		TF("ui.sub.guards.prefix", "Guards: "),
		(int)randomGuards.size(),
		TF("ui.sub.guards.suffix", "."));
	showSubtitle(guardBuf);
	}
}

void BaseMissionExecutor::applyTargetName()
{
	if (!missionData || !missionData->targetName)
	{
		return;
	}

	// A corpse is still the wanted man: skipping dead peds is why a killed
	// target fell back to the generic NPC name. The prompt name is re-applied on
	// a timer, so it also survives whatever the death state resets.
	if (!target || !ENTITY::DOES_ENTITY_EXIST(target))
	{
		return;
	}

	// _SET_PED_PROMPT_NAME: ��д�۽����Ҽ�/ILO���� NPC ʱ��ʾ���������֣�
	// Ĭ���� "Stranger" �� ped ְ��/��ϵ���Ƶ��������֡�
	PED::_0x4A48B6E03BABB4AC(target, (Any*)missionData->targetName);
}

// ---------------------------------------------------------------------------
// A red "WANTED" tag floating over the target. Without it a bounty target in a
// crowded camp looks exactly like every other armed NPC, and the fight ends
// with the wrong man on the ground.
// ---------------------------------------------------------------------------
void BaseMissionExecutor::drawTargetMarker()
{
	// Everything the tag needs comes from the ini and is re-read every two
	// seconds, which is quick enough to tune while playing and avoids a file
	// read on every frame.
	//   TargetMarker        - 1 = show. Accepted in [UI] and in [Poster], because
	//                         it has shipped in both and a wrong section silently
	//                         turned the feature off
	//   TargetMarkerBone    - 0 = fixed height above the feet (default)
	//   TargetMarkerHeight  - extra height above that bone, in cm
	//   TargetMarkerOffsetY - screen nudge, per mille of screen height
	//   TargetMarkerScale   - font size in percent
	struct MarkerTuning
	{
		int   on;
		int   bone;
		float above;
		float offsetY;
		float scale;
	};
	static MarkerTuning s_tune = { 1, 0, 1.15f, 0.0f, 0.30f };
	static int s_tuneAt = -100000;
	int tuneNow = GAMEPLAY::GET_GAME_TIMER();
	if (tuneNow - s_tuneAt > 2000)
	{
		s_tuneAt = tuneNow;
		s_tune.on      = modIniInt("UI", "TargetMarker", -1);
		if (s_tune.on < 0)
		{
			s_tune.on = modIniInt("Poster", "TargetMarker", 0);
		}
		s_tune.bone    = modIniInt("UI", "TargetMarkerBone", 0);
		s_tune.above   = (float)modIniInt("UI", "TargetMarkerHeight", 115) / 100.0f;
		s_tune.offsetY = (float)modIniInt("UI", "TargetMarkerOffsetY", 0) / 1000.0f;
		s_tune.scale   = (float)modIniInt("UI", "TargetMarkerScale", 30) / 100.0f;
	}
	if (s_tune.on == 0)
	{
		return;
	}

	if (!missionData || !target || !ENTITY::DOES_ENTITY_EXIST(target))
	{
		return;
	}
	if (stage < BountyMissionStage::LocateTarget || stage > BountyMissionStage::HandOverTarget)
	{
		return;
	}
	if (ENTITY::IS_ENTITY_DEAD(target))
	{
		return;
	}

	// Once they are roped the tag has done its job: dragging a hogtied
	// target around with a name floating over them reads wrong, and the
	// target blip already shows where they are.
	if (isPedHogtied(target))
	{
		return;
	}

	Ped player = PLAYER::PLAYER_PED_ID();
	Vector3 targetCoords = ENTITY::GET_ENTITY_COORDS(target, true, true);
	Vector3 playerCoords = ENTITY::GET_ENTITY_COORDS(player, true, true);

	// Closer trigger before the target is spotted, wider once the chase is on.
	float maxDist = (stage == BountyMissionStage::LocateTarget) ? 22.0f : 45.0f;
	float dist = distanceBetween(playerCoords, targetCoords);
	if (dist > maxDist)
	{
		return;
	}

	// Simple anchor by default: a fixed height above the entity origin, which is
	// the setup that already looked right over the target's head. Bone snapping
	// is opt-in through TargetMarkerBone (55120 = head) for anyone who wants the
	// tag to follow a tall or mounted ped exactly; a wrong id is rejected here
	// (zeros or a point metres away) and the plain anchor is used instead.
	Vector3 anchor = targetCoords;
	if (s_tune.bone > 0)
	{
		Vector3 head = PED::GET_PED_BONE_COORDS(target, s_tune.bone, 0.0f, 0.0f, 0.0f);
		float ddx = head.x - targetCoords.x;
		float ddy = head.y - targetCoords.y;
		float ddz = head.z - targetCoords.z;
		if ((ddx * ddx + ddy * ddy) < 4.0f && ddz > 0.3f && ddz < 3.0f)
		{
			anchor = head;
		}
	}
	anchor.z += s_tune.above;

	float sx = 0.0f;
	float sy = 0.0f;
	if (!GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(anchor.x, anchor.y, anchor.z, &sx, &sy))
	{
		return;
	}

	// This build answers in screen PIXELS, not the 0..1 range DRAW_TEXT takes.
	// Without the conversion every result failed the window check below, which
	// is why the tag never showed even with the switch on. drawPosterOverlay
	// already normalises the same way.
	if (fabsf(sx) > 1.5f || fabsf(sy) > 1.5f)
	{
		int scrW = 1920;
		int scrH = 1080;
		GRAPHICS::GET_SCREEN_RESOLUTION(&scrW, &scrH);
		if (scrW > 0 && scrH > 0)
		{
			sx /= (float)scrW;
			sy /= (float)scrH;
		}
	}

	// Screen-space nudge, applied after normalisation so it does not depend on
	// resolution either.
	sy += s_tune.offsetY;

	if (sx < 0.02f || sx > 0.98f || sy < 0.02f || sy > 0.96f)
	{
		return;
	}

	// One line per session: enough to tell a projection problem apart from a
	// drawing problem the next time the tag stays invisible.
	static bool s_markerLogged = false;
	if (!s_markerLogged)
	{
		s_markerLogged = true;
		std::stringstream sm;
		sm << "target marker: dist=" << (int)dist
		   << " screen=" << sx << "," << sy
		   << " stage=" << (int)stage;
		log(sm.str());
	}

	// Every piece of this line is translatable: the tag wording, the name (a
	// per mission lang entry) and the distance unit. A pack that wants no
	// prefix can set ui.target.wanted to an empty value.
	char distBuf[32];
	snprintf(distBuf, sizeof(distBuf), TF("ui.target.distance", "%dm"), (int)(dist + 0.5f));

	const char* tag = TF("ui.target.wanted", "ͨ��");
	const char* name = missionData->targetName ? missionData->targetName : "";

	char label[192];
	if (tag[0] == '\0')
	{
		sprintf(label, "%s  %s", name, distBuf);
	}
	else
	{
		sprintf(label, "%s  %s  %s", tag, name, distBuf);
	}

	// Shadow pass first so the tag stays readable against a bright sky.
	drawText(label, sx + 0.0012f, sy + 0.0012f, 0, 0, 0, 190, true, s_tune.scale, s_tune.scale);
	drawText(label, sx, sy, 214, 46, 34, 255, true, s_tune.scale, s_tune.scale);
}
void BaseMissionExecutor::decorateTarget()
{
	applyTargetName();
	targetBlip = createBlip(target, BLIP_STYLE_ENEMY);
}
