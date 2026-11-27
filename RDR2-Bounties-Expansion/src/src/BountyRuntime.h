#pragma once

// One case file at a time.
//
// The mod keeps a poster on every town board, but only one bounty may be
// carried at once: two active executors would spawn two target areas, two
// sets of prompts and two panels, and the player could not tell which one the
// review key belongs to. The slot is claimed when a poster is taken down and
// released when the paper goes back on the board, the bounty is handed in or
// it fails.
//
// The id lives in a function-local static, which gives every translation unit
// the same instance without a separate definition file.
inline int& activeBountyIdRef()
{
	static int s_activeBountyId = 0;
	return s_activeBountyId;
}

inline int activeBountyId()
{
	return activeBountyIdRef();
}

// True when somebody else already holds the slot.
inline bool isBountyBusy(int missionId)
{
	int current = activeBountyIdRef();
	return current != 0 && current != missionId;
}

// Takes the slot for this mission. Returns false when another one owns it.
inline bool claimBounty(int missionId)
{
	int& current = activeBountyIdRef();
	if (current == 0 || current == missionId)
	{
		current = missionId;
		return true;
	}
	return false;
}

inline void releaseBounty(int missionId)
{
	int& current = activeBountyIdRef();
	if (current == missionId)
	{
		current = 0;
	}
}
