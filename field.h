/*
 * field.h
 *
 *  Created on: 2010-5-8
 *      Author: Argon
 */

#ifndef FIELD_H_
#define FIELD_H_

#include "common.h"
#include "card.h"
#include "effectset.h"
#include "interpreter.h"
#include <vector>
#include <set>
#include <map>
#include <list>
#include <unordered_map>
#include <unordered_set>

#define MAX_COIN_COUNT	20

//summon action type
#define SUMMON_IN_IDLE	0
#define SUMMON_IN_CHAIN	1

class card;
struct card_data;
class duel;
class group;
class effect;

using effect_vector = std::vector<effect*>;

bool check_playerid(int32_t playerid);

struct tevent {
	card* trigger_card{ nullptr };
	group* event_cards{ nullptr };
	effect* reason_effect{ nullptr };
	uint32 event_code{ 0 };
	uint32 event_value{ 0 };
	uint32 reason{ 0 };
	uint8 event_player{ PLAYER_NONE };
	uint8 reason_player{ PLAYER_NONE };

	bool operator< (const tevent& v) const;
};
struct optarget {
	group* op_cards{ nullptr };
	uint8 op_count{ 0 };
	uint8 op_player{ PLAYER_NONE };
	int32_t op_param{ 0 };
};
struct chain {
	using opmap = std::unordered_map<uint32, optarget>;
	uint16 chain_id{ 0 };
	uint8 chain_count{ 0 };
	uint8 triggering_player{ PLAYER_NONE };
	uint8 triggering_controler{ PLAYER_NONE };
	uint16 triggering_location{ 0 };
	uint8 triggering_sequence{ 0 };
	uint8 triggering_position{ 0 };
	card_state triggering_state;
	effect* triggering_effect{ nullptr };
	group* target_cards{ nullptr };
	int32_t replace_op{ 0 };
	uint8 target_player{ PLAYER_NONE };
	int32_t target_param{ 0 };
	effect* disable_reason{ nullptr };
	uint8 disable_player{ PLAYER_NONE };
	tevent evt;
	opmap opinfos;
	uint32 flag{ 0 };
	effect_set required_handorset_effects;

	static bool chain_operation_sort(const chain& c1, const chain& c2);
	void set_triggering_state(card* pcard);
};

struct player_info {
	int32_t lp{ 8000 };
	int32_t start_count{ 5 };
	int32_t draw_count{ 1 };
	uint32 used_location{ 0 };
	uint32 disabled_location{ 0 };
	uint32 extra_p_count{ 0 };
	uint32 tag_extra_p_count{ 0 };
	int32_t szone_size{ 6 };
	card_vector list_mzone;
	card_vector list_szone;
	card_vector list_main;
	card_vector list_grave;
	card_vector list_hand;
	card_vector list_remove;
	card_vector list_extra;
	card_vector tag_list_main;
	card_vector tag_list_hand;
	card_vector tag_list_extra;
};
struct field_effect {
	using oath_effects = std::unordered_map<effect*, effect*>;
	using gain_effects = std::unordered_map<card*, effect*>;
	using grant_effect_container = std::unordered_map<effect*, gain_effects>;

	effect_container aura_effect;
	effect_container ignition_effect;
	effect_container activate_effect;
	effect_container trigger_o_effect;
	effect_container trigger_f_effect;
	effect_container quick_o_effect;
	effect_container quick_f_effect;
	effect_container continuous_effect;
	effect_indexer indexer;
	oath_effects oath;
	effect_collection pheff;
	effect_collection cheff;
	effect_collection rechargeable;
	effect_collection spsummon_count_eff;

	std::list<card*> disable_check_list;
	std::unordered_set<card*> disable_check_set;

	grant_effect_container grant_effect;
};
struct field_info {
	int32_t field_id{ 1 };
	uint16 copy_id{ 1 };
	uint16 turn_id{};
	uint16 turn_id_by_player[2]{};
	uint16 card_id{ 1 };
	uint16 phase{};
	uint8 turn_player{};
	uint8 priorities[2]{};
	uint8 can_shuffle{ TRUE };
};
struct lpcost {
	int32_t count{};
	int32_t amount{};
	int32_t lpstack[8]{};
};
struct processor_unit {
	uint16 type{ 0 };
	uint16 step{ 0 };
	effect* peffect{ nullptr };
	group* ptarget{ nullptr };
	uint32 arg1{ 0 };
	uint32 arg2{ 0 };
	uint32 arg3{ 0 };
	uint32 arg4{ 0 };
	int32_t value1{ 0 };
	int32_t value2{ 0 };
	int32_t value3{ 0 };
	int32_t value4{ 0 };
	void* ptr1{ nullptr };
	void* ptr2{ nullptr };
	void* ptr3{ nullptr };
	void* ptr4{ nullptr };
};
constexpr int SIZE_SVALUE = SIZE_RETURN_VALUE / 2;
constexpr int SIZE_IVALUE = SIZE_RETURN_VALUE / 4;
constexpr int SIZE_LVALUE = SIZE_RETURN_VALUE / 8;
union return_value {
	uint8 bvalue[SIZE_RETURN_VALUE];
	uint16 svalue[SIZE_SVALUE];
	int32_t ivalue[SIZE_IVALUE];
	int64_t lvalue[SIZE_LVALUE];
};
struct processor {
	using option_vector = std::vector<uint32>;
	using card_list = std::list<card*>;
	using event_list = std::list<tevent>;
	using chain_list = std::list<chain>;
	using instant_f_list = std::map<effect*, chain>;
	using chain_array = std::vector<chain>;
	using processor_list = std::list<processor_unit>;
	using delayed_effect_collection = std::set<std::pair<effect*, tevent>>;
	struct chain_limit_t {
		chain_limit_t(int32_t f, int32_t p): function(f), player(p) {}
		int32_t function{ 0 };
		int32_t player{ PLAYER_NONE };
	};
	using chain_limit_list = std::vector<chain_limit_t>;

	processor_list units;
	processor_list subunits;
	processor_unit damage_step_reserved;
	processor_unit summon_reserved;
	card_vector select_cards;
	card_vector unselect_cards;
	card_vector summonable_cards;
	card_vector spsummonable_cards;
	card_vector repositionable_cards;
	card_vector msetable_cards;
	card_vector ssetable_cards;
	card_vector attackable_cards;
	effect_vector select_effects;
	option_vector select_options;
	card_vector must_select_cards;
	event_list point_event;
	event_list instant_event;
	event_list queue_event;
	event_list delayed_activate_event;
	event_list full_event;
	event_list used_event;
	event_list single_event;
	event_list solving_event;
	event_list sub_solving_event;
	chain_array select_chains;
	chain_array current_chain;
	chain_array ignition_priority_chains;
	chain_list continuous_chain;
	chain_list solving_continuous;
	chain_list sub_solving_continuous;
	chain_list delayed_continuous_tp;
	chain_list delayed_continuous_ntp;
	chain_list desrep_chain;
	chain_list new_fchain;
	chain_list new_fchain_s;
	chain_list new_ochain;
	chain_list new_ochain_s;
	chain_list new_fchain_b;
	chain_list new_ochain_b;
	chain_list new_ochain_h;
	chain_list new_chains;
	delayed_effect_collection delayed_quick_tmp;
	delayed_effect_collection delayed_quick;
	instant_f_list quick_f_chain;
	card_set leave_confirmed;
	card_set special_summoning;
	card_set unable_tofield_set;
	card_set equiping_cards;
	card_set control_adjust_set[2];
	card_set unique_destroy_set;
	card_set self_destroy_set;
	card_set self_tograve_set;
	card_set trap_monster_adjust_set[2];
	card_set release_cards;
	card_set release_cards_ex;
	card_set release_cards_ex_oneof;
	card_set battle_destroy_rep;
	card_set fusion_materials;
	card_set synchro_materials;
	card_set operated_set;
	card_set discarded_set;
	card_set destroy_canceled;
	card_set indestructable_count_set;
	card_set delayed_enable_set;
	card_set set_group_pre_set;
	card_set set_group_set;
	effect_set_v disfield_effects;
	effect_set_v extra_mzone_effects;
	effect_set_v extra_szone_effects;
	std::set<effect*> reseted_effects;
	std::unordered_map<card*, uint32> readjust_map;
	std::unordered_set<card*> unique_cards[2];
	std::unordered_map<uint32, uint32> effect_count_code;
	std::unordered_map<uint32, uint32> effect_count_code_duel;
	std::unordered_map<uint32, uint32> effect_count_code_chain;
	std::unordered_map<uint32, uint32> spsummon_once_map[2];
	std::multimap<int32_t, card*, std::greater<int32_t>> xmaterial_lst;

	int32_t temp_var[4]{};
	uint32 global_flag{ 0 };
	uint16 pre_field[2]{};
	std::set<uint16> opp_mzone;
	chain_limit_list chain_limit;
	chain_limit_list chain_limit_p;
	uint8 chain_solving{ FALSE };
	uint8 conti_solving{ FALSE };
	uint8 win_player{ 5 };
	uint8 win_reason{ 0 };
	uint8 re_adjust{ FALSE };
	effect* reason_effect{ nullptr };
	uint8 reason_player{ PLAYER_NONE };
	card* summoning_card{ nullptr };
	uint8 summon_depth{ 0 };
	uint8 summon_cancelable{ FALSE };
	card* attacker{ nullptr };
	card* attack_target{ nullptr };
	uint8 attacker_player{ PLAYER_NONE };
	uint8 attack_target_player{ PLAYER_NONE };
	uint32 limit_extra_summon_zone{ 0 };
	uint32 limit_extra_summon_releasable{ 0 };
	card* limit_tuner{ nullptr };
	group* limit_syn{ nullptr };
	int32_t limit_syn_minc{ 0 };
	int32_t limit_syn_maxc{ 0 };
	group* limit_xyz{ nullptr };
	int32_t limit_xyz_minc{ 0 };
	int32_t limit_xyz_maxc{ 0 };
	group* limit_link{ nullptr };
	card* limit_link_card{ nullptr };
	int32_t limit_link_minc{ 0 };
	int32_t limit_link_maxc{ 0 };
	uint8 not_material{ FALSE };
	uint8 attack_cancelable{ FALSE };
	uint8 attack_rollback{ FALSE };
	uint8 effect_damage_step{ 0 };
	int32_t battle_damage[2]{};
	int32_t summon_count[2]{};
	uint8 extra_summon[2]{};
	int32_t spe_effect[2]{};
	int32_t last_select_hint[2]{ 0 };
	uint32 duel_options{ 0 };
	int32_t duel_rule{ CURRENT_RULE };
	uint32 copy_reset{ 0 };
	int32_t copy_reset_count{ 0 };
	uint32 last_control_changed_id{ 0 };
	uint32 set_group_used_zones{ 0 };
	uint8 set_group_seq[7]{};
	uint8 dice_result[5]{};
	uint8 coin_result[MAX_COIN_COUNT]{};
	int32_t coin_count{ 0 };
	bool is_target_ready{ false };
	bool is_gemini_summoning{ false };
	bool is_summon_negated{ false };

	uint8 to_bp{ FALSE };
	uint8 to_m2{ FALSE };
	uint8 to_ep{ FALSE };
	uint8 skip_m2{ FALSE };
	uint8 chain_attack{ FALSE };
	uint32 chain_attacker_id{ 0 };
	card* chain_attack_target{ nullptr };
	uint8 attack_player{ PLAYER_NONE };
	uint8 selfdes_disabled{ FALSE };
	uint8 overdraw[2]{};
	int32_t check_level{ 0 };
	uint8 shuffle_check_disabled{ FALSE };
	uint8 shuffle_hand_check[2]{};
	uint8 shuffle_deck_check[2]{};
	uint8 deck_reversed{ FALSE };
	uint8 remove_brainwashing{ FALSE };
	uint8 flip_delayed{ FALSE };
	uint8 damage_calculated{ FALSE };
	uint8 hand_adjusted{ FALSE };
	uint8 summon_state_count[2]{};
	uint8 normalsummon_state_count[2]{};
	uint8 flipsummon_state_count[2]{};
	uint8 spsummon_state_count[2]{};
	uint8 attack_state_count[2]{};
	uint8 battle_phase_count[2]{};
	uint8 battled_count[2]{};
	uint8 phase_action{ FALSE };
	uint32 hint_timing[2]{};
	uint8 current_player{ PLAYER_NONE };
	uint8 conti_player{ PLAYER_NONE };
	uint8 select_deck_sequence_revealed{ FALSE };
	uint8 selecting_player{ PLAYER_NONE };
	std::unordered_map<uint32, std::pair<uint32, uint32>> summon_counter;
	std::unordered_map<uint32, std::pair<uint32, uint32>> normalsummon_counter;
	std::unordered_map<uint32, std::pair<uint32, uint32>> spsummon_counter;
	std::unordered_map<uint32, std::pair<uint32, uint32>> flipsummon_counter;
	std::unordered_map<uint32, std::pair<uint32, uint32>> attack_counter;
	std::unordered_map<uint32, std::pair<uint32, uint32>> chain_counter;
	processor_list recover_damage_reserve;
	effect_vector dec_count_reserve;
};
class field {
public:
	using card_list = std::list<card*>;
	using event_list = std::list<tevent>;
	using chain_list = std::list<chain>;
	using instant_f_list = std::map<effect*, chain>;
	using chain_array = std::vector<chain>;
	using processor_list = std::list<processor_unit>;

	duel* pduel;
	player_info player[2];
	card* temp_card{};
	field_info infos;
	//lpcost cost[2];
	field_effect effects;
	processor core;
	return_value returns{};
	tevent nil_event;

	static int32_t field_used_count[32];
	explicit field(duel* pduel);
	~field() = default;
	void reload_field_info();

	void add_card(uint8 playerid, card* pcard, uint8 location, uint8 sequence, uint8 pzone = FALSE);
	void remove_card(card* pcard);
	void move_card(uint8 playerid, card* pcard, uint8 location, uint8 sequence, uint8 pzone = FALSE);
	void swap_card(card* pcard1, card* pcard2, uint8 new_sequence1, uint8 new_sequence2);
	void swap_card(card* pcard1, card* pcard2);
	void set_control(card* pcard, uint8 playerid, uint16 reset_phase, uint8 reset_count);

	int32_t get_pzone_sequence(uint8 pseq) const;
	card* get_field_card(uint8 playerid, uint32 general_location, uint8 sequence) const;
	int32_t is_location_useable(uint8 playerid, uint32 general_location, uint8 sequence) const;
	int32_t get_useable_count(card* pcard, uint8 playerid, uint8 location, uint8 uplayer, uint32 reason, uint32 zone = 0xff, uint32* list = nullptr);
	int32_t get_useable_count_fromex(card* pcard, uint8 playerid, uint8 uplayer, uint32 zone = 0xff, uint32* list = nullptr);
	int32_t get_spsummonable_count(card* pcard, uint8 playerid, uint32 zone = 0xff, uint32* list = nullptr);
	int32_t get_spsummonable_count_fromex(card* pcard, uint8 playerid, uint8 uplayer, uint32 zone = 0xff, uint32* list = nullptr);
	int32_t get_useable_count_other(card* pcard, uint8 playerid, uint8 location, uint8 uplayer, uint32 reason, uint32 zone = 0xff, uint32* list = nullptr);
	int32_t get_tofield_count(card* pcard, uint8 playerid, uint8 location, uint32 uplayer, uint32 reason, uint32 zone = 0xff, uint32* list = nullptr);
	int32_t get_useable_count_fromex_rule4(card* pcard, uint8 playerid, uint8 uplayer, uint32 zone = 0xff, uint32* list = nullptr);
	int32_t get_spsummonable_count_fromex_rule4(card* pcard, uint8 playerid, uint8 uplayer, uint32 zone = 0xff, uint32* list = nullptr);
	int32_t get_mzone_limit(uint8 playerid, uint8 uplayer, uint32 reason);
	int32_t get_szone_limit(uint8 playerid, uint8 uplayer, uint32 reason);
	uint32 get_linked_zone(int32_t playerid);
	uint32 get_rule_zone_fromex(int32_t playerid, card* pcard);
	void filter_must_use_mzone(uint8 playerid, uint8 uplayer, uint32 reason, card* pcard, uint32* flag);
	void get_linked_cards(uint8 self, uint8 s, uint8 o, card_set* cset);
	int32_t check_extra_link(int32_t playerid);
	int32_t check_extra_link(int32_t playerid, card* pcard, int32_t sequence);
	void get_cards_in_zone(card_set* cset, uint32 zone, int32_t playerid, int32_t location);
	void shuffle(uint8 playerid, uint8 location);
	void reset_sequence(uint8 playerid, uint8 location);
	void swap_deck_and_grave(uint8 playerid);
	void reverse_deck(uint8 playerid);
	void refresh_player_info(uint8 playerid);
	void tag_swap(uint8 playerid);

	void add_effect(effect* peffect, uint8 owner_player = PLAYER_NONE);
	void remove_effect(effect* peffect);
	void remove_oath_effect(effect* reason_effect);
	void release_oath_relation(effect* reason_effect);
	void reset_phase(uint32 phase);
	void reset_chain();
	void add_effect_code(uint32 code, uint32 playerid);
	uint32 get_effect_code(uint32 code, uint32 playerid);
	void dec_effect_code(uint32 code, uint32 playerid);

	void filter_field_effect(uint32 code, effect_set* eset, uint8 sort = TRUE);
	void filter_affected_cards(effect* peffect, card_set* cset);
	void filter_inrange_cards(effect* peffect, card_set* cset);
	void filter_player_effect(uint8 playerid, uint32 code, effect_set* eset, uint8 sort = TRUE);
	int32_t filter_matching_card(lua_State* L, int32_t findex, uint8 self, uint32 location1, uint32 location2, group* pgroup, card* pexception, group* pexgroup, uint32 extraargs, card** pret = nullptr, int32_t fcount = 0, int32_t is_target = FALSE);
	int32_t filter_field_card(uint8 self, uint32 location, uint32 location2, group* pgroup);
	effect* is_player_affected_by_effect(uint8 playerid, uint32 code);

	int32_t get_release_list(lua_State* L, uint8 playerid, card_set* release_list, card_set* ex_list, card_set* ex_list_oneof, int32_t use_con, int32_t use_hand, int32_t fun, int32_t exarg, card* exc, group* exg, uint32 reason);
	int32_t check_release_list(lua_State* L, uint8 playerid, int32_t count, int32_t use_con, int32_t use_hand, int32_t fun, int32_t exarg, card* exc, group* exg, uint32 reason);
	int32_t get_summon_release_list(card* target, card_set* release_list, card_set* ex_list, card_set* ex_list_oneof, group* mg = nullptr, uint32 ex = 0, uint32 releasable = 0xff00ff, uint32 pos = 0x1);
	int32_t get_summon_count_limit(uint8 playerid);
	int32_t get_draw_count(uint8 playerid);
	void get_ritual_material(uint8 playerid, effect* peffect, card_set* material, uint8 no_level = FALSE);
	void get_fusion_material(uint8 playerid, card_set* material_all, card_set* material_base, uint32 location);
	void ritual_release(const card_set& material);
	void get_xyz_material(lua_State* L, card* scard, int32_t findex, uint32 lv, int32_t maxc, group* mg);
	void get_overlay_group(uint8 self, uint8 s, uint8 o, card_set* pset);
	int32_t get_overlay_count(uint8 self, uint8 s, uint8 o);
	void update_disable_check_list(effect* peffect);
	void add_to_disable_check_list(card* pcard);
	void adjust_disable_check_list();
	void adjust_self_destroy_set();
	void erase_grant_effect(effect* peffect);
	int32_t adjust_grant_effect();
	void add_unique_card(card* pcard);
	void remove_unique_card(card* pcard);
	effect* check_unique_onfield(card* pcard, uint8 controler, uint8 location, card* icard = nullptr);
	int32_t check_spsummon_once(card* pcard, uint8 playerid);
	void check_card_counter(card* pcard, int32_t counter_type, int32_t playerid);
	void check_card_counter(group* pgroup, int32_t counter_type, int32_t playerid);
	void check_chain_counter(effect* peffect, int32_t playerid, int32_t chainid, bool cancel = false);
	void set_spsummon_counter(uint8 playerid);
	int32_t check_spsummon_counter(uint8 playerid, uint8 ct = 1);
	bool is_select_hide_deck_sequence(uint8 playerid);

	int32_t check_lp_cost(uint8 playerid, uint32 cost, uint32 must_pay);
	void save_lp_cost() {}
	void restore_lp_cost() {}
	int32_t pay_lp_cost(uint32 step, uint8 playerid, uint32 cost, uint32 must_pay);

	uint32 get_field_counter(uint8 self, uint8 s, uint8 o, uint16 countertype);
	int32_t effect_replace_check(uint32 code, const tevent& e);
	int32_t get_attack_target(card* pcard, card_vector* v, uint8 chain_attack = FALSE, bool select_target = true);
	bool confirm_attack_target();
	void attack_all_target_check();
	int32_t get_must_material_list(uint8 playerid, uint32 limit, card_set* must_list);
	int32_t check_must_material(group* mg, uint8 playerid, uint32 limit);
	void get_synchro_material(uint8 playerid, card_set* material, effect* tuner_limit = nullptr);
	
	//check material
	int32_t check_synchro_material(lua_State* L, card* pcard, int32_t findex1, int32_t findex2, int32_t min, int32_t max, card* smat, group* mg);
	int32_t check_tuner_material(lua_State* L, card* pcard, card* tuner, int32_t findex1, int32_t findex2, int32_t min, int32_t max, card* smat, group* mg);
	int32_t check_other_synchro_material(const card_vector& nsyn, int32_t lv, int32_t min, int32_t max, int32_t mcount);
	int32_t check_tribute(card* pcard, int32_t min, int32_t max, group* mg, uint8 toplayer, uint32 zone = 0x1f, uint32 releasable = 0xff00ff, uint32 pos = 0x1);
	static int32_t check_with_sum_limit(const card_vector& mats, int32_t acc, int32_t index, int32_t count, int32_t min, int32_t max, int32_t opmin);
	static int32_t check_with_sum_limit_m(const card_vector& mats, int32_t acc, int32_t index, int32_t min, int32_t max, int32_t opmin, int32_t must_count);
	static int32_t check_with_sum_greater_limit(const card_vector& mats, int32_t acc, int32_t index, int32_t opmin);
	static int32_t check_with_sum_greater_limit_m(const card_vector& mats, int32_t acc, int32_t index, int32_t opmin, int32_t must_count);
	int32_t check_xyz_material(lua_State* L, card* pcard, int32_t findex, int32_t lv, int32_t min, int32_t max, group* mg);

	int32_t is_player_can_draw(uint8 playerid);
	int32_t is_player_can_discard_deck(uint8 playerid, int32_t count);
	int32_t is_player_can_discard_deck_as_cost(uint8 playerid, int32_t count);
	int32_t is_player_can_discard_hand(uint8 playerid, card* pcard, effect* reason_effect, uint32 reason);
	int32_t is_player_can_action(uint8 playerid, uint32 actionlimit);
	int32_t is_player_can_summon(uint32 sumtype, uint8 playerid, card* pcard, uint8 toplayer);
	int32_t is_player_can_mset(uint32 sumtype, uint8 playerid, card* pcard, uint8 toplayer);
	int32_t is_player_can_sset(uint8 playerid, card* pcard);
	int32_t is_player_can_spsummon(uint8 playerid);
	int32_t is_player_can_spsummon(effect* reason_effect, uint32 sumtype, uint8 sumpos, uint8 playerid, uint8 toplayer, card* pcard);
	int32_t is_player_can_flipsummon(uint8 playerid, card* pcard);
	int32_t is_player_can_spsummon_monster(uint8 playerid, uint8 toplayer, uint8 sumpos, uint32 sumtype, card_data* pdata);
	int32_t is_player_can_spsummon_count(uint8 playerid, uint32 count);
	int32_t is_player_can_release(uint8 playerid, card* pcard, uint32 reason);
	int32_t is_player_can_place_counter(uint8 playerid, card* pcard, uint16 countertype, uint16 count);
	int32_t is_player_can_remove_counter(uint8 playerid, card* pcard, uint8 s, uint8 o, uint16 countertype, uint16 count, uint32 reason);
	int32_t is_player_can_remove_overlay_card(uint8 playerid, card* pcard, uint8 s, uint8 o, uint16 count, uint32 reason);
	int32_t is_player_can_send_to_grave(uint8 playerid, card* pcard);
	int32_t is_player_can_send_to_hand(uint8 playerid, card* pcard);
	int32_t is_player_can_send_to_deck(uint8 playerid, card* pcard);
	int32_t is_player_can_remove(uint8 playerid, card* pcard, uint32 reason);
	int32_t is_chain_negatable(uint8 chaincount);
	int32_t is_chain_disablable(uint8 chaincount);
	int32_t is_chain_disabled(uint8 chaincount);
	int32_t check_chain_target(uint8 chaincount, card* pcard);
	chain* get_chain(uint32 chaincount);
	int32_t get_cteffect(effect* peffect, int32_t playerid, int32_t store);
	int32_t get_cteffect_evt(effect* feffect, int32_t playerid, const tevent& e, int32_t store);
	int32_t check_cteffect_hint(effect* peffect, uint8 playerid);
	int32_t check_nonpublic_trigger(chain& ch);
	int32_t check_trigger_effect(const chain& ch) const;
	int32_t check_spself_from_hand_trigger(const chain& ch) const;
	int32_t is_able_to_enter_bp();

	void add_process(uint16 type, uint16 step, effect* peffect, group* target, int32_t arg1, int32_t arg2, int32_t arg3 = 0, int32_t arg4 = 0, void* ptr1 = nullptr, void* ptr2 = nullptr);
	uint32 process();
	int32_t execute_cost(uint16 step, effect* peffect, uint8 triggering_player);
	int32_t execute_operation(uint16 step, effect* peffect, uint8 triggering_player);
	int32_t execute_target(uint16 step, effect* peffect, uint8 triggering_player);
	void raise_event(card* event_card, uint32 event_code, effect* reason_effect, uint32 reason, uint8 reason_player, uint8 event_player, uint32 event_value);
	void raise_event(const card_set& event_cards, uint32 event_code, effect* reason_effect, uint32 reason, uint8 reason_player, uint8 event_player, uint32 event_value);
	void raise_single_event(card* trigger_card, card_set* event_cards, uint32 event_code, effect* reason_effect, uint32 reason, uint8 reason_player, uint8 event_player, uint32 event_value);
	int32_t check_event(uint32 code, tevent* pe = nullptr);
	int32_t check_event_c(effect* peffect, uint8 playerid, int32_t neglect_con, int32_t neglect_cost, int32_t copy_info, tevent* pe = nullptr);
	int32_t check_hint_timing(effect* peffect);
	int32_t process_phase_event(int16_t step, int32_t phase_event);
	int32_t process_point_event(int16_t step, int32_t skip_trigger, int32_t skip_freechain, int32_t skip_new);
	int32_t process_quick_effect(int16_t step, int32_t skip_freechain, uint8 priority);
	int32_t process_instant_event();
	int32_t process_single_event();
	int32_t process_single_event(effect* peffect, const tevent& e, chain_list& tp, chain_list& ntp);
	int32_t process_idle_command(uint16 step);
	int32_t process_battle_command(uint16 step);
	int32_t process_damage_step(uint16 step, uint32 new_attack);
	void calculate_battle_damage(effect** pdamchange, card** preason_card, uint8* battle_destroyed);
	int32_t process_turn(uint16 step, uint8 turn_player);

	int32_t add_chain(uint16 step);
	void solve_continuous(uint8 playerid, effect* peffect, const tevent& e);
	int32_t solve_continuous(uint16 step);
	int32_t solve_chain(uint16 step, uint32 chainend_arg1, uint32 chainend_arg2);
	int32_t break_effect();
	void adjust_instant();
	void adjust_all();
	void refresh_location_info_instant();
	int32_t refresh_location_info(uint16 step);
	int32_t adjust_step(uint16 step);

	//operations
	int32_t negate_chain(uint8 chaincount);
	int32_t disable_chain(uint8 chaincount, uint8 forced);
	void change_chain_effect(uint8 chaincount, int32_t replace_op);
	void change_target(uint8 chaincount, group* targets);
	void change_target_player(uint8 chaincount, uint8 playerid);
	void change_target_param(uint8 chaincount, int32_t param);
	void remove_counter(uint32 reason, card* pcard, uint32 rplayer, uint32 s, uint32 o, uint32 countertype, uint32 count);
	void remove_overlay_card(uint32 reason, card* pcard, uint32 rplayer, uint32 s, uint32 o, uint16 min, uint16 max);
	void get_control(const card_set& targets, effect* reason_effect, uint32 reason_player, uint32 playerid, uint32 reset_phase, uint32 reset_count, uint32 zone);
	void get_control(card* target, effect* reason_effect, uint32 reason_player, uint32 playerid, uint32 reset_phase, uint32 reset_count, uint32 zone);
	void swap_control(effect* reason_effect, uint32 reason_player, const card_set& targets1, const card_set& targets2, uint32 reset_phase, uint32 reset_count);
	void swap_control(effect* reason_effect, uint32 reason_player, card* pcard1, card* pcard2, uint32 reset_phase, uint32 reset_count);
	void equip(uint32 equip_player, card* equip_card, card* target, uint32 up, uint32 is_step);
	void draw(effect* reason_effect, uint32 reason, uint32 reason_player, uint32 playerid, int32_t count);
	void damage(effect* reason_effect, uint32 reason, uint32 reason_player, card* reason_card, uint32 playerid, int32_t amount, uint32 is_step = FALSE);
	void recover(effect* reason_effect, uint32 reason, uint32 reason_player, uint32 playerid, int32_t amount, uint32 is_step = FALSE);
	void summon(uint32 sumplayer, card* target, effect* proc, uint32 ignore_count, uint32 min_tribute, uint32 zone = 0x1f, uint32 action_type = SUMMON_IN_IDLE);
	void mset(uint32 setplayer, card* target, effect* proc, uint32 ignore_count, uint32 min_tribute, uint32 zone = 0x1f, uint32 action_type = SUMMON_IN_IDLE);
	void special_summon_rule(uint32 sumplayer, card* target, uint32 summon_type, uint32 action_type = SUMMON_IN_IDLE);
	void special_summon(const card_set& target, uint32 sumtype, uint32 sumplayer, uint32 playerid, uint32 nocheck, uint32 nolimit, uint32 positions, uint32 zone);
	void special_summon_step(card* target, uint32 sumtype, uint32 sumplayer, uint32 playerid, uint32 nocheck, uint32 nolimit, uint32 positions, uint32 zone);
	void special_summon_complete(effect* reason_effect, uint8 reason_player);
	void destroy(card_set& targets, effect* reason_effect, uint32 reason, uint32 reason_player, uint32 playerid = 2, uint32 destination = 0, uint32 sequence = 0);
	void destroy(card* target, effect* reason_effect, uint32 reason, uint32 reason_player, uint32 playerid = 2, uint32 destination = 0, uint32 sequence = 0);
	void release(const card_set& targets, effect* reason_effect, uint32 reason, uint32 reason_player);
	void release(card* target, effect* reason_effect, uint32 reason, uint32 reason_player);
	void send_to(const card_set& targets, effect* reason_effect, uint32 reason, uint32 reason_player, uint32 playerid, uint32 destination, uint32 sequence, uint32 position, uint8 send_activating = FALSE);
	void send_to(card* target, effect* reason_effect, uint32 reason, uint32 reason_player, uint32 playerid, uint32 destination, uint32 sequence, uint32 position, uint8 send_activating = FALSE);
	void move_to_field(card* target, uint32 move_player, uint32 playerid, uint32 destination, uint32 positions, uint32 enable = FALSE, uint32 ret = 0, uint32 pzone = FALSE, uint32 zone = 0xff);
	void change_position(const card_set& targets, effect* reason_effect, uint32 reason_player, uint32 au, uint32 ad, uint32 du, uint32 dd, uint32 flag, uint32 enable = FALSE);
	void change_position(card* target, effect* reason_effect, uint32 reason_player, uint32 npos, uint32 flag, uint32 enable = FALSE);
	void operation_replace(int32_t type, int32_t step, group* targets);
	void select_tribute_cards(card* target, uint8 playerid, uint8 cancelable, int32_t min, int32_t max, uint8 toplayer, uint32 zone);

	// summon
	int32_t summon(uint16 step, uint8 sumplayer, card* target, effect* proc, uint8 ignore_count, uint8 min_tribute, uint32 zone, uint32 action_type);
	int32_t mset(uint16 step, uint8 setplayer, card* ptarget, effect* proc, uint8 ignore_count, uint8 min_tribute, uint32 zone, uint32 action_type);
	int32_t flip_summon(uint16 step, uint8 sumplayer, card* target, uint32 action_type);
	int32_t special_summon_rule(uint16 step, uint8 sumplayer, card* target, uint32 summon_type, uint32 action_type);

	int32_t remove_counter(uint16 step, uint32 reason, card* pcard, uint8 rplayer, uint8 s, uint8 o, uint16 countertype, uint16 count);
	int32_t remove_overlay_card(uint16 step, uint32 reason, card* pcard, uint8 rplayer, uint8 s, uint8 o, uint16 min, uint16 max);
	int32_t get_control(uint16 step, effect* reason_effect, uint8 reason_player, group* targets, uint8 playerid, uint16 reset_phase, uint8 reset_count, uint32 zone);
	int32_t swap_control(uint16 step, effect* reason_effect, uint8 reason_player, group* targets1, group* targets2, uint16 reset_phase, uint8 reset_count);
	int32_t self_destroy(uint16 step, card* ucard, int32_t p);
	int32_t trap_monster_adjust(uint16 step);
	int32_t equip(uint16 step, uint8 equip_player, card* equip_card, card* target, uint32 up, uint32 is_step);
	int32_t draw(uint16 step, effect* reason_effect, uint32 reason, uint8 reason_player, uint8 playerid, int32_t count);
	int32_t damage(uint16 step, effect* reason_effect, uint32 reason, uint8 reason_player, card* reason_card, uint8 playerid, int32_t amount, uint32 is_step);
	int32_t recover(uint16 step, effect* reason_effect, uint32 reason, uint8 reason_player, uint8 playerid, int32_t amount, uint32 is_step);
	int32_t sset(uint16 step, uint8 setplayer, uint8 toplayer, card* ptarget, effect* reason_effect);
	int32_t sset_g(uint16 step, uint8 setplayer, uint8 toplayer, group* ptarget, uint8 confirm, effect* reason_effect);
	int32_t special_summon_step(uint16 step, group* targets, card* target, uint32 zone);
	int32_t special_summon(uint16 step, effect* reason_effect, uint8 reason_player, group* targets, uint32 zone);
	int32_t destroy_replace(uint16 step, group* targets, card* target, uint8 battle);
	int32_t destroy(uint16 step, group* targets, effect* reason_effect, uint32 reason, uint8 reason_player);
	int32_t release_replace(uint16 step, group* targets, card* target);
	int32_t release(uint16 step, group* targets, effect* reason_effect, uint32 reason, uint8 reason_player);
	int32_t send_replace(uint16 step, group* targets, card* target);
	int32_t send_to(uint16 step, group* targets, effect* reason_effect, uint32 reason, uint8 reason_player, uint8 send_activating);
	int32_t discard_deck(uint16 step, uint8 playerid, uint8 count, uint32 reason);
	int32_t move_to_field(uint16 step, card* target, uint32 enable, uint32 ret, uint32 pzone, uint32 zone);
	int32_t change_position(uint16 step, group* targets, effect* reason_effect, uint8 reason_player, uint32 enable);
	int32_t operation_replace(uint16 step, effect* replace_effect, group* targets, card* target, int32_t is_destroy);
	int32_t activate_effect(uint16 step, effect* peffect);
	int32_t select_synchro_material(int16_t step, uint8 playerid, card* pcard, int32_t min, int32_t max, card* smat, group* mg, int32_t filter1, int32_t filter2);
	int32_t select_xyz_material(int16_t step, uint8 playerid, uint32 lv, card* pcard, int32_t min, int32_t max);
	int32_t select_release_cards(int16_t step, uint8 playerid, uint8 cancelable, int32_t min, int32_t max);
	int32_t select_tribute_cards(int16_t step, card* target, uint8 playerid, uint8 cancelable, int32_t min, int32_t max, uint8 toplayer, uint32 zone);
	int32_t toss_coin(uint16 step, effect* reason_effect, uint8 reason_player, uint8 playerid, int32_t count);
	int32_t toss_dice(uint16 step, effect* reason_effect, uint8 reason_player, uint8 playerid, uint8 count1, uint8 count2);
	int32_t rock_paper_scissors(uint16 step, uint8 repeat);

	bool check_response(size_t vector_size, int32_t min_len, int32_t max_len) const;
	int32_t select_battle_command(uint16 step, uint8 playerid);
	int32_t select_idle_command(uint16 step, uint8 playerid);
	int32_t select_effect_yes_no(uint16 step, uint8 playerid, uint32 description, card* pcard);
	int32_t select_yes_no(uint16 step, uint8 playerid, uint32 description);
	int32_t select_option(uint16 step, uint8 playerid);
	int32_t select_card(uint16 step, uint8 playerid, uint8 cancelable, uint8 min, uint8 max);
	int32_t select_unselect_card(uint16 step, uint8 playerid, uint8 cancelable, uint8 min, uint8 max, uint8 finishable);
	int32_t select_chain(uint16 step, uint8 playerid, uint8 spe_count, uint8 forced);
	int32_t select_place(uint16 step, uint8 playerid, uint32 flag, uint8 count);
	int32_t select_position(uint16 step, uint8 playerid, uint32 code, uint8 positions);
	int32_t select_tribute(uint16 step, uint8 playerid, uint8 cancelable, uint8 min, uint8 max);
	int32_t select_counter(uint16 step, uint8 playerid, uint16 countertype, uint16 count, uint8 s, uint8 o);
	int32_t select_with_sum_limit(int16_t step, uint8 playerid, int32_t acc, int32_t min, int32_t max);
	int32_t sort_card(int16_t step, uint8 playerid);
	int32_t announce_race(int16_t step, uint8 playerid, int32_t count, int32_t available);
	int32_t announce_attribute(int16_t step, uint8 playerid, int32_t count, int32_t available);
	int32_t announce_card(int16_t step, uint8 playerid);
	int32_t announce_number(int16_t step, uint8 playerid);
};

//Location Use Reason
#define LOCATION_REASON_TOFIELD	0x1
#define LOCATION_REASON_CONTROL	0x2
//Chain Info
#define CHAIN_DISABLE_ACTIVATE	0x01
#define CHAIN_DISABLE_EFFECT	0x02
#define CHAIN_HAND_EFFECT		0x04
#define CHAIN_CONTINUOUS_CARD	0x08
#define CHAIN_ACTIVATING		0x10
#define CHAIN_HAND_TRIGGER		0x20
//#define CHAIN_DECK_EFFECT		0x40
#define CHAININFO_CHAIN_COUNT			0x01
#define CHAININFO_TRIGGERING_EFFECT		0x02
#define CHAININFO_TRIGGERING_PLAYER		0x04
#define CHAININFO_TRIGGERING_CONTROLER	0x08
#define CHAININFO_TRIGGERING_LOCATION	0x10
#define CHAININFO_TRIGGERING_SEQUENCE	0x20
#define CHAININFO_TARGET_CARDS			0x40
#define CHAININFO_TARGET_PLAYER			0x80
#define CHAININFO_TARGET_PARAM			0x100
#define CHAININFO_DISABLE_REASON		0x200
#define CHAININFO_DISABLE_PLAYER		0x400
#define CHAININFO_CHAIN_ID				0x800
#define CHAININFO_TYPE					0x1000
#define CHAININFO_EXTTYPE				0x2000
#define CHAININFO_TRIGGERING_POSITION	0x4000
#define CHAININFO_TRIGGERING_CODE		0x8000
#define CHAININFO_TRIGGERING_CODE2		0x10000
//#define CHAININFO_TRIGGERING_TYPE		0x20000
#define CHAININFO_TRIGGERING_LEVEL		0x40000
#define CHAININFO_TRIGGERING_RANK		0x80000
#define CHAININFO_TRIGGERING_ATTRIBUTE	0x100000
#define CHAININFO_TRIGGERING_RACE		0x200000
#define CHAININFO_TRIGGERING_ATTACK		0x400000
#define CHAININFO_TRIGGERING_DEFENSE	0x800000
//Timing
#define TIMING_DRAW_PHASE			0x1
#define TIMING_STANDBY_PHASE		0x2
#define TIMING_MAIN_END				0x4
#define TIMING_BATTLE_START			0x8
#define TIMING_BATTLE_END			0x10
#define TIMING_END_PHASE			0x20
#define TIMING_SUMMON				0x40
#define TIMING_SPSUMMON				0x80
#define TIMING_FLIPSUMMON			0x100
#define TIMING_MSET					0x200
#define TIMING_SSET					0x400
#define TIMING_POS_CHANGE			0x800
#define TIMING_ATTACK				0x1000
#define TIMING_DAMAGE_STEP			0x2000
#define TIMING_DAMAGE_CAL			0x4000
#define TIMING_CHAIN_END			0x8000
#define TIMING_DRAW					0x10000
#define TIMING_DAMAGE				0x20000
#define TIMING_RECOVER				0x40000
#define TIMING_DESTROY				0x80000
#define TIMING_REMOVE				0x100000
#define TIMING_TOHAND				0x200000
#define TIMING_TODECK				0x400000
#define TIMING_TOGRAVE				0x800000
#define TIMING_BATTLE_PHASE			0x1000000
#define TIMING_EQUIP				0x2000000
#define TIMING_BATTLE_STEP_END		0x4000000
#define TIMING_BATTLED				0x8000000

#define GLOBALFLAG_DECK_REVERSE_CHECK	0x1
#define GLOBALFLAG_BRAINWASHING_CHECK	0x2
#define GLOBALFLAG_SCRAP_CHIMERA		0x4
//#define GLOBALFLAG_DELAYED_QUICKEFFECT	0x8
#define GLOBALFLAG_DETACH_EVENT			0x10
//#define GLOBALFLAG_MUST_BE_SMATERIAL	0x20
#define GLOBALFLAG_SPSUMMON_COUNT		0x40
#define GLOBALFLAG_XMAT_COUNT_LIMIT		0x80
#define GLOBALFLAG_SELF_TOGRAVE			0x100
#define GLOBALFLAG_SPSUMMON_ONCE		0x200
#define GLOBALFLAG_TUNE_MAGICIAN		0x400
#define GLOBALFLAG_ACTIVATION_COUNT		0x800
//

#define PROCESSOR_ADJUST			1
#define PROCESSOR_HINT				2
#define PROCESSOR_TURN				3
#define PROCESSOR_WAIT				4
#define PROCESSOR_REFRESH_LOC		5
#define PROCESSOR_SELECT_IDLECMD	10
#define PROCESSOR_SELECT_EFFECTYN	11
#define PROCESSOR_SELECT_BATTLECMD	12
#define PROCESSOR_SELECT_YESNO		13
#define PROCESSOR_SELECT_OPTION		14
#define PROCESSOR_SELECT_CARD		15
#define PROCESSOR_SELECT_CHAIN		16
#define PROCESSOR_SELECT_UNSELECT_CARD	17
#define PROCESSOR_SELECT_PLACE		18
#define PROCESSOR_SELECT_POSITION	19
#define PROCESSOR_SELECT_TRIBUTE_P	20
#define PROCESSOR_SELECT_COUNTER	22
#define PROCESSOR_SELECT_SUM		23
#define PROCESSOR_SELECT_DISFIELD	24
#define PROCESSOR_SORT_CARD			25
#define PROCESSOR_SELECT_RELEASE	26
#define PROCESSOR_SELECT_TRIBUTE	27
#define PROCESSOR_POINT_EVENT		30
#define PROCESSOR_QUICK_EFFECT		31
#define PROCESSOR_IDLE_COMMAND		32
#define PROCESSOR_PHASE_EVENT		33
#define PROCESSOR_BATTLE_COMMAND	34
#define PROCESSOR_DAMAGE_STEP		35
#define PROCESSOR_ADD_CHAIN			40
#define PROCESSOR_SOLVE_CHAIN		42
#define PROCESSOR_SOLVE_CONTINUOUS	43
#define PROCESSOR_EXECUTE_COST		44
#define PROCESSOR_EXECUTE_OPERATION	45
#define PROCESSOR_EXECUTE_TARGET	46
#define PROCESSOR_DESTROY			50
#define PROCESSOR_RELEASE			51
#define PROCESSOR_SENDTO			52
#define PROCESSOR_MOVETOFIELD		53
#define PROCESSOR_CHANGEPOS			54
#define PROCESSOR_OPERATION_REPLACE	55
#define PROCESSOR_DESTROY_REPLACE	56
#define PROCESSOR_RELEASE_REPLACE	57
#define PROCESSOR_SENDTO_REPLACE	58
#define PROCESSOR_SUMMON_RULE		60
#define PROCESSOR_SPSUMMON_RULE		61
#define PROCESSOR_SPSUMMON			62
#define PROCESSOR_FLIP_SUMMON		63
#define PROCESSOR_MSET				64
#define PROCESSOR_SSET				65
#define PROCESSOR_SPSUMMON_STEP		66
#define PROCESSOR_SSET_G			67
#define PROCESSOR_DRAW				70
#define PROCESSOR_DAMAGE			71
#define PROCESSOR_RECOVER			72
#define PROCESSOR_EQUIP				73
#define PROCESSOR_GET_CONTROL		74
#define PROCESSOR_SWAP_CONTROL		75
//#define PROCESSOR_CONTROL_ADJUST	76
#define PROCESSOR_SELF_DESTROY		77
#define PROCESSOR_TRAP_MONSTER_ADJUST	78
#define PROCESSOR_PAY_LPCOST		80
#define PROCESSOR_REMOVE_COUNTER	81
#define PROCESSOR_ATTACK_DISABLE	82
#define PROCESSOR_ACTIVATE_EFFECT	83

#define PROCESSOR_ANNOUNCE_RACE		110
#define PROCESSOR_ANNOUNCE_ATTRIB	111
#define PROCESSOR_ANNOUNCE_LEVEL	112
#define PROCESSOR_ANNOUNCE_CARD		113
#define PROCESSOR_ANNOUNCE_TYPE		114
#define PROCESSOR_ANNOUNCE_NUMBER	115
#define PROCESSOR_ANNOUNCE_COIN		116
#define PROCESSOR_TOSS_DICE			117
#define PROCESSOR_TOSS_COIN			118
#define PROCESSOR_ROCK_PAPER_SCISSORS	119

#define PROCESSOR_SELECT_FUSION		131
#define PROCESSOR_SELECT_SYNCHRO	132
#define PROCESSOR_SELECT_XMATERIAL	139
#define PROCESSOR_DISCARD_HAND	150
#define PROCESSOR_DISCARD_DECK	151
#define PROCESSOR_SORT_DECK		152
#define PROCESSOR_REMOVE_OVERLAY		160

//#define PROCESSOR_SORT_CHAIN		21
//#define PROCESSOR_DESTROY_S			100
//#define PROCESSOR_RELEASE_S			101
//#define PROCESSOR_SENDTO_S			102
//#define PROCESSOR_CHANGEPOS_S		103
//#define PROCESSOR_SELECT_YESNO_S	120
//#define PROCESSOR_SELECT_OPTION_S	121
//#define PROCESSOR_SELECT_CARD_S		122
//#define PROCESSOR_SELECT_EFFECTYN_S	123
//#define PROCESSOR_SELECT_UNSELECT_CARD_S	124
//#define PROCESSOR_SELECT_PLACE_S	125
//#define PROCESSOR_SELECT_POSITION_S	126
//#define PROCESSOR_SELECT_TRIBUTE_S	127
//#define PROCESSOR_SORT_CARDS_S		128
//#define PROCESSOR_SELECT_RELEASE_S	129
//#define PROCESSOR_SELECT_TARGET		130
//#define PROCESSOR_SELECT_SUM_S		133
//#define PROCESSOR_SELECT_DISFIELD_S	134
//#define PROCESSOR_SPSUMMON_S		135
//#define PROCESSOR_SPSUMMON_STEP_S	136
//#define PROCESSOR_SPSUMMON_COMP_S	137
//#define PROCESSOR_RANDOM_SELECT_S	138
//#define PROCESSOR_DRAW_S			140
//#define PROCESSOR_DAMAGE_S			141
//#define PROCESSOR_RECOVER_S			142
//#define PROCESSOR_EQUIP_S			143
//#define PROCESSOR_GET_CONTROL_S		144
//#define PROCESSOR_SWAP_CONTROL_S	145
//#define PROCESSOR_MOVETOFIELD_S		161

#endif /* FIELD_H_ */
