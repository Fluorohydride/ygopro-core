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
using effect_container = std::multimap<uint32_t, effect*>;
using effect_indexer = std::unordered_map<effect*, effect_container::iterator>;
using effect_collection = std::unordered_set<effect*>;

using effect_filter = bool(*)(card* self, effect* peffect);
using effect_filter_target = bool(*)(card* self, effect* peffect, card* target);

struct card_state {
	uint32_t code{ 0 };
	uint32_t code2{ 0 };
	std::vector<uint16_t> setcode;
	uint32_t type{ 0 };
	uint32_t level{ 0 };
	uint32_t rank{ 0 };
	uint32_t link{ 0 };
	uint32_t lscale{ 0 };
	uint32_t rscale{ 0 };
	uint32_t attribute{ 0 };
	uint32_t race{ 0 };
	int32_t attack{ 0 };
	int32_t defense{ 0 };
	int32_t base_attack{ 0 };
	int32_t base_defense{ 0 };
	uint8_t controler{ PLAYER_NONE };
	uint8_t location{ 0 };
	uint8_t sequence{ 0 };
	uint8_t position{ 0 };
	uint32_t reason{ 0 };
	bool pzone{ false };
	card* reason_card{ nullptr };
	uint8_t reason_player{ PLAYER_NONE };
	effect* reason_effect{ nullptr };

	bool is_location(uint32_t loc) const;
	bool is_main_mzone() const {
		return location == LOCATION_MZONE && sequence >= 0 && sequence <= 4;
	}
	bool is_stzone() const {
		return location == LOCATION_SZONE && sequence >= 0 && sequence <= 4;
	}
	void init_state();
};

struct query_cache {
	uint32_t info_location{ UINT32_MAX };
	uint32_t current_code{ UINT32_MAX };
	uint32_t type{ UINT32_MAX };
	uint32_t level{ UINT32_MAX };
	uint32_t rank{ UINT32_MAX };
	uint32_t link{ UINT32_MAX };
	uint32_t attribute{ UINT32_MAX };
	uint32_t race{ UINT32_MAX };
	int32_t attack{ -1 };
	int32_t defense{ -1 };
	int32_t base_attack{ -1 };
	int32_t base_defense{ -1 };
	uint32_t reason{ UINT32_MAX };
	uint32_t status{ UINT32_MAX };
	uint32_t lscale{ UINT32_MAX };
	uint32_t rscale{ UINT32_MAX };
	uint32_t link_marker{ UINT32_MAX };

	void clear_cache();
};

struct material_info {
	// Synchron
	card* limit_tuner{ nullptr };
	group* limit_syn{ nullptr };
	int32_t limit_syn_minc{ 0 };
	int32_t limit_syn_maxc{ 0 };
	// Xyz
	group* limit_xyz{ nullptr };
	int32_t limit_xyz_minc{ 0 };
	int32_t limit_xyz_maxc{ 0 };
	// Link
	group* limit_link{ nullptr };
	card* limit_link_card{ nullptr };
	int32_t limit_link_minc{ 0 };
	int32_t limit_link_maxc{ 0 };
};
const material_info null_info;

class card {
public:
	struct effect_relation_hash {
		inline std::size_t operator()(const std::pair<effect*, uint16_t>& v) const {
			return std::hash<uint16_t>()(v.second);
		}
	};
	using effect_relation = std::unordered_set<std::pair<effect*, uint16_t>, effect_relation_hash>;
	using relation_map = std::unordered_map<card*, uint32_t>;
	using counter_map = std::map<uint16_t, uint16_t>;
	using effect_count = std::map<uint32_t, int32_t>;
	class attacker_map : public std::unordered_map<uint32_t, std::pair<card*, uint32_t>> {
	public:
		void addcard(card* pcard);
		uint32_t findcard(card* pcard);
	};
	struct sendto_param_t {
		void set(uint8_t p, uint8_t pos, uint8_t loc, uint8_t seq = 0) {
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
		uint8_t playerid{ 0 };
		uint8_t position{ 0 };
		uint8_t location{ 0 };
		uint8_t sequence{ 0 };
	};

	int32_t ref_handle;
	duel* pduel;
	card_data data;
	card_state previous;
	card_state temp;
	card_state current;
	card_state spsummon;
	query_cache q_cache;
	uint8_t owner;
	uint8_t summon_player;
	uint32_t summon_info;
	uint32_t status;
	sendto_param_t sendto_param;
	uint32_t release_param;
	uint32_t sum_param;
	uint32_t position_param;
	uint32_t spsummon_param;
	uint32_t to_field_param;
	uint8_t attack_announce_count;
	uint8_t direct_attackable;
	uint8_t announce_count;
	uint8_t attacked_count;
	uint8_t attack_all_target;
	uint8_t attack_controler;
	uint16_t cardid;
	uint32_t fieldid;
	uint32_t fieldid_r;
	uint16_t turnid;
	uint16_t turn_counter;
	uint8_t unique_pos[2];
	uint32_t unique_fieldid;
	uint32_t unique_code;
	uint32_t unique_location;
	int32_t unique_function;
	effect* unique_effect;
	uint32_t spsummon_code;
	uint16_t spsummon_counter[2];
	uint8_t assume_type;
	uint32_t assume_value;
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
	int32_t xyz_materials_previous_count_onfield;
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
	static bool check_card_setcode(uint32_t code, uint32_t value);
	bool is_extra_deck_monster() const { return !!(data.type & TYPES_EXTRA_DECK); }

	int32_t get_infos(byte* buf, uint32_t query_flag, int32_t use_cache = TRUE);
	uint32_t get_info_location() const;
	uint32_t get_original_code() const;
	std::tuple<uint32_t, uint32_t> get_original_code_rule() const;
	uint32_t get_code();
	uint32_t get_another_code();
	int32_t is_set_card(uint32_t set_code);
	int32_t is_origin_set_card(uint32_t set_code);
	int32_t is_pre_set_card(uint32_t set_code);
	int32_t is_fusion_set_card(uint32_t set_code);
	int32_t is_link_set_card(uint32_t set_code);
	int32_t is_special_summon_set_card(uint32_t set_code);
	uint32_t get_type();
	uint32_t get_fusion_type();
	uint32_t get_synchro_type();
	uint32_t get_xyz_type();
	uint32_t get_link_type();
	std::pair<int32_t, int32_t> get_base_atk_def();
	std::pair<int32_t, int32_t> get_atk_def();
	int32_t get_base_attack();
	int32_t get_attack();
	int32_t get_base_defense();
	int32_t get_defense();
	int32_t get_battle_attack();
	int32_t get_battle_defense();
	uint32_t get_level();
	uint32_t get_rank();
	uint32_t get_link();
	uint32_t get_synchro_level(card* pcard);
	uint32_t get_ritual_level(card* pcard);
	uint32_t check_xyz_level(card* pcard, uint32_t lv);
	uint32_t get_attribute();
	uint32_t get_fusion_attribute(uint8_t playerid);
	uint32_t get_link_attribute(uint8_t playerid);
	uint32_t get_grave_attribute(uint8_t playerid);
	uint32_t get_race();
	uint32_t get_link_race(uint8_t playerid);
	uint32_t get_grave_race(uint8_t playerid);
	uint32_t get_lscale();
	uint32_t get_rscale();
	uint32_t get_link_marker();
	uint32_t is_link_marker(uint32_t dir);
	uint32_t get_linked_zone();
	void get_linked_cards(card_set* cset);
	uint32_t get_mutual_linked_zone();
	void get_mutual_linked_cards(card_set * cset);
	int32_t is_link_state();
	int32_t is_extra_link_state();
	int32_t is_position(uint32_t pos) const;
	void set_status(uint32_t status, int32_t enabled);
	int32_t get_status(uint32_t status) const;
	int32_t is_status(uint32_t status) const;
	uint32_t get_column_zone(int32_t location);
	void get_column_cards(card_set* cset);
	int32_t is_all_column();
	uint8_t get_select_sequence(uint8_t *deck_seq_pointer);
	uint32_t get_select_info_location(uint8_t *deck_seq_pointer);
	int32_t is_treated_as_not_on_field() const;

	void equip(card* target, uint32_t send_msg = TRUE);
	void unequip();
	int32_t get_union_count();
	int32_t get_old_union_count();
	void xyz_overlay(const card_set& materials);
	void xyz_add(card* mat);
	void xyz_remove(card* mat);
	void apply_field_effect();
	void cancel_field_effect();
	void enable_field_effect(bool enabled);
	int32_t add_effect(effect* peffect);
	effect_indexer::iterator remove_effect(effect* peffect);
	int32_t copy_effect(uint32_t code, uint32_t reset, int32_t count);
	int32_t replace_effect(uint32_t code, uint32_t reset, int32_t count);
	void reset(uint32_t id, uint32_t reset_type);
	void reset_effect_count();
	void refresh_disable_status();
	std::tuple<uint8_t, effect*> refresh_control_status();

	void count_turn(uint16_t ct);
	void create_relation(card* target, uint32_t reset);
	int32_t is_has_relation(card* target);
	void release_relation(card* target);
	void create_relation(const chain& ch);
	int32_t is_has_relation(const chain& ch);
	void release_relation(const chain& ch);
	void clear_relate_effect();
	void create_relation(effect* peffect);
	int32_t is_has_relation(effect* peffect);
	void release_relation(effect* peffect);
	int32_t leave_field_redirect(uint32_t reason);
	int32_t destination_redirect(uint8_t destination, uint32_t reason);
	int32_t add_counter(uint8_t playerid, uint16_t countertype, uint16_t count, uint8_t singly);
	int32_t remove_counter(uint16_t countertype, uint16_t count);
	int32_t is_can_add_counter(uint8_t playerid, uint16_t countertype, uint16_t count, uint8_t singly, uint32_t loc);
	int32_t is_can_have_counter(uint16_t countertype);
	int32_t get_counter(uint16_t countertype);
	void set_material(card_set* materials);
	void add_card_target(card* pcard);
	void cancel_card_target(card* pcard);
	void delete_card_target(uint32_t send_msg);
	void clear_card_target();
	void set_special_summon_status(effect* peffect);

	template<typename T>
	void filter_effect_container(const effect_container& container, uint32_t code, effect_filter f, T& eset);
	void filter_effect_container(const effect_container& container, uint32_t code, effect_filter f, effect_collection& eset);
	void filter_effect(uint32_t code, effect_set* eset, uint8_t sort = TRUE);
	void filter_single_continuous_effect(uint32_t code, effect_set* eset, uint8_t sort = TRUE);
	void filter_self_effect(uint32_t code, effect_set* eset, uint8_t sort = TRUE);
	void filter_immune_effect();
	void filter_disable_related_cards();
	int32_t filter_summon_procedure(uint8_t playerid, effect_set* eset, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone);
	int32_t check_summon_procedure(effect* proc, uint8_t playerid, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone);
	int32_t filter_set_procedure(uint8_t playerid, effect_set* eset, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone);
	int32_t check_set_procedure(effect* proc, uint8_t playerid, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone);
	void filter_spsummon_procedure(uint8_t playerid, effect_set* eset, uint32_t summon_type, material_info info = null_info);
	void filter_spsummon_procedure_g(uint8_t playerid, effect_set* eset);
	effect* find_effect(const effect_container& container, uint32_t code, effect_filter f);
	effect* find_effect_with_target(const effect_container& container, uint32_t code, effect_filter_target f, card* target);
	effect* is_affected_by_effect(uint32_t code);
	effect* is_affected_by_effect(int32_t code, card* target);
	int32_t fusion_check(group* fusion_m, card* cg, uint32_t chkf, uint8_t not_material);
	void fusion_select(uint8_t playerid, group* fusion_m, card* cg, uint32_t chkf, uint8_t not_material);
	int32_t check_fusion_substitute(card* fcard);
	int32_t is_not_tuner(card* scard);
	int32_t is_tuner(card* scard);

	int32_t check_unique_code(card* pcard);
	void get_unique_target(card_set* cset, int32_t controler, card* icard = nullptr);
	int32_t check_cost_condition(int32_t ecode, int32_t playerid);
	int32_t check_cost_condition(int32_t ecode, int32_t playerid, int32_t sumtype);
	int32_t is_summonable_card() const;
	int32_t is_spsummonable_card();
	int32_t is_fusion_summonable_card(uint32_t summon_type);
	int32_t is_spsummonable(effect* proc, material_info info = null_info);
	int32_t is_summonable(effect* proc, uint8_t min_tribute, uint32_t zone = 0x1f, uint32_t releasable = 0xff00ff);
	int32_t is_can_be_summoned(uint8_t playerid, uint8_t ingore_count, effect* peffect, uint8_t min_tribute, uint32_t zone = 0x1f);
	int32_t is_summon_negatable(uint32_t sumtype, effect* reason_effect);
	int32_t get_summon_tribute_count();
	int32_t get_set_tribute_count();
	int32_t is_can_be_flip_summoned(uint8_t playerid);
	int32_t is_special_summonable(uint8_t playerid, uint32_t summon_type, material_info info = null_info);
	int32_t is_can_be_special_summoned(effect* reason_effect, uint32_t sumtype, uint8_t sumpos, uint8_t sumplayer, uint8_t toplayer, uint8_t nocheck, uint8_t nolimit, uint32_t zone);
	uint8_t get_spsummonable_position(effect* reason_effect, uint32_t sumtype, uint8_t sumpos, uint8_t sumplayer, uint8_t toplayer);
	int32_t is_setable_mzone(uint8_t playerid, uint8_t ignore_count, effect* peffect, uint8_t min_tribute, uint32_t zone = 0x1f);
	int32_t is_setable_szone(uint8_t playerid, uint8_t ignore_fd = 0);
	int32_t is_affect_by_effect(effect* reason_effect);
	int32_t is_can_be_disabled_by_effect(effect* reason_effect, bool is_monster_effect);
	int32_t is_destructable();
	int32_t is_destructable_by_battle(card* pcard);
	effect* check_indestructable_by_effect(effect* reason_effect, uint8_t playerid);
	int32_t is_destructable_by_effect(effect* reason_effect, uint8_t playerid);
	int32_t is_removeable(uint8_t playerid, uint8_t pos, uint32_t reason);
	int32_t is_removeable_as_cost(uint8_t playerid, uint8_t pos);
	int32_t is_releasable_by_summon(uint8_t playerid, card* pcard);
	int32_t is_releasable_by_nonsummon(uint8_t playerid, uint32_t reason);
	int32_t is_releasable_by_effect(uint8_t playerid, effect* reason_effect);
	int32_t is_capable_send_to_grave(uint8_t playerid);
	int32_t is_capable_send_to_hand(uint8_t playerid);
	int32_t is_capable_send_to_deck(uint8_t playerid, uint8_t send_activating = FALSE);
	int32_t is_capable_send_to_extra(uint8_t playerid);
	int32_t is_capable_cost_to_grave(uint8_t playerid);
	int32_t is_capable_cost_to_hand(uint8_t playerid);
	int32_t is_capable_cost_to_deck(uint8_t playerid);
	int32_t is_capable_cost_to_extra(uint8_t playerid);
	int32_t is_capable_attack();
	int32_t is_capable_attack_announce(uint8_t playerid);
	int32_t is_capable_change_position(uint8_t playerid);
	int32_t is_capable_change_position_by_effect(uint8_t playerid);
	int32_t is_capable_turn_set(uint8_t playerid);
	int32_t is_capable_change_control();
	int32_t is_control_can_be_changed(int32_t ignore_mzone, uint32_t zone);
	int32_t is_capable_be_battle_target(card* pcard);
	int32_t is_capable_be_effect_target(effect* reason_effect, uint8_t playerid);
	int32_t is_capable_overlay(uint8_t playerid);
	int32_t is_can_be_fusion_material(card* fcard, uint32_t summon_type);
	int32_t is_can_be_synchro_material(card* scard, card* tuner = nullptr);
	int32_t is_can_be_ritual_material(card* scard);
	int32_t is_can_be_xyz_material(card* scard);
	int32_t is_can_be_link_material(card* scard);
	int32_t is_original_effect_property(int32_t filter);
	int32_t is_effect_property(int32_t filter);
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
constexpr uint32_t DEFAULT_SUMMON_TYPE = SUMMON_VALUE_MAIN_TYPE | SUMMON_VALUE_SUB_TYPE | SUMMON_VALUE_CUSTOM_TYPE;

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
