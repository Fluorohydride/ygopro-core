/*
 * scriptlib.h
 *
 *  Created on: 2009-1-20
 *      Author: Argon.Sun
 */

#ifndef SCRIPTLIB_H_
#define SCRIPTLIB_H_

#include "common.h"
#include "interpreter.h"

constexpr bool match_all(uint32 x, uint32 y) {
	return (x & y) == y;
}

class scriptlib {
public:
	static int32 check_param(lua_State* L, int32 param_type, int32 index, int32 retfalse = FALSE);
	static int32 check_param_count(lua_State* L, int32 count);
	static int32 check_action_permission(lua_State* L);

	//card lib
	static int32 card_get_code(lua_State *L);
	static int32 card_get_origin_code(lua_State *L);
	static int32 card_get_origin_code_rule(lua_State *L);
	static int32 card_get_fusion_code(lua_State *L);
	static int32 card_get_link_code(lua_State *L);
	static int32 card_is_fusion_code(lua_State *L);
	static int32 card_is_link_code(lua_State *L);
	static int32 card_is_set_card(lua_State *L);
	static int32 card_is_origin_set_card(lua_State *L);
	static int32 card_is_pre_set_card(lua_State *L);
	static int32 card_is_fusion_set_card(lua_State *L);
	static int32 card_is_link_set_card(lua_State *L);
	static int32 card_is_special_summon_set_card(lua_State *L);
	static int32 card_get_type(lua_State *L);
	static int32 card_get_origin_type(lua_State *L);
	static int32 card_get_fusion_type(lua_State *L);
	static int32 card_get_synchro_type(lua_State *L);
	static int32 card_get_xyz_type(lua_State *L);
	static int32 card_get_link_type(lua_State *L);
	static int32 card_get_level(lua_State *L);
	static int32 card_get_rank(lua_State *L);
	static int32 card_get_link(lua_State *L);
	static int32 card_get_synchro_level(lua_State *L);
	static int32 card_get_ritual_level(lua_State *L);
	static int32 card_get_origin_level(lua_State *L);
	static int32 card_get_origin_rank(lua_State *L);
	static int32 card_is_xyz_level(lua_State *L);
	static int32 card_get_lscale(lua_State *L);
	static int32 card_get_origin_lscale(lua_State *L);
	static int32 card_get_rscale(lua_State *L);
	static int32 card_get_origin_rscale(lua_State *L);
	static int32 card_get_current_scale(lua_State *L);
	static int32 card_is_link_marker(lua_State *L);
	static int32 card_get_linked_group(lua_State *L);
	static int32 card_get_linked_group_count(lua_State *L);
	static int32 card_get_linked_zone(lua_State *L);
	static int32 card_get_mutual_linked_group(lua_State *L);
	static int32 card_get_mutual_linked_group_count(lua_State *L);
	static int32 card_get_mutual_linked_zone(lua_State *L);
	static int32 card_is_link_state(lua_State *L);
	static int32 card_is_extra_link_state(lua_State *L);
	static int32 card_get_column_group(lua_State *L);
	static int32 card_get_column_group_count(lua_State *L);
	static int32 card_get_column_zone(lua_State *L);
	static int32 card_is_all_column(lua_State *L);
	static int32 card_get_attribute(lua_State *L);
	static int32 card_get_origin_attribute(lua_State *L);
	static int32 card_get_fusion_attribute(lua_State *L);
	static int32 card_get_link_attribute(lua_State *L);
	static int32 card_get_attribute_in_grave(lua_State *L);
	static int32 card_get_race(lua_State *L);
	static int32 card_get_origin_race(lua_State *L);
	static int32 card_get_link_race(lua_State *L);
	static int32 card_get_race_in_grave(lua_State *L);
	static int32 card_get_attack(lua_State *L);
	static int32 card_get_origin_attack(lua_State *L);
	static int32 card_get_text_attack(lua_State *L);
	static int32 card_get_defense(lua_State *L);
	static int32 card_get_origin_defense(lua_State *L);
	static int32 card_get_text_defense(lua_State *L);
	static int32 card_get_previous_code_onfield(lua_State *L);
	static int32 card_get_previous_type_onfield(lua_State *L);
	static int32 card_get_previous_level_onfield(lua_State *L);
	static int32 card_get_previous_rank_onfield(lua_State *L);
	static int32 card_get_previous_attribute_onfield(lua_State *L);
	static int32 card_get_previous_race_onfield(lua_State *L);
	static int32 card_get_previous_attack_onfield(lua_State *L);
	static int32 card_get_previous_defense_onfield(lua_State *L);
	static int32 card_get_owner(lua_State *L);
	static int32 card_get_controler(lua_State *L);
	static int32 card_get_previous_controler(lua_State *L);
	static int32 card_set_reason(lua_State *L);
	static int32 card_get_reason(lua_State *L);
	static int32 card_get_reason_card(lua_State *L);
	static int32 card_get_reason_player(lua_State *L);
	static int32 card_get_reason_effect(lua_State *L);
	static int32 card_get_position(lua_State *L);
	static int32 card_get_previous_position(lua_State *L);
	static int32 card_get_battle_position(lua_State *L);
	static int32 card_get_location(lua_State *L);
	static int32 card_get_previous_location(lua_State *L);
	static int32 card_get_sequence(lua_State *L);
	static int32 card_get_previous_sequence(lua_State *L);
	static int32 card_get_summon_type(lua_State *L);
	static int32 card_get_summon_location(lua_State *L);
	static int32 card_get_summon_player(lua_State *L);
	static int32 card_get_destination(lua_State *L);
	static int32 card_get_leave_field_dest(lua_State *L);
	static int32 card_get_turnid(lua_State *L);
	static int32 card_get_fieldid(lua_State *L);
	static int32 card_get_fieldidr(lua_State *L);
	static int32 card_is_origin_code_rule(lua_State *L);
	static int32 card_is_code(lua_State *L);
	static int32 card_is_type(lua_State *L);
	static int32 card_is_all_types(lua_State *L);
	static int32 card_is_fusion_type(lua_State *L);
	static int32 card_is_synchro_type(lua_State *L);
	static int32 card_is_xyz_type(lua_State *L);
	static int32 card_is_link_type(lua_State *L);
	static int32 card_is_level(lua_State *L);
	static int32 card_is_rank(lua_State *L);
	static int32 card_is_link(lua_State *L);
	static int32 card_is_attack(lua_State *L);
	static int32 card_is_defense(lua_State *L);
	static int32 card_is_race(lua_State *L);
	static int32 card_is_link_race(lua_State *L);
	static int32 card_is_attribute(lua_State *L);
	static int32 card_is_fusion_attribute(lua_State *L);
	static int32 card_is_link_attribute(lua_State *L);
	static int32 card_is_non_attribute(lua_State *L);
	static int32 card_is_extra_deck_monster(lua_State *L);
	static int32 card_is_reason(lua_State *L);
	static int32 card_is_all_reasons(lua_State *L);
	static int32 card_is_summon_type(lua_State *L);
	static int32 card_is_summon_location(lua_State *L);
	static int32 card_is_summon_player(lua_State *L);
	static int32 card_get_special_summon_info(lua_State *L);
	static int32 card_is_status(lua_State *L);
	static int32 card_is_not_tuner(lua_State *L);
	static int32 card_is_tuner(lua_State* L);
	static int32 card_set_status(lua_State *L);
	static int32 card_is_dual_state(lua_State *L);
	static int32 card_enable_dual_state(lua_State *L);
	static int32 card_set_turn_counter(lua_State *L);
	static int32 card_get_turn_counter(lua_State *L);
	static int32 card_set_material(lua_State *L);
	static int32 card_get_material(lua_State *L);
	static int32 card_get_material_count(lua_State *L);
	static int32 card_get_equip_group(lua_State *L);
	static int32 card_get_equip_count(lua_State *L);
	static int32 card_get_equip_target(lua_State *L);
	static int32 card_get_pre_equip_target(lua_State *L);
	static int32 card_check_equip_target(lua_State *L);
	static int32 card_check_union_target(lua_State *L);
	static int32 card_get_union_count(lua_State *L);
	static int32 card_get_overlay_group(lua_State *L);
	static int32 card_get_overlay_count(lua_State *L);
	static int32 card_get_overlay_target(lua_State *L);
	static int32 card_check_remove_overlay_card(lua_State *L);
	static int32 card_remove_overlay_card(lua_State *L);
	static int32 card_get_attacked_group(lua_State *L);
	static int32 card_get_attacked_group_count(lua_State *L);
	static int32 card_get_attacked_count(lua_State *L);
	static int32 card_get_battled_group(lua_State *L);
	static int32 card_get_battled_group_count(lua_State *L);
	static int32 card_get_attack_announced_count(lua_State *L);
	static int32 card_is_direct_attacked(lua_State *L);
	static int32 card_set_card_target(lua_State *L);
	static int32 card_get_card_target(lua_State *L);
	static int32 card_get_first_card_target(lua_State *L);
	static int32 card_get_card_target_count(lua_State *L);
	static int32 card_is_has_card_target(lua_State *L);
	static int32 card_cancel_card_target(lua_State *L);
	static int32 card_get_owner_target(lua_State *L);
	static int32 card_get_owner_target_count(lua_State *L);
	static int32 card_get_activate_effect(lua_State *L);
	static int32 card_check_activate_effect(lua_State *L);
	static int32 card_get_tuner_limit(lua_State * L);
	static int32 card_get_hand_synchro(lua_State * L);
	static int32 card_register_effect(lua_State *L);
	static int32 card_is_has_effect(lua_State *L);
	static int32 card_reset_effect(lua_State *L);
	static int32 card_get_effect_count(lua_State *L);
	static int32 card_register_flag_effect(lua_State *L);
	static int32 card_get_flag_effect(lua_State *L);
	static int32 card_reset_flag_effect(lua_State *L);
	static int32 card_set_flag_effect_label(lua_State *L);
	static int32 card_get_flag_effect_label(lua_State *L);
	static int32 card_create_relation(lua_State *L);
	static int32 card_release_relation(lua_State *L);
	static int32 card_create_effect_relation(lua_State *L);
	static int32 card_release_effect_relation(lua_State *L);
	static int32 card_clear_effect_relation(lua_State *L);
	static int32 card_is_relate_to_effect(lua_State *L);
	static int32 card_is_relate_to_chain(lua_State *L);
	static int32 card_is_relate_to_card(lua_State *L);
	static int32 card_is_relate_to_battle(lua_State *L);
	static int32 card_copy_effect(lua_State *L);
	static int32 card_replace_effect(lua_State *L);
	static int32 card_enable_revive_limit(lua_State *L);
	static int32 card_complete_procedure(lua_State *L);
	static int32 card_is_disabled(lua_State *L);
	static int32 card_is_destructable(lua_State *L);
	static int32 card_is_summonable(lua_State *L);
	static int32 card_is_special_summonable_card(lua_State *L);
	static int32 card_is_fusion_summonable_card(lua_State *L);
	static int32 card_is_msetable(lua_State *L);
	static int32 card_is_ssetable(lua_State *L);
	static int32 card_is_special_summonable(lua_State *L);
	static int32 card_is_synchro_summonable(lua_State *L);
	static int32 card_is_xyz_summonable(lua_State *L);
	static int32 card_is_link_summonable(lua_State *L);
	static int32 card_is_can_be_summoned(lua_State *L);
	static int32 card_is_can_be_special_summoned(lua_State *L);
	static int32 card_is_able_to_hand(lua_State *L);
	static int32 card_is_able_to_grave(lua_State *L);
	static int32 card_is_able_to_deck(lua_State *L);
	static int32 card_is_able_to_extra(lua_State *L);
	static int32 card_is_able_to_remove(lua_State *L);
	static int32 card_is_able_to_hand_as_cost(lua_State *L);
	static int32 card_is_able_to_grave_as_cost(lua_State *L);
	static int32 card_is_able_to_deck_as_cost(lua_State *L);
	static int32 card_is_able_to_extra_as_cost(lua_State *L);
	static int32 card_is_able_to_deck_or_extra_as_cost(lua_State *L);
	static int32 card_is_able_to_remove_as_cost(lua_State *L);
	static int32 card_is_releasable(lua_State *L);
	static int32 card_is_releasable_by_effect(lua_State *L);
	static int32 card_is_discardable(lua_State *L);
	static int32 card_is_attackable(lua_State *L);
	static int32 card_is_chain_attackable(lua_State *L);
	static int32 card_is_faceup(lua_State *L);
	static int32 card_is_faceup_ex(lua_State *L);
	static int32 card_is_attack_pos(lua_State *L);
	static int32 card_is_facedown(lua_State *L);
	static int32 card_is_defense_pos(lua_State *L);
	static int32 card_is_position(lua_State *L);
	static int32 card_is_pre_position(lua_State *L);
	static int32 card_is_controler(lua_State *L);
	static int32 card_is_pre_controler(lua_State *L);
	static int32 card_is_onfield(lua_State *L);
	static int32 card_is_location(lua_State *L);
	static int32 card_is_pre_location(lua_State *L);
	static int32 card_is_level_below(lua_State *L);
	static int32 card_is_level_above(lua_State *L);
	static int32 card_is_rank_below(lua_State *L);
	static int32 card_is_rank_above(lua_State *L);
	static int32 card_is_link_below(lua_State *L);
	static int32 card_is_link_above(lua_State *L);
	static int32 card_is_attack_below(lua_State *L);
	static int32 card_is_attack_above(lua_State *L);
	static int32 card_is_defense_below(lua_State *L);
	static int32 card_is_defense_above(lua_State *L);
	static int32 card_is_has_level(lua_State *L);
	static int32 card_is_has_defense(lua_State *L);
	static int32 card_is_public(lua_State *L);
	static int32 card_is_forbidden(lua_State *L);
	static int32 card_is_able_to_change_controler(lua_State *L);
	static int32 card_is_controler_can_be_changed(lua_State *L);
	static int32 card_add_counter(lua_State *L);
	static int32 card_remove_counter(lua_State *L);
	static int32 card_get_counter(lua_State *L);
	static int32 card_enable_counter_permit(lua_State *L);
	static int32 card_set_counter_limit(lua_State *L);
	static int32 card_is_can_change_position(lua_State *L);
	static int32 card_is_can_turn_set(lua_State *L);
	static int32 card_is_can_add_counter(lua_State *L);
	static int32 card_is_can_remove_counter(lua_State *L);
	static int32 card_is_can_have_counter(lua_State* L);
	static int32 card_is_can_overlay(lua_State *L);
	static int32 card_is_can_be_fusion_material(lua_State *L);
	static int32 card_is_can_be_synchro_material(lua_State *L);
	static int32 card_is_can_be_ritual_material(lua_State *L);
	static int32 card_is_can_be_xyz_material(lua_State *L);
	static int32 card_is_can_be_link_material(lua_State *L);
	static int32 card_check_fusion_material(lua_State *L);
	static int32 card_check_fusion_substitute(lua_State *L);
	static int32 card_is_immune_to_effect(lua_State *L);
	static int32 card_is_can_be_disabled_by_effect(lua_State* L);
	static int32 card_is_can_be_effect_target(lua_State *L);
	static int32 card_is_can_be_battle_target(lua_State *L);
	static int32 card_add_monster_attribute(lua_State *L);
	static int32 card_cancel_to_grave(lua_State *L);
	static int32 card_get_tribute_requirement(lua_State *L);
	static int32 card_get_battle_target(lua_State *L);
	static int32 card_get_attackable_target(lua_State *L);
	static int32 card_set_hint(lua_State *L);
	static int32 card_reverse_in_deck(lua_State *L);
	static int32 card_set_unique_onfield(lua_State *L);
	static int32 card_check_unique_onfield(lua_State *L);
	static int32 card_reset_negate_effect(lua_State *L);
	static int32 card_assume_prop(lua_State *L);
	static int32 card_set_spsummon_once(lua_State *L);
	static void open_cardlib(lua_State *L);

	//Effect functions
	static int32 effect_new(lua_State *L);
	static int32 effect_newex(lua_State *L);
	static int32 effect_clone(lua_State *L);
	static int32 effect_reset(lua_State *L);
	static int32 effect_get_field_id(lua_State *L);
	static int32 effect_set_description(lua_State *L);
	static int32 effect_set_code(lua_State *L);
	static int32 effect_set_range(lua_State *L);
	static int32 effect_set_target_range(lua_State *L);
	static int32 effect_set_absolute_range(lua_State *L);
	static int32 effect_set_count_limit(lua_State *L);
	static int32 effect_set_reset(lua_State *L);
	static int32 effect_set_type(lua_State *L);
	static int32 effect_set_property(lua_State *L);
	static int32 effect_set_label(lua_State *L);
	static int32 effect_set_label_object(lua_State *L);
	static int32 effect_set_category(lua_State *L);
	static int32 effect_set_hint_timing(lua_State *L);
	static int32 effect_set_condition(lua_State *L);
	static int32 effect_set_target(lua_State *L);
	static int32 effect_set_cost(lua_State *L);
	static int32 effect_set_value(lua_State *L);
	static int32 effect_set_operation(lua_State *L);
	static int32 effect_set_owner_player(lua_State *L);
	static int32 effect_get_description(lua_State *L);
	static int32 effect_get_code(lua_State *L);
	static int32 effect_get_type(lua_State *L);
	static int32 effect_get_property(lua_State *L);
	static int32 effect_get_label(lua_State *L);
	static int32 effect_get_label_object(lua_State *L);
	static int32 effect_get_category(lua_State *L);
	static int32 effect_get_owner(lua_State *L);
	static int32 effect_get_handler(lua_State *L);
	static int32 effect_get_owner_player(lua_State *L);
	static int32 effect_get_handler_player(lua_State *L);
	static int32 effect_get_condition(lua_State *L);
	static int32 effect_get_target(lua_State *L);
	static int32 effect_get_cost(lua_State *L);
	static int32 effect_get_value(lua_State *L);
	static int32 effect_get_operation(lua_State *L);
	static int32 effect_get_active_type(lua_State *L);
	static int32 effect_get_active_code(lua_State *L);
	static int32 effect_is_active_type(lua_State *L);
	static int32 effect_is_active_code(lua_State *L);
	static int32 effect_is_active_setcode(lua_State *L);
	static int32 effect_is_has_property(lua_State *L);
	static int32 effect_is_has_category(lua_State *L);
	static int32 effect_is_has_type(lua_State *L);
	static int32 effect_is_activatable(lua_State *L);
	static int32 effect_is_activated(lua_State *L);
	static int32 effect_is_cost_checked(lua_State *L);
	static int32 effect_set_cost_check(lua_State *L);
	static int32 effect_get_activate_location(lua_State *L);
	static int32 effect_get_activate_sequence(lua_State *L);
	static int32 effect_check_count_limit(lua_State *L);
	static int32 effect_use_count_limit(lua_State *L);
	static void open_effectlib(lua_State *L);

	//Group functions
	static int32 group_new(lua_State *L);
	static int32 group_clone(lua_State *L);
	static int32 group_from_cards(lua_State *L);
	static int32 group_delete(lua_State *L);
	static int32 group_keep_alive(lua_State *L);
	static int32 group_clear(lua_State *L);
	static int32 group_add_card(lua_State *L);
	static int32 group_remove_card(lua_State *L);
	static int32 group_get_next(lua_State *L);
	static int32 group_get_first(lua_State *L);
	static int32 group_get_count(lua_State *L);
	static int32 group_for_each(lua_State *L);
	static int32 group_filter(lua_State *L);
	static int32 group_filter_count(lua_State *L);
	static int32 group_filter_select(lua_State *L);
	static int32 group_select(lua_State *L);
	static int32 group_select_unselect(lua_State *L);
	static int32 group_random_select(lua_State *L);
	static int32 group_cancelable_select(lua_State *L);
	static int32 group_is_exists(lua_State *L);
	static int32 group_check_with_sum_equal(lua_State *L);
	static int32 group_select_with_sum_equal(lua_State *L);
	static int32 group_check_with_sum_greater(lua_State *L);
	static int32 group_select_with_sum_greater(lua_State *L);
	static int32 group_get_min_group(lua_State *L);
	static int32 group_get_max_group(lua_State *L);
	static int32 group_get_sum(lua_State *L);
	static int32 group_get_class_count(lua_State *L);
	static int32 group_remove(lua_State *L);
	static int32 group_merge(lua_State *L);
	static int32 group_sub(lua_State *L);
	static int32 group_equal(lua_State *L);
	static int32 group_is_contains(lua_State *L);
	static int32 group_search_card(lua_State *L);
	static int32 group_get_bin_class_count(lua_State *L);
	static void open_grouplib(lua_State *L);

	//Duel functions
	static int32 duel_enable_global_flag(lua_State *L);
	static int32 duel_get_lp(lua_State *L);
	static int32 duel_set_lp(lua_State *L);
	static int32 duel_is_turn_player(lua_State *L);
	static int32 duel_get_turn_player(lua_State *L);
	static int32 duel_get_turn_count(lua_State *L);
	static int32 duel_get_draw_count(lua_State *L);
	static int32 duel_register_effect(lua_State *L);
	static int32 duel_register_flag_effect(lua_State *L);
	static int32 duel_get_flag_effect(lua_State *L);
	static int32 duel_reset_flag_effect(lua_State *L);
	static int32 duel_set_flag_effect_label(lua_State *L);
	static int32 duel_get_flag_effect_label(lua_State *L);
	static int32 duel_destroy(lua_State *L);
	static int32 duel_remove(lua_State *L);
	static int32 duel_sendto_grave(lua_State *L);
	static int32 duel_summon(lua_State *L);
	static int32 duel_special_summon_rule(lua_State *L);
	static int32 duel_synchro_summon(lua_State *L);
	static int32 duel_xyz_summon(lua_State *L);
	static int32 duel_link_summon(lua_State *L);
	static int32 duel_setm(lua_State *L);
	static int32 duel_sets(lua_State *L);
	static int32 duel_create_token(lua_State *L);
	static int32 duel_special_summon(lua_State *L);
	static int32 duel_special_summon_step(lua_State *L);
	static int32 duel_special_summon_complete(lua_State *L);
	static int32 duel_sendto_hand(lua_State *L);
	static int32 duel_sendto_deck(lua_State *L);
	static int32 duel_sendto_extra(lua_State *L);
	static int32 duel_get_operated_group(lua_State *L);
	static int32 duel_is_can_add_counter(lua_State *L);
	static int32 duel_remove_counter(lua_State *L);
	static int32 duel_is_can_remove_counter(lua_State *L);
	static int32 duel_get_counter(lua_State *L);
	static int32 duel_change_form(lua_State *L);
	static int32 duel_release(lua_State *L);
	static int32 duel_move_to_field(lua_State *L);
	static int32 duel_return_to_field(lua_State *L);
	static int32 duel_move_sequence(lua_State *L);
	static int32 duel_swap_sequence(lua_State *L);
	static int32 duel_activate_effect(lua_State *L);
	static int32 duel_set_chain_limit(lua_State *L);
	static int32 duel_set_chain_limit_p(lua_State *L);
	static int32 duel_get_chain_material(lua_State *L);
	static int32 duel_confirm_decktop(lua_State *L);
	static int32 duel_confirm_extratop(lua_State *L);
	static int32 duel_confirm_cards(lua_State *L);
	static int32 duel_sort_decktop(lua_State *L);
	static int32 duel_check_event(lua_State *L);
	static int32 duel_raise_event(lua_State *L);
	static int32 duel_raise_single_event(lua_State *L);
	static int32 duel_check_timing(lua_State *L);
	static int32 duel_is_environment(lua_State *L);

	static int32 duel_win(lua_State *L);
	static int32 duel_draw(lua_State *L);
	static int32 duel_damage(lua_State *L);
	static int32 duel_recover(lua_State *L);
	static int32 duel_rd_complete(lua_State *L);
	static int32 duel_equip(lua_State *L);
	static int32 duel_equip_complete(lua_State *L);
	static int32 duel_get_control(lua_State *L);
	static int32 duel_swap_control(lua_State *L);
	static int32 duel_check_lp_cost(lua_State *L);
	static int32 duel_pay_lp_cost(lua_State *L);
	static int32 duel_discard_deck(lua_State *L);
	static int32 duel_discard_hand(lua_State *L);
	static int32 duel_disable_shuffle_check(lua_State *L);
	static int32 duel_disable_self_destroy_check(lua_State *L);
	static int32 duel_shuffle_deck(lua_State *L);
	static int32 duel_shuffle_extra(lua_State *L);
	static int32 duel_shuffle_hand(lua_State *L);
	static int32 duel_shuffle_setcard(lua_State *L);
	static int32 duel_change_attacker(lua_State *L);
	static int32 duel_change_attack_target(lua_State *L);
	static int32 duel_calculate_damage(lua_State *L);
	static int32 duel_get_battle_damage(lua_State *L);
	static int32 duel_change_battle_damage(lua_State *L);
	static int32 duel_change_target(lua_State *L);
	static int32 duel_change_target_player(lua_State *L);
	static int32 duel_change_target_param(lua_State *L);
	static int32 duel_break_effect(lua_State *L);
	static int32 duel_change_effect(lua_State *L);
	static int32 duel_negate_activate(lua_State *L);
	static int32 duel_negate_effect(lua_State *L);
	static int32 duel_negate_related_chain(lua_State *L);
	static int32 duel_disable_summon(lua_State *L);
	static int32 duel_increase_summon_count(lua_State *L);
	static int32 duel_check_summon_count(lua_State *L);
	static int32 duel_get_location_count(lua_State *L);
	static int32 duel_get_mzone_count(lua_State *L);
	static int32 duel_get_location_count_fromex(lua_State *L);
	static int32 duel_get_usable_mzone_count(lua_State *L);
	static int32 duel_get_linked_group(lua_State *L);
	static int32 duel_get_linked_group_count(lua_State *L);
	static int32 duel_get_linked_zone(lua_State *L);
	static int32 duel_get_field_card(lua_State *L);
	static int32 duel_check_location(lua_State *L);
	static int32 duel_get_current_chain(lua_State *L);
	static int32 duel_get_ready_chain(lua_State* L);
	static int32 duel_get_chain_info(lua_State *L);
	static int32 duel_get_chain_event(lua_State *L);
	static int32 duel_get_first_target(lua_State *L);
	static int32 duel_get_targets_relate_to_chain(lua_State *L);
	static int32 duel_is_phase(lua_State *L);
	static int32 duel_is_main_phase(lua_State *L);
	static int32 duel_is_battle_phase(lua_State *L);
	static int32 duel_get_current_phase(lua_State *L);
	static int32 duel_skip_phase(lua_State *L);
	static int32 duel_is_damage_calculated(lua_State *L);
	static int32 duel_get_attacker(lua_State *L);
	static int32 duel_get_attack_target(lua_State* L);
	static int32 duel_get_battle_monster(lua_State* L);
	static int32 duel_disable_attack(lua_State *L);
	static int32 duel_chain_attack(lua_State *L);
	static int32 duel_readjust(lua_State *L);
	static int32 duel_adjust_instantly(lua_State *L);
	static int32 duel_adjust_all(lua_State *L);

	static int32 duel_get_field_group(lua_State *L);
	static int32 duel_get_field_group_count(lua_State *L);
	static int32 duel_get_decktop_group(lua_State *L);
	static int32 duel_get_extratop_group(lua_State *L);
	static int32 duel_get_matching_group(lua_State *L);
	static int32 duel_get_matching_count(lua_State *L);
	static int32 duel_get_first_matching_card(lua_State *L);
	static int32 duel_is_existing_matching_card(lua_State *L);
	static int32 duel_select_matching_cards(lua_State *L);
	static int32 duel_get_release_group(lua_State *L);
	static int32 duel_get_release_group_count(lua_State *L);
	static int32 duel_check_release_group(lua_State *L);
	static int32 duel_select_release_group(lua_State *L);
	static int32 duel_check_release_group_ex(lua_State *L);
	static int32 duel_select_release_group_ex(lua_State *L);
	static int32 duel_get_tribute_group(lua_State *L);
	static int32 duel_get_tribute_count(lua_State *L);
	static int32 duel_check_tribute(lua_State *L);
	static int32 duel_select_tribute(lua_State *L);
	static int32 duel_get_target_count(lua_State *L);
	static int32 duel_is_existing_target(lua_State *L);
	static int32 duel_select_target(lua_State *L);
	static int32 duel_get_must_material(lua_State *L);
	static int32 duel_check_must_material(lua_State *L);
	static int32 duel_select_fusion_material(lua_State *L);
	static int32 duel_set_fusion_material(lua_State *L);
	static int32 duel_set_synchro_material(lua_State *L);
	static int32 duel_get_synchro_material(lua_State *L);
	static int32 duel_select_synchro_material(lua_State *L);
	static int32 duel_check_synchro_material(lua_State *L);
	static int32 duel_select_tuner_material(lua_State *L);
	static int32 duel_check_tuner_material(lua_State *L);
	static int32 duel_get_ritual_material(lua_State *L);
	static int32 duel_get_ritual_material_ex(lua_State *L);
	static int32 duel_release_ritual_material(lua_State *L);
	static int32 duel_get_fusion_material(lua_State *L);
	static int32 duel_is_summon_cancelable(lua_State *L);
	static int32 duel_set_must_select_cards(lua_State *L);
	static int32 duel_grab_must_select_cards(lua_State *L);
	static int32 duel_set_target_card(lua_State *L);
	static int32 duel_clear_target_card(lua_State *L);
	static int32 duel_set_target_player(lua_State *L);
	static int32 duel_set_target_param(lua_State *L);
	static int32 duel_set_operation_info(lua_State *L);
	static int32 duel_get_operation_info(lua_State *L);
	static int32 duel_get_operation_count(lua_State *L);
	static int32 duel_clear_operation_info(lua_State *L);
	static int32 duel_check_xyz_material(lua_State *L);
	static int32 duel_select_xyz_material(lua_State *L);
	static int32 duel_overlay(lua_State *L);
	static int32 duel_get_overlay_group(lua_State *L);
	static int32 duel_get_overlay_count(lua_State *L);
	static int32 duel_check_remove_overlay_card(lua_State *L);
	static int32 duel_remove_overlay_card(lua_State *L);

	static int32 duel_hint(lua_State *L);
	static int32 duel_hint_selection(lua_State *L);
	static int32 duel_select_effect_yesno(lua_State *L);
	static int32 duel_select_yesno(lua_State *L);
	static int32 duel_select_option(lua_State *L);
	static int32 duel_select_sequence(lua_State *L);
	static int32 duel_select_position(lua_State *L);
	static int32 duel_select_disable_field(lua_State *L);
	static int32 duel_select_field(lua_State *L);
	static int32 duel_announce_race(lua_State *L);
	static int32 duel_announce_attribute(lua_State *L);
	static int32 duel_announce_level(lua_State *L);
	static int32 duel_announce_card(lua_State *L);
	static int32 duel_announce_type(lua_State *L);
	static int32 duel_announce_number(lua_State *L);
	static int32 duel_announce_coin(lua_State *L);
	static int32 duel_toss_coin(lua_State *L);
	static int32 duel_toss_dice(lua_State *L);
	static int32 duel_rock_paper_scissors(lua_State *L);
	static int32 duel_get_coin_result(lua_State *L);
	static int32 duel_get_dice_result(lua_State *L);
	static int32 duel_set_coin_result(lua_State *L);
	static int32 duel_set_dice_result(lua_State *L);

	static int32 duel_is_player_affected_by_effect(lua_State *L);
	static int32 duel_is_player_can_draw(lua_State *L);
	static int32 duel_is_player_can_discard_deck(lua_State *L);
	static int32 duel_is_player_can_discard_deck_as_cost(lua_State *L);
	static int32 duel_is_player_can_summon(lua_State *L);
	static int32 duel_is_player_can_mset(lua_State *L);
	static int32 duel_is_player_can_sset(lua_State *L);
	static int32 duel_is_player_can_spsummon(lua_State *L);
	static int32 duel_is_player_can_flipsummon(lua_State *L);
	static int32 duel_is_player_can_spsummon_monster(lua_State *L);
	static int32 duel_is_player_can_spsummon_count(lua_State *L);
	static int32 duel_is_player_can_release(lua_State *L);
	static int32 duel_is_player_can_remove(lua_State *L);
	static int32 duel_is_player_can_send_to_hand(lua_State *L);
	static int32 duel_is_player_can_send_to_grave(lua_State *L);
	static int32 duel_is_player_can_send_to_deck(lua_State *L);
	static int32 duel_is_player_can_additional_summon(lua_State *L);
	static int32 duel_is_chain_negatable(lua_State *L);
	static int32 duel_is_chain_disablable(lua_State *L);
	static int32 duel_is_chain_disabled(lua_State *L);
	static int32 duel_check_chain_target(lua_State *L);
	static int32 duel_check_chain_uniqueness(lua_State *L);
	static int32 duel_get_activity_count(lua_State *L);
	static int32 duel_check_phase_activity(lua_State *L);
	static int32 duel_add_custom_activity_counter(lua_State *L);
	static int32 duel_get_custom_activity_count(lua_State *L);
	static int32 duel_is_able_to_enter_bp(lua_State *L);
	static int32 duel_get_battled_count(lua_State *L);

	//specific card functions
	static int32 duel_swap_deck_and_grave(lua_State *L);
	static int32 duel_majestic_copy(lua_State *L);

	static void open_duellib(lua_State *L);

	//group metamethods
	//__len is in the group lib, which is same as group_get_count
	static int32 group_meta_add(lua_State *L);
	static int32 group_meta_sub(lua_State *L);
	static int32 group_meta_band(lua_State *L);
	static int32 group_meta_bxor(lua_State *L);

	//preload
	static int32 debug_message(lua_State *L);
	static int32 debug_add_card(lua_State *L);
	static int32 debug_set_player_info(lua_State *L);
	static int32 debug_pre_summon(lua_State *L);
	static int32 debug_pre_equip(lua_State *L);
	static int32 debug_pre_set_target(lua_State *L);
	static int32 debug_pre_add_counter(lua_State *L);
	static int32 debug_reload_field_begin(lua_State *L);
	static int32 debug_reload_field_end(lua_State *L);
	static int32 debug_set_ai_name(lua_State *L);
	static int32 debug_show_hint(lua_State *L);
	static void open_debuglib(lua_State *L);
};

#endif /* SCRIPTLIB_H_ */
