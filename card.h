/*
 * card.h
 *
 *  Created on: 2010-4-8
 *      Author: Argon
 */

#ifndef CARD_H_
#define CARD_H_

#include "common.h"
#include "effectset.h"
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

class card;
class duel;
class effect;
class group;
struct chain;

struct card_data {
	uint32 code;
	uint32 alias;
	uint64 setcode;
	uint32 type;
	uint32 level;
	uint32 attribute;
	uint32 race;
	int32 attack;
	int32 defense;
	uint32 lscale;
	uint32 rscale;
	uint32 link_marker;
};

struct card_state {
	uint32 code;
	uint32 code2;
	uint16 setcode;
	uint32 type;
	uint32 level;
	uint32 rank;
	uint32 link;
	uint32 lscale;
	uint32 rscale;
	uint32 attribute;
	uint32 race;
	int32 attack;
	int32 defense;
	int32 base_attack;
	int32 base_defense;
	uint8 controler;
	uint8 location;
	uint8 sequence;
	uint8 position;
	uint32 reason;
	bool pzone;
	card* reason_card;
	uint8 reason_player;
	effect* reason_effect;
	bool is_location(int32 loc) const;
};

struct query_cache {
	uint32 code;
	uint32 alias;
	uint32 type;
	uint32 level;
	uint32 rank;
	uint32 link;
	uint32 attribute;
	uint32 race;
	int32 attack;
	int32 defense;
	int32 base_attack;
	int32 base_defense;
	uint32 reason;
	int32 is_public;
	int32 is_disabled;
	uint32 lscale;
	uint32 rscale;
	uint32 link_marker;
};

class card {
public:
	struct effect_relation_hash {
		inline std::size_t operator()(const std::pair<effect*, uint16>& v) const {
			return std::hash<uint16>()(v.second);
		}
	};
	typedef std::vector<card*> card_vector;
	typedef std::multimap<uint32, effect*> effect_container;
	typedef std::set<card*, card_sort> card_set;
	typedef std::unordered_map<effect*, effect_container::iterator> effect_indexer;
	typedef std::unordered_set<std::pair<effect*, uint16>, effect_relation_hash> effect_relation;
	typedef std::unordered_map<card*, uint32> relation_map;
	typedef std::map<uint16, std::array<uint16, 2> > counter_map;
	typedef std::map<uint32, int32> effect_count;
	class attacker_map : public std::unordered_map<uint16, std::pair<card*, uint32> > {
	public:
		void addcard(card* pcard);
	};
	int32 scrtype;
	int32 ref_handle;
	duel* pduel;
	card_data data;
	card_state previous;
	card_state temp;
	card_state current;
	query_cache q_cache;
	uint8 owner;
	uint8 summon_player;
	uint32 summon_info;
	uint32 status;
	uint32 operation_param;
	uint32 release_param;
	uint32 sum_param;
	uint32 position_param;
	uint32 spsummon_param;
	uint32 to_field_param;
	uint8 direct_attackable;
	uint8 announce_count;
	uint8 attacked_count;
	uint8 attack_all_target;
	uint8 attack_controler;
	uint16 cardid;
	uint32 fieldid;
	uint32 fieldid_r;
	uint16 turnid;
	uint16 turn_counter;
	uint8 unique_pos[2];
	uint32 unique_fieldid;
	uint32 unique_code;
	uint32 unique_location;
	int32 unique_function;
	effect* unique_effect;
	uint32 spsummon_code;
	uint16 spsummon_counter[2];
	uint16 spsummon_counter_rst[2];
	uint8 assume_type;
	uint32 assume_value;
	card* equiping_target;
	card* pre_equip_target;
	card* overlay_target;
	relation_map relations;
	counter_map counters;
	effect_count indestructable_effects;
	attacker_map announced_cards;
	attacker_map attacked_cards;
	attacker_map battled_cards;
	card_set equiping_cards;
	card_set material_cards;
	card_set effect_target_owner;
	card_set effect_target_cards;
	card_vector xyz_materials;
	effect_container single_effect;
	effect_container field_effect;
	effect_container equip_effect;
	effect_container xmaterial_effect;
	effect_indexer indexer;
	effect_relation relate_effect;
	effect_set_v immune_effect;

	explicit card(duel* pd);
	~card();
	static bool card_operation_sort(card* c1, card* c2);

	uint32 get_infos(byte* buf, int32 query_flag, int32 use_cache = TRUE);
	uint32 get_info_location();
	uint32 get_code();
	uint32 get_another_code();
	int32 is_set_card(uint32 set_code);
	int32 is_origin_set_card(uint32 set_code);
	int32 is_pre_set_card(uint32 set_code);
	int32 is_fusion_set_card(uint32 set_code);
	uint32 get_type();
	uint32 get_fusion_type();
	uint32 get_synchro_type();
	uint32 get_xyz_type();
	uint32 get_link_type();
	int32 get_base_attack();
	int32 get_attack();
	int32 get_base_defense();
	int32 get_defense();
	uint32 get_level();
	uint32 get_rank();
	uint32 get_link();
	uint32 get_synchro_level(card* pcard);
	uint32 get_ritual_level(card* pcard);
	uint32 check_xyz_level(card* pcard, uint32 lv);
	uint32 get_attribute();
	uint32 get_fusion_attribute(uint8 playerid);
	uint32 get_race();
	uint32 get_lscale();
	uint32 get_rscale();
	uint32 get_link_marker();
	int32 is_link_marker(uint32 dir);
	uint32 get_linked_zone();
	void get_linked_cards(card_set* cset);
	uint32 get_mutual_linked_zone();
	void get_mutual_linked_cards(card_set * cset);
	int32 is_link_state();
	int32 is_position(int32 pos);
	void set_status(uint32 status, int32 enabled);
	int32 get_status(uint32 status);
	int32 is_status(uint32 status);
	uint32 get_column_zone(int32 loc1, int32 left, int32 right);
	void get_column_cards(card_set* cset, int32 left, int32 right);
	int32 is_all_column();

	void equip(card *target, uint32 send_msg = TRUE);
	void unequip();
	int32 get_union_count();
	int32 get_old_union_count();
	void xyz_overlay(card_set* materials);
	void xyz_add(card* mat, card_set* des);
	void xyz_remove(card* mat);
	void apply_field_effect();
	void cancel_field_effect();
	void enable_field_effect(bool enabled);
	int32 add_effect(effect* peffect);
	void remove_effect(effect* peffect);
	void remove_effect(effect* peffect, effect_container::iterator it);
	int32 copy_effect(uint32 code, uint32 reset, uint32 count);
	int32 replace_effect(uint32 code, uint32 reset, uint32 count);
	void reset(uint32 id, uint32 reset_type);
	void reset_effect_count();
	void refresh_disable_status();
	uint8 refresh_control_status();

	void count_turn(uint16 ct);
	void create_relation(card* target, uint32 reset);
	int32 is_has_relation(card* target);
	void release_relation(card* target);
	void create_relation(const chain& ch);
	int32 is_has_relation(const chain& ch);
	void release_relation(const chain& ch);
	void clear_relate_effect();
	void create_relation(effect* peffect);
	int32 is_has_relation(effect* peffect);
	void release_relation(effect* peffect);
	int32 leave_field_redirect(uint32 reason);
	int32 destination_redirect(uint8 destination, uint32 reason);
	int32 add_counter(uint8 playerid, uint16 countertype, uint16 count, uint8 singly);
	int32 remove_counter(uint16 countertype, uint16 count);
	int32 is_can_add_counter(uint8 playerid, uint16 countertype, uint16 count, uint8 singly);
	int32 get_counter(uint16 countertype);
	void set_material(card_set* materials);
	void add_card_target(card* pcard);
	void cancel_card_target(card* pcard);

	void filter_effect(int32 code, effect_set* eset, uint8 sort = TRUE);
	void filter_single_effect(int32 code, effect_set* eset, uint8 sort = TRUE);
	void filter_single_continuous_effect(int32 code, effect_set* eset, uint8 sort = TRUE);
	void filter_immune_effect();
	void filter_disable_related_cards();
	int32 filter_summon_procedure(uint8 playerid, effect_set* eset, uint8 ignore_count, uint8 min_tribute, uint32 zone);
	int32 check_summon_procedure(effect* peffect, uint8 playerid, uint8 ignore_count, uint8 min_tribute, uint32 zone);
	int32 filter_set_procedure(uint8 playerid, effect_set* eset, uint8 ignore_count, uint8 min_tribute, uint32 zone);
	int32 check_set_procedure(effect* peffect, uint8 playerid, uint8 ignore_count, uint8 min_tribute, uint32 zone);
	void filter_spsummon_procedure(uint8 playerid, effect_set* eset, uint32 summon_type);
	void filter_spsummon_procedure_g(uint8 playerid, effect_set* eset);
	effect* is_affected_by_effect(int32 code);
	effect* is_affected_by_effect(int32 code, card* target);
	effect* check_control_effect();
	int32 fusion_check(group* fusion_m, card* cg, uint32 chkf);
	void fusion_select(uint8 playerid, group* fusion_m, card* cg, uint32 chkf);
	int32 check_fusion_substitute(card* fcard);

	int32 check_unique_code(card* pcard);
	void get_unique_target(card_set* cset, int32 controler);
	int32 check_cost_condition(int32 ecode, int32 playerid);
	int32 check_cost_condition(int32 ecode, int32 playerid, int32 sumtype);
	int32 is_summonable_card();
	int32 is_fusion_summonable_card(uint32 summon_type);
	int32 is_spsummonable(effect* peffect);
	int32 is_summonable(effect* peffect, uint8 min_tribute, uint32 zone = 0x1f);
	int32 is_can_be_summoned(uint8 playerid, uint8 ingore_count, effect* peffect, uint8 min_tribute, uint32 zone = 0x1f);
	int32 get_summon_tribute_count();
	int32 get_set_tribute_count();
	int32 is_can_be_flip_summoned(uint8 playerid);
	int32 is_special_summonable(uint8 playerid, uint32 summon_type);
	int32 is_can_be_special_summoned(effect* reason_effect, uint32 sumtype, uint8 sumpos, uint8 sumplayer, uint8 toplayer, uint8 nocheck, uint8 nolimit, uint32 zone, uint8 nozoneusedcheck = 0);
	int32 is_setable_mzone(uint8 playerid, uint8 ignore_count, effect* peffect, uint8 min_tribute, uint32 zone = 0x1f);
	int32 is_setable_szone(uint8 playerid, uint8 ignore_fd = 0);
	int32 is_affect_by_effect(effect* peffect);
	int32 is_destructable();
	int32 is_destructable_by_battle(card* pcard);
	effect* check_indestructable_by_effect(effect* peffect, uint8 playerid);
	int32 is_destructable_by_effect(effect* peffect, uint8 playerid);
	int32 is_removeable(uint8 playerid);
	int32 is_removeable_as_cost(uint8 playerid);
	int32 is_releasable_by_summon(uint8 playerid, card* pcard);
	int32 is_releasable_by_nonsummon(uint8 playerid);
	int32 is_releasable_by_effect(uint8 playerid, effect* peffect);
	int32 is_capable_send_to_grave(uint8 playerid);
	int32 is_capable_send_to_hand(uint8 playerid);
	int32 is_capable_send_to_deck(uint8 playerid);
	int32 is_capable_send_to_extra(uint8 playerid);
	int32 is_capable_cost_to_grave(uint8 playerid);
	int32 is_capable_cost_to_hand(uint8 playerid);
	int32 is_capable_cost_to_deck(uint8 playerid);
	int32 is_capable_cost_to_extra(uint8 playerid);
	int32 is_capable_attack();
	int32 is_capable_attack_announce(uint8 playerid);
	int32 is_capable_change_position(uint8 playerid);
	int32 is_capable_turn_set(uint8 playerid);
	int32 is_capable_change_control();
	int32 is_control_can_be_changed(int32 ignore_mzone, uint32 zone);
	int32 is_capable_be_battle_target(card* pcard);
	int32 is_capable_be_effect_target(effect* peffect, uint8 playerid);
	int32 is_can_be_fusion_material(card* fcard);
	int32 is_can_be_synchro_material(card* scard, card* tuner = 0);
	int32 is_can_be_ritual_material(card* scard);
	int32 is_can_be_xyz_material(card* scard);
	int32 is_can_be_link_material(card* scard);
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
#define RACE_CYBERS			0x1000000	//
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
//Summon Type
#define SUMMON_TYPE_NORMAL		0x10000000
#define SUMMON_TYPE_ADVANCE		0x11000000
#define SUMMON_TYPE_DUAL		0x12000000
#define SUMMON_TYPE_FLIP		0x20000000
#define SUMMON_TYPE_SPECIAL		0x40000000
#define SUMMON_TYPE_FUSION		0x43000000
#define SUMMON_TYPE_RITUAL		0x45000000
#define SUMMON_TYPE_SYNCHRO		0x46000000
#define SUMMON_TYPE_XYZ			0x49000000
#define SUMMON_TYPE_PENDULUM	0x4a000000
#define SUMMON_TYPE_LINK		0x4c000000
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
#define STATUS_UNION				0x100000
#define STATUS_ATTACK_CANCELED		0x200000
#define STATUS_INITIALIZING			0x400000
#define STATUS_ACTIVATED			0x800000
#define STATUS_JUST_POS				0x1000000
#define STATUS_CONTINUOUS_POS		0x2000000
#define STATUS_FORBIDDEN			0x4000000
#define STATUS_ACT_FROM_HAND		0x8000000
#define STATUS_OPPO_BATTLE			0x10000000
#define STATUS_FLIP_SUMMON_TURN		0x20000000
#define STATUS_SPSUMMON_TURN		0x40000000
//Counter
#define COUNTER_WITHOUT_PERMIT	0x1000
#define COUNTER_NEED_ENABLE		0x2000
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
#define QUERY_IS_DISABLED	0x80000
#define QUERY_IS_PUBLIC		0x100000
#define QUERY_LSCALE		0x200000
#define QUERY_RSCALE		0x400000
#define QUERY_LINK			0x800000

#define ASSUME_CODE			1
#define ASSUME_TYPE			2
#define ASSUME_LEVEL		3
#define ASSUME_RANK			4
#define ASSUME_ATTRIBUTE	5
#define ASSUME_RACE			6
#define ASSUME_ATTACK		7
#define ASSUME_DEFENSE		8

#define LINK_MARKER_BOTTOM_LEFT		0001
#define LINK_MARKER_BOTTOM			0002
#define LINK_MARKER_BOTTOM_RIGHT	0004
#define LINK_MARKER_LEFT			0010
#define LINK_MARKER_RIGHT			0040
#define LINK_MARKER_TOP_LEFT		0100
#define LINK_MARKER_TOP				0200
#define LINK_MARKER_TOP_RIGHT		0400

#endif /* CARD_H_ */
