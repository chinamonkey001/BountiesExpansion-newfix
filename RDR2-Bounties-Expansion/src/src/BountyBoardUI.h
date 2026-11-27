#pragma once

// Wrapper for the game's own bounty board UI (the "bounty_board" flowblock).
//
// BountiesExpansion 1.4.4 never drew a poster panel itself. It filled this
// flowblock through databinding and then pushed the poster object into the
// engine's entity mask, so the words and the portrait were rendered by the
// game directly onto the paper, with correct perspective from any angle.
//
// Full sequence recovered from the 1.4.4 binary:
//
//   _UIFLOWBLOCK_REQUEST(0x71F0EF31)
//   _DATABINDING_ADD_DATA_CONTAINER_FROM_PATH("", "bounty_board_data")
//   _DATABINDING_ADD_DATA_CONTAINER(root, "Poster0")
//     isVisible / isDiffVisible / type / difficulty / price / body /
//     header / name / tex / txd
//   _UIFLOWBLOCK_ENTER(fb, joaat("bounty_board"))
//   UI_STATE_MACHINE_CREATE(0xAA5FE152, fb)
//   REQUEST_MODEL(joaat("mp005_p_mp_bountyposter01x")) + wait
//   ENABLE_ENTITYMASK()
//   ... _ADD_ENTITY_TO_ENTITY_MASK_WITH_INTENSITY(poster, 2, 1.0f)
//
// The poster model is a blank sheet; without the mask it renders as a plain
// grey board. That is why spawning it alone looked empty in earlier builds.
class BountyBoardUI
{
public:
	// Requests the flowblock at startup and resets the state machine before the
	// game (or a leftover mod) installs one. Called once from initialize().
	//
	// Timing is the whole point: this build keeps a state machine alive for the
	// session, and once it exists UI_STATE_MACHINE_CREATE refuses to build
	// another one (it answers 0), so nothing can be reset later. ZEEPosterFix -
	// the only thing that makes a poster print here - does exactly this at
	// startup: create, then destroy, while the slot is still free.
	static void resetStateMachine();

	// Re-creates and immediately destroys the state machine after the contents
	// have been written. The engine rebuilds it on the next frame and re-reads
	// the databinding, which is the only moment the sheet actually picks the
	// values up on this build - the same thing ZEEPosterFix triggers, except it
	// happens right after 1.4.4 has filled the tree.
	static void rebind();

	// Requests the flowblock, builds the databinding tree, waits for the
	// flowblock and the poster model to load, enters the block and enables the
	// entity mask. Blocking, but every wait is bounded. Returns false when the
	// UI is unavailable and the caller must fall back.
	static bool open();

	// Pushes the poster contents into the databinding tree. Call after open().
	// portraitHash is the joaat of a pedshot texture, pass 0 for no portrait.
	static void fill(const char* name, const char* header, const char* body,
	                 const char* price, unsigned int portraitHash, int portraitIndex = 1);

	// Same as fill(), but only when the tree is currently owned by another
	// mission. There is a single global databinding tree, so the poster the
	// player is actually standing in front of has to claim it.
	// portraitIndex is the final texture number (portrait_<n>), resolved by the
	// caller from the criminal's description; one face belongs to one wanted
	// criminal instead of one per town.
	static void fillFor(UINT64 owner, const char* name, const char* header,
	                    const char* body, const char* price,
	                    unsigned int portraitHash, int portraitIndex = 1);
	static bool needsFill(UINT64 owner);

	// Picks the paper model that actually exists in this game build. 1.4.4 uses
	// the blank MP paper, but that asset is not guaranteed to be present in
	// story mode, so the vanilla printed poster is used as a last resort (it
	// still gets the mask, and the mask replaces the baked artwork).
	// Returns a pointer to a static string, cast it to char* for createProp().
	static const char* pickPosterModel(bool deadOrAlive);

	// Projects the poster object through the entity mask, so the flowblock UI
	// paints its contents on the paper. Returns false if the UI is not open.
	static bool attach(Object entity);

	// 1.4.4 does not leave the poster as a free standing object: the flowblock
	// only renders onto a prop that belongs to an "inspect poster" scenario
	// point (that is how the engine knows it is a readable document). It finds
	// the vanilla board's scenario point and swaps its PrimaryItem prop. Our
	// boards stand on their own coordinates, so we create the scenario point
	// ourselves and associate the paper with it.
	static bool createPosterScenario(float x, float y, float z, float heading,
	                                 bool deadOrAlive);
	static bool associate(Object entity);
	static void deleteScenario();

	// Per frame keep-alive for an attached poster: entity mask switches and the
	// mask itself are re-applied, and the databinding is rewritten periodically.
	// The game drops both when the UI app that owns the render target stops, so
	// one shot at spawn time is not enough.
	static bool tick(Object entity);

	// Drops the object from the entity mask again.
	static void detach(Object entity);

	// Closes the UI if the game still has it running.
	static void close();

	// True when open() has succeeded at least once, i.e. the native poster is
	// usable in this session.
	// Native paper is opt-in through BountiesExpansion/BountiesExpansion.ini
	// (NativeUI=1). Everything below is a no-op while it is off.
	static bool configEnabled();

	// Returns the name of the portrait dictionary that is actually loaded, or
	// NULL while none is. Only reports - never requests - so callers can draw
	// without flooding the streamer.
	static const char* loadedPortraitDict();

	// Same lookup, but it also asks the streamer for the dictionary - throttled
	// to one request every couple of seconds. drawSprite() is a pure draw now
	// (unthrottled requests in there used to flood the streamer and freeze the
	// game), so any caller that may run before a board has been filled needs
	// this variant or the portrait never appears.
	static const char* portraitDictForDraw();


	static float configDistance();

	// True only while the game board UI app is actually running. The flowblock
	// can enter and still never spawn that app, and then the blank sheet has
	// nothing to print on - which is what made the board look empty.
	static bool boardAppReady();

	static bool isAvailable();
	static bool isShowing();
	static int lastError();

	// Writes one line about the current state into the mod log, for diagnosis.
	static void logState();

private:
	static bool buildTree();
	static bool loadPosterModel();
	static void writeData();
	static void logMask(Object entity);
};
