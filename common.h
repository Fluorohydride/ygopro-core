/*
 * common.h
 *
 *  Created on: 2009-1-7
 *      Author: Argon.Sun
 */

#ifndef COMMON_H_
#define COMMON_H_

#ifdef _MSC_VER
#pragma warning(disable: 4244)
#endif

typedef unsigned long uptr;
typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef unsigned char byte;
typedef long ptr;
typedef long long int64;
typedef int int32;
typedef short int16;
typedef signed char int8;
typedef int BOOL;

#define MATCH_ALL(x,y) (((x)&(y))==(y))
#define MATCH_ANY(x,y) ((x)&(y))
#define ADD_BIT(x,y) ((x)|=(y))
#define REMOVE_BIT(x,y) ((x)&=~(y))

#define OPERATION_SUCCESS 1
#define OPERATION_FAIL 0
#define OPERATION_CANCELED -1
#define TRUE 1
#define FALSE 0
#ifndef NULL
#define NULL 0
#endif
struct card_sort {
	bool operator()(void* const & c1, void* const & c2) const;
};

//Locations
#define LOCATION_DECK		0x01		//
#define LOCATION_HAND		0x02		//
#define LOCATION_MZONE		0x04		//
#define LOCATION_SZONE		0x08		//
#define LOCATION_GRAVE		0x10		//
#define LOCATION_REMOVED	0x20		//
#define LOCATION_EXTRA		0x40		//
#define LOCATION_OVERLAY	0x80		//
#define LOCATION_ONFIELD	0x0c		//
#define LOCATION_FZONE		0x100		//
#define LOCATION_PZONE		0x200		//

//Positions
#define POS_FACEUP_ATTACK		0x1
#define POS_FACEDOWN_ATTACK		0x2
#define POS_FACEUP_DEFENSE		0x4
#define POS_FACEDOWN_DEFENSE	0x8
#define POS_FACEUP				0x5
#define POS_FACEDOWN			0xa
#define POS_ATTACK				0x3
#define POS_DEFENSE				0xc
//Flip effect flags
#define NO_FLIP_EFFECT			0x10000
#define FLIP_SET_AVAILABLE		0x20000

//Types
#define TYPE_MONSTER		0x1			//
#define TYPE_SPELL			0x2			//
#define TYPE_TRAP			0x4			//
#define TYPE_NORMAL			0x10		//
#define TYPE_EFFECT			0x20		//
#define TYPE_FUSION			0x40		//
#define TYPE_RITUAL			0x80		//
#define TYPE_TRAPMONSTER	0x100		//
#define TYPE_SPIRIT			0x200		//
#define TYPE_UNION			0x400		//
#define TYPE_DUAL			0x800		//
#define TYPE_TUNER			0x1000		//
#define TYPE_SYNCHRO		0x2000		//
#define TYPE_TOKEN			0x4000		//
#define TYPE_QUICKPLAY		0x10000		//
#define TYPE_CONTINUOUS		0x20000		//
#define TYPE_EQUIP			0x40000		//
#define TYPE_FIELD			0x80000		//
#define TYPE_COUNTER		0x100000	//
#define TYPE_FLIP			0x200000	//
#define TYPE_TOON			0x400000	//
#define TYPE_XYZ			0x800000	//
#define TYPE_PENDULUM		0x1000000	//
#define TYPE_SPSUMMON		0x2000000	//
#define TYPE_LINK			0x4000000	//

//Attributes
#define ATTRIBUTE_EARTH		0x01		//
#define ATTRIBUTE_WATER		0x02		//
#define ATTRIBUTE_FIRE		0x04		//
#define ATTRIBUTE_WIND		0x08		//
#define ATTRIBUTE_LIGHT		0x10		//
#define ATTRIBUTE_DARK		0x20		//
#define ATTRIBUTE_DEVINE	0x40		//

//Races
#define RACE_WARRIOR		0x1			//
#define RACE_SPELLCASTER	0x2			//
#define RACE_FAIRY			0x4			//
#define RACE_FIEND			0x8			//
#define RACE_ZOMBIE			0x10		//
#define RACE_MACHINE		0x20		//
#define RACE_AQUA			0x40		//
#define RACE_PYRO			0x80		//
#define RACE_ROCK			0x100		//
#define RACE_WINDBEAST		0x200		//
#define RACE_PLANT			0x400		//
#define RACE_INSECT			0x800		//
#define RACE_THUNDER		0x1000		//
#define RACE_DRAGON			0x2000		//
#define RACE_BEAST			0x4000		//
#define RACE_BEASTWARRIOR	0x8000		//
#define RACE_DINOSAUR		0x10000		//
#define RACE_FISH			0x20000		//
#define RACE_SEASERPENT		0x40000		//
#define RACE_REPTILE		0x80000		//
#define RACE_PSYCHO			0x100000	//
#define RACE_DEVINE			0x200000	//
#define RACE_CREATORGOD		0x400000	//
#define RACE_WYRM			0x800000	//
#define RACE_CYBERSE		0x1000000	//

//Reason
#define REASON_DESTROY		0x1		//
#define REASON_RELEASE		0x2		//
#define REASON_TEMPORARY	0x4		//
#define REASON_MATERIAL		0x8		//
#define REASON_SUMMON		0x10	//
#define REASON_BATTLE		0x20	//
#define REASON_EFFECT		0x40	//
#define REASON_COST			0x80	//
#define REASON_ADJUST		0x100	//
#define REASON_LOST_TARGET	0x200	//
#define REASON_RULE			0x400	//
#define REASON_SPSUMMON		0x800	//
#define REASON_DISSUMMON	0x1000	//
#define REASON_FLIP			0x2000	//
#define REASON_DISCARD		0x4000	//
#define REASON_RDAMAGE		0x8000	//
#define REASON_RRECOVER		0x10000	//
#define REASON_RETURN		0x20000	//
#define REASON_FUSION		0x40000	//
#define REASON_SYNCHRO		0x80000	//
#define REASON_RITUAL		0x100000	//
#define REASON_XYZ			0x200000	//
#define REASON_REPLACE		0x1000000	//
#define REASON_DRAW			0x2000000	//
#define REASON_REDIRECT		0x4000000	//
//#define REASON_REVEAL			0x8000000	//
#define REASON_LINK			0x10000000	//

//Status
#define STATUS_DISABLED				0x0001	//
#define STATUS_TO_ENABLE			0x0002	//
#define STATUS_TO_DISABLE			0x0004	//
#define STATUS_PROC_COMPLETE		0x0008	//
#define STATUS_SET_TURN				0x0010	//
#define STATUS_NO_LEVEL				0x0020	//
#define STATUS_BATTLE_RESULT		0x0040	//
#define STATUS_SPSUMMON_STEP		0x0080	//
#define STATUS_FORM_CHANGED			0x0100	//
#define STATUS_SUMMONING			0x0200	//
#define STATUS_EFFECT_ENABLED		0x0400	//
#define STATUS_SUMMON_TURN			0x0800	//
#define STATUS_DESTROY_CONFIRMED	0x1000	//
#define STATUS_LEAVE_CONFIRMED		0x2000	//
#define STATUS_BATTLE_DESTROYED		0x4000	//
#define STATUS_COPYING_EFFECT		0x8000	//
#define STATUS_CHAINING				0x10000	//
#define STATUS_SUMMON_DISABLED		0x20000	//
#define STATUS_ACTIVATE_DISABLED	0x40000	//
#define STATUS_EFFECT_REPLACED		0x80000
#define STATUS_FUTURE_FUSION		0x100000
#define STATUS_ATTACK_CANCELED		0x200000
#define STATUS_INITIALIZING			0x400000
//#define STATUS_ACTIVATED			0x800000
#define STATUS_JUST_POS				0x1000000
#define STATUS_CONTINUOUS_POS		0x2000000
#define STATUS_FORBIDDEN			0x4000000
#define STATUS_ACT_FROM_HAND		0x8000000
#define STATUS_OPPO_BATTLE			0x10000000
#define STATUS_FLIP_SUMMON_TURN		0x20000000
#define STATUS_SPSUMMON_TURN		0x40000000

//Query list
#define QUERY_CODE			0x1
#define QUERY_POSITION		0x2
#define QUERY_ALIAS			0x4
#define QUERY_TYPE			0x8
#define QUERY_LEVEL			0x10
#define QUERY_RANK			0x20
#define QUERY_ATTRIBUTE		0x40
#define QUERY_RACE			0x80
#define QUERY_ATTACK		0x100
#define QUERY_DEFENSE		0x200
#define QUERY_BASE_ATTACK	0x400
#define QUERY_BASE_DEFENSE	0x800
#define QUERY_REASON		0x1000
#define QUERY_REASON_CARD	0x2000
#define QUERY_EQUIP_CARD	0x4000
#define QUERY_TARGET_CARD	0x8000
#define QUERY_OVERLAY_CARD	0x10000
#define QUERY_COUNTERS		0x20000
#define QUERY_OWNER			0x40000
#define QUERY_STATUS		0x80000
#define QUERY_LSCALE		0x200000
#define QUERY_RSCALE		0x400000
#define QUERY_LINK			0x800000

//Link markers
#define LINK_MARKER_BOTTOM_LEFT		0001
#define LINK_MARKER_BOTTOM			0002
#define LINK_MARKER_BOTTOM_RIGHT	0004
#define LINK_MARKER_LEFT			0010
#define LINK_MARKER_RIGHT			0040
#define LINK_MARKER_TOP_LEFT		0100
#define LINK_MARKER_TOP				0200
#define LINK_MARKER_TOP_RIGHT		0400

//Messages
#define MSG_RETRY				1
#define MSG_HINT				2
#define MSG_WAITING				3
#define MSG_START				4
#define MSG_WIN					5
#define MSG_UPDATE_DATA			6
#define MSG_UPDATE_CARD			7
#define MSG_REQUEST_DECK		8
#define MSG_SELECT_BATTLECMD	10
#define MSG_SELECT_IDLECMD		11
#define MSG_SELECT_EFFECTYN		12
#define MSG_SELECT_YESNO		13
#define MSG_SELECT_OPTION		14
#define MSG_SELECT_CARD			15
#define MSG_SELECT_CHAIN		16
#define MSG_SELECT_PLACE		18
#define MSG_SELECT_POSITION		19
#define MSG_SELECT_TRIBUTE		20
#define MSG_SORT_CHAIN			21
#define MSG_SELECT_COUNTER		22
#define MSG_SELECT_SUM			23
#define MSG_SELECT_DISFIELD		24
#define MSG_SORT_CARD			25
#define MSG_SELECT_UNSELECT_CARD	26
#define MSG_CONFIRM_DECKTOP		30
#define MSG_CONFIRM_CARDS		31
#define MSG_SHUFFLE_DECK		32
#define MSG_SHUFFLE_HAND		33
#define MSG_REFRESH_DECK		34
#define MSG_SWAP_GRAVE_DECK		35
#define MSG_SHUFFLE_SET_CARD	36
#define MSG_REVERSE_DECK		37
#define MSG_DECK_TOP			38
#define MSG_SHUFFLE_EXTRA		39
#define MSG_NEW_TURN			40
#define MSG_NEW_PHASE			41
#define MSG_CONFIRM_EXTRATOP	42
#define MSG_MOVE				50
#define MSG_POS_CHANGE			53
#define MSG_SET					54
#define MSG_SWAP				55
#define MSG_FIELD_DISABLED		56
#define MSG_SUMMONING			60
#define MSG_SUMMONED			61
#define MSG_SPSUMMONING			62
#define MSG_SPSUMMONED			63
#define MSG_FLIPSUMMONING		64
#define MSG_FLIPSUMMONED		65
#define MSG_CHAINING			70
#define MSG_CHAINED				71
#define MSG_CHAIN_SOLVING		72
#define MSG_CHAIN_SOLVED		73
#define MSG_CHAIN_END			74
#define MSG_CHAIN_NEGATED		75
#define MSG_CHAIN_DISABLED		76
#define MSG_CARD_SELECTED		80
#define MSG_RANDOM_SELECTED		81
#define MSG_BECOME_TARGET		83
#define MSG_DRAW				90
#define MSG_DAMAGE				91
#define MSG_RECOVER				92
#define MSG_EQUIP				93
#define MSG_LPUPDATE			94
#define MSG_UNEQUIP				95
#define MSG_CARD_TARGET			96
#define MSG_CANCEL_TARGET		97
#define MSG_PAY_LPCOST			100
#define MSG_ADD_COUNTER			101
#define MSG_REMOVE_COUNTER		102
#define MSG_ATTACK				110
#define MSG_BATTLE				111
#define MSG_ATTACK_DISABLED		112
#define MSG_DAMAGE_STEP_START	113
#define MSG_DAMAGE_STEP_END		114
#define MSG_MISSED_EFFECT		120
#define MSG_BE_CHAIN_TARGET		121
#define MSG_CREATE_RELATION		122
#define MSG_RELEASE_RELATION	123
#define MSG_TOSS_COIN			130
#define MSG_TOSS_DICE			131
#define MSG_ROCK_PAPER_SCISSORS	132
#define MSG_HAND_RES			133
#define MSG_ANNOUNCE_RACE		140
#define MSG_ANNOUNCE_ATTRIB		141
#define MSG_ANNOUNCE_CARD		142
#define MSG_ANNOUNCE_NUMBER		143
#define MSG_ANNOUNCE_CARD_FILTER	144
#define MSG_CARD_HINT			160
#define MSG_TAG_SWAP			161
#define MSG_RELOAD_FIELD		162	// Debug.ReloadFieldEnd()
#define MSG_AI_NAME				163
#define MSG_SHOW_HINT			164
#define MSG_PLAYER_HINT			165
#define MSG_MATCH_KILL			170
#define MSG_CUSTOM_MSG			180

//Hints
#define HINT_EVENT				1
#define HINT_MESSAGE			2
#define HINT_SELECTMSG			3
#define HINT_OPSELECTED			4
#define HINT_EFFECT				5
#define HINT_RACE				6
#define HINT_ATTRIB				7
#define HINT_CODE				8
#define HINT_NUMBER				9
#define HINT_CARD				10
//
#define CHINT_TURN				1
#define CHINT_CARD				2
#define CHINT_RACE				3
#define CHINT_ATTRIBUTE			4
#define CHINT_NUMBER			5
#define CHINT_DESC_ADD			6
#define CHINT_DESC_REMOVE		7
//
#define PHINT_DESC_ADD			6
#define PHINT_DESC_REMOVE		7
//
#define EDESC_OPERATION			1
#define EDESC_RESET				2

//
#define OPCODE_ADD				0x40000000
#define OPCODE_SUB				0x40000001
#define OPCODE_MUL				0x40000002
#define OPCODE_DIV				0x40000003
#define OPCODE_AND				0x40000004
#define OPCODE_OR				0x40000005
#define OPCODE_NEG				0x40000006
#define OPCODE_NOT				0x40000007
#define OPCODE_ISCODE			0x40000100
#define OPCODE_ISSETCARD		0x40000101
#define OPCODE_ISTYPE			0x40000102
#define OPCODE_ISRACE			0x40000103
#define OPCODE_ISATTRIBUTE		0x40000104

//Player
#define PLAYER_NONE		2	//
#define PLAYER_ALL		3	//

//Phase
#define PHASE_DRAW			0x01
#define PHASE_STANDBY		0x02
#define PHASE_MAIN1			0x04
#define PHASE_BATTLE_START	0x08
#define PHASE_BATTLE_STEP	0x10
#define PHASE_DAMAGE		0x20
#define PHASE_DAMAGE_CAL	0x40
#define PHASE_BATTLE		0x80
#define PHASE_MAIN2			0x100
#define PHASE_END			0x200

//Options
#define DUEL_TEST_MODE			0x01
#define DUEL_ATTACK_FIRST_TURN	0x02
//#define DUEL_NO_CHAIN_HINT		0x04
#define DUEL_OBSOLETE_RULING	0x08
#define DUEL_PSEUDO_SHUFFLE		0x10
#define DUEL_TAG_MODE			0x20
#define DUEL_SIMPLE_AI			0x40

#endif /* COMMON_H_ */
