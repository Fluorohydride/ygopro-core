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
#include <tuple>

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

	void clear();
};

struct card_state {
	card_state()
		: code(0), code2(0), setcode(0), type(0), level(0), rank(0), link(0), lscale(0), rscale(0), attribute(0), race(0), attack(0), defense(0), base_attack(0), base_defense(0), 
		controler(PLAYER_NONE), location(0), sequence(0), position(0), reason(0), pzone(false), reason_card(nullptr), reason_player(PLAYER_NONE), reason_effect(nullptr) {}

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
	int32 status;
	uint32 lscale;
	uint32 rscale;
	uint32 link_marker;
};

struct material_info {
	// Synchron
	card* limit_tuner;
	group* limit_syn;
	int32 limit_syn_minc;
	int32 limit_syn_maxc;
	// Xyz
	group* limit_xyz;
	int32 limit_xyz_minc;
	int32 limit_xyz_maxc;
	// Link
	group* limit_link;
	card* limit_link_card;
	int32 limit_link_minc;
	int32 limit_link_maxc;

	material_info()
		: limit_tuner(nullptr), limit_syn(nullptr), limit_syn_minc(0), limit_syn_maxc(0), limit_xyz(nullptr), limit_xyz_minc(0), limit_xyz_maxc(0), 
		limit_link(nullptr), limit_link_card(nullptr), limit_link_minc(0), limit_link_maxc(0) {}
};
const material_info null_info;

class card {
public:
	struct effect_relation_hash {
		inline std::size_t operator()(const std::pair<effect*, uint16>& v) const {
			return std::hash<uint16>()(v.second);
		}
	};
	using card_vector = std::vector<card*>;
	using effect_container = std::multimap<uint32, effect*>;
	using card_set = std::set<card*, card_sort>;
	using effect_indexer = std::unordered_map<effect*, effect_container::iterator>;
	using effect_relation = std::unordered_set<std::pair<effect*, uint16>, effect_relation_hash>;
	using relation_map = std::unordered_map<card*, uint32>;
	using counter_map = std::map<uint16, std::array<uint16, 2>>;
	using effect_count = std::map<uint32, int32>;
	class attacker_map : public std::unordered_map<uint16, std::pair<card*, uint32>> {
	public:
		void addcard(card* pcard);
		uint32 findcard(card* pcard);
	};
	struct sendto_param_t {
		void set(uint8 p, uint8 pos, uint8 loc, uint8 seq = 0) {
			playerid = p;
			position = pos;
			location = loc;
			sequence = seq;
		}
		void clear() {
			playerid = 0;
			position = 0;
			location = 0;
			sequence = 0;
		}
		uint8 playerid;
		uint8 position;
		uint8 location;
		uint8 sequence;
	};
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
	sendto_param_t sendto_param;
	uint32 release_param;
	uint32 sum_param;
	uint32 position_param;
	uint32 spsummon_param;
	uint32 to_field_param;
	uint8 attack_announce_count;
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
	effect_container target_effect;
	effect_container xmaterial_effect;
	effect_indexer indexer;
	effect_relation relate_effect;
	effect_set_v immune_effect;

	explicit card(duel* pd);
	~card() = default;
	static bool card_operation_sort(card* c1, card* c2);
	const bool is_extra_deck_monster() { return !!(data.type & (TYPE_FUSION | TYPE_SYNCHRO | TYPE_XYZ | TYPE_LINK)); }

	uint32 get_infos(byte* buf, int32 query_flag, int32 use_cache = TRUE);
	uint32 get_info_location();
	uint32 second_code(uint32 code);
	uint32 get_code();
	uint32 get_another_code();
	int32 is_set_card(uint32 set_code);
	int32 is_origin_set_card(uint32 set_code);
	int32 is_pre_set_card(uint32 set_code);
	int32 is_fusion_set_card(uint32 set_code);
	int32 is_link_set_card(uint32 set_code);
	uint32 get_type();
	uint32 get_fusion_type();
	uint32 get_synchro_type();
	uint32 get_xyz_type();
	uint32 get_link_type();
	int32 get_base_attack();
	int32 get_attack();
	int32 get_base_defense();
	int32 get_defense();
	int32 get_battle_attack();
	int32 get_battle_defense();
	uint32 get_level();
	uint32 get_rank();
	uint32 get_link();
	uint32 get_synchro_level(card* pcard);
	uint32 get_ritual_level(card* pcard);
	uint32 check_xyz_level(card* pcard, uint32 lv);
	uint32 get_attribute();
	uint32 get_fusion_attribute(uint8 playerid);
	uint32 get_link_attribute(uint8 playerid);
	uint32 get_grave_attribute(uint8 playerid);
	uint32 get_race();
	uint32 get_link_race(uint8 playerid);
	uint32 get_grave_race(uint8 playerid);
	uint32 get_lscale();
	uint32 get_rscale();
	uint32 get_link_marker();
	int32 is_link_marker(uint32 dir);
	uint32 get_linked_zone();
	void get_linked_cards(card_set* cset);
	uint32 get_mutual_linked_zone();
	void get_mutual_linked_cards(card_set * cset);
	int32 is_link_state();
	int32 is_extra_link_state();
	int32 is_position(int32 pos);
	void set_status(uint32 status, int32 enabled);
	int32 get_status(uint32 status);
	int32 is_status(uint32 status);
	uint32 get_column_zone(int32 location);
	void get_column_cards(card_set* cset);
	int32 is_all_column();

	void equip(card *target, uint32 send_msg = TRUE);
	void unequip();
	int32 get_union_count();
	int32 get_old_union_count();
	void xyz_overlay(card_set* materials);
	void xyz_add(card* mat);
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
	std::tuple<uint8, effect*> refresh_control_status();

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
	int32 is_can_add_counter(uint8 playerid, uint16 countertype, uint16 count, uint8 singly, uint32 loc);
	int32 is_can_have_counter(uint16 countertype);
	int32 get_counter(uint16 countertype);
	void set_material(card_set* materials);
	void add_card_target(card* pcard);
	void cancel_card_target(card* pcard);
	void clear_card_target();

	void filter_effect(int32 code, effect_set* eset, uint8 sort = TRUE);
	void filter_single_continuous_effect(int32 code, effect_set* eset, uint8 sort = TRUE);
	void filter_self_effect(int32 code, effect_set* eset, uint8 sort = TRUE);
	void filter_immune_effect();
	void filter_disable_related_cards();
	int32 filter_summon_procedure(uint8 playerid, effect_set* eset, uint8 ignore_count, uint8 min_tribute, uint32 zone);
	int32 check_summon_procedure(effect* proc, uint8 playerid, uint8 ignore_count, uint8 min_tribute, uint32 zone);
	int32 filter_set_procedure(uint8 playerid, effect_set* eset, uint8 ignore_count, uint8 min_tribute, uint32 zone);
	int32 check_set_procedure(effect* proc, uint8 playerid, uint8 ignore_count, uint8 min_tribute, uint32 zone);
	void filter_spsummon_procedure(uint8 playerid, effect_set* eset, uint32 summon_type, material_info info = null_info);
	void filter_spsummon_procedure_g(uint8 playerid, effect_set* eset);
	effect* is_affected_by_effect(int32 code);
	effect* is_affected_by_effect(int32 code, card* target);
	int32 fusion_check(group* fusion_m, card* cg, uint32 chkf, uint8 not_material);
	void fusion_select(uint8 playerid, group* fusion_m, card* cg, uint32 chkf, uint8 not_material);
	int32 check_fusion_substitute(card* fcard);
	int32 is_not_tuner(card* scard);

	int32 check_unique_code(card* pcard);
	void get_unique_target(card_set* cset, int32 controler, card* icard = 0);
	int32 check_cost_condition(int32 ecode, int32 playerid);
	int32 check_cost_condition(int32 ecode, int32 playerid, int32 sumtype);
	int32 is_summonable_card();
	int32 is_fusion_summonable_card(uint32 summon_type);
	int32 is_spsummonable(effect* proc, material_info info = null_info);
	int32 is_summonable(effect* proc, uint8 min_tribute, uint32 zone = 0x1f, uint32 releasable = 0xff00ff);
	int32 is_can_be_summoned(uint8 playerid, uint8 ingore_count, effect* peffect, uint8 min_tribute, uint32 zone = 0x1f);
	int32 get_summon_tribute_count();
	int32 get_set_tribute_count();
	int32 is_can_be_flip_summoned(uint8 playerid);
	int32 is_special_summonable(uint8 playerid, uint32 summon_type, material_info info = null_info);
	int32 is_can_be_special_summoned(effect* reason_effect, uint32 sumtype, uint8 sumpos, uint8 sumplayer, uint8 toplayer, uint8 nocheck, uint8 nolimit, uint32 zone);
	int32 is_setable_mzone(uint8 playerid, uint8 ignore_count, effect* peffect, uint8 min_tribute, uint32 zone = 0x1f);
	int32 is_setable_szone(uint8 playerid, uint8 ignore_fd = 0);
	int32 is_affect_by_effect(effect* reason_effect);
	int32 is_destructable();
	int32 is_destructable_by_battle(card* pcard);
	effect* check_indestructable_by_effect(effect* reason_effect, uint8 playerid);
	int32 is_destructable_by_effect(effect* reason_effect, uint8 playerid);
	int32 is_removeable(uint8 playerid, uint8 pos, uint32 reason);
	int32 is_removeable_as_cost(uint8 playerid, uint8 pos);
	int32 is_releasable_by_summon(uint8 playerid, card* pcard);
	int32 is_releasable_by_nonsummon(uint8 playerid);
	int32 is_releasable_by_effect(uint8 playerid, effect* reason_effect);
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
	int32 is_capable_change_position_by_effect(uint8 playerid);
	int32 is_capable_turn_set(uint8 playerid);
	int32 is_capable_change_control();
	int32 is_control_can_be_changed(int32 ignore_mzone, uint32 zone);
	int32 is_capable_be_battle_target(card* pcard);
	int32 is_capable_be_effect_target(effect* reason_effect, uint8 playerid);
	int32 is_capable_overlay(uint8 playerid);
	int32 is_can_be_fusion_material(card* fcard, uint32 summon_type);
	int32 is_can_be_synchro_material(card* scard, card* tuner = 0);
	int32 is_can_be_ritual_material(card* scard);
	int32 is_can_be_xyz_material(card* scard);
	int32 is_can_be_link_material(card* scard);
};

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
//Counter
#define COUNTER_WITHOUT_PERMIT	0x1000
//#define COUNTER_NEED_ENABLE		0x2000

#define ASSUME_CODE			1
#define ASSUME_TYPE			2
#define ASSUME_LEVEL		3
#define ASSUME_RANK			4
#define ASSUME_ATTRIBUTE	5
#define ASSUME_RACE			6
#define ASSUME_ATTACK		7
#define ASSUME_DEFENSE		8

//double-name cards
#define CARD_MARINE_DOLPHIN	78734254
#define CARD_TWINKLE_MOSS	13857930

#endif /* CARD_H_ */
