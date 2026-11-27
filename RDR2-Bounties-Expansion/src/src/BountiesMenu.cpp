#pragma execution_character_set("utf-8")
#include "Main.h";

BountiesMenu::BountiesMenu()
{
	mode = BountiesMenuMode::Closed;
	closePrompt = new Prompt("关闭", GAMEPLAY::GET_HASH_KEY("INPUT_FRONTEND_CANCEL"), PromptMode::Standard);
	closePrompt->hide();
}

void BountiesMenu::open()
{
	mode = BountiesMenuMode::MissionsList;
}

void BountiesMenu::close()
{
	mode = BountiesMenuMode::Closed;
}

void BountiesMenu::showMissionDetails(BountyMissionData* missionData)
{

	if (displayedMission)
	{
		displayedMission = missionData;
		mode = BountiesMenuMode::MissionDetails;
	}
}

void BountiesMenu::update()
{
	if (mode == BountiesMenuMode::Closed)
	{
		closePrompt->hide();
		return;
	}

	closePrompt->show();
	if (closePrompt->isActivatedByPlayer())
	{
		mode = BountiesMenuMode::Closed;
	}

	switch (mode)
	{
	case BountiesMenuMode::MissionDetails:
		printMission();
		break;
	case BountiesMenuMode::MissionsList:

		break;
	}
}

void BountiesMenu::printMission()
{
	drawTitle(displayedMission->targetName);
	drawHeader("尚未结案的");

	float y = 0.170f;
	float x = menuX + MENU_WIDTH / 2;
	float spacing = 0.038f;
	drawRect(menuX, 0.124f, MENU_WIDTH, 0.45, 0, 0, 0, MENU_ALPHA);

	drawText((char*)"悬  赏", x, y, 255, 255, 255, 255, true, 0.5f, 0.5f);
	drawText((char*)displayedMission->rewardStr, x, y = y + spacing, 255, 255, 255, 255, true, 0.5f, 0.5f);
	drawText((char*)displayedMission->targetName, x, y = y + spacing, 255, 255, 255, 255, true, 0.5f, 0.5f);
	drawText(deadOrAlive(displayedMission->requiredTargetCondition), x, y = y + spacing, 255, 255, 255, 255, true, 0.35f, 0.35f);
	drawText((char*)displayedMission->description, menuX + 0.004, y = y + spacing * 2, 255, 255, 255, 255, false, 0.35f, 0.35f);
}

void BountiesMenu::printMissionsList()
{
	drawTitle("赏金狩猎");

	drawHeader("收集海报:");
	float y = 0.170f;
	float x = menuX + MENU_WIDTH / 2;
	float spacing = 0.032f;
	drawRect(menuX, 0.124f, MENU_WIDTH, 0.45, 0, 0, 0, MENU_ALPHA);

	
}

char* BountiesMenu::deadOrAlive(TargetCondition cond)
{
	if (cond == TargetCondition::Alive)
		return "必须活捉.";
	return "生死不论.";
}