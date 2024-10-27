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
#include "card_data.h"
#include "sort.h"
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

using card_set = std::set<card*, card_sort>;
using card_vector = std::vector<card*>;
using effect_container = std::multimap<uint32, effect*>;
using effect_indexer = std::unordered_map<effect*, effect_container::iterator>;
using effect_collection = std::unordered_set<effect*>;

using effect_filter = bool(*)(card* self, effect* peffect);
using effect_filter_target = bool(*)(card* self, effect* peffect, card* target);

struct card_state {
	uint32 code{ 0 };
	uint32 code2{ 0 };
	std::vector<uint16_t> setcode;
	uint32 type{ 0 };
	uint32 level{ 0 };
	uint32 rank{ 0 };
	uint32 link{ 0 };
	uint32 lscale{ 0 };
	uint32 rscale{ 0 };
	uint32 attribute{ 0 };
	uint32 race{ 0 };
	int32 attack{ 0 };
	int32 defense{ 0 };
	int32 base_attack{ 0 };
	int32 base_defense{ 0 };
	uint8 controler{ PLAYER_NONE };
	uint8 location{ 0 };
	uint8 sequence{ 0 };
	uint8 position{ 0 };
	uint32 reason{ 0 };
	bool pzone{ false };
	card* reason_card{ nullptr };
	uint8 reason_player{ PLAYER_NONE };
	effect* reason_effect{ nullptr };

	bool is_location(int32 loc) const;
	bool is_main_mzone() const {
		return location == LOCATION_MZONE && sequence >= 0 && sequence <= 4;
	}
	bool is_stzone() const {
		return location == LOCATION_SZONE && sequence >= 0 && sequence <= 4;
	}
	void init_state();
};

struct query_cache {
	uint32 info_location{ UINT32_MAX };
	uint32 current_code{ UINT32_MAX };
	uint32 type{ UINT32_MAX };
	uint32 level{ UINT32_MAX };
	uint32 rank{ UINT32_MAX };
	uint32 link{ UINT32_MAX };
	uint32 attribute{ UINT32_MAX };
	uint32 race{ UINT32_MAX };
	int32 attack{ -1 };
	int32 defense{ -1 };
	int32 base_attack{ -1 };
	int32 base_defense{ -1 };
	uint32 reason{ UINT32_MAX };
	uint32 status{ UINT32_MAX };
	uint32 lscale{ UINT32_MAX };
	uint32 rscale{ UINT32_MAX };
	uint32 link_marker{ UINT32_MAX };

	void clear_cache();
};

struct material_info {
	// Synchron
	card* limit_tuner{ nullptr };
	group* limit_syn{ nullptr };
	int32 limit_syn_minc{ 0 };
	int32 limit_syn_maxc{ 0 };
	// Xyz
	group* limit_xyz{ nullptr };
	int32 limit_xyz_minc{ 0 };
	int32 limit_xyz_maxc{ 0 };
	// Link
	group* limit_link{ nullptr };
	card* limit_link_card{ nullptr };
	int32 limit_link_minc{ 0 };
	int32 limit_link_maxc{ 0 };
};
const material_info null_info;

constexpr uint32 CARD_MARINE_DOLPHIN = 78734254;
constexpr uint32 CARD_TWINKLE_MOSS = 13857930;
constexpr uint32 CARD_TIMAEUS = 1784686;
constexpr uint32 CARD_CRITIAS = 11082056;
constexpr uint32 CARD_HERMOS = 46232525;

class card {
public:
	struct effect_relation_hash {
		inline std::size_t operator()(const std::pair<effect*, uint16>& v) const {
			return std::hash<uint16>()(v.second);
		}
	};
	using effect_relation = std::unordered_set<std::pair<effect*, uint16>, effect_relation_hash>;
	using relation_map = std::unordered_map<card*, uint32>;
	using counter_map = std::map<uint16, uint16>;
	using effect_count = std::map<uint32, int32>;
	class attacker_map : public std::unordered_map<uint32, std::pair<card*, uint32>> {
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
		uint8 playerid{ 0 };
		uint8 position{ 0 };
		uint8 location{ 0 };
		uint8 sequence{ 0 };
	};
	static const std::unordered_map<uint32, uint32> second_code;

	int32 ref_handle;
	duel* pduel;
	card_data data;
	card_state previous;
	card_state temp;
	card_state current;
	card_state spsummon;
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
	int32 xyz_materials_previous_count_onfield;
	effect_container single_effect;
	effect_container field_effect;
	effect_container equip_effect;
	effect_container target_effect;
	effect_container xmaterial_effect;
	effect_indexer indexer;
	effect_relation relate_effect;
	effect_set_v immune_effect;
	effect_collection initial_effect;
	effect_collection owning_effect;

	explicit card(duel* pd);
	~card() = default;
	static bool card_operation_sort(card* c1, card* c2);
	bool is_extra_deck_monster() const { return !!(data.type & TYPES_EXTRA_DECK); }

	int32 get_infos(byte* buf, uint32 query_flag, int32 use_cache = TRUE);
	uint32 get_info_location() const;
	uint32 get_original_code() const;
	std::tuple<uint32, uint32> get_original_code_rule() const;
	uint32 get_code();
	uint32 get_another_code();
	static bool check_card_setcode(uint32 code, uint32 value);
	int32 is_set_card(uint32 set_code);
	int32 is_origin_set_card(uint32 set_code);
	int32 is_pre_set_card(uint32 set_code);
	int32 is_fusion_set_card(uint32 set_code);
	int32 is_link_set_card(uint32 set_code);
	int32 is_special_summon_set_card(uint32 set_code);
	uint32 get_type();
	uint32 get_fusion_type();
	uint32 get_synchro_type();
	uint32 get_xyz_type();
	uint32 get_link_type();
	std::pair<int32, int32> get_base_atk_def();
	std::pair<int32, int32> get_atk_def();
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
	int32 get_status(uint32 status) const;
	int32 is_status(uint32 status) const;
	uint32 get_column_zone(int32 location);
	void get_column_cards(card_set* cset);
	int32 is_all_column();
	uint8 get_select_sequence(uint8 *deck_seq_pointer);
	uint32 get_select_info_location(uint8 *deck_seq_pointer);
	int32 is_treated_as_not_on_field();

	void equip(card* target, uint32 send_msg = TRUE);
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
	effect_indexer::iterator remove_effect(effect* peffect);
	int32 copy_effect(uint32 code, uint32 reset, int32 count);
	int32 replace_effect(uint32 code, uint32 reset, int32 count);
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
	void delete_card_target(uint32 send_msg);
	void clear_card_target();
	void set_special_summon_status(effect* peffect);

	template<typename T>
	void filter_effect_container(const effect_container& container, uint32 code, effect_filter f, T& eset);
	void filter_effect(uint32 code, effect_set* eset, uint8 sort = TRUE);
	void filter_single_continuous_effect(uint32 code, effect_set* eset, uint8 sort = TRUE);
	void filter_self_effect(uint32 code, effect_set* eset, uint8 sort = TRUE);
	void filter_immune_effect();
	void filter_disable_related_cards();
	int32 filter_summon_procedure(uint8 playerid, effect_set* eset, uint8 ignore_count, uint8 min_tribute, uint32 zone);
	int32 check_summon_procedure(effect* proc, uint8 playerid, uint8 ignore_count, uint8 min_tribute, uint32 zone);
	int32 filter_set_procedure(uint8 playerid, effect_set* eset, uint8 ignore_count, uint8 min_tribute, uint32 zone);
	int32 check_set_procedure(effect* proc, uint8 playerid, uint8 ignore_count, uint8 min_tribute, uint32 zone);
	void filter_spsummon_procedure(uint8 playerid, effect_set* eset, uint32 summon_type, material_info info = null_info);
	void filter_spsummon_procedure_g(uint8 playerid, effect_set* eset);
	effect* find_effect(const effect_container& container, uint32 code, effect_filter f);
	effect* find_effect_with_target(const effect_container& container, uint32 code, effect_filter_target f, card* target);
	effect* is_affected_by_effect(uint32 code);
	effect* is_affected_by_effect(int32 code, card* target);
	int32 fusion_check(group* fusion_m, card* cg, uint32 chkf, uint8 not_material);
	void fusion_select(uint8 playerid, group* fusion_m, card* cg, uint32 chkf, uint8 not_material);
	int32 check_fusion_substitute(card* fcard);
	int32 is_not_tuner(card* scard);
	int32 is_tuner(card* scard);

	int32 check_unique_code(card* pcard);
	void get_unique_target(card_set* cset, int32 controler, card* icard = nullptr);
	int32 check_cost_condition(int32 ecode, int32 playerid);
	int32 check_cost_condition(int32 ecode, int32 playerid, int32 sumtype);
	int32 is_summonable_card() const;
	int32 is_spsummonable_card();
	int32 is_fusion_summonable_card(uint32 summon_type);
	int32 is_spsummonable(effect* proc, material_info info = null_info);
	int32 is_summonable(effect* proc, uint8 min_tribute, uint32 zone = 0x1f, uint32 releasable = 0xff00ff);
	int32 is_can_be_summoned(uint8 playerid, uint8 ingore_count, effect* peffect, uint8 min_tribute, uint32 zone = 0x1f);
	int32 is_summon_negatable(uint32 sumtype, effect* reason_effect);
	int32 get_summon_tribute_count();
	int32 get_set_tribute_count();
	int32 is_can_be_flip_summoned(uint8 playerid);
	int32 is_special_summonable(uint8 playerid, uint32 summon_type, material_info info = null_info);
	int32 is_can_be_special_summoned(effect* reason_effect, uint32 sumtype, uint8 sumpos, uint8 sumplayer, uint8 toplayer, uint8 nocheck, uint8 nolimit, uint32 zone);
	uint8 get_spsummonable_position(effect* reason_effect, uint32 sumtype, uint8 sumpos, uint8 sumplayer, uint8 toplayer);
	int32 is_setable_mzone(uint8 playerid, uint8 ignore_count, effect* peffect, uint8 min_tribute, uint32 zone = 0x1f);
	int32 is_setable_szone(uint8 playerid, uint8 ignore_fd = 0);
	int32 is_affect_by_effect(effect* reason_effect);
	int32 is_can_be_disabled_by_effect(effect* reason_effect, bool is_monster_effect);
	int32 is_destructable();
	int32 is_destructable_by_battle(card* pcard);
	effect* check_indestructable_by_effect(effect* reason_effect, uint8 playerid);
	int32 is_destructable_by_effect(effect* reason_effect, uint8 playerid);
	int32 is_removeable(uint8 playerid, uint8 pos, uint32 reason);
	int32 is_removeable_as_cost(uint8 playerid, uint8 pos);
	int32 is_releasable_by_summon(uint8 playerid, card* pcard);
	int32 is_releasable_by_nonsummon(uint8 playerid, uint32 reason);
	int32 is_releasable_by_effect(uint8 playerid, effect* reason_effect);
	int32 is_capable_send_to_grave(uint8 playerid);
	int32 is_capable_send_to_hand(uint8 playerid);
	int32 is_capable_send_to_deck(uint8 playerid, uint8 send_activating = FALSE);
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
	int32 is_can_be_synchro_material(card* scard, card* tuner = nullptr);
	int32 is_can_be_ritual_material(card* scard);
	int32 is_can_be_xyz_material(card* scard);
	int32 is_can_be_link_material(card* scard);
	int32 is_original_effect_property(int32 filter);
	int32 is_effect_property(int32 filter);
};

//Summon Type in summon_info
#define SUMMON_TYPE_NORMAL		0x10000000U
#define SUMMON_TYPE_ADVANCE		0x11000000U
#define SUMMON_TYPE_FLIP		0x20000000U
#define SUMMON_TYPE_SPECIAL		0x40000000U
#define SUMMON_TYPE_FUSION		0x43000000U
#define SUMMON_TYPE_RITUAL		0x45000000U
#define SUMMON_TYPE_SYNCHRO		0x46000000U
#define SUMMON_TYPE_XYZ			0x49000000U
#define SUMMON_TYPE_PENDULUM	0x4a000000U
#define SUMMON_TYPE_LINK		0x4c000000U

//Gemini Summon
#define SUMMON_TYPE_DUAL		0x12000000U

//bitfield blocks
#define SUMMON_VALUE_MAIN_TYPE		0xf0000000U
#define SUMMON_VALUE_SUB_TYPE		0x0f000000U
#define SUMMON_VALUE_LOCATION		0x00ff0000U
#define SUMMON_VALUE_CUSTOM_TYPE	0x0000ffffU
constexpr uint32 DEFAULT_SUMMON_TYPE = SUMMON_VALUE_MAIN_TYPE | SUMMON_VALUE_SUB_TYPE | SUMMON_VALUE_CUSTOM_TYPE;

#define SUMMON_VALUE_FUTURE_FUSION	0x18

//Counter
#define COUNTER_WITHOUT_PERMIT	0x1000U
//#define COUNTER_NEED_ENABLE		0x2000

//Assume
#define ASSUME_CODE			1
#define ASSUME_TYPE			2
#define ASSUME_LEVEL		3
#define ASSUME_RANK			4
#define ASSUME_ATTRIBUTE	5
#define ASSUME_RACE			6
#define ASSUME_ATTACK		7
#define ASSUME_DEFENSE		8

//Special Summon effect info
#define SUMMON_INFO_CODE			0x01
#define SUMMON_INFO_CODE2			0x02
#define SUMMON_INFO_TYPE			0x04
#define SUMMON_INFO_LEVEL			0x08
#define SUMMON_INFO_RANK			0x10
#define SUMMON_INFO_ATTRIBUTE		0x20
#define SUMMON_INFO_RACE			0x40
#define SUMMON_INFO_ATTACK			0x80
#define SUMMON_INFO_DEFENSE			0x100
#define SUMMON_INFO_REASON_EFFECT	0x200
#define SUMMON_INFO_REASON_PLAYER	0x400

#endif /* CARD_H_ */
