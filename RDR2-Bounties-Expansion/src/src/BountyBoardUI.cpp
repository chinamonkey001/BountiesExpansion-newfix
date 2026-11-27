#include "Main.h"
#include "ModConfig.h"

// ---------------------------------------------------------------------------
// Native hashes. Every one of these was read straight out of the
// BountiesExpansion 1.4.4 module (mov r64,imm64 right before the nativeInit
// call) and then matched against the RDR3 native database, so nothing here is
// guessed.
// ---------------------------------------------------------------------------
namespace
{
	// The native paper route is opt-in: masking an entity while the flowblock
	// UI app is not running froze the game at the end of the loading screen,
	// so it only runs when the ini asks for it and the player is at the board.
	bool  s_cfgLoaded = false;
	bool  s_cfgEnabled = false;
	float s_cfgDistance = 25.0f;
	bool  s_disabledLogged = false;

	void loadConfigOnce()
	{
		if (s_cfgLoaded)
		{
			return;
		}
		s_cfgLoaded = true;

		// GetPrivateProfileIntA is (section, key, default, file). The file path
		// is built from the game exe so it works no matter what the current
		// directory happens to be when the script boots.
		char ini[MAX_PATH];
		char exePath[MAX_PATH];
		ini[0] = '\0';
		if (GetModuleFileNameA(NULL, exePath, MAX_PATH) > 0)
		{
			char* lastSlash = strrchr(exePath, '\\');
			if (lastSlash != NULL)
			{
				lastSlash[1] = '\0';
				sprintf(ini, "%sBountiesExpansion\\BountiesExpansion.ini", exePath);
			}
		}
		if (ini[0] == '\0' || GetFileAttributesA(ini) == INVALID_FILE_ATTRIBUTES)
		{
			sprintf(ini, "BountiesExpansion/BountiesExpansion.ini");
		}

		s_cfgEnabled = GetPrivateProfileIntA("Poster", "NativeUI", 0, ini) != 0;

		int dist = GetPrivateProfileIntA("Poster", "NativeUIDistance", 25, ini);
		if (dist < 5)  { dist = 5; }
		if (dist > 80) { dist = 80; }
		s_cfgDistance = (float)dist;

		std::stringstream ss;
		ss << "bounty board ui: config file=" << ini
		   << " native=" << (s_cfgEnabled ? 1 : 0)
		   << " distance=" << dist;
		log(ss.str());
	}

	// UISTATEMACHINE
	const UINT64 N_FLOWBLOCK_REQUEST = 0xC0081B34E395CE48ULL;
	const UINT64 N_FLOWBLOCK_LOADED  = 0x10A93C057B6BD944ULL;
	const UINT64 N_FLOWBLOCK_ENTER   = 0x3B7519720C9DCB45ULL;
	const UINT64 N_SM_EXISTS         = 0x5D15569C0FEBF757ULL;
	const UINT64 N_SM_CREATE         = 0x4C6F2C4B7A03A266ULL;
	const UINT64 N_SM_DESTROY        = 0x4EB122210A90E2D8ULL;
	const UINT64 N_SM_DESTROY_CLEAR  = 0x2738D68D2B4E09E7ULL;

	// DATABINDING
	const UINT64 N_ADD_CONTAINER_PATH = 0x0C827D175F1292F4ULL;
	const UINT64 N_ADD_CONTAINER      = 0xEB4F9A3537EEABCDULL;
	const UINT64 N_ADD_BOOL           = 0x58BAA5F635DA2FF4ULL;
	const UINT64 N_ADD_INT            = 0x307A3247C5457BDEULL;
	const UINT64 N_ADD_STRING         = 0x617FCA1C5652BBADULL;
	const UINT64 N_ADD_HASH           = 0x8538F1205D60ECA6ULL;
	const UINT64 N_WRITE_BOOL         = 0xAB888B4B91046770ULL;
	const UINT64 N_WRITE_INT          = 0x335C3F6B3766B8D9ULL;
	const UINT64 N_WRITE_STRING       = 0xE1BD342F2872AEE9ULL;
	const UINT64 N_WRITE_HASH_STRING  = 0xACDEF586BD71B1FDULL;

	// "From parent" writes address the entry by name inside a container instead
	// of by the id ADD handed back. They are the only ones that still work once
	// the state machine has been destroyed and rebuilt: the ids point into the
	// old tree, which is why 1.4.4 on its own keeps painting a blank sheet.
	const UINT64 N_GET_CONTAINER_PATH  = 0x0C827D175F1292F2ULL;
	const UINT64 N_GET_CONTAINER_CHILD = 0x0C827D175F1292F3ULL;
	const UINT64 N_WRITE_STR_PARENT    = 0x4FF713B2F17A391EULL;
	const UINT64 N_WRITE_HASH_PARENT   = 0x0971F04E1EAA7AE8ULL;
	const UINT64 N_WRITE_INT_PARENT    = 0x9EFA98238BA08FC4ULL;
	const UINT64 N_WRITE_BOOL_PARENT   = 0xBDFE546E4C2D0E21ULL;
	const UINT64 N_READ_STRING_PARENT  = 0x6323AD277C4A2AFBULL;
	const UINT64 N_READ_BOOL_PARENT    = 0xA8EDE09FE07BD77FULL;
	const UINT64 N_READ_INT_PARENT     = 0xFFC566A4801F6B40ULL;

	// UIAPPS
	const UINT64 N_UIAPP_RUNNING     = 0x4E511D093A86AD49ULL;
	const UINT64 N_UIAPP_CLOSE       = 0x2FF10C9C3F92277EULL;
	const UINT64 N_UIAPP_CAN_LAUNCH  = 0xE555EC27D65EDE80ULL;

	// GRAPHICS (entity mask: this is what paints the UI onto the paper)
	const UINT64 N_ENABLE_ENTITYMASK   = 0xFAAD23DE7A54FC14ULL;
	const UINT64 N_MASK_ADD_INTENSITY  = 0x958DEBD9353C0935ULL;
	const UINT64 N_MASK_REMOVE         = 0x56A786E87FF53478ULL;
	const UINT64 N_GET_MASK_LAYERS     = 0xE8A8378BF651079CULL;

	// TASK (scenario points: this is what makes the engine treat the paper as
	// an inspectable document and render the flowblock onto it)
	const UINT64 N_CREATE_SCENARIO_POINT    = 0x94B745CE41DB58A1ULL;
	const UINT64 N_SCENARIO_ASSOCIATE_PROP  = 0x8360C47380B6F351ULL;
	const UINT64 N_SCENARIO_DISASSOCIATE    = 0x6EF4E31B4D5D2DA0ULL;
	const UINT64 N_SCENARIO_DELETE          = 0x81948DFE4F5A0283ULL;
	const UINT64 N_SCENARIO_SET_ACTIVE      = 0xEEE4829304F93EEEULL;
	const UINT64 N_SCENARIO_GET_PROP        = 0x295514F198EFD0CAULL;

	// STREAMING
	const UINT64 N_REQUEST_MODEL   = 0xFA28FE3A6246FC30ULL;
	const UINT64 N_HAS_MODEL_LOADED = 0x1283B8B89DD5D1B6ULL;
	const UINT64 N_MODEL_NO_LONGER_NEEDED = 0x4AD96EF928BD4F9AULL;
	const UINT64 N_IS_MODEL_IN_CDIMAGE    = 0xD6F3B6D7716CFF8EULL;
	const UINT64 N_IS_MODEL_VALID         = 0x392C8D8E07B70EFCULL;

	// GAMEPLAY
	const UINT64 N_HASH_KEY = 0xFD340785ADF8CFB7ULL;
	const UINT64 N_DECOR_SET_INT = 0xE88F4D7F52A6090FULL;

	// NETWORK / GRAPHICS: the portrait. This is the piece 1.4.4 uses and the
	// reason ZEEPosterFix can put a face on the sheet: the flowblock samples a
	// texture whose name the network layer hands back, and a tex/txd hash that
	// points at nothing makes it draw an entirely blank sheet - no portrait and
	// no letters either.
	const UINT64 N_TXD_EXIST = 0x7332461FC59EB7ECULL;   // (const char*) -> BOOL
	const UINT64 N_TXD_LOADED = 0x54D6900929CCF162ULL;  // (const char*) -> BOOL
	const UINT64 N_TXD_REQUEST = 0xC1BA29DF5631B0F8ULL; // (const char*)


	const UINT64 N_PEDSHOT_BACKUP = 0x356F9FB0698C1FEBULL;  // -> const char*
	const UINT64 N_PEDSHOT_AVAILABLE = 0xAF6E67D073D2DCE2ULL;
	const UINT64 N_PEDSHOT_INIT_CLEANUP = 0x55285F885F662169ULL;    // ()
	const UINT64 N_PEDSHOT_SET_TYPE = 0x196D3ACBEBA4A44BULL;        // (int)
	const UINT64 N_PEDSHOT_GENERATE = 0xD9C24F53631F2372ULL;        // (const char*, Ped, int) -> BOOL
	const UINT64 N_PEDSHOT_FINISH_CLEANUP = 0xC2B8164C3BE871A4ULL;  // ()
	const UINT64 N_PEDSHOT_PREV_CLEANUP = 0x3E2FDDBE435A8787ULL;     // ()
	const UINT64 N_NETWORK_PERSONA_WRITE_LOCAL = 0x2A48D9567940598FULL; // (const char*,int,int,int) -> BOOL
	const UINT64 N_NETWORK_UPLOAD_PENDING = 0xA21E3BAD0A42D199ULL;   // () -> BOOL
	const UINT64 N_IS_PED_READY_TO_RENDER = 0xA0BC8FAED8CFEB3CULL;   // (Ped) -> BOOL

	// 1.4.4 renders the portrait into this dictionary before it fills the
	// databinding; the name is read straight out of its binary at 0x1800C4DE8.
	// Reading the texture name without ever producing the texture hands the
	// flowblock an empty sampler, and an empty sampler makes it skip the whole
	// draw - the sheet stays white and the letters go with the portrait.
	const char* PEDSHOT_TARGET = "MINIGAME_PROFILE_PHOTO";

	// 1.4.4 stamps every poster with this decor right after masking it.
	const char* DECOR_LETTER_ITEM = "letter_item";
	const char* DECOR_DOCUMENT    = "DOCUMENT_TEST_BOUNTY_POSTER";

	// Constants taken verbatim from 1.4.4.
	const UINT32 FLOWBLOCK_HASH = 0x71F0EF31;
	const UINT32 UIAPP_HASH     = 0xDBEBF573;
	const UINT64 SM_HASH        = 0xFFFFFFFFAA5FE152ULL;
	const int    MASK_LAYER     = 2;      // 1.4.4 always passes layer 2

	const char* POSTER_MODEL = "mp005_p_mp_bountyposter01x";

	// Story mode does not necessarily stream the MP paper, so the vanilla
	// printed posters double as a fallback: the entity mask replaces their
	// baked artwork with the flowblock output, so the text can still come out
	// localised even though the base model ships with English art.
	const char* VANILLA_ALIVE = "p_pos_wantedalive01x";
	const char* VANILLA_DEAD  = "p_pos_wanteddead01x";

	// 1.4.4 writes joaat("bounty_posters.ped_picture") into both tex and txd:
	// the flowblock samples the portrait from that texture dictionary. Writing
	// zero here leaves the flowblock with nothing to sample, which is one of
	// the reasons the paper came out empty.
	// The portrait is a (dictionary, texture) pair. 1.4.4 can write one
	// combined hash into both slots because it generates the pedshot itself,
	// so its dictionary and texture share a name. We do not generate one, so
	// the pair has to be spelled out. A hash pointing at a texture that does
	// not exist makes the flowblock sample nothing and the whole poster then
	// draws as a bare sheet - which is exactly what happened.
	// "bounty_posters" does not exist in this build (the probe returned
	// exist=0 after 60 frames of waiting), and a poster that samples a
	// missing texture draws nothing at all - not even the text. This pair
	// is the one the script menu already paints with, so it is guaranteed
	// to be resident. It is a diagnostic stand-in until the poster gets a
	// real portrait.
	const char* PORTRAIT_TXD = "generic_textures";
	const char* PORTRAIT_TEX = "menu_header_1a";

	// The scenario type the vanilla bounty boards use. Creating one of these and
	// attaching the paper to it is what makes the engine render the flowblock
	// on the prop instead of showing a blank sheet.
	const char* SCENARIO_ALIVE = "WORLD_PLAYER_INSPECT_POSTER_144CM_w32_2_h48_2_Fold_vh_p_cs_wantedAlive01x";
	const char* SCENARIO_DEAD  = "WORLD_PLAYER_INSPECT_POSTER_144CM_w32_2_h48_2_Fold_vh_p_cs_wantedDead01x";
	const char* SCENARIO_SLOT  = "PrimaryItem";

	int    s_scenario = 0;

	UINT64 s_flowblock = 0;
	UINT64 s_poster = 0;
	UINT64 s_isVisible = 0;
	UINT64 s_isDiffVisible = 0;
	UINT64 s_type = 0;
	UINT64 s_difficulty = 0;
	UINT64 s_price = 0;
	UINT64 s_body = 0;
	UINT64 s_header = 0;
	UINT64 s_name = 0;
	UINT64 s_tex = 0;
	UINT64 s_txd = 0;

	UINT64 s_owner = 0;
	UINT64 s_attached = 0;
	bool s_showing = false;
	bool s_failed = false;
	bool s_available = false;
	int  s_error = 0;
	int  s_frames = 0;

	// Last contents, kept so the databinding can be rewritten periodically.
	std::string s_lastName;
	std::string s_lastHeader;
	std::string s_lastBody;
	std::string s_lastPrice;
	unsigned int s_lastPortrait = 0;
	int s_portraitIndex = 1;

	// Portrait resolution state. The pedshot texture does not exist until the
	// network layer has produced it, so the name is polled on every rewrite and
	// cached once it shows up.
	UINT64 s_shotHash = 0;
	int    s_shotTries = 0;
	bool   s_shotLogged = false;
	bool   s_shotGenerated = false;
	UINT64 s_shotPed = 0;
	int    s_readbackLogs = 0;
	UINT32 s_writtenTxd = 0xFFFFFFFFu;
	UINT32 s_writtenTex = 0xFFFFFFFFu;

	inline UINT64 Ptr(const char* s) { return (UINT64)(uintptr_t)s; }
	inline UINT64 Id(UINT64 v) { return (UINT64)(UINT32)v; }

	// 1.4.4 calls _REQUEST_PEDSHOT_TEXTURE_LOCAL_BACKUP_DOWNLOAD(0, 1) and gets
	// the texture name back, then writes joaat(name) into BOTH tex and txd. A
	// hash with no texture behind it drops the whole flowblock render - the
	// sheet goes white and the letters go with it - which is exactly the blank
	// poster. Hardcoding a stand-in texture therefore cannot work; the name has
	// to come from the engine.
	void resolvePedshot()
	{
		if (s_shotHash != 0)
		{
			return;
		}
		if (s_shotTries >= 80)      // ~60 s of rewrites, then stop asking
		{
			return;
		}
		s_shotTries++;

		// The texture has to exist before its name is worth writing: the
		// flowblock drops the entire draw when the sampler is empty, which
		// takes the letters down with the portrait. 1.4.4 runs this exact
		// sequence (against a cloned target ped); until the real target
		// portrait is wired up, the player stands in so the sheet at least
		// has a usable sampler.
		if (!s_shotGenerated && modIniInt("Poster", "GeneratePedshot", 0) != 0)
		{
			s_shotGenerated = true;

			Ped player = PLAYER::PLAYER_PED_ID();
			BOOL available = invoke<BOOL>(N_PEDSHOT_AVAILABLE);
			if (!available && player && ENTITY::DOES_ENTITY_EXIST(player))
			{
				// The full 1.4.4 chain. Generating alone only fills a buffer in
				// memory: the texture name stays empty until
				// _NETWORK_PERSONA_PHOTO_WRITE_LOCAL has pushed it into the local
				// photo store, which is why every earlier "generate ok=1" attempt
				// still read back an empty name.
				int readyGuard = 0;
				while (!invoke<BOOL>(N_IS_PED_READY_TO_RENDER, (UINT64)player) && readyGuard < 120)
				{
					WAIT(0);
					readyGuard++;
				}

				s_shotPed = (UINT64)player;
				invoke<Void>(N_PEDSHOT_PREV_CLEANUP);
				invoke<Void>(N_PEDSHOT_SET_TYPE, (UINT64)0);
				BOOL ok = invoke<BOOL>(N_PEDSHOT_GENERATE, Ptr(PEDSHOT_TARGET),
				                       (UINT64)player, (UINT64)0);
				BOOL wrote = invoke<BOOL>(N_NETWORK_PERSONA_WRITE_LOCAL, Ptr(PEDSHOT_TARGET),
				                          (UINT64)0, (UINT64)1, (UINT64)0);

				int upGuard = 0;
				while (invoke<BOOL>(N_NETWORK_UPLOAD_PENDING) && upGuard < 120)
				{
					WAIT(0);
					upGuard++;
				}

				std::stringstream sg;
				sg << "bounty board ui: pedshot generate target=" << PEDSHOT_TARGET
				   << " ped=" << (UINT32)player
				   << " ready=" << readyGuard
				   << " ok=" << (ok ? 1 : 0)
				   << " writeLocal=" << (wrote ? 1 : 0)
				   << " uploadWait=" << upGuard;
				log(sg.str());
			}
		}

		// PEDSHOT_IS_AVAILABLE stays 0 in story mode even after a successful
		// generate (the log shows ok=1 and available=0 forever), so waiting on
		// it only blocks the portrait. The name is taken straight away: the
		// flowblock tolerates an empty sampler - the letters already print with
		// txd=0 - so a stale name cannot make the sheet worse.
		const char* name = invoke<const char*>(N_PEDSHOT_BACKUP, (UINT64)0, (UINT64)1);
		if (name == NULL || IsBadReadPtr(name, 1) || name[0] == '\0')
		{
			// The ped index may be what the call actually wants; 0 only means
			// "local player" on some builds.
			if (s_shotPed != 0)
			{
				name = invoke<const char*>(N_PEDSHOT_BACKUP, s_shotPed, (UINT64)1);
			}
		}
		if (name == NULL || IsBadReadPtr(name, 1) || name[0] == '\0')
		{
			// The photo is there (available=1) but the name lookup keeps coming
			// back empty, so the dictionary the generate call wrote into is used
			// directly - 1.4.4 itself ends up with one hash in both tex and txd.
			if (s_shotGenerated && s_shotPed != 0)
			{
				UINT32 dictHash = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr(PEDSHOT_TARGET));
				if (dictHash != 0)
				{
					s_shotHash = dictHash;
					std::stringstream sf;
					sf << "bounty board ui: pedshot falling back to dictionary \""
					   << PEDSHOT_TARGET << "\" hash=" << dictHash;
					log(sf.str());
					return;
				}
			}

			// Every attempt is logged for the first 20 tries: the name sometimes
			// shows up only after the generate call has settled, and a silent
			// retry loop hides whether that ever happens.
			if (s_shotTries <= 20)
			{
				BOOL avail = invoke<BOOL>(N_PEDSHOT_AVAILABLE);
				std::stringstream ss;
				ss << "bounty board ui: pedshot name empty (try " << s_shotTries
				   << ", available=" << (avail ? 1 : 0)
				   << ", generated=" << (s_shotGenerated ? 1 : 0) << ")";
				log(ss.str());
			}
			return;
		}

		UINT32 h = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr(name));
		if (h == 0)
		{
			return;
		}

		s_shotHash = h;
		std::stringstream ss;
		ss << "bounty board ui: pedshot texture \"" << name << "\" hash=" << h;
		log(ss.str());
	}
}

const char* BountyBoardUI::loadedPortraitDict()
{
	if (invoke<BOOL>(N_TXD_LOADED, Ptr("be_portraits_nya")))
	{
		return "be_portraits_nya";
	}
	if (invoke<BOOL>(N_TXD_LOADED, Ptr("be_portraits")))
	{
		return "be_portraits";
	}
	return NULL;
}

const char* BountyBoardUI::portraitDictForDraw()
{
	const char* ready = loadedPortraitDict();
	if (ready != NULL)
	{
		return ready;
	}

	// Ask at most every two seconds. Requesting an already resident dictionary
	// is a no-op, so a slow poll is safe; a per-frame request is what used to
	// back the streamer up and stall the game.
	static DWORD lastRequest = 0;
	DWORD now = GetTickCount();
	if (lastRequest == 0 || (now - lastRequest) > 2000)
	{
		lastRequest = now;
		invoke<Void>(N_TXD_REQUEST, Ptr("be_portraits_nya"));
		invoke<Void>(N_TXD_REQUEST, Ptr("be_portraits"));
	}
	return NULL;
}

bool BountyBoardUI::configEnabled()
{
	loadConfigOnce();
	return s_cfgEnabled;
}

bool BountyBoardUI::boardAppReady()
{
	return invoke<BOOL>(N_UIAPP_RUNNING, (UINT64)UIAPP_HASH) != 0;
}

float BountyBoardUI::configDistance()
{
	loadConfigOnce();
	return s_cfgDistance;
}

bool BountyBoardUI::isShowing()   { return s_showing; }
bool BountyBoardUI::isAvailable() { return s_available; }
int  BountyBoardUI::lastError()   { return s_error; }

void BountyBoardUI::logState()
{
	std::stringstream ss;
	ss << "bounty board ui: flowblock=" << (UINT32)s_flowblock
	   << " showing=" << (s_showing ? 1 : 0)
	   << " available=" << (s_available ? 1 : 0)
	   << " failed=" << (s_failed ? 1 : 0)
	   << " error=" << s_error
	   << " ui=" << (invoke<BOOL>(N_UIAPP_RUNNING, (UINT64)UIAPP_HASH) ? 1 : 0)
	   << " ids=" << (UINT32)s_name << "/" << (UINT32)s_header << "/"
	   << (UINT32)s_body << "/" << (UINT32)s_price
	   << "/" << (UINT32)s_tex << "/" << (UINT32)s_txd;
	log(ss.str());
}

void BountyBoardUI::logMask(Object entity)
{
	float l0 = 0.0f, l1 = 0.0f, l2 = 0.0f, l3 = 0.0f;
	invoke<Void>(N_GET_MASK_LAYERS, (UINT64)entity,
	             (UINT64)(uintptr_t)&l0, (UINT64)(uintptr_t)&l1,
	             (UINT64)(uintptr_t)&l2, (UINT64)(uintptr_t)&l3);

	std::stringstream ss;
	ss << "bounty board ui: mask " << l0 << "/" << l1 << "/" << l2 << "/" << l3
	   << " ui=" << (invoke<BOOL>(N_UIAPP_RUNNING, (UINT64)UIAPP_HASH) ? 1 : 0)
	   << " canLaunch=" << (invoke<BOOL>(N_UIAPP_CAN_LAUNCH, (UINT64)UIAPP_HASH) ? 1 : 0)
	   << " name=" << s_lastName;
	log(ss.str());
}

bool BountyBoardUI::buildTree()
{
	// The flowblock may already have been requested by resetStateMachine() at
	// startup; asking twice for the same block returns a fresh id but the
	// request itself can be refused, so an existing one is reused.
	UINT64 fb = s_flowblock;
	if ((UINT32)fb == 0)
	{
		fb = invoke<UINT64>(N_FLOWBLOCK_REQUEST, (UINT64)FLOWBLOCK_HASH);
		if ((UINT32)fb == 0)
		{
			s_error = 1;               // flowblock rejected the request
			log("bounty board ui: flowblock request refused");
			return false;
		}
		s_flowblock = Id(fb);
	}

	std::stringstream ss;
	ss << "bounty board ui: flowblock requested, id " << (UINT32)s_flowblock;
	log(ss.str());

	// The tree mirrors 1.4.4 exactly:
	//   bounty_board_data
	//     Poster0
	//       isVisible / isDiffVisible / type / difficulty / price / body /
	//       header / name / tex / txd
	UINT64 root = Id(invoke<UINT64>(N_ADD_CONTAINER_PATH, Ptr(""), Ptr("bounty_board_data")));
	UINT64 poster = Id(invoke<UINT64>(N_ADD_CONTAINER, root, Ptr("Poster0")));
	s_poster = poster;

	if (poster == 0)
	{
		s_error = 2;               // container tree could not be created
		log("bounty board ui: Poster0 container was not created");
		return false;
	}

	s_isVisible     = Id(invoke<UINT64>(N_ADD_BOOL,   poster, Ptr("isVisible"), (UINT64)0));
	s_isDiffVisible = Id(invoke<UINT64>(N_ADD_BOOL,   poster, Ptr("isDiffVisible"), (UINT64)0));
	s_type          = Id(invoke<UINT64>(N_ADD_INT,    poster, Ptr("type"), (UINT64)0));
	s_difficulty    = Id(invoke<UINT64>(N_ADD_INT,    poster, Ptr("difficulty"), (UINT64)0));
	s_price         = Id(invoke<UINT64>(N_ADD_STRING, poster, Ptr("price"), Ptr("")));
	s_body          = Id(invoke<UINT64>(N_ADD_STRING, poster, Ptr("body"), Ptr("")));
	s_header        = Id(invoke<UINT64>(N_ADD_STRING, poster, Ptr("header"), Ptr("")));
	s_name          = Id(invoke<UINT64>(N_ADD_STRING, poster, Ptr("name"), Ptr("")));
	s_tex           = Id(invoke<UINT64>(N_ADD_HASH,   poster, Ptr("tex"), (UINT64)0));
	s_txd           = Id(invoke<UINT64>(N_ADD_HASH,   poster, Ptr("txd"), (UINT64)0));

	logState();
	return true;
}

// 1.4.4 requests the poster model right after creating the state machine.
// Here the request is issued but not waited on: createProp() already waits for
// the model it is given, and blocking twice on the same asset only makes the
// mission start stall. The diagnostics written here are what tells us whether
// the MP paper exists in this build at all.
bool BountyBoardUI::loadPosterModel()
{
	UINT64 model = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr(POSTER_MODEL));
	if (model == 0)
	{
		s_error = 4;
		log("bounty board ui: poster model hash is zero");
		return false;
	}

	BOOL inCd  = invoke<BOOL>(N_IS_MODEL_IN_CDIMAGE, model);
	BOOL valid = invoke<BOOL>(N_IS_MODEL_VALID, model);
	BOOL ready = invoke<BOOL>(N_HAS_MODEL_LOADED, model);

	std::stringstream ss;
	ss << "bounty board ui: model " << POSTER_MODEL
	   << " hash=" << (UINT32)model
	   << " inCd=" << (inCd ? 1 : 0)
	   << " valid=" << (valid ? 1 : 0)
	   << " loaded=" << (ready ? 1 : 0);
	log(ss.str());

	if (!ready)
	{
		invoke<Void>(N_REQUEST_MODEL, model, (UINT64)0);
	}
	return true;
}

void BountyBoardUI::rebind()
{
	if (!s_showing || s_flowblock == 0)
	{
		return;
	}

	// Create then destroy in the same frame. The engine rebuilds the machine on
	// the following frame, and that rebuild is what re-reads the databinding -
	// writing values into an already bound machine produces nothing on screen,
	// which is why every earlier attempt looked like a blank sheet even though
	// the read back showed the correct name and isVisible=1.
	UINT64 sm = invoke<UINT64>(N_SM_CREATE, SM_HASH, s_flowblock);
	invoke<Void>(N_SM_DESTROY, sm);

	std::stringstream ss;
	ss << "bounty board ui: rebind after fill (create=" << (UINT32)sm << ", destroyed)";
	log(ss.str());
}

void BountyBoardUI::resetStateMachine()
{
	loadConfigOnce();
	if (!s_cfgEnabled)
	{
		return;
	}

	// Warm the portrait dictionary while the world is still loading. It is
	// requested again whenever a board is filled, but the case file (F7) can be
	// opened before any board has been touched, and a cold dictionary there
	// leaves the portrait slot empty.
	invoke<Void>(N_TXD_REQUEST, Ptr("be_portraits_nya"));
	invoke<Void>(N_TXD_REQUEST, Ptr("be_portraits"));

	// Warm the portrait dictionary while the world is still loading. It is
	// requested again whenever a board is filled, but the case file (F7) can be
	// opened before any board has been touched, and a cold dictionary there
	// leaves the portrait slot empty.
	invoke<Void>(N_TXD_REQUEST, Ptr("be_portraits_nya"));
	invoke<Void>(N_TXD_REQUEST, Ptr("be_portraits"));

	UINT64 fb = invoke<UINT64>(N_FLOWBLOCK_REQUEST, (UINT64)FLOWBLOCK_HASH);
	if ((UINT32)fb == 0)
	{
		log("bounty board ui: startup reset - flowblock request refused");
		return;
	}
	s_flowblock = Id(fb);

	// The package is usually still streaming in while the script boots, so a
	// wait here is normal. It is bounded so a missing UI never stalls the game.
	int guard = 0;
	while (!invoke<BOOL>(N_FLOWBLOCK_LOADED, s_flowblock) && guard < 240)
	{
		WAIT(0);
		guard++;
	}

	// 1.4.4 builds the databinding tree in the same breath as the flowblock
	// request (BountyPostersManager::initializing bounty board -> flowblock
	// loaded -> statemachine created -> poster model -> init completed). The
	// tree has to exist before the flowblock binds, so adding it later - when
	// the player walks up to a board - reaches a binding that is already
	// finished and nothing lands. Build it here instead.
	bool treeOk = buildTree();

	// The state machine is deliberately NOT touched here. Creating it before the
	// tree carries any values is what leaves the board blank: the machine binds
	// to an empty tree and never looks again. rebind() does the create/destroy
	// once the contents are in.
	UINT64 existing = invoke<UINT64>(N_SM_EXISTS, SM_HASH);

	std::stringstream ss;
	ss << "bounty board ui: startup reset flowblock=" << (UINT32)s_flowblock
	   << " loaded=" << (invoke<BOOL>(N_FLOWBLOCK_LOADED, s_flowblock) ? 1 : 0)
	   << " waited=" << guard
	   << " tree=" << (treeOk ? 1 : 0)
	   << " exists=" << (UINT32)existing;
	log(ss.str());

}

bool BountyBoardUI::open()
{
	log("bounty board ui: open begin");
	loadConfigOnce();
	if (!s_cfgEnabled)
	{
		s_error = 100;
		if (!s_disabledLogged)
		{
			s_disabledLogged = true;
			log("bounty board ui: native paper disabled (NativeUI=0 in BountiesExpansion/BountiesExpansion.ini)");
		}
		return false;
	}

	// Touching the mask or the flowblock before the world is up is what
	// wedged the loading screen. A valid player ped means the session is
	// actually running.
	{
		Ped player = PLAYER::PLAYER_PED_ID();
		if (!player || !ENTITY::DOES_ENTITY_EXIST(player))
		{
			s_error = 101;
			return false;
		}
	}

	if (s_showing)
	{
		// A board can be re-spawned while the UI is already up; drop the
		// owner so the new poster gets its own contents written.
		s_owner = 0;
		return true;
	}
	if (s_flowblock == 0 || s_name == 0)
	{
		if (!buildTree())
		{
			s_failed = true;
			return false;
		}
	}

	// Wait for the flowblock to load. Missions are created while the player can
	// still be on the other side of the map, so the UI package is often not
	// resident yet and a short wait is the same as failing outright. The guard
	// is only there to keep a genuinely missing UI from hanging the fiber: it
	// is short on purpose, because this runs on the gameplay thread while the
	// player is standing at the board.
	int guard = 0;
	while (!invoke<BOOL>(N_FLOWBLOCK_LOADED, s_flowblock) && guard < 300)
	{
		WAIT(0);
		guard++;
	}
	if (guard >= 300)
	{
		s_error = 3;               // never finished loading
		log("bounty board ui: flowblock did not load in time, caller may retry");
		return false;
	}

	// 1.4.4 enters the flowblock before creating the state machine, but
	// ZEEPosterFix - the only configuration that prints on this build - never
	// enters, and entering right before UI_STATE_MACHINE_CREATE makes that call
	// hand back 0 here. It stays off unless the ini asks for it.
	if (modIniInt("Poster", "Enter", 0) != 0)
	{
		UINT32 blockKey = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr("bounty_board"));
		UINT64 enterOk = invoke<UINT64>(N_FLOWBLOCK_ENTER, s_flowblock, (UINT64)blockKey);
		std::stringstream ss;
		ss << "bounty board ui: flowblock enter=" << (UINT32)enterOk
		   << " block=" << blockKey;
		log(ss.str());
	}

	// resetStateMachine() freed the slot while the session was still booting,
	// so this create should finally be accepted. Nothing is destroyed here: this
	// machine is the one that renders the sheet.
	{
		UINT64 existing = invoke<UINT64>(N_SM_EXISTS, SM_HASH);
		UINT64 sm = 0;
		if ((UINT32)existing == 0)
		{
			sm = invoke<UINT64>(N_SM_CREATE, SM_HASH, s_flowblock);
		}

		std::stringstream ss;
		ss << "bounty board ui: state machine exists=" << (UINT32)existing
		   << " create=" << (UINT32)sm;
		log(ss.str());
	}

	if (!loadPosterModel())
	{
		s_failed = true;
		return false;
	}

	// Diagnostics for the portrait: if the dictionary is missing from this
	// build the flowblock samples nothing and the poster stays a bare sheet,
	// which is indistinguishable from the UI never rendering at all.
	{
		const UINT64 N_TXD_EXIST  = 0x7332461FC59EB7ECULL;
		const UINT64 N_TXD_LOADED = 0x54D6900929CCF162ULL;
		const UINT64 N_TXD_REQUEST = 0xC1BA29DF5631B0F8ULL;
		BOOL exB = invoke<BOOL>(N_TXD_EXIST, Ptr("generic_textures"));
		BOOL exM = invoke<BOOL>(N_TXD_EXIST, Ptr("mp005_p_mp_bountyposter01x"));
		BOOL ldB = invoke<BOOL>(N_TXD_LOADED, Ptr("generic_textures"));
		invoke<Void>(N_TXD_REQUEST, Ptr("generic_textures"));
		int waitTxd = 0;
		while (!invoke<BOOL>(N_TXD_LOADED, Ptr("generic_textures")) && waitTxd < 60)
		{
			WAIT(0);
			waitTxd++;
		}
		BOOL ldAfter = invoke<BOOL>(N_TXD_LOADED, Ptr("generic_textures"));
		std::stringstream ss;
		ss << "bounty board ui: txd bounty_posters exist=" << (exB ? 1 : 0)
		   << " loaded=" << (ldB ? 1 : 0) << " afterRequest=" << (ldAfter ? 1 : 0)
		   << " waited=" << waitTxd
		   << " mp005Txd=" << (exM ? 1 : 0);
		log(ss.str());
	}

	// 1.4.4 never waits for the board UI app. Its own log shows the
	// databinding being filled and the poster being masked straight after
	// "initialization complete", with no app running at all - the entity
	// mask plus the flowblock are enough on their own. The guard that used
	// to sit here asked for an app that only the game's own board
	// interaction can start, so it always failed and every paper stayed
	// vanilla. Dropped.

	invoke<Void>(N_ENABLE_ENTITYMASK);
	log("bounty board ui: entity mask enabled");

	s_owner = 0;
	s_showing = true;
	s_available = true;
	s_failed = false;
	s_error = 0;
	s_frames = 0;
	logState();
	return true;
}

const char* BountyBoardUI::pickPosterModel(bool deadOrAlive)
{
	const char* vanilla = deadOrAlive ? VANILLA_DEAD : VANILLA_ALIVE;

	UINT32 mpHash = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr(POSTER_MODEL));
	if (mpHash != 0)
	{
		BOOL inCd  = invoke<BOOL>(N_IS_MODEL_IN_CDIMAGE, mpHash);
		BOOL valid = invoke<BOOL>(N_IS_MODEL_VALID, mpHash);
		if (inCd || valid)
		{
			return POSTER_MODEL;
		}

		std::stringstream ss;
		ss << "bounty board ui: " << POSTER_MODEL
		   << " is not in this build, using " << vanilla;
		log(ss.str());
	}
	return vanilla;
}

void BountyBoardUI::writeData()
{
	if (!s_showing || s_name == 0)
	{
		return;
	}

	// The state machine is reset right after it is created, so the ids that ADD
	// handed back may point into a tree the engine has already dropped. Every
	// write therefore goes through the container path plus the entry name - the
	// same route ZEEPosterFix takes, and the reason its board prints while a
	// plain 1.4.4 board stays blank.
	UINT64 poster = 0;
	{
		UINT64 root = invoke<UINT64>(N_GET_CONTAINER_PATH, Ptr("bounty_board_data"));
		if ((UINT32)root != 0)
		{
			poster = Id(invoke<UINT64>(N_GET_CONTAINER_CHILD, Id(root), (UINT64)0));
		}
	}
	if (poster == 0)
	{
		poster = s_poster;         // fall back to the tree buildTree() created
	}
	if (poster == 0)
	{
		return;
	}

	// 1.4.4 writes the same texture hash into both tex and txd, and that hash
	// is the pedshot name the network layer hands back. Until the engine can
	// produce one, the dictionary that ships with the game is used instead: a
	// tex/txd pair that resolves to nothing makes the flowblock drop the whole
	// draw, letters included.
	// [Poster] Portrait selects the sampler, so the "is the sheet blank because
	// of the texture" question can be answered without a rebuild:
	//   0 = leave tex/txd at 0 (the flowblock decides what to do)
	//   1 = pedshot name from the network layer (needs GeneratePedshot=1)
	//   2 = the game's own bounty_posters / ped_picture pair
	int portraitMode = modIniInt("Poster", "Portrait", 2);
	UINT64 txdHash = 0;
	UINT64 texHash = 0;
	if (portraitMode == 1)
	{
		resolvePedshot();
		txdHash = s_shotHash;
		texHash = s_shotHash;
	}
	else if (portraitMode == 2)
	{
		txdHash = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr("bounty_posters"));
		texHash = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr("ped_picture"));
	}
	else if (portraitMode == 6)
	{
		// Custom dictionary packed by the user. Both names are requested
		// because the loader may register the file under its own suffix
		// ("_nya") instead of the plain base name.
		static const char* dicts[] = { "be_portraits", "be_portraits_nya" };
		invoke<Void>(N_TXD_REQUEST, Ptr(dicts[0]));
		invoke<Void>(N_TXD_REQUEST, Ptr(dicts[1]));

		const char* use = NULL;
		for (int i = 0; i < 2 && use == NULL; i++)
		{
			if (invoke<BOOL>(N_TXD_EXIST, Ptr(dicts[i])) ||
			    invoke<BOOL>(N_TXD_LOADED, Ptr(dicts[i])))
			{
				use = dicts[i];
			}
		}

		if (use != NULL)
		{
			// The caller already resolved which face this criminal wears.
			std::string texName = std::string("portrait_")
				+ std::to_string(s_portraitIndex > 0 ? s_portraitIndex : 1);

			txdHash = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr(use));
			texHash = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr(texName.c_str()));

			if (s_readbackLogs == 0)
			{
				std::stringstream sc;
				sc << "bounty board ui: custom portrait dict \"" << use
				   << "\" loaded=" << (invoke<BOOL>(N_TXD_LOADED, Ptr(use)) ? 1 : 0)
				   << " tex=" << texName;
				log(sc.str());
			}
		}
		else if (s_readbackLogs == 0)
		{
			log("bounty board ui: custom portrait dict not found (tried be_portraits, be_portraits_nya)");
		}
	}
	else if (portraitMode == 5)
	{
		invoke<Void>(N_TXD_REQUEST, Ptr("generic_textures"));
		txdHash = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr("generic_textures"));
		texHash = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr("menu_header_1a"));
	}
	else if (portraitMode == 4)
	{
		invoke<Void>(N_TXD_REQUEST, Ptr("p_pos_wantedalive01x"));
		txdHash = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr("p_pos_wantedalive01x"));
		texHash = txdHash;
	}

	// 1.4.4 never writes through a parent/name lookup - it writes straight into
	// the ids that ADD handed back (WRITE_DATA_BOOL / INT / STRING /
	// HASH_STRING). That is the only combination proven to work in this build;
	// the name based writes silently drop bool nodes, which is exactly how the
	// poster ended up with a correct name but isVisible still 0.
	if (s_name != 0)
	{
		invoke<Void>(N_WRITE_STRING, s_name,   Ptr(s_lastName.c_str()));
		invoke<Void>(N_WRITE_STRING, s_header, Ptr(s_lastHeader.c_str()));
		invoke<Void>(N_WRITE_STRING, s_body,   Ptr(s_lastBody.c_str()));
		invoke<Void>(N_WRITE_STRING, s_price,  Ptr(s_lastPrice.c_str()));

		if (txdHash != 0 && texHash != 0)
		{
			invoke<Void>(N_WRITE_HASH_STRING, s_txd, txdHash);
			invoke<Void>(N_WRITE_HASH_STRING, s_tex, texHash);
		}

		invoke<Void>(N_WRITE_INT,  s_type,      (UINT64)1);
		invoke<Void>(N_WRITE_BOOL, s_isVisible, (UINT64)1);
	}
	else
	{
		// The tree was rebuilt by the engine and the ids are gone: fall back to
		// the name lookup so at least the text keeps updating.
		invoke<Void>(N_WRITE_STR_PARENT, poster, Ptr("name"),   Ptr(s_lastName.c_str()));
		invoke<Void>(N_WRITE_STR_PARENT, poster, Ptr("header"), Ptr(s_lastHeader.c_str()));
		invoke<Void>(N_WRITE_STR_PARENT, poster, Ptr("body"),   Ptr(s_lastBody.c_str()));
		invoke<Void>(N_WRITE_STR_PARENT, poster, Ptr("price"),  Ptr(s_lastPrice.c_str()));

		if (txdHash != 0 && texHash != 0)
		{
			invoke<Void>(N_WRITE_HASH_PARENT, poster, Ptr("txd"), txdHash);
			invoke<Void>(N_WRITE_HASH_PARENT, poster, Ptr("tex"), texHash);
		}

		invoke<Void>(N_WRITE_INT_PARENT,  poster, Ptr("type"),          (UINT64)1);
		invoke<Void>(N_WRITE_BOOL_PARENT, poster, Ptr("isVisible"),     (UINT64)1);
		invoke<Void>(N_WRITE_INT_PARENT,  poster, Ptr("isVisible"),     (UINT64)1);
	}

	// The portrait is produced a few frames after the first fill, and a bound
	// state machine never looks at the tree again - so the sampler changing has
	// to trigger the same reset the fill does, or the photo never shows up.
	if ((UINT32)txdHash != s_writtenTxd || (UINT32)texHash != s_writtenTex)
	{
		s_writtenTxd = (UINT32)txdHash;
		s_writtenTex = (UINT32)texHash;
		rebind();
	}

	// The portrait is produced a few frames after the first fill, and a bound
	// state machine never looks at the tree again - so the sampler changing has
	// to trigger the same reset the fill does, or the photo never shows up.
	if ((UINT32)txdHash != s_writtenTxd || (UINT32)texHash != s_writtenTex)
	{
		s_writtenTxd = (UINT32)txdHash;
		s_writtenTex = (UINT32)texHash;
		rebind();
	}

	// One read-back proves whether the parent-name writes land on the node the
	// flowblock actually samples: a wrong container still writes happily and
	// changes nothing on screen.
	if (portraitMode == 4 && s_readbackLogs == 0)
	{
		std::stringstream sd;
		sd << "bounty board ui: portrait dict p_pos_wantedalive01x exist="
		   << (invoke<BOOL>(N_TXD_EXIST, Ptr("p_pos_wantedalive01x")) ? 1 : 0)
		   << " loaded=" << (invoke<BOOL>(N_TXD_LOADED, Ptr("p_pos_wantedalive01x")) ? 1 : 0);
		log(sd.str());
	}


	if (s_readbackLogs < 1)
	{
		s_readbackLogs++;
		const char* backName = invoke<const char*>(N_READ_STRING_PARENT, poster, Ptr("name"));
		BOOL backBool = invoke<BOOL>(N_READ_BOOL_PARENT, poster, Ptr("isVisible"));
		INT32 backInt = (INT32)invoke<INT32>(N_READ_INT_PARENT, poster, Ptr("isVisible"));

		std::stringstream ss;
		ss << "bounty board ui: readback poster=" << (UINT32)poster
		   << " name=" << ((backName && !IsBadReadPtr(backName, 1)) ? backName : "<null>")
		   << " isVisible.bool=" << (backBool ? 1 : 0)
		   << " isVisible.int=" << backInt
		   << " txd=" << (UINT32)txdHash << " tex=" << (UINT32)texHash;
		log(ss.str());
	}

	// The visible flag is written through both accessors on purpose: the read
	// back in step40 showed name landing correctly while isVisible stayed 0,
	// which means the bool write is silently dropped (wrong entry type or a
	// different node). Whichever accessor the flowblock actually reads, one of
	// these two sets it.
	invoke<Void>(N_WRITE_INT_PARENT,  poster, Ptr("type"),          (UINT64)1);
	invoke<Void>(N_WRITE_BOOL_PARENT, poster, Ptr("isVisible"),     (UINT64)1);
	invoke<Void>(N_WRITE_INT_PARENT,  poster, Ptr("isVisible"),     (UINT64)1);
	invoke<Void>(N_WRITE_BOOL_PARENT, poster, Ptr("isDiffVisible"), (UINT64)0);
}

void BountyBoardUI::fill(const char* name, const char* header, const char* body,
                         const char* price, unsigned int portraitHash, int portraitIndex)
{
	s_portraitIndex = (portraitIndex > 0) ? portraitIndex : 1;
	if (!s_showing || s_name == 0)
	{
		return;
	}

	s_lastName   = name ? name : "";
	s_lastHeader = header ? header : "";
	s_lastBody   = body ? body : "";
	s_lastPrice  = price ? price : "";
	s_lastPortrait = portraitHash;

	writeData();
	log("bounty board ui: poster filled");
}

bool BountyBoardUI::needsFill(UINT64 owner)
{
	return s_showing && s_owner != owner;
}

void BountyBoardUI::fillFor(UINT64 owner, const char* name, const char* header,
                            const char* body, const char* price,
                            unsigned int portraitHash, int portraitIndex)
{
	if (!needsFill(owner))
	{
		return;
	}
	s_owner = owner;
	fill(name, header, body, price, portraitHash, portraitIndex);
}

bool BountyBoardUI::attach(Object entity)
{
	if (!s_showing || !entity || !ENTITY::DOES_ENTITY_EXIST(entity))
	{
		return false;
	}
	if (s_attached == (UINT64)entity)
	{
		return true;
	}

	invoke<Void>(N_ENABLE_ENTITYMASK);
	invoke<Void>(N_MASK_ADD_INTENSITY, (UINT64)entity, (UINT64)MASK_LAYER, 1.0f);
	ENTITY::SET_ENTITY_VISIBLE(entity, true);

	// 1.4.4 tags the poster as a readable document at this point.
	UINT64 docHash = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr(DECOR_DOCUMENT));
	invoke<Void>(N_DECOR_SET_INT, (UINT64)entity, Ptr(DECOR_LETTER_ITEM), docHash);

	s_attached = (UINT64)entity;

	log("bounty board ui: poster added to entity mask");
	return true;
}

// The entity mask and the global mask switch are not persistent: the renderer
// drops them as soon as the UI app that owns the render target stops, which is
// exactly what happened in game (the log showed the app being closed by the
// game a second after it was opened). Reapplying them every frame keeps the
// paper painted, and rewriting the data makes the UI repaint when the player
// walks up to a different board.
bool BountyBoardUI::tick(Object entity)
{
	if (!s_showing)
	{
		return false;
	}
	if (!entity || !ENTITY::DOES_ENTITY_EXIST(entity))
	{
		return false;
	}

	s_frames++;

	// Log only when the mask moves to a different prop: doing it every frame
	// would flood the log, and a stale handle here is exactly the kind of
	// native that parks the script after a teleport.
	if (s_attached != (UINT64)entity)
	{
		std::stringstream st;
		st << "bounty board ui: mask attach poster=" << (UINT32)entity;
		log(st.str());
	}

	invoke<Void>(N_ENABLE_ENTITYMASK);
	invoke<Void>(N_MASK_ADD_INTENSITY, (UINT64)entity, (UINT64)MASK_LAYER, 1.0f);
	s_attached = (UINT64)entity;

	if ((s_frames % 45) == 1)
	{
		writeData();
	}
	return true;
}

bool BountyBoardUI::createPosterScenario(float x, float y, float z, float heading,
                                         bool deadOrAlive)
{
	deleteScenario();

	// Both inspect-poster scenario types share the same layout, so when the
	// engine refuses one (the alive variant returned 0 on every board) the
	// sibling type still gives the paper something to sit in.
	const char* primary   = deadOrAlive ? SCENARIO_DEAD : SCENARIO_ALIVE;
	const char* alternate = deadOrAlive ? SCENARIO_ALIVE : SCENARIO_DEAD;

	for (int attempt = 0; attempt < 2 && s_scenario == 0; attempt++)
	{
		const char* scenarioName = attempt == 0 ? primary : alternate;
		UINT32 type = (UINT32)invoke<UINT32>(N_HASH_KEY, Ptr(scenarioName));
		if (type == 0)
		{
			continue;
		}

		// (type, x, y, z, heading, p5, p6, p7) - 1.4.4 uses 0/0/false too.
		s_scenario = (int)invoke<INT32>(N_CREATE_SCENARIO_POINT, (UINT64)type,
		                                x, y, z, heading,
		                                (UINT64)0, (UINT64)0, (UINT64)0);

		std::stringstream ss;
		ss << "bounty board ui: scenario point " << s_scenario << " type=" << type
		   << " name=" << scenarioName;
		log(ss.str());
	}

	if (s_scenario == 0)
	{
		return false;
	}

	invoke<Void>(N_SCENARIO_SET_ACTIVE, (UINT64)s_scenario, (UINT64)1);
	return true;
}

bool BountyBoardUI::associate(Object entity)
{
	if (s_scenario == 0 || !entity || !ENTITY::DOES_ENTITY_EXIST(entity))
	{
		return false;
	}

	// A scenario can only hold one prop per slot, so the slot is cleared first.
	// The prop that used to sit there is deliberately NOT deleted: for a freshly
	// created scenario point the returned entity is not reliably one of ours,
	// and deleting it took the whole game down at startup.
	invoke<Void>(N_SCENARIO_DISASSOCIATE, (UINT64)s_scenario, Ptr(SCENARIO_SLOT));

	BOOL ok = invoke<BOOL>(N_SCENARIO_ASSOCIATE_PROP, (UINT64)s_scenario,
	                       (UINT64)entity, Ptr(SCENARIO_SLOT), (UINT64)1);

	std::stringstream ss;
	ss << "bounty board ui: primary item associated=" << (ok ? 1 : 0);
	log(ss.str());
	return ok != 0;
}

void BountyBoardUI::deleteScenario()
{
	if (s_scenario != 0)
	{
		invoke<Void>(N_SCENARIO_DELETE, (UINT64)s_scenario);
		s_scenario = 0;
	}
}

void BountyBoardUI::detach(Object entity)
{
	if (entity && ENTITY::DOES_ENTITY_EXIST(entity))
	{
		invoke<Void>(N_MASK_REMOVE, (UINT64)entity);
	}
	if (s_attached == (UINT64)entity)
	{
		s_attached = 0;
	}
}

void BountyBoardUI::close()
{
	// The UI app belongs to the game, closing it broke the board for the whole
	// session in an earlier build (the log showed the game shutting it down a
	// second after it was opened). Only our own state is dropped here.
	deleteScenario();
	s_showing = false;
	s_attached = 0;
	s_owner = 0;
}
