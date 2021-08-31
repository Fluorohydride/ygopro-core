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
	return e1->id < e2->id;
};
effect::effect(duel* pd) {
	ref_handle = 0;
	pduel = pd;
	owner = 0;
	handler = 0;
	description = 0;
	effect_owner = PLAYER_NONE;
	card_type = 0;
	active_type = 0;
	active_location = 0;
	active_sequence = 0;
	active_handler = 0;
	id = 0;
	code = 0;
	type = 0;
	flag[0] = 0;
	flag[1] = 0;
	copy_id = 0;
	range = 0;
	s_range = 0;
	o_range = 0;
	count_limit = 0;
	count_limit_max = 0;
	reset_count = 0;
	reset_flag = 0;
	count_code = 0;
	category = 0;
	label.reserve(4);
	label_object = 0;
	hint_timing[0] = 0;
	hint_timing[1] = 0;
	status = 0;
	condition = 0;
	cost = 0;
	target = 0;
	value = 0;
	operation = 0;
}
int32 effect::is_disable_related() {
	if (code == EFFECT_IMMUNE_EFFECT || code == EFFECT_DISABLE || code == EFFECT_CANNOT_DISABLE || code == EFFECT_FORBIDDEN)
		return TRUE;
	return FALSE;
}
int32 effect::is_self_destroy_related() {
	if(code == EFFECT_UNIQUE_CHECK || code == EFFECT_SELF_DESTROY || code == EFFECT_SELF_TOGRAVE)
		return TRUE;
	return FALSE;
}
int32 effect::is_can_be_forbidden() {
	if (is_flag(EFFECT_FLAG_CANNOT_DISABLE) && !is_flag(EFFECT_FLAG_CANNOT_NEGATE))
		return FALSE;
	if (code == EFFECT_CHANGE_CODE)
		return FALSE;
	return TRUE;
}
// check if a single/field/equip effect is available
// check properties: range, EFFECT_FLAG_OWNER_RELATE, STATUS_BATTLE_DESTROYED, STATUS_EFFECT_ENABLED, disabled/forbidden
// check fucntions: condition
int32 effect::is_available() {
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
		if(is_flag(EFFECT_FLAG_OWNER_RELATE) && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && powner->is_status(STATUS_DISABLED))
			return FALSE;
		if(powner == phandler && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && phandler->get_status(STATUS_DISABLED))
			return FALSE;
	}
	if (type & EFFECT_TYPE_EQUIP) {
		if(handler->current.controler == PLAYER_NONE)
			return FALSE;
		if(is_flag(EFFECT_FLAG_OWNER_RELATE) && is_can_be_forbidden() && owner->is_status(STATUS_FORBIDDEN))
			return FALSE;
		if(owner == handler && is_can_be_forbidden() && handler->get_status(STATUS_FORBIDDEN))
			return FALSE;
		if(is_flag(EFFECT_FLAG_OWNER_RELATE) && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && owner->is_status(STATUS_DISABLED))
			return FALSE;
		if(owner == handler && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && handler->get_status(STATUS_DISABLED))
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
			if((phandler->current.location & LOCATION_ONFIELD) && !phandler->is_position(POS_FACEUP))
				return FALSE;
			if(is_flag(EFFECT_FLAG_OWNER_RELATE) && is_can_be_forbidden() && powner->is_status(STATUS_FORBIDDEN))
				return FALSE;
			if(powner == phandler && is_can_be_forbidden() && phandler->get_status(STATUS_FORBIDDEN))
				return FALSE;
			if(is_flag(EFFECT_FLAG_OWNER_RELATE) && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && powner->is_status(STATUS_DISABLED))
				return FALSE;
			if(powner == phandler && !is_flag(EFFECT_FLAG_CANNOT_DISABLE) && phandler->get_status(STATUS_DISABLED))
				return FALSE;
			if(phandler->is_status(STATUS_BATTLE_DESTROYED))
				return FALSE;
		}
	}
	if (!condition)
		return TRUE;
	pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
	int32 res = pduel->lua->check_condition(condition, 1);
	if(res) {
		if(!(status & EFFECT_STATUS_AVAILABLE))
			id = pduel->game_field->infos.field_id++;
		status |= EFFECT_STATUS_AVAILABLE;
	} else
		status &= ~EFFECT_STATUS_AVAILABLE;
	return res;
}
// check if a effect is EFFECT_TYPE_SINGLE and is ready
// check: range, enabled, condition
int32 effect::is_single_ready() {
	if(type & EFFECT_TYPE_ACTIONS)
		return FALSE;
	if((type & (EFFECT_TYPE_SINGLE | EFFECT_TYPE_XMATERIAL)) && !(type & EFFECT_TYPE_FIELD)) {
		card* phandler = get_handler();
		card* powner = get_owner();
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
	int32 res = pduel->lua->check_condition(condition, 1);
	return res;
}
// reset_count: count of effect reset
// count_limit: left count of activation
// count_limit_max: max count of activation
int32 effect::check_count_limit(uint8 playerid) {
	if(is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
		if(count_limit == 0)
			return FALSE;
		if(count_code) {
			uint32 code = count_code & 0xfffffff;
			uint32 count = count_limit_max;
			if(code == 1) {
				if(pduel->game_field->get_effect_code((count_code & 0xf0000000) | get_handler()->fieldid, PLAYER_NONE) >= count)
					return FALSE;
			} else {
				if(pduel->game_field->get_effect_code(count_code, playerid) >= count)
					return FALSE;
			}
		}
	}
	return TRUE;
}
// check if an EFFECT_TYPE_ACTIONS effect can be activated
// for triggering effects, it checks EFFECT_FLAG_DAMAGE_STEP, EFFECT_FLAG_SET_AVAILABLE
int32 effect::is_activateable(uint8 playerid, const tevent& e, int32 neglect_cond, int32 neglect_cost, int32 neglect_target, int32 neglect_loc, int32 neglect_faceup) {
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
			uint32 zone = 0xff;
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
			int32 ecode = 0;
			if(handler->current.location == LOCATION_HAND && !neglect_loc) {
				if(handler->data.type & TYPE_TRAP)
					ecode = EFFECT_TRAP_ACT_IN_HAND;
				else if((handler->data.type & TYPE_SPELL) && pduel->game_field->infos.turn_player != playerid) {
					if(handler->data.type & TYPE_QUICKPLAY)
						ecode = EFFECT_QP_ACT_IN_NTPHAND;
					else
						return FALSE;
				}
			} else if(handler->current.location == LOCATION_SZONE) {
				if((handler->data.type & TYPE_TRAP) && handler->get_status(STATUS_SET_TURN))
					ecode = EFFECT_TRAP_ACT_IN_SET_TURN;
				if((handler->data.type & TYPE_SPELL) && (handler->data.type & TYPE_QUICKPLAY) && handler->get_status(STATUS_SET_TURN))
					ecode = EFFECT_QP_ACT_IN_SET_TURN;
			}
			if(ecode) {
				bool available = false;
				effect_set eset;
				handler->filter_effect(ecode, &eset);
				for(int32 i = 0; i < eset.size(); ++i) {
					if(eset[i]->check_count_limit(playerid)) {
						available = true;
						break;
					}
				}
				if(!available)
					return FALSE;
			}
			if(handler->is_status(STATUS_FORBIDDEN))
				return FALSE;
			if(handler->is_affected_by_effect(EFFECT_CANNOT_TRIGGER))
				return FALSE;
		} else if(!(type & EFFECT_TYPE_CONTINUOUS)) {
			card* phandler = get_handler();
			if(!(phandler->get_type() & TYPE_MONSTER) && (get_active_type() & TYPE_MONSTER))
				return FALSE;
			if((phandler->get_type() & TYPE_CONTINUOUS) && (phandler->get_type() & TYPE_EQUIP))
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
				|| pduel->game_field->core.duel_rule >= 5 && phandler->current.location == LOCATION_EXTRA && (phandler->current.position & POS_FACEDOWN)) {
				if((type & EFFECT_TYPE_SINGLE) && code != EVENT_TO_DECK)
					return FALSE;
				if((type & EFFECT_TYPE_FIELD) && !(range & (LOCATION_DECK | LOCATION_EXTRA)))
					return FALSE;
			}
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
			if(((type & EFFECT_TYPE_FIELD) || ((type & EFFECT_TYPE_SINGLE) && is_flag(EFFECT_FLAG_SINGLE_RANGE))) && (phandler->current.location & LOCATION_ONFIELD)
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
	uint8 op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = this;
	pduel->game_field->core.reason_player = playerid;
	int32 result = TRUE;
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
int32 effect::is_action_check(uint8 playerid) {
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, EFFECT_CANNOT_ACTIVATE, &eset);
	for(int32 i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(eset[i]->check_value_condition(2))
			return FALSE;
	}
	eset.clear();
	pduel->game_field->filter_player_effect(playerid, EFFECT_ACTIVATE_COST, &eset);
	for(int32 i = 0; i < eset.size(); ++i) {
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
int32 effect::is_activate_ready(effect* reason_effect, uint8 playerid, const tevent& e, int32 neglect_cond, int32 neglect_cost, int32 neglect_target) {
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
	if(!neglect_cost && cost && !(type & EFFECT_TYPE_CONTINUOUS)) {
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
			return FALSE;
		}
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
			return FALSE;
		}
	}
	return TRUE;
}
int32 effect::is_activate_ready(uint8 playerid, const tevent& e, int32 neglect_cond, int32 neglect_cost, int32 neglect_target) {
	return is_activate_ready(this, playerid, e, neglect_cond, neglect_cost, neglect_target);
}
// check functions: condition
int32 effect::is_condition_check(uint8 playerid, const tevent& e) {
	card* phandler = get_handler();
	if(!(type & EFFECT_TYPE_ACTIVATE) && (phandler->current.location & (LOCATION_ONFIELD | LOCATION_REMOVED)) && !phandler->is_position(POS_FACEUP))
		return FALSE;
	if(!condition)
		return TRUE;
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8 op = pduel->game_field->core.reason_player;
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
int32 effect::is_activate_check(uint8 playerid, const tevent& e, int32 neglect_cond, int32 neglect_cost, int32 neglect_target) {
	pduel->game_field->save_lp_cost();
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8 op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = this;
	pduel->game_field->core.reason_player = playerid;
	int32 result = is_activate_ready(playerid, e, neglect_cond, neglect_cost, neglect_target);
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	pduel->game_field->restore_lp_cost();
	return result;
}
int32 effect::is_target(card* pcard) {
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
		if(pcard->get_status(STATUS_SUMMONING | STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED | STATUS_SPSUMMON_STEP))
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
int32 effect::is_fit_target_function(card* pcard) {
	if(target) {
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		if (!pduel->lua->check_condition(target, 2)) {
			return FALSE;
		}
	}
	return TRUE;
}
int32 effect::is_target_player(uint8 playerid) {
	if(!is_flag(EFFECT_FLAG_PLAYER_TARGET))
		return FALSE;
	uint8 self = get_handler_player();
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
int32 effect::is_player_effect_target(card* pcard) {
	if(target) {
		handler->pduel->lua->add_param(this, PARAM_TYPE_EFFECT);
		handler->pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		if(!handler->pduel->lua->check_condition(target, 2)) {
			return FALSE;
		}
	}
	return TRUE;
}
int32 effect::is_immuned(card* pcard) {
	const effect_set_v& effects = pcard->immune_effect;
	for (int32 i = 0; i < effects.size(); ++i) {
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
int32 effect::is_chainable(uint8 tp) {
	if(!(type & EFFECT_TYPE_ACTIONS))
		return FALSE;
	int32 sp = get_speed();
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
//return: this can be reset by reset_level or not
//RESET_DISABLE is valid only when owner == handler
int32 effect::reset(uint32 reset_level, uint32 reset_type) {
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
		uint8 pid = get_owner_player();
		uint8 tp = handler->pduel->game_field->infos.turn_player;
		if((((reset_flag & RESET_SELF_TURN) && pid == tp) || ((reset_flag & RESET_OPPO_TURN) && pid != tp)) 
				&& (reset_level & 0x3ff & reset_flag))
			reset_count--;
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
void effect::dec_count(uint32 playerid) {
	if(!is_flag(EFFECT_FLAG_COUNT_LIMIT))
		return;
	if(count_limit == 0)
		return;
	if(count_code == 0 || is_flag(EFFECT_FLAG_NO_TURN_RESET))
		count_limit -= 1;
	if(count_code) {
		uint32 code = count_code & 0xfffffff;
		if(code == 1)
			pduel->game_field->add_effect_code((count_code & 0xf0000000) | get_handler()->fieldid, PLAYER_NONE);
		else
			pduel->game_field->add_effect_code(count_code, playerid);
	}
}
void effect::recharge() {
	if(is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
		count_limit = count_limit_max;
	}
}
int32 effect::get_value(uint32 extraargs) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		int32 res = pduel->lua->get_function_value(value, 1 + extraargs);
		return res;
	} else {
		pduel->lua->params.clear();
		return (int32)value;
	}
}
int32 effect::get_value(card* pcard, uint32 extraargs) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD, TRUE);
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		int32 res = pduel->lua->get_function_value(value, 2 + extraargs);
		return res;
	} else {
		pduel->lua->params.clear();
		return (int32)value;
	}
}
int32 effect::get_value(effect* peffect, uint32 extraargs) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT, TRUE);
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		int32 res = pduel->lua->get_function_value(value, 2 + extraargs);
		return res;
	} else {
		pduel->lua->params.clear();
		return (int32)value;
	}
}
void effect::get_value(uint32 extraargs, std::vector<int32>* result) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		pduel->lua->get_function_value(value, 1 + extraargs, result);
	} else {
		pduel->lua->params.clear();
		result->push_back((int32)value);
	}
}
void effect::get_value(card* pcard, uint32 extraargs, std::vector<int32>* result) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD, TRUE);
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		pduel->lua->get_function_value(value, 2 + extraargs, result);
	} else {
		pduel->lua->params.clear();
		result->push_back((int32)value);
	}
}
void effect::get_value(effect* peffect, uint32 extraargs, std::vector<int32>* result) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT, TRUE);
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		pduel->lua->get_function_value(value, 2 + extraargs, result);
	} else {
		pduel->lua->params.clear();
		result->push_back((int32)value);
	}
}
int32 effect::check_value_condition(uint32 extraargs) {
	if(is_flag(EFFECT_FLAG_FUNC_VALUE)) {
		pduel->lua->add_param(this, PARAM_TYPE_EFFECT, TRUE);
		int32 res = pduel->lua->check_condition(value, 1 + extraargs);
		return res;
	} else {
		pduel->lua->params.clear();
		return (int32)value;
	}
}
void* effect::get_label_object() {
	return pduel->lua->get_ref_object(label_object);
}
int32 effect::get_speed() {
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
	int32 ref = ceffect->ref_handle;
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
uint8 effect::get_owner_player() {
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
uint8 effect::get_handler_player() {
	if(is_flag(EFFECT_FLAG_FIELD_ONLY))
		return effect_owner;
	return get_handler()->current.controler;
}
int32 effect::in_range(card* pcard) {
	if(type & EFFECT_TYPE_XMATERIAL)
		return handler->overlay_target ? TRUE : FALSE;
	return pcard->current.is_location(range);
}
int32 effect::in_range(const chain& ch) {
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
uint32 effect::get_active_type() {
	if(type & 0x7f0) {
		if(active_type)
			return active_type;
		else if((type & EFFECT_TYPE_ACTIVATE) && (get_handler()->data.type & TYPE_PENDULUM))
			return TYPE_PENDULUM + TYPE_SPELL;
		else
			return get_handler()->get_type();
	} else
		return owner->get_type();
}
