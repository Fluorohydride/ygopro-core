/*
 * effect.cpp
 *
 *  Created on: 2010-5-7
 *      Author: Argon
 */

#include "effect.h"
#include "card.h"
#include "duel.h"
#include "group.h"
#include "interpreter.h"

bool effect_sort_id(const effect* e1, const effect* e2) {
	int32_t is_single1 = e1->is_initial_single();
	int32_t is_single2 = e2->is_initial_single();
	if (is_single1 != is_single2)
		return is_single1 > is_single2;
	return e1->id < e2->id;
}
// return: code is an event reserved for EFFECT_TYPE_CONTINUOUS or not
bool is_continuous_event(uint32_t code) {
	if (code & EVENT_CUSTOM)
		return false;
	else if (code & 0xf0000) // EVENT_ADD_COUNTER, EVENT_REMOVE_COUNTER
		return false;
	else if (code & 0xf000) // EVENT_PHASE_START must be continuous, but other EVENT_PHASE must not be
		return !!(code & EVENT_PHASE_START);
	else
		return continuous_event.find(code) != continuous_event.end();
}

effect::effect(duel* pd)
	: pduel(pd) {
	label.reserve(4);
}
int32_t effect::is_disable_related() const {
	if (code == EFFECT_IMMUNE_EFFECT || code == EFFECT_DISABLE || code == EFFECT_CANNOT_DISABLE || code == EFFECT_FORBIDDEN)
		return TRUE;
	return FALSE;
}
int32_t effect::is_self_destroy_related() const {
	if(code == EFFECT_UNIQUE_CHECK || code == EFFECT_SELF_DESTROY || code == EFFECT_SELF_TOGRAVE)
		return TRUE;
	return FALSE;
}
int32_t effect::is_can_be_forbidden() const {
	if (is_flag(EFFECT_FLAG_CANNOT_DISABLE) && !is_flag(EFFECT_FLAG_CANNOT_NEGATE))
		return FALSE;
	return TRUE;
}
// check if a single/field/equip effect is available
// check properties: range, EFFECT_FLAG_OWNER_RELATE, STATUS_BATTLE_DESTROYED, STATUS_EFFECT_ENABLED, disabled/forbidden
// check fucntions: condition
int32_t effect::is_available(int32_t neglect_disabled) {
	if (type & EFFECT_TYPE_ACTIONS)
		return FALSE;
	if ((type & (EFFECT_TYPE_SINGLE | EFFECT_TYPE_XMATERIAL)) && !(type & EFFECT_TYPE_FIELD)) {
		card* phandler = get_handler();
		card* powner = get_owner();
		if (phandler->current.controler == PLAYER_NONE)
			return FALSE;
		if(is_flag(EFFECT_FLAG_SINGLE_RANGE) && !in_range(phandler))
			return FALSE;
		if(is_flag(EFFECT_FLAG_SINGLE_RANGE) && !phandler->get_status(STATUS_EFFECT_ENABLED) && !is_flag(EFFECT_FLAG_IMMEDIATELY_APPLY))
			return FALSE;
		if(is_flag(EFFECT_FLAG_SINGLE_RANGE) && (phandler->current.location & LOCATION_ONFIELD) && !phandler->is_position(POS_FACEUP))
			return FALSE;
		if(is_flag(EFFECT_FLAG_OWNER_RELATE) && is_can_be_forbidden() && powner->is_status(STATUS_FORBIDDEN))
			return FALSE;
		if(powner == phandler && is_can_be_forbidden() && phandler->get_status(STATUS_FORBIDDEN))
			return FALSE;
		if(is_flag(EFFECT_FLAG_OWNER_RELATE) && !(is_flag(EFFECT_FLAG_CANNOT_DISABLE) || neglect_disabled) && powner->is_status(STATUS_DISABLED))
			return FALSE;
		if(powner == phandler && !(is_flag(EFFECT_FLAG_CANNOT_DISABLE) || neglect_disabled) && phandler->get_status(STATUS_DISABLED))
			return FALSE;
	}
	if (type & EFFECT_TYPE_EQUIP) {
		if(handler->current.controler == PLAYER_NONE)
			return FALSE;
		if(is_flag(EFFECT_FLAG_OWNER_RELATE) && is_can_be_forbidden() && owner->is_status(STATUS_FORBIDDEN))
			return FALSE;
		if(owner == handler && is_can_be_forbidden() && handler->get_status(STATUS_FORBIDDEN))
			return FALSE;
		if(is_flag(EFFECT_FLAG_OWNER_RELATE) && !(is_flag(EFFECT_FLAG_CANNOT_DISABLE) || neglect_disabled) && owner->is_status(STATUS_DISABLED))
			return FALSE;
		if(owner == handler && !(is_flag(EFFECT_FLAG_CANNOT_DISABLE) || neglect_disabled) && handler->get_status(STATUS_DISABLED))
			return FALSE;
		if(!is_flag(EFFECT_FLAG_SET_AVAILABLE)) {
			if(!(handler->get_status(STATUS_EFFECT_ENABLED)))
				return FALSE;
			if(!handler->is_position(POS_FACEUP))
				return FALSE;
		}
	}
	if (type & (EFFECT_TYPE_FIELD | EFFECT_TYPE_TARGET)) {
		card* phandler = get_handler();
		card* powner = get_owner();
		if (!is_flag(EFFECT_FLAG_FIELD_ONLY)) {
			if(phandler->current.controler == PLAYER_NONE)
				return FALSE;
			if(!in_range(phandler))
				return FALSE;
			if(!phandler->get_status(STATUS_EFFECT_ENABLED) && !is_flag(EFFECT_FLAG_IMMEDIATELY_APPLY))
				return FALSE;
			if (phandler->current.is_location(LOCATION_ONFIELD) && !phandler->is_position(POS_FACEUP))
				return FALSE;
			if (phandler->current.is_stzone() && (phandler->data.type & (TYPE_SPELL | TYPE_TRAP)) && phandler->is_affected_by_effect(EFFECT_CHANGE_TYPE))
				return FALSE;
			if(is_flag(EFFECT_FLAG_OWNER_RELATE) && is_can_be_forbidden() && powner->is_status(STATUS_FORBIDDEN))
				return FALSE;
			if(powner == phandler && is_can_be_forbidden() && phandler->get_status(STATUS_FORBIDDEN))
				return FALSE;
			if(is_flag(EFFECT_FLAG_OWNER_RELATE) && !(is_flag(EFFECT_FLAG_CANNOT_DISABLE) || neglect_disabled) && powner->is_status(STATUS_DISABLED))
				return FALSE;
			if(powner == phandler && !(is_flag(EFFECT_FLAG_CANNOT_DISABLE) || neglect_disabled) && phandler->get_status(STATUS_DISABLED))
				return FALSE;
			if(phandler->is_status(STATUS_BATTLE_DESTROYED))
				return FALSE;
		}
	}
	if (!condition)
		return TRUE;
	pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
	int32_t res = pduel->lua->check_condition(condition, 1);
	if(res) {
		if(!(status & EFFECT_STATUS_AVAILABLE))
			id = pduel->game_field->infos.field_id++;
		status |= EFFECT_STATUS_AVAILABLE;
	} else
		status &= ~EFFECT_STATUS_AVAILABLE;
	return res;
}
// check if a count limit effect counter is available, which should be available even if the effect is disabled
int32_t effect::limit_counter_is_available() {
	return is_available(TRUE);
}
// check if a effect is EFFECT_TYPE_SINGLE and is ready
// check: range, enabled, condition
int32_t effect::is_single_ready() {
	if(type & EFFECT_TYPE_ACTIONS)
		return FALSE;
	if((type & (EFFECT_TYPE_SINGLE | EFFECT_TYPE_XMATERIAL)) && !(type & EFFECT_TYPE_FIELD)) {
		card* phandler = get_handler();
		if(phandler->current.controler == PLAYER_NONE)
			return FALSE;
		if(is_flag(EFFECT_FLAG_SINGLE_RANGE) && !in_range(phandler))
			return FALSE;
		if(is_flag(EFFECT_FLAG_SINGLE_RANGE) && !phandler->get_status(STATUS_EFFECT_ENABLED) && !is_flag(EFFECT_FLAG_IMMEDIATELY_APPLY))
			return FALSE;
		if(is_flag(EFFECT_FLAG_SINGLE_RANGE) && (phandler->current.location & LOCATION_ONFIELD) && !phandler->is_position(POS_FACEUP))
			return FALSE;
	}
	else
		return FALSE;
	if(!condition)
		return TRUE;
	pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
	int32_t res = pduel->lua->check_condition(condition, 1);
	return res;
}
int32_t effect::check_count_limit(uint8_t playerid) {
	if(is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
		if(count_limit == 0)
			return FALSE;
		if(count_code) {
			uint32_t limit_code = count_code & MAX_CARD_ID;
			uint32_t limit_type = count_code & 0xf0000000U;
			int32_t count = count_limit_max;
			if(limit_code == EFFECT_COUNT_CODE_SINGLE) {
				if(pduel->game_field->get_effect_code(limit_type | get_handler()->activate_count_id, PLAYER_NONE) >= count)
					return FALSE;
			} else {
				if(pduel->game_field->get_effect_code(count_code, playerid) >= count)
					return FALSE;
			}
		}
	}
	return TRUE;
}
// check activate in hand/in set turn
int32_t effect::get_required_handorset_effects(effect_set* eset, uint8_t playerid, const tevent& e, int32_t neglect_loc) {
	eset->clear();
	if(!(type & EFFECT_TYPE_ACTIVATE))
		return 1;
	int32_t ecode = 0;
	if (handler->current.location == LOCATION_HAND && !neglect_loc)
	{
		if(handler->data.type & TYPE_TRAP)
			ecode = EFFECT_TRAP_ACT_IN_HAND;
		else if((handler->data.type & TYPE_SPELL) && pduel->game_field->infos.turn_player != playerid) {
			if(handler->data.type & TYPE_QUICKPLAY)
				ecode = EFFECT_QP_ACT_IN_NTPHAND;
			else
				return FALSE;
		}
	}
	else if (handler->current.location == LOCATION_SZONE)
	{
		if((handler->data.type & TYPE_TRAP) && handler->get_status(STATUS_SET_TURN))
			ecode = EFFECT_TRAP_ACT_IN_SET_TURN;
		if((handler->data.type & TYPE_SPELL) && (handler->data.type & TYPE_QUICKPLAY) && handler->get_status(STATUS_SET_TURN))
			ecode = EFFECT_QP_ACT_IN_SET_TURN;
	}
	if (!ecode)
		return 1;
	int32_t available = 0;
	effect_set tmp_eset;
	handler->filter_effect(ecode, &tmp_eset);
	if(!tmp_eset.size())
		return available;
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_player = playerid;
	pduel->game_field->save_lp_cost();
	for(effect_set::size_type i = 0; i < tmp_eset.size(); ++i) {
		auto peffect = tmp_eset[i];
		if(peffect->check_count_limit(playerid)) {
			pduel->game_field->core.reason_effect = peffect;
			pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			pduel->lua->add_param(e.event_cards , PARAM_TYPE_GROUP);
			pduel->lua->add_param(e.event_player, PARAM_TYPE_INT);
			pduel->lua->add_param(e.event_value, PARAM_TYPE_INT);
			pduel->lua->add_param(e.reason_effect , PARAM_TYPE_EFFECT);
			pduel->lua->add_param(e.reason, PARAM_TYPE_INT);
			pduel->lua->add_param(e.reason_player, PARAM_TYPE_INT);
			pduel->lua->add_param(0, PARAM_TYPE_INT);
			pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
			if(pduel->lua->check_condition(peffect->cost, 10)) {
				available = 2;
				eset->push_back(peffect);
			}
		}
	}
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	pduel->game_field->restore_lp_cost();
	return available;
}
// check if an EFFECT_TYPE_ACTIONS effect can be activated
// for triggering effects, it checks EFFECT_FLAG_DAMAGE_STEP, EFFECT_FLAG_SET_AVAILABLE
int32_t effect::is_activateable(uint8_t playerid, const tevent& e, int32_t neglect_cond, int32_t neglect_cost, int32_t neglect_target, int32_t neglect_loc, int32_t neglect_faceup) {
	if(!(type & EFFECT_TYPE_ACTIONS))
		return FALSE;
	if(!check_count_limit(playerid))
		return FALSE;
	if (!is_flag(EFFECT_FLAG_FIELD_ONLY)) {
		if (type & EFFECT_TYPE_ACTIVATE) {
			if(handler->current.controler != playerid)
				return FALSE;
			if(pduel->game_field->check_unique_onfield(handler, playerid, LOCATION_SZONE))
				return FALSE;
			if(!(handler->data.type & TYPE_COUNTER)) {
				if((code < 1132 || code > 1149) && pduel->game_field->infos.phase == PHASE_DAMAGE && !is_flag(EFFECT_FLAG_DAMAGE_STEP)
					&& !pduel->game_field->get_cteffect(this, playerid, FALSE))
					return FALSE;
				if((code < 1134 || code > 1136) && pduel->game_field->infos.phase == PHASE_DAMAGE_CAL && !is_flag(EFFECT_FLAG_DAMAGE_CAL)
					&& !pduel->game_field->get_cteffect(this, playerid, FALSE))
					return FALSE;
			}
			uint32_t zone = 0xff;
			if(!(handler->data.type & (TYPE_FIELD | TYPE_PENDULUM)) && is_flag(EFFECT_FLAG_LIMIT_ZONE)) {
				pduel->lua->add_param(playerid, PARAM_TYPE_INT);
				pduel->lua->add_param(e.event_cards , PARAM_TYPE_GROUP);
				pduel->lua->add_param(e.event_player, PARAM_TYPE_INT);
				pduel->lua->add_param(e.event_value, PARAM_TYPE_INT);
				pduel->lua->add_param(e.reason_effect , PARAM_TYPE_EFFECT);
				pduel->lua->add_param(e.reason, PARAM_TYPE_INT);
				pduel->lua->add_param(e.reason_player, PARAM_TYPE_INT);
				zone = get_value(7);
				if(!zone)
					zone = 0xff;
			}
			if(handler->current.location == LOCATION_SZONE) {
				if(handler->is_position(POS_FACEUP))
					return FALSE;
				if(handler->equiping_target)
					return FALSE;
				if(!(handler->data.type & (TYPE_FIELD | TYPE_PENDULUM)) && is_flag(EFFECT_FLAG_LIMIT_ZONE) && !(zone & (1u << handler->current.sequence)))
					return FALSE;
			} else {
				if(handler->data.type & TYPE_MONSTER) {
					if(!(handler->data.type & TYPE_PENDULUM))
						return FALSE;
					if(!pduel->game_field->is_location_useable(playerid, LOCATION_PZONE, 0)
						&& !pduel->game_field->is_location_useable(playerid, LOCATION_PZONE, 1))
						return FALSE;
				} else if(!(handler->data.type & TYPE_FIELD)
					&& pduel->game_field->get_useable_count(handler, playerid, LOCATION_SZONE, playerid, LOCATION_REASON_TOFIELD, zone) <= 0)
					return FALSE;
			}
			// check activate in hand/in set turn
			effect_set eset;
			if(!get_required_handorset_effects(&eset, playerid, e, neglect_loc))
				return FALSE;
			if(handler->is_status(STATUS_FORBIDDEN))
				return FALSE;
			if(handler->is_affected_by_effect(EFFECT_CANNOT_TRIGGER))
				return FALSE;
		} else if(!(type & EFFECT_TYPE_CONTINUOUS)) {
			card* phandler = get_handler();
			if(!(phandler->get_type() & TYPE_MONSTER) && (get_active_type(FALSE) & TYPE_MONSTER))
				return FALSE;
			if((type & EFFECT_TYPE_QUICK_O) && is_flag(EFFECT_FLAG_DELAY) && !in_range(phandler))
				return FALSE;
			if(!neglect_faceup && (phandler->current.location & (LOCATION_ONFIELD | LOCATION_REMOVED))) {
				if(!phandler->is_position(POS_FACEUP) && !is_flag(EFFECT_FLAG_SET_AVAILABLE))
					return FALSE;
				if(phandler->is_position(POS_FACEUP) && !phandler->is_status(STATUS_EFFECT_ENABLED))
					return FALSE;
			}
			if(!(type & (EFFECT_TYPE_FLIP | EFFECT_TYPE_TRIGGER_F)) && !((type & EFFECT_TYPE_TRIGGER_O) && (type & EFFECT_TYPE_SINGLE))) {
				if((code < 1132 || code > 1149) && pduel->game_field->infos.phase == PHASE_DAMAGE && !is_flag(EFFECT_FLAG_DAMAGE_STEP))
					return FALSE;
				if((code < 1134 || code > 1136) && pduel->game_field->infos.phase == PHASE_DAMAGE_CAL && !is_flag(EFFECT_FLAG_DAMAGE_CAL))
					return FALSE;
			}
			if(phandler->current.location == LOCATION_OVERLAY)
				return FALSE;
			if(phandler->current.location == LOCATION_DECK
				|| pduel->game_field->core.duel_rule >= MASTER_RULE_2020 && phandler->current.location == LOCATION_EXTRA && (phandler->current.position & POS_FACEDOWN)) {
				if((type & EFFECT_TYPE_SINGLE) && code != EVENT_TO_DECK)
					return FALSE;
				if((type & EFFECT_TYPE_FIELD) && !(range & (LOCATION_DECK | LOCATION_EXTRA)))
					return FALSE;
			}
			if (phandler->current.is_stzone() && (phandler->data.type & (TYPE_SPELL|TYPE_TRAP)) && phandler->is_affected_by_effect(EFFECT_CHANGE_TYPE))
				return FALSE;
			if((type & EFFECT_TYPE_FIELD) && (phandler->current.controler != playerid) && !is_flag(EFFECT_FLAG_BOTH_SIDE | EFFECT_FLAG_EVENT_PLAYER))
				return FALSE;
			if(phandler->is_status(STATUS_FORBIDDEN))
				return FALSE;
			if(phandler->is_affected_by_effect(EFFECT_CANNOT_TRIGGER))
				return FALSE;
		} else {
			card* phandler = get_handler();
			if((type & EFFECT_TYPE_FIELD) && phandler->is_status(STATUS_BATTLE_DESTROYED))
				return FALSE;
			if(((type & EFFECT_TYPE_FIELD) || ((type & EFFECT_TYPE_SINGLE) && is_flag(EFFECT_FLAG_SINGLE_RANGE)))
				&& (phandler->current.location & LOCATION_ONFIELD)
				&& (!phandler->is_position(POS_FACEUP) || !phandler->is_status(STATUS_EFFECT_ENABLED)))
				return FALSE;
			if((type & EFFECT_TYPE_SINGLE) && is_flag(EFFECT_FLAG_SINGLE_RANGE) && !in_range(phandler))
				return FALSE;
			if(is_flag(EFFECT_FLAG_OWNER_RELATE) && is_can_be_forbidden() && owner->is_status(STATUS_FORBIDDEN))
				return FALSE;
			if(phandler == owner && is_can_be_forbidden() && phandler->is_status(STATUS_FORBIDDEN))
				return FALSE;
			if(is_flag(EFFECT_FLAG_OWNER_RELATE) && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && owner->is_status(STATUS_DISABLED))
				return FALSE;
			if(phandler == owner && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && phandler->is_status(STATUS_DISABLED))
				return FALSE;
		}
	} else {
		if((get_owner_player() != playerid) && !is_flag(EFFECT_FLAG_BOTH_SIDE))
			return FALSE;
	}
	pduel->game_field->save_lp_cost();
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = this;
	pduel->game_field->core.reason_player = playerid;
	int32_t result = TRUE;
	if(!(type & EFFECT_TYPE_CONTINUOUS))
		result = is_action_check(playerid);
	if(result)
		result = is_activate_ready(playerid, e, neglect_cond, neglect_cost, neglect_target);
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	pduel->game_field->restore_lp_cost();
	return result;
}
// check functions: value of EFFECT_CANNOT_ACTIVATE, target, cost of EFFECT_ACTIVATE_COST
int32_t effect::is_action_check(uint8_t playerid) {
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, EFFECT_CANNOT_ACTIVATE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(eset[i]->check_value_condition(2))
			return FALSE;
	}
	eset.clear();
	pduel->game_field->filter_player_effect(playerid, EFFECT_ACTIVATE_COST, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(!pduel->lua->check_condition(eset[i]->target, 3))
			continue;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(!pduel->lua->check_condition(eset[i]->cost, 3))
			return FALSE;
	}
	return TRUE;
}
// check functions: condition, cost(chk=0), target(chk=0)
int32_t effect::is_activate_ready(effect* reason_effect, uint8_t playerid, const tevent& e, int32_t neglect_cond, int32_t neglect_cost, int32_t neglect_target) {
	if(!neglect_cond && condition) {
		pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(e.event_cards, PARAM_TYPE_GROUP);
		pduel->lua->add_param(e.event_player, PARAM_TYPE_INT);
		pduel->lua->add_param(e.event_value, PARAM_TYPE_INT);
		pduel->lua->add_param(e.reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(e.reason, PARAM_TYPE_INT);
		pduel->lua->add_param(e.reason_player, PARAM_TYPE_INT);
		if(!pduel->lua->check_condition(condition, 8)) {
			return FALSE;
		}
	}
	if(!neglect_cost && !(type & EFFECT_TYPE_CONTINUOUS)) {
		reason_effect->cost_checked = TRUE;
		if(cost) {
			pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			pduel->lua->add_param(e.event_cards, PARAM_TYPE_GROUP);
			pduel->lua->add_param(e.event_player, PARAM_TYPE_INT);
			pduel->lua->add_param(e.event_value, PARAM_TYPE_INT);
			pduel->lua->add_param(e.reason_effect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(e.reason, PARAM_TYPE_INT);
			pduel->lua->add_param(e.reason_player, PARAM_TYPE_INT);
			pduel->lua->add_param(0, PARAM_TYPE_INT);
			if(!pduel->lua->check_condition(cost, 9)) {
				reason_effect->cost_checked = FALSE;
				return FALSE;
			}
		}
	} else {
		reason_effect->cost_checked = FALSE;
	}
	if(!neglect_target && target) {
		pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(e.event_cards, PARAM_TYPE_GROUP);
		pduel->lua->add_param(e.event_player, PARAM_TYPE_INT);
		pduel->lua->add_param(e.event_value, PARAM_TYPE_INT);
		pduel->lua->add_param(e.reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(e.reason, PARAM_TYPE_INT);
		pduel->lua->add_param(e.reason_player, PARAM_TYPE_INT);
		pduel->lua->add_param(0, PARAM_TYPE_INT);
		if(!pduel->lua->check_condition(target, 9)) {
			reason_effect->cost_checked = FALSE;
			return FALSE;
		}
	}
	reason_effect->cost_checked = FALSE;
	return TRUE;
}
int32_t effect::is_activate_ready(uint8_t playerid, const tevent& e, int32_t neglect_cond, int32_t neglect_cost, int32_t neglect_target) {
	return is_activate_ready(this, playerid, e, neglect_cond, neglect_cost, neglect_target);
}
// check functions: condition
int32_t effect::is_condition_check(uint8_t playerid, const tevent& e) {
	card* phandler = get_handler();
	if(!(type & EFFECT_TYPE_ACTIVATE) && (phandler->current.location & (LOCATION_ONFIELD | LOCATION_REMOVED)) && !phandler->is_position(POS_FACEUP))
		return FALSE;
	if(!condition)
		return TRUE;
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = this;
	pduel->game_field->core.reason_player = playerid;
	pduel->game_field->save_lp_cost();
	pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
	pduel->lua->add_param(playerid, PARAM_TYPE_INT);
	pduel->lua->add_param(e.event_cards , PARAM_TYPE_GROUP);
	pduel->lua->add_param(e.event_player, PARAM_TYPE_INT);
	pduel->lua->add_param(e.event_value, PARAM_TYPE_INT);
	pduel->lua->add_param(e.reason_effect , PARAM_TYPE_EFFECT);
	pduel->lua->add_param(e.reason, PARAM_TYPE_INT);
	pduel->lua->add_param(e.reason_player, PARAM_TYPE_INT);
	if(!pduel->lua->check_condition(condition, 8)) {
		pduel->game_field->restore_lp_cost();
		pduel->game_field->core.reason_effect = oreason;
		pduel->game_field->core.reason_player = op;
		return FALSE;
	}
	pduel->game_field->restore_lp_cost();
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	return TRUE;
}
int32_t effect::is_activate_check(uint8_t playerid, const tevent& e, int32_t neglect_cond, int32_t neglect_cost, int32_t neglect_target) {
	pduel->game_field->save_lp_cost();
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = this;
	pduel->game_field->core.reason_player = playerid;
	int32_t result = is_activate_ready(playerid, e, neglect_cond, neglect_cost, neglect_target);
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	pduel->game_field->restore_lp_cost();
	return result;
}
int32_t effect::is_target(card* pcard) {
	if(type & EFFECT_TYPE_ACTIONS)
		return FALSE;
	if(type & (EFFECT_TYPE_SINGLE | EFFECT_TYPE_EQUIP | EFFECT_TYPE_XMATERIAL) && !(type & EFFECT_TYPE_FIELD))
		return TRUE;
	if((type & EFFECT_TYPE_TARGET) && !(type & EFFECT_TYPE_FIELD)) {
		return is_fit_target_function(pcard);
	}
	if(!is_flag(EFFECT_FLAG_SET_AVAILABLE) && (pcard->current.location & LOCATION_ONFIELD)
			&& !pcard->is_position(POS_FACEUP))
		return FALSE;
	if(!is_flag(EFFECT_FLAG_IGNORE_RANGE)) {
		if(pcard->is_treated_as_not_on_field())
			return FALSE;
		if(is_flag(EFFECT_FLAG_SPSUM_PARAM))
			return FALSE;
		if(is_flag(EFFECT_FLAG_ABSOLUTE_TARGET)) {
			if(pcard->current.controler == 0) {
				if(!pcard->current.is_location(s_range))
					return FALSE;
			} else {
				if(!pcard->current.is_location(o_range))
					return FALSE;
			}
		} else {
			if(pcard->current.controler == get_handler_player()) {
				if(!pcard->current.is_location(s_range))
					return FALSE;
			} else {
				if(!pcard->current.is_location(o_range))
					return FALSE;
			}
		}
	}
	return is_fit_target_function(pcard);
}
int32_t effect::is_fit_target_function(card* pcard) {
	if(target) {
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		if (!pduel->lua->check_condition(target, 2)) {
			return FALSE;
		}
	}
	return TRUE;
}
int32_t effect::is_target_player(uint8_t playerid) {
	if(!is_flag(EFFECT_FLAG_PLAYER_TARGET))
		return FALSE;
	uint8_t self = get_handler_player();
	if(is_flag(EFFECT_FLAG_ABSOLUTE_TARGET)) {
		if(s_range && playerid == 0)
			return TRUE;
		if(o_range && playerid == 1)
			return TRUE;
	} else {
		if(s_range && self == playerid)
			return TRUE;
		if(o_range && self != playerid)
			return TRUE;
	}
	return FALSE;
}
int32_t effect::is_player_effect_target(card* pcard) {
	if(target) {
		handler->pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
		handler->pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		if(!handler->pduel->lua->check_condition(target, 2)) {
			return FALSE;
		}
	}
	return TRUE;
}
int32_t effect::is_immuned(card* pcard) {
	const effect_set_v& effects = pcard->immune_effect;
	for (effect_set::size_type i = 0; i < effects.size(); ++i) {
		effect* peffect = effects[i];
		if(peffect->is_available() && peffect->value) {
			pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
			if(peffect->check_value_condition(2))
				return TRUE;
		}
	}
	return FALSE;
}
int32_t effect::is_chainable(uint8_t tp) {
	if(!(type & EFFECT_TYPE_ACTIONS))
		return FALSE;
	int32_t sp = get_speed();
	// Curse of Field is the exception
	if((type & EFFECT_TYPE_ACTIVATE) && (sp <= 1) && !is_flag(EFFECT_FLAG2_COF))
		return FALSE;
	if(pduel->game_field->core.current_chain.size()) {
		if(!is_flag(EFFECT_FLAG_FIELD_ONLY) && (type & EFFECT_TYPE_TRIGGER_O)
				&& (get_handler()->current.location == LOCATION_HAND)) {
			if(pduel->game_field->core.current_chain.rbegin()->triggering_effect->get_speed() > 2)
				return FALSE;
		} else if(sp < pduel->game_field->core.current_chain.rbegin()->triggering_effect->get_speed())
			return FALSE;
	}
	for(const auto& ch_lim : pduel->game_field->core.chain_limit) {
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(ch_lim.player, PARAM_TYPE_INT);
		pduel->lua->add_param(tp, PARAM_TYPE_INT);
		if(!pduel->lua->check_condition(ch_lim.function, 3))
			return FALSE;
	}
	for(const auto& ch_lim_p : pduel->game_field->core.chain_limit_p) {
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(ch_lim_p.player, PARAM_TYPE_INT);
		pduel->lua->add_param(tp, PARAM_TYPE_INT);
		if(!pduel->lua->check_condition(ch_lim_p.function, 3))
			return FALSE;
	}
	return TRUE;
}
int32_t effect::is_hand_trigger() const {
	return (range & LOCATION_HAND) && (type & EFFECT_TYPE_TRIGGER_O) && get_code_type() != CODE_PHASE;
}
int32_t effect::is_initial_single() const {
	return (type & EFFECT_TYPE_SINGLE) && is_flag(EFFECT_FLAG_SINGLE_RANGE) && is_flag(EFFECT_FLAG_INITIAL);
}
int32_t effect::is_monster_effect() const {
	if (range & (LOCATION_SZONE | LOCATION_FZONE | LOCATION_PZONE))
		return FALSE;
	return TRUE;
}
//return: this can be reset by reset_level or not
//RESET_DISABLE is valid only when owner == handler
int32_t effect::reset(uint32_t reset_level, uint32_t reset_type) {
	switch (reset_type) {
	case RESET_EVENT: {
		if(!(reset_flag & RESET_EVENT))
			return FALSE;
		if(owner != handler)
			reset_level &= ~RESET_DISABLE;
		if(reset_level & 0xffff0000 & reset_flag)
			return TRUE;
		return FALSE;
		break;
	}
	case RESET_CARD: {
		return owner && (owner->data.code == reset_level);
		break;
	}
	case RESET_PHASE: {
		if(!(reset_flag & RESET_PHASE))
			return FALSE;
		uint8_t pid = get_owner_player();
		uint8_t tp = handler->pduel->game_field->infos.turn_player;
		if((((reset_flag & RESET_SELF_TURN) && pid == tp) || ((reset_flag & RESET_OPPO_TURN) && pid != tp))
				&& (reset_level & 0x3ff & reset_flag))
			--reset_count;
		if(reset_count == 0)
			return TRUE;
		return FALSE;
		break;
	}
	case RESET_CODE: {
		return (code == reset_level) && (type & EFFECT_TYPE_SINGLE) && !(type & EFFECT_TYPE_ACTIONS);
		break;
	}
	case RESET_COPY: {
		return copy_id == reset_level;
		break;
	}
	}
	return FALSE;
}
void effect::dec_count(uint8_t playerid) {
	if(!is_flag(EFFECT_FLAG_COUNT_LIMIT))
		return;
	if(count_limit == 0)
		return;
	if(count_code == 0 || is_flag(EFFECT_FLAG_NO_TURN_RESET))
		count_limit -= 1;
	if(count_code) {
		uint32_t limit_code = count_code & MAX_CARD_ID;
		uint32_t limit_type = count_code & 0xf0000000;
		if(limit_code == EFFECT_COUNT_CODE_SINGLE)
			pduel->game_field->add_effect_code(limit_type | get_handler()->activate_count_id, PLAYER_NONE);
		else
			pduel->game_field->add_effect_code(count_code, playerid);
	}
}
void effect::recharge() {
	if(is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
		count_limit = count_limit_max;
	}
}
int32_t effect::get_value(uint32_t extraargs) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		int32_t res = pduel->lua->get_function_value(value, 1 + extraargs);
		return res;
	} else {
		pduel->lua->params.clear();
		return value;
	}
}
int32_t effect::get_value(card* pcard, uint32_t extraargs) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD, TRUE);
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		int32_t res = pduel->lua->get_function_value(value, 2 + extraargs);
		return res;
	} else {
		pduel->lua->params.clear();
		return value;
	}
}
int32_t effect::get_value(effect* peffect, uint32_t extraargs) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT, TRUE);
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		int32_t res = pduel->lua->get_function_value(value, 2 + extraargs);
		return res;
	} else {
		pduel->lua->params.clear();
		return value;
	}
}
void effect::get_value(uint32_t extraargs, std::vector<lua_Integer>& result) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		pduel->lua->get_function_value(value, 1 + extraargs, result);
	} else {
		pduel->lua->params.clear();
		result.push_back(value);
	}
}
void effect::get_value(card* pcard, uint32_t extraargs, std::vector<lua_Integer>& result) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD, TRUE);
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		pduel->lua->get_function_value(value, 2 + extraargs, result);
	} else {
		pduel->lua->params.clear();
		result.push_back(value);
	}
}
void effect::get_value(effect* peffect, uint32_t extraargs, std::vector<lua_Integer>& result) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT, TRUE);
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		pduel->lua->get_function_value(value, 2 + extraargs, result);
	} else {
		pduel->lua->params.clear();
		result.push_back(value);
	}
}
int32_t effect::get_integer_value() {
	return is_flag(EFFECT_FLAG_FUNC_VALUE) ? 0 : value;
}
int32_t effect::check_value_condition(uint32_t extraargs) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		int32_t res = pduel->lua->check_condition(value, 1 + extraargs);
		return res;
	} else {
		pduel->lua->params.clear();
		return value;
	}
}
void* effect::get_label_object() {
	return pduel->lua->get_ref_object(label_object);
}
int32_t effect::get_speed() {
	if(!(type & EFFECT_TYPE_ACTIONS))
		return 0;
	if(type & (EFFECT_TYPE_TRIGGER_O | EFFECT_TYPE_TRIGGER_F | EFFECT_TYPE_IGNITION))
		return 1;
	else if(type & (EFFECT_TYPE_QUICK_O | EFFECT_TYPE_QUICK_F))
		return 2;
	else if(type & EFFECT_TYPE_ACTIVATE) {
		if(handler->data.type & TYPE_MONSTER)
			return 0;
		else if(handler->data.type & TYPE_SPELL) {
			if(handler->data.type & TYPE_QUICKPLAY)
				return 2;
			return 1;
		} else {
			if (handler->data.type & TYPE_COUNTER)
				return 3;
			return 2;
		}
	}
	return 0;
}
effect* effect::clone() {
	effect* ceffect = pduel->new_effect();
	int32_t ref = ceffect->ref_handle;
	*ceffect = *this;
	ceffect->ref_handle = ref;
	ceffect->handler = 0;
	if(condition)
		ceffect->condition = pduel->lua->clone_function_ref(condition);
	if(cost)
		ceffect->cost = pduel->lua->clone_function_ref(cost);
	if(target)
		ceffect->target = pduel->lua->clone_function_ref(target);
	if(operation)
		ceffect->operation = pduel->lua->clone_function_ref(operation);
	if(value && is_flag(EFFECT_FLAG_FUNC_VALUE))
		ceffect->value = pduel->lua->clone_function_ref(value);
	return ceffect;
}
card* effect::get_owner() const {
	if(active_handler)
		return active_handler;
	if(type & EFFECT_TYPE_XMATERIAL)
		return handler->overlay_target;
	return owner;
}
uint8_t effect::get_owner_player() const {
	if(effect_owner != PLAYER_NONE)
		return effect_owner;
	return get_owner()->current.controler;
}
card* effect::get_handler() const {
	if(active_handler)
		return active_handler;
	if(type & EFFECT_TYPE_XMATERIAL)
		return handler->overlay_target;
	return handler;
}
uint8_t effect::get_handler_player() const {
	if(is_flag(EFFECT_FLAG_FIELD_ONLY))
		return effect_owner;
	return get_handler()->current.controler;
}
int32_t effect::in_range(card* pcard) const {
	if(type & EFFECT_TYPE_XMATERIAL)
		return handler->overlay_target ? TRUE : FALSE;
	return pcard->current.is_location(range);
}
int32_t effect::in_range(const chain& ch) const {
	if(type & EFFECT_TYPE_XMATERIAL)
		return handler->overlay_target ? TRUE : FALSE;
	return range & ch.triggering_location;
}
void effect::set_activate_location() {
	card* phandler = get_handler();
	active_location = phandler->current.location;
	active_sequence = phandler->current.sequence;
}
void effect::set_active_type() {
	card* phandler = get_handler();
	active_type = phandler->get_type();
	if(active_type & TYPE_TRAPMONSTER)
		active_type &= ~TYPE_TRAP;
}
uint32_t effect::get_active_type(uint8_t uselast) {
	if(type & EFFECT_TYPES_CHAIN_LINK) {
		if(active_type && uselast)
			return active_type;
		else if((type & EFFECT_TYPE_ACTIVATE) && (get_handler()->data.type & TYPE_PENDULUM))
			return TYPE_PENDULUM + TYPE_SPELL;
		else
			return get_handler()->get_type();
	} else
		return owner->get_type();
}
code_type effect::get_code_type() const {
	// start from the highest bit
	if (code & 0xf0000000)
		return CODE_CUSTOM;
	else if (code & 0xf0000)
		return CODE_COUNTER;
	else if (code & 0xf000)
		return CODE_PHASE;
	else
		return CODE_VALUE;
}
