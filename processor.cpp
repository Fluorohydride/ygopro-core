/*
 * processor.cpp
 *
 *  Created on: 2010-8-3
 *      Author: Argon
 */

#include "field.h"
#include "duel.h"
#include "card.h"
#include "group.h"
#include "effect.h"
#include "interpreter.h"
#include "ocgapi.h"
#include <iterator>

void field::add_process(uint16 type, uint16 step, effect* peffect, group* target, ptr arg1, ptr arg2, ptr arg3, ptr arg4, void* ptr1, void* ptr2) {
	processor_unit new_unit;
	new_unit.type = type;
	new_unit.step = step;
	new_unit.peffect = peffect;
	new_unit.ptarget = target;
	new_unit.arg1 = arg1;
	new_unit.arg2 = arg2;
	new_unit.arg3 = arg3;
	new_unit.arg4 = arg4;
	new_unit.ptr1 = ptr1;
	new_unit.ptr2 = ptr2;
	core.subunits.push_back(new_unit);
}
int32 field::process() {
	if (core.subunits.size())
		core.units.splice(core.units.begin(), core.subunits);
	if (core.units.size() == 0)
		return PROCESSOR_END + pduel->bufferlen;
	auto it = core.units.begin();
	switch (it->type) {
	case PROCESSOR_ADJUST: {
		if (adjust_step(it->step))
			core.units.pop_front();
		else {
			it->step++;
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_TURN: {
		if (process_turn(it->step, it->arg1))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_WAIT: {
		core.units.pop_front();
		return PROCESSOR_WAITING + pduel->bufferlen;
	}
	case PROCESSOR_REFRESH_LOC: {
		if (refresh_location_info(it->step))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_BATTLECMD: {
		if (select_battle_command(it->step, it->arg1)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SELECT_IDLECMD: {
		if (select_idle_command(it->step, it->arg1)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SELECT_EFFECTYN: {
		if (select_effect_yes_no(it->step, it->arg1, it->arg2, (card*)it->ptarget)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SELECT_YESNO: {
		if (select_yes_no(it->step, it->arg1, it->arg2)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SELECT_OPTION: {
		if (select_option(it->step, it->arg1)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SELECT_CARD: {
		if (select_card(it->step, it->arg1 & 0xff, (it->arg1 >> 16) & 0xff, (it->arg2) & 0xff, (it->arg2 >> 16) & 0xff)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SELECT_UNSELECT_CARD: {
		if (select_unselect_card(it->step, it->arg1 & 0xff, (it->arg1 >> 16) & 0xff, (it->arg2) & 0xff, (it->arg2 >> 16) & 0xff, (it->arg3) & 0xff)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SELECT_CHAIN: {
		if (select_chain(it->step, it->arg1, (it->arg2 & 0xffff), it->arg2 >> 16)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SELECT_DISFIELD:
	case PROCESSOR_SELECT_PLACE: {
		if (select_place(it->step, it->arg1, it->arg2, it->arg3)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SELECT_POSITION: {
		if (select_position(it->step, it->arg1 & 0xffff, it->arg2, (it->arg1 >> 16) & 0xffff)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SELECT_TRIBUTE_P: {
		if (select_tribute(it->step, it->arg1 & 0xff, (it->arg1 >> 16) & 0xff, (it->arg2) & 0xff, (it->arg2 >> 16) & 0xff)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SORT_CHAIN: {
		if (sort_chain(it->step, it->arg1)) {
			core.units.pop_front();
		} else {
			it->step++;
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_COUNTER: {
		if (select_counter(it->step, it->arg1, it->arg2, it->arg3, it->arg4 >> 8, it->arg4 & 0xff)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SELECT_SUM: {
		if (select_with_sum_limit(it->step, it->arg2 & 0xffff, it->arg1, (it->arg2 >> 16) & 0xff, (it->arg2 >> 24) & 0xff)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SORT_CARD: {
		if (sort_card(it->step, it->arg1, it->arg2)) {
			core.units.pop_front();
			return pduel->bufferlen;
		} else {
			it->step = 1;
			return PROCESSOR_WAITING + pduel->bufferlen;
		}
	}
	case PROCESSOR_SELECT_RELEASE: {
		if (select_release_cards(it->step, it->arg1 & 0xff, (it->arg1 >> 16) & 0xff, (it->arg2) & 0xff, (it->arg2 >> 16) & 0xff))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_TRIBUTE: {
		if (select_tribute_cards(it->step, (card*)it->ptarget, it->arg1 & 0xff, (it->arg1 >> 16) & 0xff, (it->arg2) & 0xff, (it->arg2 >> 16) & 0xff, it->arg3, it->arg4))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_POINT_EVENT: {
		if(process_point_event(it->step, it->arg1 & 0xff, (it->arg1 >> 8) & 0xff, it->arg2))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_QUICK_EFFECT: {
		if(process_quick_effect(it->step, it->arg1, it->arg2))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_IDLE_COMMAND: {
		if(process_idle_command(it->step))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_PHASE_EVENT: {
		if(process_phase_event(it->step, it->arg1))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_BATTLE_COMMAND: {
		if(process_battle_command(it->step))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_DAMAGE_STEP: {
		if(process_damage_step(it->step, it->arg2))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_ADD_CHAIN: {
		if (add_chain(it->step))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SOLVE_CHAIN: {
		if (solve_chain(it->step, it->arg1, it->arg2))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SOLVE_CONTINUOUS: {
		if (solve_continuous(it->step, it->peffect, it->arg1))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_EXECUTE_COST: {
		if (execute_cost(it->step, it->peffect, it->arg1)) {
			core.units.pop_front();
			core.solving_event.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_EXECUTE_OPERATION: {
		if (execute_operation(it->step, it->peffect, it->arg1)) {
			core.units.pop_front();
			core.solving_event.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_EXECUTE_TARGET: {
		if (execute_target(it->step, it->peffect, it->arg1)) {
			core.units.pop_front();
			core.solving_event.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_DESTROY: {
		if (destroy(it->step, it->ptarget, it->peffect, it->arg1, it->arg2))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_RELEASE: {
		if (release(it->step, it->ptarget, it->peffect, it->arg1, it->arg2))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SENDTO: {
		if (send_to(it->step, it->ptarget, it->peffect, it->arg1, it->arg2))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_DESTROY_STEP: {
		if(destroy(it->step, it->ptarget, (card*)it->ptr1, it->arg2))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_RELEASE_STEP: {
		if (release(it->step, it->ptarget, (card*)it->ptr1))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SENDTO_STEP: {
		if (send_to(it->step, it->ptarget, (card*)it->ptr1))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_MOVETOFIELD: {
		if (move_to_field(it->step, (card*)it->ptarget, it->arg1, it->arg2 & 0xff, (it->arg2 >> 8) & 0xff, it->arg3))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_CHANGEPOS: {
		if (change_position(it->step, it->ptarget, it->peffect, it->arg1, it->arg2))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_OPERATION_REPLACE: {
		if (operation_replace(it->step, it->peffect, it->ptarget, (card*)it->ptr1, it->arg1))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_ACTIVATE_EFFECT: {
		if (activate_effect(it->step, it->peffect))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SUMMON_RULE: {
		if (summon(it->step, it->arg1 & 0xff, (card*)it->ptarget, it->peffect, (it->arg1 >> 8) & 0xff, (it->arg1 >> 16) & 0xff, (it->arg1 >> 24) & 0xff))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SPSUMMON_RULE: {
		if (special_summon_rule(it->step, it->arg1, (card*)it->ptarget, it->arg2))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SPSUMMON: {
		if (special_summon(it->step, it->peffect, it->arg1, it->ptarget, it->arg2))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_FLIP_SUMMON: {
		if (flip_summon(it->step, it->arg1, (card*)(it->ptarget)))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_MSET: {
		if (mset(it->step, it->arg1 & 0xff, (card*)it->ptarget, it->peffect, (it->arg1 >> 8) & 0xff, (it->arg1 >> 16) & 0xff, (it->arg1 >> 24) & 0xff))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SSET: {
		if (sset(it->step, it->arg1, it->arg2, (card*)(it->ptarget)))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SPSUMMON_STEP: {
		if (special_summon_step(it->step, it->ptarget, (card*)(it->ptr1), it->arg1))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SSET_G: {
		if (sset_g(it->step, it->arg1, it->arg2, it->ptarget)) {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_DRAW	: {
		if (draw(it->step, it->peffect, it->arg1, (it->arg2 >> 28) & 0xf, (it->arg2 >> 24) & 0xf, it->arg2 & 0xffffff))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_DAMAGE: {
		int32 reason = it->arg1;
		effect* reason_effect = 0;
		card* reason_card = 0;
		if(reason & REASON_BATTLE)
			reason_card = (card*)it->peffect;
		else
			reason_effect = it->peffect;
		if (damage(it->step, reason_effect, reason, (it->arg2 >> 26) & 0x3, reason_card, (it->arg2 >> 24) & 0x3, it->arg2 & 0xffffff, (it->arg2 >> 28) & 0x1)) {
			if(it->step == 9) {
				it->step = 1;
				core.recover_damage_reserve.splice(core.recover_damage_reserve.end(), core.units, it);
			} else
				core.units.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_RECOVER: {
		if (recover(it->step, it->peffect, it->arg1, (it->arg2 >> 26) & 0x3, (it->arg2 >> 24) & 0x3, it->arg2 & 0xffffff, (it->arg2 >> 28) & 0x1)) {
			if(it->step == 9) {
				it->step = 1;
				core.recover_damage_reserve.splice(core.recover_damage_reserve.end(), core.units, it);
			} else
				core.units.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_EQUIP: {
		if (equip(it->step, it->arg2 & 0xffff, (card*)it->ptr1, (card*)it->ptarget, (it->arg2 >> 16) & 0xff, (it->arg2 >> 24) & 0xff))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_GET_CONTROL: {
		if (get_control(it->step, it->peffect, (it->arg2 >> 28) & 0xf, it->ptarget, (it->arg2 >> 24) & 0xf, (it->arg2 >> 8) & 0x3ff, it->arg2 & 0xff, it->arg3)) {
			core.units.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SWAP_CONTROL: {
		if (swap_control(it->step, it->peffect, it->arg1, it->ptarget, (group*)it->ptr1, it->arg2, it->arg3)) {
			core.units.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_CONTROL_ADJUST: {
		if (control_adjust(it->step)) {
			core.units.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SELF_DESTROY: {
		if (self_destroy(it->step, (card*)it->ptr1, it->arg1)) {
			core.units.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_PAY_LPCOST: {
		if (pay_lp_cost(it->step, it->arg1, it->arg2))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_REMOVE_COUNTER: {
		if (remove_counter(it->step, it->arg4, (card*)it->ptarget, (it->arg1 >> 16) & 0xff, (it->arg1 >> 8) & 0xff, it->arg1 & 0xff, it->arg2, it->arg3)) {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_BOOLEAN);
			core.units.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_ATTACK_DISABLE: {
		if(it->step == 0) {
			card* attacker = core.attacker;
			if(!attacker
			        || (attacker->fieldid_r != core.pre_field[0])
			        || (attacker->current.location != LOCATION_MZONE)
			        || !attacker->is_capable_attack()
			        || !attacker->is_affect_by_effect(core.reason_effect)) {
				returns.ivalue[0] = 0;
				pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_BOOLEAN);
				core.units.pop_front();
			} else {
				effect* peffect = pduel->new_effect();
				peffect->code = EFFECT_ATTACK_DISABLED;
				peffect->type = EFFECT_TYPE_SINGLE;
				attacker->add_effect(peffect);
				attacker->set_status(STATUS_ATTACK_CANCELED, TRUE);
				raise_event(attacker, EVENT_ATTACK_DISABLED, core.reason_effect, 0, core.reason_player, PLAYER_NONE, 0);
				process_instant_event();
				it->step++;
			}
		} else {
			returns.ivalue[0] = 1;
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_BOOLEAN);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_DESTROY_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_DESTROY, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_RELEASE_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_RELEASE, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SENDTO_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SENDTO, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_CHANGEPOS_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_CHANGEPOS, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_ANNOUNCE_RACE: {
		if(announce_race(it->step, it->arg1 & 0xffff, it->arg1 >> 16, it->arg2)) {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		} else {
			it->step++;
		}
		return PROCESSOR_WAITING + pduel->bufferlen;
	}
	case PROCESSOR_ANNOUNCE_ATTRIB: {
		if(announce_attribute(it->step, it->arg1 & 0xffff, it->arg1 >> 16, it->arg2)) {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		} else {
			it->step++;
		}
		return PROCESSOR_WAITING + pduel->bufferlen;
	}
	case PROCESSOR_ANNOUNCE_CARD: {
		if(announce_card(it->step, it->arg1, it->arg2)) {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		} else {
			it->step++;
		}
		return PROCESSOR_WAITING + pduel->bufferlen;
	}
	case PROCESSOR_ANNOUNCE_NUMBER: {
		if(announce_number(it->step, it->arg1)) {
			pduel->lua->add_param(core.select_options[returns.ivalue[0]], PARAM_TYPE_INT);
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		} else {
			it->step++;
		}
		return PROCESSOR_WAITING + pduel->bufferlen;
	}
	case PROCESSOR_TOSS_DICE: {
		if(toss_dice(it->step, it->peffect, it->arg1 >> 16, it->arg1 & 0xff, it->arg2 & 0xff, it->arg2 >> 16)) {
			for(int32 i = 0; i < (it->arg2 & 0xff) + (it->arg2 >> 16); ++i)
				pduel->lua->add_param(core.dice_result[i], PARAM_TYPE_INT);
			core.units.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_TOSS_COIN: {
		if (toss_coin(it->step, it->peffect, (it->arg1 >> 16), it->arg1 & 0xff, it->arg2)) {
			for(int32 i = 0; i < it->arg2; ++i)
				pduel->lua->add_param(core.coin_result[i], PARAM_TYPE_INT);
			core.units.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_ROCK_PAPER_SCISSORS: {
		if (rock_paper_scissors(it->step, it->arg1)) {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_EFFECTYN_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SELECT_EFFECTYN, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_BOOLEAN);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_YESNO_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SELECT_YESNO, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_BOOLEAN);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_OPTION_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SELECT_OPTION, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_OPSELECTED);
			pduel->write_buffer8(it->arg1);
			pduel->write_buffer32(core.select_options[returns.ivalue[0]]);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_CARD_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SELECT_CARD, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			group* pgroup = pduel->new_group();
			card* pcard;
			for(int32 i = 0; i < returns.bvalue[0]; ++i) {
				pcard = core.select_cards[returns.bvalue[i + 1]];
				pgroup->container.insert(pcard);
			}
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_UNSELECT_CARD_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SELECT_UNSELECT_CARD, 0, it->peffect, it->ptarget, it->arg1, it->arg2, it->arg3);
			it->step++;
		} else {
			if(returns.bvalue[0] == -1)
				pduel->lua->add_param((void*)0, PARAM_TYPE_GROUP);
			else {
				card* pcard;
				if(returns.bvalue[1] < core.select_cards.size())
					pcard = core.select_cards[returns.bvalue[1]];
				else
					pcard = core.unselect_cards[returns.bvalue[1] - core.select_cards.size()];
				pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
			}
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_POSITION_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SELECT_POSITION, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_RELEASE_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SELECT_RELEASE, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			group* pgroup = pduel->new_group();
			card* pcard;
			for(int32 i = 0; i < returns.bvalue[0]; ++i) {
				pcard = core.select_cards[returns.bvalue[i + 1]];
				pgroup->container.insert(pcard);
			}
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_TRIBUTE_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SELECT_TRIBUTE, 0, it->peffect, it->ptarget, it->arg1, it->arg2, it->arg3, it->arg4);
			it->step++;
		} else {
			group* pgroup = pduel->new_group();
			card* pcard;
			for(int32 i = 0; i < returns.bvalue[0]; ++i) {
				pcard = core.select_cards[returns.bvalue[i + 1]];
				pgroup->container.insert(pcard);
			}
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SORT_CARDS_S:
		core.units.pop_front();
		break;
	case PROCESSOR_SELECT_TARGET: {
		if(it->step == 0) {
			add_process(PROCESSOR_SELECT_CARD, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			chain* ch = get_chain(0);
			if(ch) {
				if(!ch->target_cards) {
					ch->target_cards = pduel->new_group();
					ch->target_cards->is_readonly = TRUE;
				}
				group* tg = ch->target_cards;
				effect* peffect = ch->triggering_effect;
				if(peffect->type & EFFECT_TYPE_CONTINUOUS) {
					for(int32 i = 0; i < returns.bvalue[0]; ++i)
						tg->container.insert(core.select_cards[returns.bvalue[i + 1]]);
					pduel->lua->add_param(tg, PARAM_TYPE_GROUP);
				} else {
					group* pret = pduel->new_group();
					for(int32 i = 0; i < returns.bvalue[0]; ++i) {
						tg->container.insert(core.select_cards[returns.bvalue[i + 1]]);
						pret->container.insert(core.select_cards[returns.bvalue[i + 1]]);
					}
					if((returns.bvalue[0] > 0) && peffect->is_flag(EFFECT_FLAG_CARD_TARGET)) {
						for(int32 i = 0; i < returns.bvalue[0]; ++i) {
							card* pcard = core.select_cards[returns.bvalue[i + 1]];
							if(pcard->current.location & 0x30)
								move_card(pcard->current.controler, pcard, pcard->current.location, 0);
							pduel->write_buffer8(MSG_BECOME_TARGET);
							pduel->write_buffer8(1);
							pduel->write_buffer32(pcard->get_info_location());
						}
					}
					for(auto cit = pret->container.begin(); cit != pret->container.end(); ++cit)
						(*cit)->create_relation(*ch);
					pduel->lua->add_param(pret, PARAM_TYPE_GROUP);
				}
			}
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_FUSION: {
		if(it->step == 0) {
			tevent e;
			e.event_cards = it->ptarget;
			e.reason_effect = it->peffect;
			e.reason_player = it->arg1;
			core.fusion_materials.clear();
			if(!it->peffect) {
				core.units.pop_front();
				return pduel->bufferlen;
			}
			core.sub_solving_event.push_back(e);
			pduel->lua->add_param(it->ptr1, PARAM_TYPE_CARD);
			pduel->lua->add_param(it->arg1 >> 16, PARAM_TYPE_INT);
			add_process(PROCESSOR_EXECUTE_OPERATION, 0, it->peffect, 0, it->arg1 & 0xffff, 0);
			it->step++;
		} else {
			group* pgroup = pduel->new_group(core.fusion_materials);
			if(it->ptr1)
				pgroup->container.insert((card*)it->ptr1);
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_SYNCHRO: {
		int32 ret = TRUE;
		if(!(it->arg1 >> 16))
			ret = select_synchro_material(it->step, it->arg1 & 0xffff,  (card*)it->ptarget, it->arg2 & 0xffff, it->arg2 >> 16, 0, (group*)it->peffect);
		else
			ret = select_synchro_material(it->step, it->arg1 & 0xffff,  (card*)it->ptarget, it->arg2 & 0xffff, it->arg2 >> 16, (card*)it->peffect, 0);
		if(ret)
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_SUM_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SELECT_SUM, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			group* pgroup = pduel->new_group();
			int32 mcount = core.must_select_cards.size();
			for(int32 i = mcount; i < returns.bvalue[0]; ++i) {
				card* pcard = core.select_cards[returns.bvalue[i + 1]];
				pgroup->container.insert(pcard);
			}
			core.must_select_cards.clear();
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_DISFIELD_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SELECT_DISFIELD, 0, it->peffect, it->ptarget, it->arg1, it->arg2, it->arg3);
			it->step++;
		} else {
			int32 playerid = it->arg1;
			int32 count = it->arg3;
			int32 dfflag = 0;
			uint8 p, l, s, pa = 0;
			for(int32 i = 0; i < count; ++i) {
				p = returns.bvalue[pa];
				l = returns.bvalue[pa + 1];
				s = returns.bvalue[pa + 2];
				dfflag |= 0x1u << (s + (p == playerid ? 0 : 16) + (l == LOCATION_MZONE ? 0 : 8));
				pa += 3;
			}
			pduel->lua->add_param(dfflag, PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SPSUMMON_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SPSUMMON, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SPSUMMON_STEP_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SPSUMMON_STEP, 0, it->peffect, it->ptarget, it->arg1, it->arg2, it->arg3, it->arg4, it->ptr1);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_BOOLEAN);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SPSUMMON_COMP_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SPSUMMON, 1, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_RANDOM_SELECT_S: {
		uint32 count = it->arg2;
		group* pgroup = it->ptarget;
		group* newgroup = pduel->new_group();
		if(count > pgroup->container.size())
			count = pgroup->container.size();
		if(count == 0) {
			pduel->lua->add_param(newgroup, PARAM_TYPE_GROUP);
			core.units.pop_front();
			return pduel->bufferlen;
		}
		duel* pduel = pgroup->pduel;
		if(count == pgroup->container.size())
			newgroup->container = pgroup->container;
		else {
			while(newgroup->container.size() < count) {
				int32 i = pduel->get_next_integer(0, pgroup->container.size() - 1);
				auto cit = pgroup->container.begin();
				std::advance(cit, i);
				newgroup->container.insert(*cit);
			}
		}
		pduel->lua->add_param(newgroup, PARAM_TYPE_GROUP);
		pduel->write_buffer8(MSG_RANDOM_SELECTED);
		pduel->write_buffer8(it->arg1);
		pduel->write_buffer8(count);
		for(auto cit = newgroup->container.begin(); cit != newgroup->container.end(); ++cit) {
			pduel->write_buffer32((*cit)->get_info_location());
		}
		core.units.pop_front();
		return pduel->bufferlen;
	}
	case PROCESSOR_SELECT_XMATERIAL: {
		if (select_xyz_material(it->step, it->arg1 & 0xffff, it->arg1 >> 16, (card*)it->ptarget, it->arg2 & 0xffff, it->arg2 >> 16))
			core.units.pop_front();
		else
			it->step++;
		return pduel->bufferlen;
	}
	case PROCESSOR_DRAW_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_DRAW, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_DAMAGE_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_DAMAGE, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_RECOVER_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_RECOVER, 0, it->peffect, it->ptarget, it->arg1, it->arg2);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_EQUIP_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_EQUIP, 0, it->peffect, it->ptarget, it->arg1, it->arg2, it->arg3, it->arg4, it->ptr1);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_BOOLEAN);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_GET_CONTROL_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_GET_CONTROL, 0, it->peffect, it->ptarget, it->arg1, it->arg2, it->arg3);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SWAP_CONTROL_S: {
		if(it->step == 0) {
			add_process(PROCESSOR_SWAP_CONTROL, 0, it->peffect, it->ptarget, it->arg1, it->arg2, it->arg3, it->arg4, it->ptr1);
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_BOOLEAN);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_DISCARD_HAND_S: {
		if(it->step == 0) {
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_SELECTMSG);
			pduel->write_buffer8(it->arg1);
			if(it->arg3 & REASON_DISCARD)
				pduel->write_buffer32(501);
			else
				pduel->write_buffer32(504);
			add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, it->arg1, it->arg2);
			it->step++;
		} else if(it->step == 1) {
			card_set cset;
			card* pcard;
			for(int32 i = 0; i < returns.bvalue[0]; ++i) {
				pcard = core.select_cards[returns.bvalue[i + 1]];
				cset.insert(pcard);
			}
			if(cset.size())
				send_to(&cset, core.reason_effect, it->arg3, core.reason_player, it->arg1, LOCATION_GRAVE, 0, POS_FACEUP);
			else
				returns.ivalue[0] = 0;
			it->step++;
		} else {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_DISCARD_DECK_S: {
		if(discard_deck(it->step, it->arg1 & 0xff, it->arg1 >> 16, it->arg2)) {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_INT);
			core.units.pop_front();
		} else {
			it->step++;
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_SORT_DECK_S: {
		uint8 sort_player = it->arg1 & 0xffff;
		uint8 target_player = it->arg1 >> 16;
		uint8 count = it->arg2, i = 0;
		if(count > player[target_player].list_main.size())
			count = player[target_player].list_main.size();
		if(it->step == 0) {
			core.select_cards.clear();
			for(auto clit = player[target_player].list_main.rbegin(); i < count; ++i, ++clit)
				core.select_cards.push_back(*clit);
			add_process(PROCESSOR_SORT_CARD, 0, 0, 0, sort_player, 0);
			it->step++;
		} else {
			if(returns.bvalue[0] != -1) {
				card* tc[16];
				for(i = 0; i < count; ++i)
					player[target_player].list_main.pop_back();
				for(i = 0; i < count; ++i)
					tc[(int32)returns.bvalue[i]] = core.select_cards[i];
				for(i = 0; i < count; ++i) {
					player[target_player].list_main.push_back(tc[count - i - 1]);
					tc[count - i - 1]->current.sequence = player[target_player].list_main.size() - 1;
				}
			}
			if(core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK) {
				if(count > 0) {
					card* ptop = player[target_player].list_main.back();
					if(core.deck_reversed || (ptop->current.position == POS_FACEUP_DEFENSE)) {
						pduel->write_buffer8(MSG_DECK_TOP);
						pduel->write_buffer8(target_player);
						pduel->write_buffer8(0);
						if(ptop->current.position != POS_FACEUP_DEFENSE)
							pduel->write_buffer32(ptop->data.code);
						else
							pduel->write_buffer32(ptop->data.code | 0x80000000);
					}
				}
			}
			core.units.pop_front();
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_REMOVEOL_S: {
		if(remove_overlay_card(it->step, it->arg3, (card*)(it->ptarget), it->arg1 >> 16,
		                       (it->arg1 >> 8) & 0xff, it->arg1 & 0xff, it->arg2 & 0xffff, it->arg2 >> 16)) {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_BOOLEAN);
			core.units.pop_front();
		} else {
			it->step++;
		}
		return pduel->bufferlen;
	}
	case PROCESSOR_MOVETOFIELD_S: {
		if (move_to_field(it->step, (card*)it->ptarget, it->arg1, it->arg2 & 0xff, (it->arg2 >> 8) & 0xff, it->arg3)) {
			pduel->lua->add_param(returns.ivalue[0], PARAM_TYPE_BOOLEAN);
			core.units.pop_front();
		} else
			it->step++;
		return pduel->bufferlen;
	}
	}
	return pduel->bufferlen;
}
int32 field::execute_cost(uint16 step, effect * triggering_effect, uint8 triggering_player) {
	if(!triggering_effect->cost) {
		core.solving_event.splice(core.solving_event.begin(), core.sub_solving_event);
		pduel->lua->params.clear();
		return TRUE;
	}
	if (step == 0) {
		core.solving_event.splice(core.solving_event.begin(), core.sub_solving_event);
		const tevent& e = core.solving_event.front();
		pduel->lua->add_param(1, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.reason_player, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.reason, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.reason_effect , PARAM_TYPE_EFFECT, true);
		pduel->lua->add_param(e.event_value, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.event_player, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.event_cards , PARAM_TYPE_GROUP, true);
		pduel->lua->add_param(triggering_player, PARAM_TYPE_INT, true);
		pduel->lua->add_param(triggering_effect, PARAM_TYPE_EFFECT, true);
		if(core.check_level == 0) {
			core.shuffle_deck_check[0] = FALSE;
			core.shuffle_deck_check[1] = FALSE;
			core.shuffle_hand_check[0] = FALSE;
			core.shuffle_hand_check[1] = FALSE;
		}
		core.shuffle_check_disabled = FALSE;
		core.check_level++;
	}
	core.reason_effect = triggering_effect;
	core.reason_player = triggering_player;
	uint32 count = pduel->lua->params.size();
	uint32 yield_value = 0;
	int32 result = pduel->lua->call_coroutine(triggering_effect->cost, count, &yield_value, step);
	returns.ivalue[0] = yield_value;
	if (result == COROUTINE_FINISH || result == COROUTINE_ERROR || result == OPERATION_FAIL) {
		core.reason_effect = 0;
		core.reason_player = PLAYER_NONE;
		core.check_level--;
		if(core.check_level == 0) {
			if(core.shuffle_hand_check[0])
				shuffle(0, LOCATION_HAND);
			if(core.shuffle_hand_check[1])
				shuffle(1, LOCATION_HAND);
			if(core.shuffle_deck_check[0])
				shuffle(0, LOCATION_DECK);
			if(core.shuffle_deck_check[1])
				shuffle(1, LOCATION_DECK);
		}
		core.shuffle_check_disabled = FALSE;
		return TRUE;
	}
	return FALSE;
}
int32 field::execute_operation(uint16 step, effect * triggering_effect, uint8 triggering_player) {
	if(!triggering_effect->operation) {
		core.solving_event.splice(core.solving_event.begin(), core.sub_solving_event);
		pduel->lua->params.clear();
		return TRUE;
	}
	if (step == 0) {
		core.solving_event.splice(core.solving_event.begin(), core.sub_solving_event);
		const tevent& e = core.solving_event.front();
		pduel->lua->add_param(e.reason_player, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.reason, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.reason_effect , PARAM_TYPE_EFFECT, true);
		pduel->lua->add_param(e.event_value, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.event_player, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.event_cards , PARAM_TYPE_GROUP, true);
		pduel->lua->add_param(triggering_player, PARAM_TYPE_INT, true);
		pduel->lua->add_param(triggering_effect, PARAM_TYPE_EFFECT, true);
		if(core.check_level == 0) {
			core.shuffle_deck_check[0] = FALSE;
			core.shuffle_deck_check[1] = FALSE;
			core.shuffle_hand_check[0] = FALSE;
			core.shuffle_hand_check[1] = FALSE;
		}
		core.shuffle_check_disabled = FALSE;
		core.check_level++;
	}
	core.reason_effect = triggering_effect;
	core.reason_player = triggering_player;
	uint32 count = pduel->lua->params.size();
	uint32 yield_value = 0;
	int32 result = pduel->lua->call_coroutine(triggering_effect->operation, count, &yield_value, step);
	returns.ivalue[0] = yield_value;
	if (result == COROUTINE_FINISH || result == COROUTINE_ERROR || result == OPERATION_FAIL) {
		core.reason_effect = 0;
		core.reason_player = PLAYER_NONE;
		core.check_level--;
		if(core.check_level == 0) {
			if(core.shuffle_hand_check[0])
				shuffle(0, LOCATION_HAND);
			if(core.shuffle_hand_check[1])
				shuffle(1, LOCATION_HAND);
			if(core.shuffle_deck_check[0])
				shuffle(0, LOCATION_DECK);
			if(core.shuffle_deck_check[1])
				shuffle(1, LOCATION_DECK);
			//cost[0].count = 0;
			//cost[1].count = 0;
			//cost[0].amount = 0;
			//cost[1].amount = 0;
		}
		core.shuffle_check_disabled = FALSE;
		return TRUE;
	}
	return FALSE;
}
int32 field::execute_target(uint16 step, effect * triggering_effect, uint8 triggering_player) {
	if(!triggering_effect->target) {
		core.solving_event.splice(core.solving_event.begin(), core.sub_solving_event);
		pduel->lua->params.clear();
		return TRUE;
	}
	if (step == 0) {
		core.solving_event.splice(core.solving_event.begin(), core.sub_solving_event);
		const tevent& e = core.solving_event.front();
		pduel->lua->add_param(1, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.reason_player, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.reason, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.reason_effect , PARAM_TYPE_EFFECT, true);
		pduel->lua->add_param(e.event_value, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.event_player, PARAM_TYPE_INT, true);
		pduel->lua->add_param(e.event_cards , PARAM_TYPE_GROUP, true);
		pduel->lua->add_param(triggering_player, PARAM_TYPE_INT, true);
		pduel->lua->add_param(triggering_effect, PARAM_TYPE_EFFECT, true);
		if(core.check_level == 0) {
			core.shuffle_deck_check[0] = FALSE;
			core.shuffle_deck_check[1] = FALSE;
			core.shuffle_hand_check[0] = FALSE;
			core.shuffle_hand_check[1] = FALSE;
		}
		core.shuffle_check_disabled = FALSE;
		core.check_level++;
	}
	core.reason_effect = triggering_effect;
	core.reason_player = triggering_player;
	uint32 count = pduel->lua->params.size();
	uint32 yield_value = 0;
	int32 result = pduel->lua->call_coroutine(triggering_effect->target, count, &yield_value, step);
	returns.ivalue[0] = yield_value;
	if (result == COROUTINE_FINISH || result == COROUTINE_ERROR || result == OPERATION_FAIL) {
		core.reason_effect = 0;
		core.reason_player = PLAYER_NONE;
		core.check_level--;
		if(core.check_level == 0) {
			if(core.shuffle_hand_check[0])
				shuffle(0, LOCATION_HAND);
			if(core.shuffle_hand_check[1])
				shuffle(1, LOCATION_HAND);
			if(core.shuffle_deck_check[0])
				shuffle(0, LOCATION_DECK);
			if(core.shuffle_deck_check[1])
				shuffle(1, LOCATION_DECK);
		}
		core.shuffle_check_disabled = FALSE;
		return TRUE;
	}
	return FALSE;
}
// add events to core.queue_event
void field::raise_event(card* event_card, uint32 event_code, effect* reason_effect, uint32 reason, uint8 reason_player, uint8 event_player, uint32 event_value) {
	tevent new_event;
	new_event.trigger_card = 0;
	if (event_card) {
		group* pgroup = pduel->new_group(event_card);
		pgroup->is_readonly = TRUE;
		new_event.event_cards = pgroup;
	} else
		new_event.event_cards = 0;
	new_event.event_code = event_code;
	new_event.reason_effect = reason_effect;
	new_event.reason = reason;
	new_event.reason_player = reason_player;
	new_event.event_player = event_player;
	new_event.event_value = event_value;
	core.queue_event.push_back(new_event);
}
void field::raise_event(card_set* event_cards, uint32 event_code, effect* reason_effect, uint32 reason, uint8 reason_player, uint8 event_player, uint32 event_value) {
	tevent new_event;
	new_event.trigger_card = 0;
	if (event_cards) {
		group* pgroup = pduel->new_group(*event_cards);
		pgroup->is_readonly = TRUE;
		new_event.event_cards = pgroup;
	} else
		new_event.event_cards = 0;
	new_event.event_code = event_code;
	new_event.reason_effect = reason_effect;
	new_event.reason = reason;
	new_event.reason_player = reason_player;
	new_event.event_player = event_player;
	new_event.event_value = event_value;
	core.queue_event.push_back(new_event);
}
// add events to core.single_event
void field::raise_single_event(card* trigger_card, card_set* event_cards, uint32 event_code, effect * reason_effect, uint32 reason, uint8 reason_player, uint8 event_player, uint32 event_value) {
	tevent new_event;
	new_event.trigger_card = trigger_card;
	if (event_cards) {
		group* pgroup = pduel->new_group(*event_cards);
		pgroup->is_readonly = TRUE;
		new_event.event_cards = pgroup;
	} else
		new_event.event_cards = 0;
	new_event.event_code = event_code;
	new_event.reason_effect = reason_effect;
	new_event.reason = reason;
	new_event.reason_player = reason_player;
	new_event.event_player = event_player;
	new_event.event_value = event_value;
	core.single_event.push_back(new_event);
}
// called by Duel.CheckEvent()
int32 field::check_event(uint32 code, tevent * pe) {
	for(auto eit = core.point_event.begin(); eit != core.point_event.end(); ++eit) {
		if(eit->event_code == code) {
			if(pe)
				*pe = *eit;
			return TRUE;
		}
	}
	for(auto eit = core.instant_event.begin(); eit != core.instant_event.end(); ++eit) {
		if(eit->event_code == code) {
			if(pe)
				*pe = *eit;
			return TRUE;
		}
	}
	return FALSE;
}
// called by Duel.CheckActivateEffect()
int32 field::check_event_c(effect* peffect, uint8 playerid, int32 neglect_con, int32 neglect_cost, int32 copy_info, tevent* pe) {
	if(peffect->code == EVENT_FREE_CHAIN) {
		return peffect->is_activate_ready(playerid, nil_event, neglect_con, neglect_cost, FALSE);
	}
	for(auto eit = core.point_event.begin(); eit != core.point_event.end(); ++eit) {
		if(eit->event_code == peffect->code &&
		        peffect->is_activate_ready(playerid, *eit, neglect_con, neglect_cost, FALSE)) {
			if(pe)
				*pe = *eit;
			if(copy_info && !pduel->lua->no_action && core.current_chain.size()) {
				core.current_chain.back().evt = *eit;
			}
			return TRUE;
		}
	}
	for(auto eit = core.instant_event.begin(); eit != core.instant_event.end(); ++eit) {
		if(eit->event_code == peffect->code &&
		        peffect->is_activate_ready(playerid, *eit, neglect_con, neglect_cost, FALSE)) {
			if(pe)
				*pe = *eit;
			if(copy_info && !pduel->lua->no_action && core.current_chain.size()) {
				core.current_chain.back().evt = *eit;
			}
			return TRUE;
		}
	}
	return FALSE;
}
int32 field::check_hint_timing(effect* peffect) {
	int32 p = peffect->get_handler_player();
	if(p == 0)
		return (peffect->hint_timing[0] & core.hint_timing[0]) || (peffect->hint_timing[1] & core.hint_timing[1]);
	else
		return (peffect->hint_timing[0] & core.hint_timing[1]) || (peffect->hint_timing[1] & core.hint_timing[0]);
}
int32 field::process_phase_event(int16 step, int32 phase) {
	switch(step) {
	case 0: {
		if((phase == PHASE_DRAW && is_player_affected_by_effect(infos.turn_player, EFFECT_SKIP_DP))
		        || (phase == PHASE_STANDBY && is_player_affected_by_effect(infos.turn_player, EFFECT_SKIP_SP))
		        || (phase == PHASE_BATTLE_START && is_player_affected_by_effect(infos.turn_player, EFFECT_SKIP_BP))) {
			core.units.begin()->step = 24;
			return FALSE;
		}
		int32 phase_event = EVENT_PHASE + phase;
		effect* peffect;
		nil_event.event_code = phase_event;
		nil_event.event_player = infos.turn_player;
		int32 check_player = infos.turn_player;
		if(core.units.begin()->arg2 & 0x2)
			check_player = 1 - infos.turn_player;
		chain newchain;
		core.select_chains.clear();
		int32 tf_count = 0, to_count = 0, fc_count = 0, cn_count = 0;
		auto pr = effects.trigger_f_effect.equal_range(phase_event);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			peffect->set_activate_location();
			if(!peffect->is_activateable(check_player, nil_event))
				continue;
			peffect->id = infos.field_id++;
			newchain.triggering_effect = peffect;
			core.select_chains.push_back(newchain);
			tf_count++;
		}
		pr = effects.continuous_effect.equal_range(phase_event);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			//effects.continuous_effect may be changed in is_activateable (e.g. Rescue Cat)
			if(peffect->get_handler_player() != check_player || !peffect->is_activateable(check_player, nil_event))
				continue;
			peffect->id = infos.field_id++;
			newchain.triggering_effect = peffect;
			core.select_chains.push_back(newchain);
			cn_count++;
		}
		for(auto eit = effects.pheff.begin(); eit != effects.pheff.end();) {
			peffect = *eit++;
			if(peffect->code != EFFECT_SET_CONTROL)
				continue;
			if(peffect->get_owner_player() != check_player)
				continue;
			if(!(peffect->reset_flag & phase))
				continue;
			uint8 pid = peffect->get_handler_player();
			uint8 tp = infos.turn_player;
			if(!(((peffect->reset_flag & RESET_SELF_TURN) && pid == tp) || ((peffect->reset_flag & RESET_OPPO_TURN) && pid != tp)))
				continue;
			if(peffect->reset_count != 1)
				continue;
			card* phandler = peffect->get_handler();
			if(peffect->get_owner_player() != phandler->current.controler) {
				if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY))
					remove_effect(peffect);
				else
					peffect->handler->remove_effect(peffect);
				continue;
			}
			newchain.triggering_effect = peffect;
			core.select_chains.push_back(newchain);
			cn_count++;
		}
		core.spe_effect[check_player] = 0;
		if(!core.hand_adjusted) {
			pr = effects.trigger_o_effect.equal_range(phase_event);
			for(auto eit = pr.first; eit != pr.second;) {
				effect* peffect = eit->second;
				++eit;
				peffect->set_activate_location();
				if(!peffect->is_activateable(check_player, nil_event))
					continue;
				peffect->id = infos.field_id++;
				newchain.triggering_effect = peffect;
				core.select_chains.push_back(newchain);
				to_count++;
				core.spe_effect[check_player]++;
			}
			if(phase == PHASE_DRAW)
				core.hint_timing[infos.turn_player] = TIMING_DRAW_PHASE;
			else if(phase == PHASE_STANDBY)
				core.hint_timing[infos.turn_player] = TIMING_STANDBY_PHASE;
			else if(phase == PHASE_BATTLE_START)
				core.hint_timing[infos.turn_player] = TIMING_BATTLE_START;
			else if(phase == PHASE_BATTLE)
				core.hint_timing[infos.turn_player] = TIMING_BATTLE_END;
			else
				core.hint_timing[infos.turn_player] = TIMING_END_PHASE;
			pr = effects.activate_effect.equal_range(EVENT_FREE_CHAIN);
			for(auto eit = pr.first; eit != pr.second;) {
				effect* peffect = eit->second;
				++eit;
				peffect->set_activate_location();
				if(!peffect->is_chainable(check_player) || !peffect->is_activateable(check_player, nil_event))
					continue;
				peffect->id = infos.field_id++;
				newchain.triggering_effect = peffect;
				if(check_hint_timing(peffect))
					core.spe_effect[check_player]++;
				core.select_chains.push_back(newchain);
				fc_count++;
			}
			pr = effects.quick_o_effect.equal_range(EVENT_FREE_CHAIN);
			for(auto eit = pr.first; eit != pr.second;) {
				effect* peffect = eit->second;
				++eit;
				peffect->set_activate_location();
				if(!peffect->is_chainable(check_player) || !peffect->is_activateable(check_player, nil_event))
					continue;
				peffect->id = infos.field_id++;
				newchain.triggering_effect = peffect;
				if(check_hint_timing(peffect))
					core.spe_effect[check_player]++;
				core.select_chains.push_back(newchain);
				fc_count++;
			}
			pr = effects.continuous_effect.equal_range(EVENT_FREE_CHAIN);
			for(auto eit = pr.first; eit != pr.second;) {
				effect* peffect = eit->second;
				++eit;
				if(peffect->get_handler_player() != check_player || !peffect->is_activateable(check_player, nil_event))
					continue;
				peffect->id = infos.field_id++;
				newchain.triggering_effect = peffect;
				core.select_chains.push_back(newchain);
				fc_count++;
			}
		}
		if(core.select_chains.size() == 0) {
			returns.ivalue[0] = -1;
			core.units.begin()->step = 1;
			return FALSE;
		} else if(tf_count == 0 && cn_count == 1 && to_count == 0 && fc_count == 0) {
			returns.ivalue[0] = 0;
			core.units.begin()->step = 1;
			return FALSE;
		} else {
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_EVENT);
			pduel->write_buffer8(check_player);
			if(infos.phase == PHASE_DRAW)
				pduel->write_buffer32(20);
			else if(infos.phase == PHASE_STANDBY)
				pduel->write_buffer32(21);
			else if(infos.phase == PHASE_BATTLE_START)
				pduel->write_buffer32(28);
			else if(infos.phase == PHASE_BATTLE)
				pduel->write_buffer32(25);
			else
				pduel->write_buffer32(26);
			if(tf_count == 0 && to_count == 1 && fc_count == 0 && cn_count == 0) {
				add_process(PROCESSOR_SELECT_EFFECTYN, 0, 0, (group*)core.select_chains[0].triggering_effect->get_handler(), check_player, 0);
				return FALSE;
			} else {
				add_process(PROCESSOR_SELECT_CHAIN, 0, 0, 0, check_player, core.spe_effect[check_player] | (tf_count + cn_count ? 0x10000 : 0));
				core.units.begin()->step = 1;
				return FALSE;
			}
		}
		return FALSE;
	}
	case 1: {
		returns.ivalue[0]--;
		return FALSE;
	}
	case 2: {
		int32 priority_passed = core.units.begin()->arg2 & 0x1;
		int32 is_opponent = core.units.begin()->arg2 & 0x2;
		if(returns.ivalue[0] == -1) {
			if(priority_passed)
				core.units.begin()->step = 19;
			else {
				priority_passed = 1;
				is_opponent ^= 0x2;
				core.units.begin()->arg2 = is_opponent | priority_passed;
				core.units.begin()->step = -1;
			}
			return FALSE;
		}
		priority_passed = 0;
		core.units.begin()->arg2 = is_opponent | priority_passed;
		chain newchain = core.select_chains[returns.ivalue[0]];
		effect* peffect = newchain.triggering_effect;
		card* phandler = peffect->get_handler();
		if(!(peffect->type & EFFECT_TYPE_ACTIONS)) {
			if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY))
				remove_effect(peffect);
			else
				peffect->handler->remove_effect(peffect);
			adjust_all();
			core.units.begin()->step = 3;
		} else if(!(peffect->type & EFFECT_TYPE_CONTINUOUS)) {
			int32 check_player = infos.turn_player;
			if(is_opponent)
				check_player = 1 - infos.turn_player;
			newchain.flag = 0;
			newchain.chain_id = infos.field_id++;
			newchain.evt = nil_event;
			newchain.set_triggering_place(phandler);
			newchain.triggering_player = check_player;
			core.new_chains.push_back(newchain);
			phandler->set_status(STATUS_CHAINING, TRUE);
			peffect->dec_count(check_player);
			core.select_chains.clear();
			add_process(PROCESSOR_ADD_CHAIN, 0, 0, 0, 0, 0);
			add_process(PROCESSOR_QUICK_EFFECT, 0, 0, 0, FALSE, 1 - check_player);
			infos.priorities[0] = 0;
			infos.priorities[1] = 0;
		} else {
			core.select_chains.clear();
			core.sub_solving_event.push_back(nil_event);
			add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, peffect, 0, peffect->get_handler_player(), 0);
			core.units.begin()->step = 3;
		}
		return FALSE;
	}
	case 3: {
		for(auto it = core.chain_limit.begin(); it != core.chain_limit.end(); ++it)
			luaL_unref(pduel->lua->lua_state, LUA_REGISTRYINDEX, it->function);
		core.chain_limit.clear();
		for(auto cait = core.current_chain.begin(); cait != core.current_chain.end(); ++cait)
			cait->triggering_effect->get_handler()->set_status(STATUS_CHAINING, FALSE);
		add_process(PROCESSOR_SOLVE_CHAIN, 0, 0, 0, FALSE, 0);
		core.units.begin()->step = -1;
		return FALSE;
	}
	case 4: {
		adjust_instant();
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, 0);
		core.units.begin()->step = -1;
		return FALSE;
	}
	case 20: {
		if(phase != PHASE_END) {
			core.units.begin()->step = 24;
			return FALSE;
		}
		int32 limit = 6;
		effect_set eset;
		filter_player_effect(infos.turn_player, EFFECT_HAND_LIMIT, &eset);
		if(eset.size())
			limit = eset.get_last()->get_value();
		int32 hd = player[infos.turn_player].list_hand.size();
		if(hd <= limit) {
			core.units.begin()->step = 24;
			return FALSE;
		}
		core.select_cards.clear();
		for(auto cit = player[infos.turn_player].list_hand.begin(); cit != player[infos.turn_player].list_hand.end(); ++cit)
			core.select_cards.push_back(*cit);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(infos.turn_player);
		pduel->write_buffer32(501);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, infos.turn_player, hd - limit + ((hd - limit) << 16));
		return FALSE;
	}
	case 21: {
		card_set cset;
		for(int32 i = 0; i < returns.bvalue[0]; ++i)
			cset.insert(core.select_cards[returns.bvalue[i + 1]]);
		send_to(&cset, 0, REASON_RULE + REASON_DISCARD + REASON_ADJUST, infos.turn_player, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
		return FALSE;
	}
	case 22: {
		core.hand_adjusted = TRUE;
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, 0);
		core.units.begin()->step = -1;
		core.units.begin()->arg2 = 0;
		return FALSE;
	}
	case 25: {
		core.hint_timing[infos.turn_player] = 0;
		reset_phase(phase);
		adjust_all();
		return FALSE;
	}
	case 26: {
		core.quick_f_chain.clear();
		core.instant_event.clear();
		core.point_event.clear();
		core.full_event.clear();
		return TRUE;
	}
	}
	return TRUE;
}
// move the events from core.instant_event to core.point_event
// skip_trigger = lower byte of arg1, skip_freechain = higher byte of arg1, skip_new = arg2
int32 field::process_point_event(int16 step, int32 skip_trigger, int32 skip_freechain, int32 skip_new) {
	switch(step) {
	case 0: {
		core.select_chains.clear();
		core.point_event.splice(core.point_event.end(), core.instant_event);
		if(skip_trigger) {
			core.units.begin()->step = 7;
			return FALSE;
		}
		core.new_fchain_s.splice(core.new_fchain_s.begin(), core.new_fchain);
		core.new_ochain_s.splice(core.new_ochain_s.begin(), core.new_ochain);
		core.tpchain.clear();
		core.ntpchain.clear();
		core.delayed_quick.clear();
		core.delayed_quick_break.swap(core.delayed_quick);
		core.current_player = infos.turn_player;
		core.units.begin()->step = 1;
		return FALSE;
	}
	case 1: {
		return FALSE;
	}
	case 2: {
		//forced trigger
		core.select_chains.clear();
		for (auto clit = core.new_fchain_s.begin(); clit != core.new_fchain_s.end(); ) {
			effect* peffect = clit->triggering_effect;
			card* phandler = peffect->get_handler();
			if(!peffect->is_flag(EFFECT_FLAG_EVENT_PLAYER | EFFECT_FLAG_BOTH_SIDE) && phandler->is_has_relation(*clit)) {
				clit->triggering_player = phandler->current.controler;
				clit->set_triggering_place(phandler);
			}
			uint8 tp = clit->triggering_player;
			bool act = true;
			if(peffect->is_chainable(tp) && peffect->is_activateable(tp, clit->evt, TRUE)
				&& (!(peffect->type & EFFECT_TYPE_FIELD) || phandler->is_has_relation(*clit))
				&& (peffect->code == EVENT_FLIP && infos.phase == PHASE_DAMAGE
					|| (clit->triggering_location & 0x43) && (clit->triggering_position & POS_FACEDOWN)
					|| !(phandler->current.location & 0x43) || phandler->is_position(POS_FACEUP))) {
				if(peffect->is_flag(EFFECT_FLAG_CHAIN_UNIQUE)) {
					for(auto tpit = core.current_chain.begin(); tpit != core.current_chain.end(); ++tpit) {
						if(tpit->triggering_effect->get_handler()->data.code == phandler->data.code && tpit->triggering_player == tp) {
							act = false;
							break;
						}
					}
				}
			} else
				act = false;
			if(act) {
				if(tp == core.current_player)
					core.select_chains.push_back(*clit);
			} else {
				core.new_fchain_s.erase(clit++);
				continue;
			}
			++clit;
		}
		if(core.select_chains.size() == 0) {
			returns.ivalue[0] = -1;
		} else if(core.select_chains.size() == 1) {
			returns.ivalue[0] = 0;
		} else {
			add_process(PROCESSOR_SELECT_CHAIN, 0, 0, 0, core.current_player, 0x7f | 0x10000);
		}
		return FALSE;
	}
	case 3: {
		if(returns.ivalue[0] == -1) {
			if(core.new_fchain_s.size()) {
				core.current_player = 1 - infos.turn_player;
				core.units.begin()->step = 1;
			} else {
				core.current_player = infos.turn_player;
			}
			return FALSE;
		}
		chain newchain = core.select_chains[returns.ivalue[0]];
		effect* peffect = newchain.triggering_effect;
		uint8 tp = newchain.triggering_player;
		peffect->get_handler()->set_status(STATUS_CHAINING, TRUE);
		peffect->dec_count(tp);
		core.new_chains.push_back(newchain);
		add_process(PROCESSOR_ADD_CHAIN, 0, 0, 0, 0, 0);
		core.new_fchain_s.remove_if([=](chain ch) { return ch.chain_id == newchain.chain_id; });
		core.units.begin()->step = 1;
		return FALSE;
	}
	case 4: {
		//optional trigger
		core.select_chains.clear();
		for (auto clit = core.new_ochain_s.begin(); clit != core.new_ochain_s.end(); ) {
			effect* peffect = clit->triggering_effect;
			card* phandler = peffect->get_handler();
			if((!peffect->is_flag(EFFECT_FLAG_EVENT_PLAYER | EFFECT_FLAG_BOTH_SIDE) && phandler->is_has_relation(*clit))
				|| (!peffect->is_flag(EFFECT_FLAG_FIELD_ONLY) && (peffect->type & EFFECT_TYPE_FIELD)
					&& (peffect->range & LOCATION_HAND) && phandler->current.location == LOCATION_HAND)) {
				if(!phandler->is_has_relation(*clit))
					phandler->create_relation(*clit);
				clit->triggering_player = phandler->current.controler;
				clit->set_triggering_place(phandler);
			}
			uint8 tp = clit->triggering_player;
			bool act = true;
			if(peffect->is_chainable(tp) && peffect->is_activateable(tp, clit->evt, TRUE)
				&& (!(peffect->type & EFFECT_TYPE_FIELD) || phandler->is_has_relation(*clit))
				&& (peffect->code == EVENT_FLIP && infos.phase == PHASE_DAMAGE
					|| (clit->triggering_location & 0x43) && (clit->triggering_position & POS_FACEDOWN)
					|| !(phandler->current.location & 0x43) || phandler->is_position(POS_FACEUP))) {
				if(!peffect->is_flag(EFFECT_FLAG_FIELD_ONLY) && clit->triggering_location == LOCATION_HAND
					&& (((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE) && phandler->is_has_relation(*clit))
						|| (peffect->range & LOCATION_HAND))) {
					core.new_ochain_h.push_back(*clit);
					act = false;
				} else if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY) || !(peffect->type & EFFECT_TYPE_FIELD) || peffect->in_range(*clit)) {
					if(peffect->is_flag(EFFECT_FLAG_CHAIN_UNIQUE)) {
						for(auto tpit = core.current_chain.begin(); tpit != core.current_chain.end(); ++tpit) {
							if(tpit->triggering_effect->get_handler()->data.code == phandler->data.code && tpit->triggering_player == tp) {
								act = false;
								break;
							}
						}
					}
				} else
					act = false;
			} else act = false;
			if(act) {
				if(tp == core.current_player)
					core.select_chains.push_back(*clit);
			} else {
				core.new_ochain_s.erase(clit++);
				continue;
			}
			++clit;
		}
		if(core.select_chains.size() == 0) {
			returns.ivalue[0] = -1;
			core.units.begin()->step = 5;
			return FALSE;
		} else if(core.select_chains.size() == 1 && !core.current_chain.size()) {
			add_process(PROCESSOR_SELECT_EFFECTYN, 0, 0, (group*)core.select_chains[0].triggering_effect->get_handler(), core.current_player, 0);
			return FALSE;
		} else {
			add_process(PROCESSOR_SELECT_CHAIN, 0, 0, 0, core.current_player, 0x7f);
			core.units.begin()->step = 5;
			return FALSE;
		}
		return FALSE;
	}
	case 5: {
		returns.ivalue[0]--;
		return FALSE;
	}
	case 6: {
		if(returns.ivalue[0] == -1) {
			for(auto cit = core.select_chains.begin(); cit != core.select_chains.end(); ++cit)
				core.new_ochain_s.remove_if([=](chain ch) { return ch.chain_id == cit->chain_id; });
			if(core.new_ochain_s.size()) {
				core.current_player = 1 - infos.turn_player;
				core.units.begin()->step = 3;
			} else {
				core.current_player = infos.turn_player;
				core.units.begin()->step = 6;
			}
			return FALSE;
		}
		chain newchain = core.select_chains[returns.ivalue[0]];
		effect* peffect = newchain.triggering_effect;
		uint8 tp = newchain.triggering_player;
		peffect->get_handler()->set_status(STATUS_CHAINING, TRUE);
		peffect->dec_count(tp);
		core.new_chains.push_back(newchain);
		add_process(PROCESSOR_ADD_CHAIN, 0, 0, 0, 0, 0);
		core.new_ochain_s.remove_if([=](chain ch) { return ch.chain_id == newchain.chain_id; });
		core.units.begin()->step = 3;
		return FALSE;
	}
	case 7: {
		core.select_chains.clear();
		return FALSE;
	}
	case 8: {
		if((core.duel_rule >= 2) || (infos.phase != PHASE_MAIN1 && infos.phase != PHASE_MAIN2))
			return FALSE;
		// Obsolete ignition effect ruling
		tevent e;
		if(core.current_chain.size() == 0 &&
		        (check_event(EVENT_SUMMON_SUCCESS, &e) || check_event(EVENT_SPSUMMON_SUCCESS, &e) || check_event(EVENT_FLIP_SUMMON_SUCCESS, &e))
		        && e.reason_player == infos.turn_player) {
			chain newchain;
			tevent e;
			e.event_cards = 0;
			e.event_value = 0;
			e.event_player = PLAYER_NONE;
			e.reason_effect = 0;
			e.reason = 0;
			e.reason_player = PLAYER_NONE;
			for(auto eit = effects.ignition_effect.begin(); eit != effects.ignition_effect.end();) {
				effect* peffect = eit->second;
				++eit;
				card* phandler = peffect->get_handler();
				e.event_code = peffect->code;
				if(phandler->current.location == LOCATION_MZONE && peffect->is_chainable(infos.turn_player)
				        && peffect->is_activateable(infos.turn_player, e)) {
					newchain.flag = 0;
					newchain.chain_id = infos.field_id++;
					newchain.evt = e;
					newchain.triggering_effect = peffect;
					newchain.set_triggering_place(phandler);
					newchain.triggering_player = infos.turn_player;
					core.select_chains.push_back(newchain);
				}
			}
		}
		return FALSE;
	}
	case 9: {
		infos.priorities[0] = 0;
		infos.priorities[1] = 0;
		if(core.current_chain.size() == 0) {
			if(!core.hand_adjusted)
				add_process(PROCESSOR_QUICK_EFFECT, 0, 0, 0, skip_freechain, infos.turn_player);
		} else
			add_process(PROCESSOR_QUICK_EFFECT, 0, 0, 0, skip_freechain, 1 - core.current_chain.back().triggering_player);
		return FALSE;
	}
	case 10: {
		core.new_ochain_h.clear();
		core.full_event.clear();
		for(auto it = core.chain_limit.begin(); it != core.chain_limit.end(); ++it)
			luaL_unref(pduel->lua->lua_state, LUA_REGISTRYINDEX, it->function);
		core.chain_limit.clear();
		if(core.current_chain.size()) {
			for(auto cait = core.current_chain.begin(); cait != core.current_chain.end(); ++cait)
				cait->triggering_effect->get_handler()->set_status(STATUS_CHAINING, FALSE);
			add_process(PROCESSOR_SOLVE_CHAIN, 0, 0, 0, skip_trigger | ((skip_freechain | skip_new) << 8), skip_new);
		} else {
			core.used_event.splice(core.used_event.end(), core.point_event);
			for(auto it = core.chain_limit_p.begin(); it != core.chain_limit_p.end(); ++it)
				luaL_unref(pduel->lua->lua_state, LUA_REGISTRYINDEX, it->function);
			core.chain_limit_p.clear();
			reset_chain();
			returns.ivalue[0] = FALSE;
		}
		return TRUE;
	}
	case 30: {
		int32 check_player = infos.turn_player;
		nil_event.event_code = EVENT_FREE_CHAIN;
		chain newchain;
		core.select_chains.clear();
		core.spe_effect[check_player] = 0;
		auto pr = effects.continuous_effect.equal_range(EVENT_FREE_CHAIN);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			newchain.triggering_effect = peffect;
			if(peffect->get_handler_player() == check_player && peffect->is_activateable(check_player, nil_event)) {
				core.select_chains.push_back(newchain);
				core.spe_effect[check_player]++;
			}
		}
		if(!core.select_chains.empty())
			add_process(PROCESSOR_SELECT_CHAIN, 0, 0, 0, check_player, core.spe_effect[check_player]);
		else
			core.units.begin()->step = 31;
		return FALSE;
	}
	case 31: {
		if(returns.ivalue[0] == -1)
			return FALSE;
		const chain& newchain = core.select_chains[returns.ivalue[0]];
		effect* peffect = newchain.triggering_effect;
		core.select_chains.clear();
		core.sub_solving_event.push_back(nil_event);
		add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, peffect, 0, peffect->get_handler_player(), 0);
		core.units.begin()->step = 29;
		return FALSE;
	}
	case 32: {
		int32 check_player = 1 - infos.turn_player;
		nil_event.event_code = EVENT_FREE_CHAIN;
		chain newchain;
		core.select_chains.clear();
		core.spe_effect[check_player] = 0;
		auto pr = effects.continuous_effect.equal_range(EVENT_FREE_CHAIN);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			newchain.triggering_effect = peffect;
			if(peffect->get_handler_player() == check_player && peffect->is_activateable(check_player, nil_event)) {
				core.select_chains.push_back(newchain);
				core.spe_effect[check_player]++;
			}
		}
		if(!core.select_chains.empty())
			add_process(PROCESSOR_SELECT_CHAIN, 0, 0, 0, check_player, core.spe_effect[check_player]);
		else
			core.units.begin()->step = -1;
		return FALSE;
	}
	case 33: {
		if(returns.ivalue[0] == -1) {
			core.units.begin()->step = -1;
			return FALSE;
		}
		const chain& newchain = core.select_chains[returns.ivalue[0]];
		effect* peffect = newchain.triggering_effect;
		core.select_chains.clear();
		core.sub_solving_event.push_back(nil_event);
		add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, peffect, 0, peffect->get_handler_player(), 0);
		core.units.begin()->step = 31;
		return FALSE;
	}
	}
	return TRUE;
}
int32 field::process_quick_effect(int16 step, int32 skip_freechain, uint8 priority) {
	switch(step) {
	case 0: {
		core.tpchain.clear();
		core.ntpchain.clear();
		if(!core.quick_f_chain.size())
			return FALSE;
		bool act = true;
		for(auto ifit = core.quick_f_chain.begin(); ifit != core.quick_f_chain.end(); ++ifit) {
			effect* peffect = ifit->first;
			card* phandler = peffect->get_handler();
			if(peffect->is_chainable(ifit->second.triggering_player) && peffect->check_count_limit(ifit->second.triggering_player)
					&& phandler->is_has_relation(ifit->second)) {
				if(ifit->second.triggering_player == infos.turn_player) {
					act = true;
					if(peffect->is_flag(EFFECT_FLAG_CHAIN_UNIQUE)) {
						for (auto cait = core.tpchain.begin(); cait != core.tpchain.end(); ++cait) {
							if (cait->triggering_effect->get_handler()->data.code == phandler->data.code) {
								act = false;
								break;
							}
						}
						for (auto cait = core.current_chain.begin(); cait != core.current_chain.end(); ++cait) {
							if ((cait->triggering_effect->get_handler()->data.code == phandler->data.code)
							        && (cait->triggering_player == infos.turn_player)) {
								act = false;
								break;
							}
						}
					}
					if(act) {
						core.tpchain.push_back(ifit->second);
						phandler->set_status(STATUS_CHAINING, TRUE);
						peffect->dec_count(infos.turn_player);
					}
				} else {
					act = true;
					if(peffect->is_flag(EFFECT_FLAG_CHAIN_UNIQUE)) {
						for (auto cait = core.ntpchain.begin(); cait != core.ntpchain.end(); ++cait) {
							if (cait->triggering_effect->get_handler()->data.code == phandler->data.code) {
								act = false;
								break;
							}
						}
						for (auto cait = core.current_chain.begin(); cait != core.current_chain.end(); ++cait) {
							if ((cait->triggering_effect->get_handler()->data.code == phandler->data.code)
							        && (cait->triggering_player != infos.turn_player)) {
								act = false;
								break;
							}
						}
					}
					if(act) {
						core.ntpchain.push_back(ifit->second);
						phandler->set_status(STATUS_CHAINING, TRUE);
						peffect->dec_count(1 - infos.turn_player);
					}
				}
			}
		}
		core.quick_f_chain.clear();
		if(core.tpchain.size() > 1)
			add_process(PROCESSOR_SORT_CHAIN, 0, 0, 0, 1, infos.turn_player);
		if(core.ntpchain.size() > 1)
			add_process(PROCESSOR_SORT_CHAIN, 0, 0, 0, 0, infos.turn_player);
		return FALSE;
	}
	case 1: {
		if(core.tpchain.size() == 0 && core.ntpchain.size() == 0) {
			chain newchain;
			auto evit = core.point_event.begin();
			bool pev = true;
			if(evit == core.point_event.end()) {
				evit = core.instant_event.begin();
				pev = false;
			}
			while(pev || (evit != core.instant_event.end())) {
				auto pr = effects.activate_effect.equal_range(evit->event_code);
				for(auto eit = pr.first; eit != pr.second;) {
					effect* peffect = eit->second;
					++eit;
					peffect->set_activate_location();
					if(!peffect->is_flag(EFFECT_FLAG_DELAY) && peffect->is_chainable(priority) && peffect->is_activateable(priority, *evit)) {
						card* phandler = peffect->get_handler();
						newchain.flag = 0;
						newchain.chain_id = infos.field_id++;
						newchain.evt = *evit;
						newchain.triggering_effect = peffect;
						newchain.set_triggering_place(phandler);
						newchain.triggering_player = priority;
						core.select_chains.push_back(newchain);
					}
				}
				pr = effects.quick_o_effect.equal_range(evit->event_code);
				for(auto eit = pr.first; eit != pr.second;) {
					effect* peffect = eit->second;
					++eit;
					peffect->set_activate_location();
					if(peffect->is_chainable(priority) && peffect->is_activateable(priority, *evit)) {
						card* phandler = peffect->get_handler();
						newchain.flag = 0;
						newchain.chain_id = infos.field_id++;
						newchain.evt = *evit;
						newchain.triggering_effect = peffect;
						newchain.set_triggering_place(phandler);
						newchain.triggering_player = priority;
						core.select_chains.push_back(newchain);
						core.delayed_quick_tmp.erase(std::make_pair(peffect, *evit));
						core.delayed_quick_break.erase(std::make_pair(peffect, *evit));
					}
				}
				++evit;
				if(pev && evit == core.point_event.end()) {
					evit = core.instant_event.begin();
					pev = false;
				}
			}
			for(auto clit = core.new_ochain_h.begin(); clit != core.new_ochain_h.end(); ++clit) {
				effect* peffect = clit->triggering_effect;
				card* phandler = peffect->get_handler();
				bool act = true;
				if(clit->triggering_player == priority && !phandler->is_status(STATUS_CHAINING) && phandler->is_has_relation(*clit)
				        && peffect->is_chainable(priority) && peffect->is_activateable(priority, clit->evt, TRUE)) {
					for(auto cait = core.current_chain.begin(); cait != core.current_chain.end(); ++cait) {
						if(cait->triggering_player == priority) {
							if(std::any_of(core.new_ochain_h.begin(), core.new_ochain_h.end(), [=](chain ch) { return ch.chain_id == cait->chain_id; })) {
								act = false;
								break;
							}
							if(peffect->is_flag(EFFECT_FLAG_CHAIN_UNIQUE) && cait->triggering_effect->get_handler()->data.code == phandler->data.code) {
								act = false;
								break;
							}
						}
					}
				} else
					act = false;
				if(act)
					core.select_chains.push_back(*clit);
			}
			//delayed activate
			for(auto evit = core.full_event.begin(); evit != core.full_event.end(); ++evit) {
				auto pr = effects.activate_effect.equal_range(evit->event_code);
				for(auto eit = pr.first; eit != pr.second;) {
					effect* peffect = eit->second;
					++eit;
					peffect->set_activate_location();
					if(peffect->is_flag(EFFECT_FLAG_DELAY) && peffect->is_chainable(priority) && peffect->is_activateable(priority, *evit)) {
						card* phandler = peffect->get_handler();
						newchain.flag = 0;
						newchain.chain_id = infos.field_id++;
						newchain.evt = *evit;
						newchain.triggering_effect = peffect;
						newchain.set_triggering_place(phandler);
						newchain.triggering_player = priority;
						core.select_chains.push_back(newchain);
					}
				}
			}
			// delayed quick
			for(auto eit = core.delayed_quick.begin(); eit != core.delayed_quick.end();) {
				effect* peffect = eit->first;
				++eit;
				peffect->set_activate_location();
				const tevent& evt = eit->second;
				if(peffect->is_chainable(priority) && peffect->is_activateable(priority, evt, TRUE, FALSE, FALSE)) {
					card* phandler = peffect->get_handler();
					newchain.flag = 0;
					newchain.chain_id = infos.field_id++;
					newchain.evt = evt;
					newchain.triggering_effect = peffect;
					newchain.set_triggering_place(phandler);
					newchain.triggering_player = priority;
					core.select_chains.push_back(newchain);
				}
			}
			core.spe_effect[priority] = core.select_chains.size();
			if(!skip_freechain) {
				nil_event.event_code = EVENT_FREE_CHAIN;
				auto pr = effects.activate_effect.equal_range(EVENT_FREE_CHAIN);
				for(auto eit = pr.first; eit != pr.second;) {
					effect* peffect = eit->second;
					++eit;
					peffect->set_activate_location();
					if(peffect->is_chainable(priority) && peffect->is_activateable(priority, nil_event)) {
						card* phandler = peffect->get_handler();
						newchain.flag = 0;
						newchain.chain_id = infos.field_id++;
						newchain.evt = nil_event;
						newchain.triggering_effect = peffect;
						newchain.set_triggering_place(phandler);
						newchain.triggering_player = priority;
						core.select_chains.push_back(newchain);
						if(check_hint_timing(peffect))
							core.spe_effect[priority]++;
					}
				}
				pr = effects.quick_o_effect.equal_range(EVENT_FREE_CHAIN);
				for(auto eit = pr.first; eit != pr.second;) {
					effect* peffect = eit->second;
					++eit;
					peffect->set_activate_location();
					if(peffect->is_chainable(priority) && peffect->is_activateable(priority, nil_event)) {
						card* phandler = peffect->get_handler();
						newchain.flag = 0;
						newchain.chain_id = infos.field_id++;
						newchain.evt = nil_event;
						newchain.triggering_effect = peffect;
						newchain.set_triggering_place(phandler);
						newchain.triggering_player = priority;
						core.select_chains.push_back(newchain);
						if(check_hint_timing(peffect))
							core.spe_effect[priority]++;
					}
				}
			}
			if(core.current_chain.size() || (core.hint_timing[0] & TIMING_ATTACK) || (core.hint_timing[1] & TIMING_ATTACK))
				core.spe_effect[priority] = core.select_chains.size();
			add_process(PROCESSOR_SELECT_CHAIN, 0, 0, 0, priority, core.spe_effect[priority]);
		} else {
			core.new_chains.splice(core.new_chains.end(), core.tpchain);
			core.new_chains.splice(core.new_chains.end(), core.ntpchain);
			add_process(PROCESSOR_ADD_CHAIN, 0, 0, 0, 0, 0);
			add_process(PROCESSOR_QUICK_EFFECT, 0, 0, 0, FALSE, 1 - core.new_chains.back().triggering_player);
			infos.priorities[0] = 0;
			infos.priorities[1] = 0;
			return TRUE;
		}
		return FALSE;
	}
	case 2: {
		if(core.select_chains.size() && returns.ivalue[0] != -1) {
			chain newchain = core.select_chains[returns.ivalue[0]];
			core.new_chains.push_back(newchain);
			effect* peffect = newchain.triggering_effect;
			core.delayed_quick.erase(std::make_pair(peffect, newchain.evt));
			peffect->get_handler()->set_status(STATUS_CHAINING, TRUE);
			peffect->dec_count(priority);
			add_process(PROCESSOR_ADD_CHAIN, 0, 0, 0, 0, 0);
			add_process(PROCESSOR_QUICK_EFFECT, 0, 0, 0, FALSE, 1 - priority);
			infos.priorities[0] = 0;
			infos.priorities[1] = 0;
		} else {
			infos.priorities[priority] = 1;
			if(!infos.priorities[0] || !infos.priorities[1])
				add_process(PROCESSOR_QUICK_EFFECT, 1, 0, 0, skip_freechain, 1 - priority);
			else {
				core.hint_timing[0] &= TIMING_DAMAGE_STEP | TIMING_DAMAGE_CAL;
				core.hint_timing[1] &= TIMING_DAMAGE_STEP | TIMING_DAMAGE_CAL;
				core.delayed_quick.clear();
			}
		}
		core.select_chains.clear();
		return TRUE;
	}
	}
	return TRUE;
}
// classify core.queue_event, process continuous effects, and move them to core.instant_event
int32 field::process_instant_event() {
	if (core.queue_event.size() == 0)
		return TRUE;
	chain newchain;
	effect_vector tp;
	effect_vector ntp;
	event_list tev;
	event_list ntev;
	effect_vector::iterator eit;
	event_list::iterator evit;
	for(auto elit = core.queue_event.begin(); elit != core.queue_event.end(); ++elit) {
		//continuous events
		auto pr = effects.continuous_effect.equal_range(elit->event_code);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			uint8 owner_player = peffect->get_handler_player();
			if(peffect->is_activateable(owner_player, *elit)) {
				if(peffect->is_flag(EFFECT_FLAG_DELAY) && (core.chain_solving || core.conti_solving)) {
					if(owner_player == infos.turn_player) {
						core.delayed_tp.push_back(peffect);
						core.delayed_tev.push_back(*elit);
					} else {
						core.delayed_ntp.push_back(peffect);
						core.delayed_ntev.push_back(*elit);
					}
				} else {
					if(owner_player == infos.turn_player) {
						tp.push_back(peffect);
						tev.push_back(*elit);
					} else {
						ntp.push_back(peffect);
						ntev.push_back(*elit);
					}
				}
			}
		}
		if(elit->event_code == EVENT_ADJUST || ((elit->event_code & 0xf000) == EVENT_PHASE_START))
			continue;
		//triggers
		pr = effects.trigger_f_effect.equal_range(elit->event_code);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			card* phandler = peffect->get_handler();
			if(!phandler->is_status(STATUS_EFFECT_ENABLED) || !peffect->is_condition_check(phandler->current.controler, *elit))
				continue;
			peffect->set_activate_location();
			newchain.flag = 0;
			newchain.chain_id = infos.field_id++;
			newchain.evt = *elit;
			newchain.triggering_effect = peffect;
			newchain.set_triggering_place(phandler);
			if(peffect->is_flag(EFFECT_FLAG_EVENT_PLAYER) && (elit->event_player == 0 || elit->event_player == 1))
				newchain.triggering_player = elit->event_player;
			else newchain.triggering_player = phandler->current.controler;
			core.new_fchain.push_back(newchain);
			phandler->create_relation(newchain);
		}
		pr = effects.trigger_o_effect.equal_range(elit->event_code);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			card* phandler = peffect->get_handler();
			if(!phandler->is_status(STATUS_EFFECT_ENABLED) || !peffect->is_condition_check(phandler->current.controler, *elit))
				continue;
			peffect->set_activate_location();
			newchain.flag = 0;
			newchain.chain_id = infos.field_id++;
			newchain.evt = *elit;
			newchain.triggering_effect = peffect;
			newchain.set_triggering_place(phandler);
			if(peffect->is_flag(EFFECT_FLAG_EVENT_PLAYER) && (elit->event_player == 0 || elit->event_player == 1))
				newchain.triggering_player = elit->event_player;
			else newchain.triggering_player = phandler->current.controler;
			core.new_ochain.push_back(newchain);
			if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY)
			        || ((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE))
			        || !(peffect->range & LOCATION_HAND) || (peffect->range & phandler->current.location))
				phandler->create_relation(newchain);
		}
		//instant_f
		pr = effects.quick_f_effect.equal_range(elit->event_code);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			card* phandler = peffect->get_handler();
			peffect->set_activate_location();
			if(peffect->is_activateable(phandler->current.controler, *elit)) {
				newchain.flag = 0;
				newchain.chain_id = infos.field_id++;
				newchain.evt = *elit;
				newchain.triggering_effect = peffect;
				newchain.set_triggering_place(phandler);
				if(peffect->is_flag(EFFECT_FLAG_EVENT_PLAYER) && (elit->event_player == 0 || elit->event_player == 1))
					newchain.triggering_player = elit->event_player;
				else newchain.triggering_player = phandler->current.controler;
				core.quick_f_chain[peffect] = newchain;
				phandler->create_relation(newchain);
			}
		}
		// delayed activate effect
		core.full_event.push_back(*elit);
		// delayed quick effect
		pr = effects.quick_o_effect.equal_range(elit->event_code);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			if(peffect->is_flag(EFFECT_FLAG_DELAY) && peffect->is_condition_check(peffect->get_handler()->current.controler, *elit))
				core.delayed_quick_tmp.emplace(peffect, *elit);
		}
	}
	for(eit = tp.begin(), evit = tev.begin(); eit != tp.end(); ++eit, ++evit) {
		core.sub_solving_event.push_back(*evit);
		add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, (*eit), 0, (*eit)->get_handler_player(), 0);
	}
	for(eit = ntp.begin(), evit = ntev.begin(); eit != ntp.end(); ++eit, ++evit) {
		core.sub_solving_event.push_back(*evit);
		add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, (*eit), 0, (*eit)->get_handler_player(), 0);
	}
	core.instant_event.splice(core.instant_event.end(), core.queue_event);
	return TRUE;
}
int32 field::process_single_event() {
	if(core.single_event.size() == 0)
		return TRUE;
	effect_vector tp;
	effect_vector ntp;
	event_list tev;
	event_list ntev;
	for(auto elit = core.single_event.begin(); elit != core.single_event.end(); ++elit) {
		card* starget = elit->trigger_card;
		uint32 ev = elit->event_code;
		auto pr = starget->single_effect.equal_range(ev);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			process_single_event(peffect, *elit, tp, ntp, tev, ntev);
		}
		for(auto ovit = starget->xyz_materials.begin(); ovit != starget->xyz_materials.end(); ++ovit) {
			pr = (*ovit)->xmaterial_effect.equal_range(ev);
			for(auto eit = pr.first; eit != pr.second;) {
				effect* peffect = eit->second;
				++eit;
				if(peffect->type & EFFECT_TYPE_FIELD)
					continue;
				process_single_event(peffect, *elit, tp, ntp, tev, ntev);
			}
		}
	}
	effect_vector::iterator eit;
	event_list::iterator evit;
	for(eit = tp.begin(), evit = tev.begin(); eit != tp.end(); ++eit, ++evit) {
		core.sub_solving_event.push_back(*evit);
		add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, (*eit), 0, (*eit)->get_handler_player(), 0);
	}
	for(eit = ntp.begin(), evit = ntev.begin(); eit != ntp.end(); ++eit, ++evit) {
		core.sub_solving_event.push_back(*evit);
		add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, (*eit), 0, (*eit)->get_handler_player(), 0);
	}
	core.single_event.clear();
	return TRUE;
}
int32 field::process_single_event(effect* peffect, const tevent& e, effect_vector& tp, effect_vector& ntp, event_list& tev, event_list& ntev) {
	if(!(peffect->type & EFFECT_TYPE_ACTIONS))
		return FALSE;
	if((peffect->type & EFFECT_TYPE_FLIP) && (e.event_value & (NO_FLIP_EFFECT >> 16)))
		return FALSE;
	//continuous & trigger (single)
	if(peffect->type & EFFECT_TYPE_CONTINUOUS) {
		uint8 owner_player = peffect->get_handler_player();
		if(peffect->is_activateable(owner_player, e)) {
			if(peffect->is_flag(EFFECT_FLAG_DELAY) && (core.chain_solving || core.conti_solving)) {
				if(owner_player == infos.turn_player) {
					core.delayed_tp.push_back(peffect);
					core.delayed_tev.push_back(e);
				} else {
					core.delayed_ntp.push_back(peffect);
					core.delayed_ntev.push_back(e);
				}
			} else {
				if(owner_player == infos.turn_player) {
					tp.push_back(peffect);
					tev.push_back(e);
				} else {
					ntp.push_back(peffect);
					ntev.push_back(e);
				}
			}
		}
	} else {
		card* phandler = peffect->get_handler();
		if(!peffect->is_condition_check(phandler->current.controler, e))
			return FALSE;
		peffect->set_activate_location();
		chain newchain;
		newchain.flag = 0;
		newchain.chain_id = infos.field_id++;
		newchain.evt = e;
		newchain.triggering_effect = peffect;
		newchain.set_triggering_place(phandler);
		if(peffect->is_flag(EFFECT_FLAG_EVENT_PLAYER) && (e.event_player == 0 || e.event_player == 1))
			newchain.triggering_player = e.event_player;
		else {
			if(phandler->current.reason & REASON_TEMPORARY)
				newchain.triggering_player = phandler->previous.controler;
			else
				newchain.triggering_player = newchain.triggering_controler;
		}
		if(core.flip_delayed && e.event_code == EVENT_FLIP) {
			if (peffect->type & EFFECT_TYPE_TRIGGER_O)
				core.new_ochain_b.push_back(newchain);
			else
				core.new_fchain_b.push_back(newchain);
		} else {
			if (peffect->type & EFFECT_TYPE_TRIGGER_O)
				core.new_ochain.push_back(newchain);
			else
				core.new_fchain.push_back(newchain);
		}
		phandler->create_relation(newchain);
		effect* deffect;
		if(deffect = phandler->is_affected_by_effect(EFFECT_DISABLE_EFFECT)) {
			effect* negeff = pduel->new_effect();
			negeff->owner = deffect->owner;
			negeff->type = EFFECT_TYPE_SINGLE;
			negeff->code = EFFECT_DISABLE_CHAIN;
			negeff->value = newchain.chain_id;
			negeff->reset_flag = RESET_EVENT | deffect->get_value();
			phandler->add_effect(negeff);
		}
	}
	return TRUE;
}
int32 field::process_idle_command(uint16 step) {
	switch(step) {
	case 0: {
		effect* peffect;
		bool must_attack = false;
		core.select_chains.clear();
		chain newchain;
		nil_event.event_code = EVENT_FREE_CHAIN;
		core.to_bp = TRUE;
		core.to_ep = TRUE;
		if((!(core.duel_options & DUEL_ATTACK_FIRST_TURN) && infos.turn_id == 1) || infos.phase == PHASE_MAIN2 || is_player_affected_by_effect(infos.turn_player, EFFECT_CANNOT_BP))
			core.to_bp = FALSE;
		if(infos.phase == PHASE_MAIN1) {
			for(auto cit = player[infos.turn_player].list_mzone.begin(); cit != player[infos.turn_player].list_mzone.end(); ++cit) {
				card* pcard = *cit;
				if(pcard && pcard->is_capable_attack() && pcard->is_affected_by_effect(EFFECT_MUST_ATTACK)) {
					must_attack = true;
					break;
				}
			}
			if(core.to_bp && (must_attack || is_player_affected_by_effect(infos.turn_player, EFFECT_CANNOT_EP)))
				core.to_ep = FALSE;
		}
		if((infos.phase == PHASE_MAIN1 && is_player_affected_by_effect(infos.turn_player, EFFECT_SKIP_M1))
		        || (infos.phase == PHASE_MAIN2 && is_player_affected_by_effect(infos.turn_player, EFFECT_SKIP_M2))) {
			if(core.to_bp && core.to_ep) {
				core.select_options.clear();
				core.select_options.push_back(80);
				core.select_options.push_back(81);
				add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, infos.turn_player, 0);
				core.units.begin()->step = 11;
			} else if(core.to_bp) {
				core.units.begin()->arg1 = 6;
				core.units.begin()->step = 10;
				reset_phase(infos.phase);
				adjust_all();
			} else {
				core.units.begin()->arg1 = 7;
				core.units.begin()->step = 10;
				reset_phase(infos.phase);
				adjust_all();
			}
			return FALSE;
		}
		if((infos.phase == PHASE_MAIN2) && core.skip_m2) {
			core.skip_m2 = FALSE;
			returns.ivalue[0] = 7;
			return FALSE;
		}
		auto pr = effects.activate_effect.equal_range(EVENT_FREE_CHAIN);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			peffect->set_activate_location();
			newchain.triggering_effect = peffect;
			if(peffect->is_activateable(infos.turn_player, nil_event))
				core.select_chains.push_back(newchain);
		}
		pr = effects.quick_o_effect.equal_range(EVENT_FREE_CHAIN);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			peffect->set_activate_location();
			newchain.triggering_effect = peffect;
			if(peffect->is_activateable(infos.turn_player, nil_event))
				core.select_chains.push_back(newchain);
		}
		pr = effects.continuous_effect.equal_range(EVENT_FREE_CHAIN);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			newchain.triggering_effect = peffect;
			if(peffect->get_handler_player() == infos.turn_player && peffect->is_activateable(infos.turn_player, nil_event))
				core.select_chains.push_back(newchain);
		}
		for(auto eit = effects.ignition_effect.begin(); eit != effects.ignition_effect.end();) {
			peffect = eit->second;
			++eit;
			peffect->set_activate_location();
			newchain.triggering_effect = peffect;
			if(peffect->is_activateable(infos.turn_player, nil_event))
				core.select_chains.push_back(newchain);
		}
		core.summonable_cards.clear();
		for(auto clit = player[infos.turn_player].list_hand.begin(); clit != player[infos.turn_player].list_hand.end(); ++clit)
			if((*clit)->is_can_be_summoned(infos.turn_player, FALSE, 0, 0))
				core.summonable_cards.push_back(*clit);
		for(auto clit = player[infos.turn_player].list_mzone.begin(); clit != player[infos.turn_player].list_mzone.end(); ++clit) {
			card* pcard = *clit;
			if(pcard && pcard->is_can_be_summoned(infos.turn_player, FALSE, 0, 0))
				core.summonable_cards.push_back(pcard);
		}
		core.spsummonable_cards.clear();
		effect_set eset;
		filter_field_effect(EFFECT_SPSUMMON_PROC, &eset);
		for(int32 i = 0; i < eset.size(); ++i) {
			card* pcard = eset[i]->get_handler();
			if(!eset[i]->check_count_limit(pcard->current.controler))
				continue;
			if(pcard->current.controler == infos.turn_player && pcard->is_special_summonable(infos.turn_player, 0))
				core.spsummonable_cards.push_back(pcard);
		}
		eset.clear();
		filter_field_effect(EFFECT_SPSUMMON_PROC_G, &eset);
		for(int32 i = 0; i < eset.size(); ++i) {
			card* pcard = eset[i]->get_handler();
			if(!eset[i]->check_count_limit(infos.turn_player))
				continue;
			if(pcard->current.controler != infos.turn_player && !eset[i]->is_flag(EFFECT_FLAG_BOTH_SIDE))
				continue;
			effect* oreason = core.reason_effect;
			uint8 op = core.reason_player;
			core.reason_effect = eset[i];
			core.reason_player = pcard->current.controler;
			save_lp_cost();
			pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
			pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
			if(pduel->lua->check_condition(eset[i]->condition, 2))
				core.spsummonable_cards.push_back(pcard);
			restore_lp_cost();
			core.reason_effect = oreason;
			core.reason_player = op;
		}
		core.repositionable_cards.clear();
		for(auto clit = player[infos.turn_player].list_mzone.begin(); clit != player[infos.turn_player].list_mzone.end(); ++clit) {
			card* pcard = *clit;
			if(pcard && ((pcard->is_position(POS_FACEUP | POS_FACEDOWN_ATTACK) && pcard->is_capable_change_position(infos.turn_player))
		        || (pcard->is_position(POS_FACEDOWN) && pcard->is_can_be_flip_summoned(infos.turn_player))))
				core.repositionable_cards.push_back(pcard);
		}
		core.msetable_cards.clear();
		core.ssetable_cards.clear();
		for(auto clit = player[infos.turn_player].list_hand.begin(); clit != player[infos.turn_player].list_hand.end(); ++clit) {
			if((*clit)->is_setable_mzone(infos.turn_player, FALSE, 0, 0))
				core.msetable_cards.push_back(*clit);
			if((*clit)->is_setable_szone(infos.turn_player))
				core.ssetable_cards.push_back(*clit);
		}
		add_process(PROCESSOR_SELECT_IDLECMD, 0, 0, 0, infos.turn_player, 0);
		return FALSE;
	}
	case 1: {
		uint32 ctype = returns.ivalue[0] & 0xffff;
		uint32 sel = returns.ivalue[0] >> 16;
		if(ctype == 5) {
			chain newchain = core.select_chains[sel];
			effect* peffect = newchain.triggering_effect;
			if(peffect->type & EFFECT_TYPE_CONTINUOUS) {
				core.select_chains.clear();
				core.sub_solving_event.push_back(nil_event);
				add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, peffect, 0, peffect->get_handler_player(), 0);
				core.units.begin()->step = -1;
				return FALSE;
			}
			card* phandler = peffect->get_handler();
			newchain.flag = 0;
			newchain.chain_id = infos.field_id++;
			newchain.evt.event_code = peffect->code;
			newchain.evt.event_player = PLAYER_NONE;
			newchain.evt.event_value = 0;
			newchain.evt.event_cards = 0;
			newchain.evt.reason = 0;
			newchain.evt.reason_effect = 0;
			newchain.evt.reason_player = PLAYER_NONE;
			newchain.set_triggering_place(phandler);
			newchain.triggering_player = infos.turn_player;
			core.new_chains.push_back(newchain);
			phandler->set_status(STATUS_CHAINING, TRUE);
			peffect->dec_count(infos.turn_player);
			core.select_chains.clear();
			add_process(PROCESSOR_ADD_CHAIN, 0, 0, 0, 0, 0);
			add_process(PROCESSOR_QUICK_EFFECT, 0, 0, 0, FALSE, 1 - infos.turn_player);
			infos.priorities[0] = 0;
			infos.priorities[1] = 0;
			core.select_chains.clear();
			return FALSE;
		} else if(ctype == 0) {
			core.units.begin()->step = 4;
			return FALSE;
		} else if(ctype == 1) {
			core.units.begin()->step = 5;
			return FALSE;
		} else if(ctype == 2) {
			core.units.begin()->step = 6;
			return FALSE;
		} else if(ctype == 3) {
			core.units.begin()->step = 7;
			return FALSE;
		} else if(ctype == 4) {
			core.units.begin()->step = 8;
			return FALSE;
		} else if (ctype == 8) {
			core.units.begin()->step = -1;
			shuffle(infos.turn_player, LOCATION_HAND);
			infos.can_shuffle = FALSE;
			return FALSE;
		} else {
			core.units.begin()->step = 9;
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_EVENT);
			pduel->write_buffer8(1 - infos.turn_player);
			pduel->write_buffer32(23);
			core.select_chains.clear();
			core.hint_timing[infos.turn_player] = TIMING_MAIN_END;
			add_process(PROCESSOR_QUICK_EFFECT, 0, 0, 0, FALSE, 1 - infos.turn_player);
			infos.priorities[infos.turn_player] = 1;
			infos.priorities[1 - infos.turn_player] = 0;
			core.units.begin()->arg1 = ctype;
			return FALSE;
		}
		return TRUE;
	}
	case 2: {
		for(auto it = core.chain_limit.begin(); it != core.chain_limit.end(); ++it)
			luaL_unref(pduel->lua->lua_state, LUA_REGISTRYINDEX, it->function);
		core.chain_limit.clear();
		for(auto cait = core.current_chain.begin(); cait != core.current_chain.end(); ++cait)
			cait->triggering_effect->get_handler()->set_status(STATUS_CHAINING, FALSE);
		add_process(PROCESSOR_SOLVE_CHAIN, 0, 0, 0, FALSE, 0);
		core.units.begin()->step = -1;
		return FALSE;
	}
	case 5: {
		card* target = core.summonable_cards[returns.ivalue[0] >> 16];
		core.summon_cancelable = TRUE;
		summon(infos.turn_player, target, 0, FALSE, 0);
		core.units.begin()->step = -1;
		return FALSE;
	}
	case 6: {
		card* target = core.spsummonable_cards[returns.ivalue[0] >> 16];
		core.summon_cancelable = TRUE;
		special_summon_rule(infos.turn_player, target, 0);
		core.units.begin()->step = -1;
		return FALSE;
	}
	case 7: {
		card* target = core.repositionable_cards[returns.ivalue[0] >> 16];
		if(target->is_position(POS_FACEUP_ATTACK)) {
			core.phase_action = TRUE;
			change_position(target, 0, infos.turn_player, POS_FACEUP_DEFENSE, FALSE);
			adjust_all();
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, 0);
		} else if(target->is_position(POS_FACEUP_DEFENSE)) {
			core.phase_action = TRUE;
			change_position(target, 0, infos.turn_player, POS_FACEUP_ATTACK, FALSE);
			adjust_all();
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, 0);
		} else if(target->is_position(POS_FACEDOWN_ATTACK)) {
			core.units.begin()->ptarget = (group*)target;
			int32 positions = 0;
			if(target->is_capable_change_position(infos.turn_player))
				positions |= POS_FACEDOWN_DEFENSE;
			if(target->is_can_be_flip_summoned(infos.turn_player))
				positions |= POS_FACEUP_ATTACK;
			add_process(PROCESSOR_SELECT_POSITION, 0, 0, 0, infos.turn_player + (positions << 16), target->data.code);
			core.units.begin()->step = 12;
			return FALSE;
		} else
			add_process(PROCESSOR_FLIP_SUMMON, 0, 0, (group*)target, target->current.controler, 0);
		target->set_status(STATUS_FORM_CHANGED, TRUE);
		core.units.begin()->step = -1;
		return FALSE;
	}
	case 8: {
		card* target = core.msetable_cards[returns.ivalue[0] >> 16];
		core.summon_cancelable = TRUE;
		mset(target->current.controler, target, 0, FALSE, 0);
		core.units.begin()->step = -1;
		return FALSE;
	}
	case 9: {
		card* target = core.ssetable_cards[returns.ivalue[0] >> 16];
		add_process(PROCESSOR_SSET, 0, 0, (group*)target, target->current.controler, target->current.controler);
		core.units.begin()->step = -1;
		return FALSE;
	}
	case 10: {
		//end announce
		for(auto it = core.chain_limit.begin(); it != core.chain_limit.end(); ++it)
			luaL_unref(pduel->lua->lua_state, LUA_REGISTRYINDEX, it->function);
		core.chain_limit.clear();
		if(core.current_chain.size()) {
			for(auto cait = core.current_chain.begin(); cait != core.current_chain.end(); ++cait)
				cait->triggering_effect->get_handler()->set_status(STATUS_CHAINING, FALSE);
			add_process(PROCESSOR_SOLVE_CHAIN, 0, 0, 0, FALSE, 0);
			core.units.begin()->step = -1;
			return FALSE;
		}
		reset_phase(infos.phase);
		adjust_all();
		return FALSE;
	}
	case 11: {
		returns.ivalue[0] = core.units.begin()->arg1;
		infos.can_shuffle = TRUE;
		return TRUE;
	}
	case 12: {
		if(returns.ivalue[0] == 0)
			core.units.begin()->arg1 = 6;
		else
			core.units.begin()->arg1 = 7;
		reset_phase(infos.phase);
		adjust_all();
		core.units.begin()->step = 10;
		return FALSE;
	}
	case 13: {
		card* target = (card*)core.units.begin()->ptarget;
		if(returns.ivalue[0] == POS_FACEUP_ATTACK)
			add_process(PROCESSOR_FLIP_SUMMON, 0, 0, (group*)target, target->current.controler, 0);
		else {
			core.phase_action = TRUE;
			change_position(target, 0, infos.turn_player, POS_FACEDOWN_DEFENSE, FALSE);
			adjust_all();
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, 0);
		}
		target->set_status(STATUS_FORM_CHANGED, TRUE);
		core.units.begin()->step = -1;
		return FALSE;
	}
	}
	return TRUE;
}
// core.attack_state_count: Duel.GetActivityCount()
int32 field::process_battle_command(uint16 step) {
	switch(step) {
	case 0: {
		effect* peffect = 0;
		core.select_chains.clear();
		chain newchain;
		nil_event.event_code = EVENT_FREE_CHAIN;
		if(!core.chain_attack) {
			core.chain_attacker_id = 0;
			core.chain_attack_target = 0;
		}
		core.attack_player = FALSE;
		core.attacker = 0;
		core.attack_target = 0;
		if((peffect = is_player_affected_by_effect(infos.turn_player, EFFECT_SKIP_BP))) {
			core.units.begin()->step = 41;
			core.units.begin()->arg1 = 2;
			if(is_player_affected_by_effect(infos.turn_player, EFFECT_BP_TWICE))
				core.units.begin()->arg2 = 1;
			else core.units.begin()->arg2 = 0;
			if(!peffect->value) {
				infos.phase = PHASE_BATTLE;
				add_process(PROCESSOR_PHASE_EVENT, 0, 0, 0, PHASE_BATTLE, 0);
			} else {
				core.hint_timing[infos.turn_player] = 0;
				reset_phase(PHASE_BATTLE);
				adjust_all();
			}
			return FALSE;
		}
		auto pr = effects.activate_effect.equal_range(EVENT_FREE_CHAIN);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			peffect->set_activate_location();
			newchain.triggering_effect = peffect;
			if(peffect->is_activateable(infos.turn_player, nil_event) && peffect->get_speed() > 1)
				core.select_chains.push_back(newchain);
		}
		pr = effects.quick_o_effect.equal_range(EVENT_FREE_CHAIN);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			peffect->set_activate_location();
			newchain.triggering_effect = peffect;
			if(peffect->is_activateable(infos.turn_player, nil_event))
				core.select_chains.push_back(newchain);
		}
		pr = effects.continuous_effect.equal_range(EVENT_FREE_CHAIN);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			newchain.triggering_effect = peffect;
			if(peffect->get_handler_player() == infos.turn_player && peffect->is_activateable(infos.turn_player, nil_event))
				core.select_chains.push_back(newchain);
		}
		core.attackable_cards.clear();
		card_vector first_attack;
		card_vector must_attack;
		if(!is_player_affected_by_effect(infos.turn_player, EFFECT_CANNOT_ATTACK_ANNOUNCE)) {
			for(auto cit = player[infos.turn_player].list_mzone.begin(); cit != player[infos.turn_player].list_mzone.end(); ++cit) {
				card* pcard = *cit;
				if(!pcard)
					continue;
				if(!pcard->is_capable_attack_announce(infos.turn_player))
					continue;
				uint8 chain_attack = FALSE;
				if(core.chain_attack && core.chain_attacker_id == pcard->fieldid)
					chain_attack = TRUE;
				card_vector cv;
				get_attack_target(pcard, &cv, chain_attack);
				if(cv.size() == 0 && pcard->direct_attackable == 0)
					continue;
				core.attackable_cards.push_back(pcard);
				if(pcard->is_affected_by_effect(EFFECT_FIRST_ATTACK))
					first_attack.push_back(pcard);
				if(pcard->is_affected_by_effect(EFFECT_MUST_ATTACK))
					must_attack.push_back(pcard);
			}
			if(first_attack.size())
				core.attackable_cards = first_attack;
		}
		core.to_m2 = TRUE;
		core.to_ep = TRUE;
		if(must_attack.size() || is_player_affected_by_effect(infos.turn_player, EFFECT_CANNOT_M2))
			core.to_m2 = FALSE;
		if(must_attack.size())
			core.to_ep = FALSE;
		core.attack_cancelable = TRUE;
		add_process(PROCESSOR_SELECT_BATTLECMD, 0, 0, 0, infos.turn_player, 0);
		return FALSE;
	}
	case 1: {
		int32 ctype = returns.ivalue[0] & 0xffff;
		int32 sel = returns.ivalue[0] >> 16;
		if(ctype == 0) {
			chain newchain = core.select_chains[sel];
			effect* peffect = newchain.triggering_effect;
			if(peffect->type & EFFECT_TYPE_CONTINUOUS) {
				core.select_chains.clear();
				core.sub_solving_event.push_back(nil_event);
				add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, peffect, 0, peffect->get_handler_player(), 0);
				core.units.begin()->step = -1;
				return FALSE;
			}
			card* phandler = peffect->get_handler();
			newchain.flag = 0;
			newchain.chain_id = infos.field_id++;
			newchain.evt.event_code = peffect->code;
			newchain.evt.event_player = PLAYER_NONE;
			newchain.evt.event_value = 0;
			newchain.evt.event_cards = 0;
			newchain.evt.reason = 0;
			newchain.evt.reason_effect = 0;
			newchain.evt.reason_player = PLAYER_NONE;
			newchain.set_triggering_place(phandler);
			newchain.triggering_player = infos.turn_player;
			core.new_chains.push_back(newchain);
			phandler->set_status(STATUS_CHAINING, TRUE);
			peffect->dec_count(infos.turn_player);
			core.select_chains.clear();
			add_process(PROCESSOR_ADD_CHAIN, 0, 0, 0, 0, 0);
			add_process(PROCESSOR_QUICK_EFFECT, 0, 0, 0, FALSE, 1 - infos.turn_player);
			infos.priorities[0] = 0;
			infos.priorities[1] = 0;
			core.select_chains.clear();
			return FALSE;
		} else if(ctype == 1) {
			core.units.begin()->step = 2;
			core.units.begin()->arg3 = FALSE;
			card* attacker = core.attackable_cards[sel];
			core.attacker = attacker;
			core.attacker->set_status(STATUS_ATTACK_CANCELED, FALSE);
			core.pre_field[0] = core.attacker->fieldid_r;
			effect_set eset;
			filter_player_effect(infos.turn_player, EFFECT_ATTACK_COST, &eset, FALSE);
			core.attacker->filter_effect(EFFECT_ATTACK_COST, &eset);
			for(int32 i = 0; i < eset.size(); ++i) {
				if(eset[i]->operation) {
					core.attack_cancelable = FALSE;
					core.sub_solving_event.push_back(nil_event);
					add_process(PROCESSOR_EXECUTE_OPERATION, 0, eset[i], 0, infos.turn_player, 0);
					adjust_all();
				}
			}
			return FALSE;
		} else {
			core.units.begin()->step = 39;
			core.units.begin()->arg1 = ctype;
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_EVENT);
			pduel->write_buffer8(1 - infos.turn_player);
			pduel->write_buffer32(29);
			core.select_chains.clear();
			core.hint_timing[infos.turn_player] = TIMING_BATTLE_STEP_END;
			add_process(PROCESSOR_QUICK_EFFECT, 0, 0, 0, FALSE, 1 - infos.turn_player);
			infos.priorities[infos.turn_player] = 1;
			infos.priorities[1 - infos.turn_player] = 0;
			return FALSE;
		}
		return TRUE;
	}
	case 2: {
		for(auto it = core.chain_limit.begin(); it != core.chain_limit.end(); ++it)
			luaL_unref(pduel->lua->lua_state, LUA_REGISTRYINDEX, it->function);
		core.chain_limit.clear();
		for(auto cait = core.current_chain.begin(); cait != core.current_chain.end(); ++cait)
			cait->triggering_effect->get_handler()->set_status(STATUS_CHAINING, FALSE);
		add_process(PROCESSOR_SOLVE_CHAIN, 0, 0, 0, FALSE, 0);
		core.units.begin()->step = -1;
		return FALSE;
	}
	case 3: {
		core.units.begin()->arg1 = FALSE;
		if(core.attacker->current.location != LOCATION_MZONE || core.attacker->current.controler != infos.turn_player || core.attacker->fieldid_r != core.pre_field[0]) {
			core.units.begin()->arg3 = TRUE;
			core.units.begin()->step = 6;
			return FALSE;
		}
		return FALSE;
	}
	case 4: {
		// select attack target(replay start point)
		core.attack_player = FALSE;
		core.select_cards.clear();
		auto atype = get_attack_target(core.attacker, &core.select_cards, core.chain_attack);
		// direct attack
		if(core.attacker->direct_attackable) {
			if(core.select_cards.size() == 0) {
				returns.ivalue[0] = -2;
				core.units.begin()->step = 5;
				return FALSE;
			}
			for(auto cit = player[1 - infos.turn_player].list_mzone.begin(); cit != player[1 - infos.turn_player].list_mzone.end(); ++cit) {
				if(*cit) {
					add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, infos.turn_player, 31);
					return FALSE;
				}
			}
			returns.ivalue[0] = -2;
			core.units.begin()->step = 5;
			return FALSE;
		}
		// no target and not direct attackable
		if(core.select_cards.size() == 0) {
			core.units.begin()->arg3 = TRUE;
			core.units.begin()->step = 6;
			return FALSE;
		}
		// must attack monster
		if(atype == 3 || is_player_affected_by_effect(infos.turn_player, EFFECT_PATRICIAN_OF_DARKNESS)) {
			if(core.select_cards.size() == 1)
				returns.bvalue[1] = 0;
			else {
				pduel->write_buffer8(MSG_BECOME_TARGET);
				pduel->write_buffer8(1);
				pduel->write_buffer32(core.attacker->get_info_location());
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_SELECTMSG);
				pduel->write_buffer8(1 - infos.turn_player);
				pduel->write_buffer32(549);
				add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, 1 - infos.turn_player, 0x10001);
			}
		} else {
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_SELECTMSG);
			pduel->write_buffer8(infos.turn_player);
			pduel->write_buffer32(549);
			add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, infos.turn_player + (core.attack_cancelable ? 0x20000 : 0), 0x10001);
		}
		core.units.begin()->step = 5;
		return FALSE;
	}
	case 5: {
		// the answer of "direct attack or not"
		if(returns.ivalue[0]) {
			returns.ivalue[0] = -2;
		} else {
			if(core.select_cards.size()) {
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_SELECTMSG);
				pduel->write_buffer8(infos.turn_player);
				pduel->write_buffer32(549);
				add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, infos.turn_player + (core.attack_cancelable ? 0x20000 : 0), 0x10001);
			} else {
				core.units.begin()->arg3 = TRUE;
				core.units.begin()->step = 6;
			}
		}
		return FALSE;
	}
	case 6: {
		if(returns.ivalue[0] == -1) {//cancel attack manually
			if(core.units.begin()->arg1)
				core.chain_attack = FALSE;
			core.units.begin()->step = -1;
			return FALSE;
		}
		if(returns.ivalue[0] == -2)
			core.attack_target = 0;
		else
			core.attack_target = core.select_cards[returns.bvalue[1]];
		if(core.attack_target)
			core.pre_field[1] = core.attack_target->fieldid_r;
		else
			core.pre_field[1] = 0;
		return FALSE;
	}
	case 7: {
		if(!core.units.begin()->arg1) {
			core.phase_action = TRUE;
			if(core.chain_attack && core.chain_attacker_id != core.attacker->fieldid) {
				core.chain_attack = FALSE;
				core.chain_attacker_id = 0;
			}
			core.attack_state_count[infos.turn_player]++;
			check_card_counter(core.attacker, 5, infos.turn_player);
			core.attacker->announce_count++;
			if(!core.units.begin()->arg3)
				core.attacker->announced_cards.addcard(core.attack_target);
		}
		if(core.units.begin()->arg3) {
			core.chain_attack = FALSE;
			core.units.begin()->step = -1;
		}
		return FALSE;
	}
	case 8: {
		bool evt = false;
		attack_all_target_check();
		pduel->write_buffer8(MSG_ATTACK);
		pduel->write_buffer32(core.attacker->get_info_location());
		if(core.attack_target) {
			evt = true;
			raise_single_event(core.attack_target, 0, EVENT_BE_BATTLE_TARGET, 0, 0, 0, 1 - infos.turn_player, 0);
			raise_event(core.attack_target, EVENT_BE_BATTLE_TARGET, 0, 0, 0, 1 - infos.turn_player, 0);
			pduel->write_buffer32(core.attack_target->get_info_location());
		} else
			pduel->write_buffer32(0);
		core.attack_rollback = FALSE;
		core.opp_mzone.clear();
		for(uint32 i = 0; i < player[1 - infos.turn_player].list_mzone.size(); ++i) {
			card* pcard = player[1 - infos.turn_player].list_mzone[i];
			if(pcard)
				core.opp_mzone.insert(pcard->fieldid_r);
		}
		//core.units.begin()->arg1 ---> is rollbacked
		if(!core.units.begin()->arg1) {
			evt = true;
			raise_single_event(core.attacker, 0, EVENT_ATTACK_ANNOUNCE, 0, 0, 0, infos.turn_player, 0);
			raise_event(core.attacker, EVENT_ATTACK_ANNOUNCE, 0, 0, 0, infos.turn_player, 0);
		}
		core.attacker->attack_controler = core.attacker->current.controler;
		core.pre_field[0] = core.attacker->fieldid_r;
		if(evt) {
			process_single_event();
			process_instant_event();
		}
		core.hint_timing[infos.turn_player] = TIMING_ATTACK;
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, 0);
		return FALSE;
	}
	case 9: {
		if(is_player_affected_by_effect(infos.turn_player, EFFECT_SKIP_BP) || core.attack_rollback) {
			core.units.begin()->step = 10;
			return FALSE;
		}
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_EVENT);
		pduel->write_buffer8(0);
		pduel->write_buffer32(24);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_EVENT);
		pduel->write_buffer8(1);
		pduel->write_buffer32(24);
		core.hint_timing[0] = TIMING_BATTLE_PHASE;
		core.hint_timing[1] = TIMING_BATTLE_PHASE;
		add_process(PROCESSOR_POINT_EVENT, 30, 0, 0, 0, 0);
		return FALSE;
	}
	case 10: {
		if(returns.ivalue[0])
			core.units.begin()->step = 8;
		else
			adjust_all();
		return FALSE;
	}
	case 11: {
		uint8 rollback = core.attack_rollback;
		bool atk_disabled = false;
		uint32 acon = core.attacker->attack_controler;
		uint32 afid = core.pre_field[0];
		if(core.attacker->is_affected_by_effect(EFFECT_ATTACK_DISABLED)) {
			core.attacker->reset(EFFECT_ATTACK_DISABLED, RESET_CODE);
			atk_disabled = true;
			pduel->write_buffer8(MSG_ATTACK_DISABLED);
			core.attacker->set_status(STATUS_ATTACK_CANCELED, TRUE);
		}
		effect* peffect = is_player_affected_by_effect(infos.turn_player, EFFECT_SKIP_BP);
		if(peffect) {
			core.units.begin()->step = 41;
			core.units.begin()->arg1 = 2;
			if(is_player_affected_by_effect(infos.turn_player, EFFECT_BP_TWICE))
				core.units.begin()->arg2 = 1;
			else
				core.units.begin()->arg2 = 0;
			reset_phase(PHASE_DAMAGE);
			if(core.attacker->fieldid_r == afid && !atk_disabled) {
				core.attacker->attacked_cards.addcard(core.attack_target);
			}
			if(!peffect->value) {
				infos.phase = PHASE_BATTLE;
				add_process(PROCESSOR_PHASE_EVENT, 0, 0, 0, PHASE_BATTLE, 0);
			} else {
				core.hint_timing[infos.turn_player] = 0;
				reset_phase(PHASE_BATTLE);
				adjust_all();
			}
			return FALSE;
		}
		if(atk_disabled || !core.attacker->is_capable_attack() || core.attacker->is_status(STATUS_ATTACK_CANCELED)
		        || core.attacker->current.controler != acon || core.attacker->fieldid_r != afid) {
			core.chain_attack = FALSE;
			if(core.attacker->fieldid_r == afid && !atk_disabled) {
				core.attacker->attacked_cards.addcard(core.attack_target);
			}
			core.units.begin()->step = -1;
			reset_phase(PHASE_DAMAGE);
			adjust_all();
			return FALSE;
		}
		if(!confirm_attack_target())
			rollback = true;
		// go to damage step
		if(!rollback) {
			core.units.begin()->step = 18;
			return FALSE;
		}
		// attack canceled
		card_vector cv;
		get_attack_target(core.attacker, &cv, core.chain_attack);
		if(!cv.size() && !core.attacker->direct_attackable) {
			core.chain_attack = FALSE;
			core.units.begin()->step = -1;
			reset_phase(PHASE_DAMAGE);
			adjust_all();
			return FALSE;
		}
		// replay
		if(!core.attacker->is_affected_by_effect(EFFECT_MUST_ATTACK))
			add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, infos.turn_player, 30);
		else {
			returns.ivalue[0] = TRUE;
		}
		return FALSE;
	}
	case 12: {
		// answer of "replay or not"
		if(returns.ivalue[0]) {
			core.units.begin()->arg1 = TRUE;
			core.units.begin()->arg3 = FALSE;
			core.attack_cancelable = TRUE;
			core.units.begin()->step = 3;
			return FALSE;
		}
		core.chain_attack = FALSE;
		core.units.begin()->step = -1;
		reset_phase(PHASE_DAMAGE);
		adjust_all();
		return FALSE;
	}
	case 19: {
		infos.phase = PHASE_DAMAGE;
		core.chain_attack = FALSE;
		core.units.begin()->arg1 = FALSE;
		core.damage_calculated = FALSE;
		core.selfdes_disabled = TRUE;
		core.flip_delayed = TRUE;
		core.attacker->attack_controler = core.attacker->current.controler;
		core.pre_field[0] = core.attacker->fieldid_r;
		if(core.attack_target) {
			core.attack_target->attack_controler = core.attack_target->current.controler;
			core.pre_field[1] = core.attack_target->fieldid_r;
		} else
			core.pre_field[1] = 0;
		core.attacker->attacked_count++;
		core.attacker->attacked_cards.addcard(core.attack_target);
		core.battled_count[infos.turn_player]++;
		adjust_all();
		return FALSE;
	}
	case 20: {
		// start of PHASE_DAMAGE;
		pduel->write_buffer8(MSG_DAMAGE_STEP_START);
		raise_single_event(core.attacker, 0, EVENT_BATTLE_START, 0, 0, 0, 0, 0);
		if(core.attack_target) {
			raise_single_event(core.attack_target, 0, EVENT_BATTLE_START, 0, 0, 0, 0, 1);
		}
		raise_event((card*)0, EVENT_BATTLE_START, 0, 0, 0, 0, 0);
		process_single_event();
		process_instant_event();
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_EVENT);
		pduel->write_buffer8(0);
		pduel->write_buffer32(40);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_EVENT);
		pduel->write_buffer8(1);
		pduel->write_buffer32(40);
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, TRUE);
		return FALSE;
	}
	case 21: {
		if(core.attacker->current.location != LOCATION_MZONE || core.attacker->fieldid_r != core.pre_field[0]
		        || core.attacker->current.controler != core.attacker->attack_controler
		        || (core.attack_target && (core.attack_target->current.location != LOCATION_MZONE
		                                || core.attack_target->current.controler != core.attack_target->attack_controler
		                                || core.attack_target->fieldid_r != core.pre_field[1]))) {
			core.units.begin()->step = 32;
			return FALSE;
		}
		if(!core.attack_target) {
			return FALSE;
		}
		core.attacker->temp.position = core.attacker->current.position;
		core.attack_target->temp.position = core.attack_target->current.position;
		if(core.attack_target->is_position(POS_FACEDOWN)) {
			change_position(core.attack_target, 0, PLAYER_NONE, core.attack_target->current.position >> 1, 0, TRUE);
			adjust_all();
		}
		return FALSE;
	}
	case 22: {
		raise_single_event(core.attacker, 0, EVENT_BATTLE_CONFIRM, 0, 0, 0, 0, 0);
		if(core.attack_target) {
			if(core.attack_target->temp.position & POS_FACEDOWN)
				core.pre_field[1] = core.attack_target->fieldid_r;
			raise_single_event(core.attack_target, 0, EVENT_BATTLE_CONFIRM, 0, 0, 0, 0, 1);
		}
		raise_event((card*)0, EVENT_BATTLE_CONFIRM, 0, 0, 0, 0, 0);
		process_single_event();
		process_instant_event();
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_EVENT);
		pduel->write_buffer8(0);
		pduel->write_buffer32(41);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_EVENT);
		pduel->write_buffer8(1);
		pduel->write_buffer32(41);
		core.hint_timing[infos.turn_player] = TIMING_DAMAGE_STEP;
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, 0);
		return FALSE;
	}
	case 23: {
		if(core.attacker->current.location != LOCATION_MZONE || core.attacker->fieldid_r != core.pre_field[0]
		        || ((core.attacker->current.position & POS_DEFENSE) && !(core.attacker->is_affected_by_effect(EFFECT_DEFENSE_ATTACK)))
		        || core.attacker->current.controler != core.attacker->attack_controler
		        || (core.attack_target && (core.attack_target->current.location != LOCATION_MZONE
		                                || core.attack_target->current.controler != core.attack_target->attack_controler
		                                || core.attack_target->fieldid_r != core.pre_field[1]))) {
			core.units.begin()->step = 32;
			return FALSE;
		}
		infos.phase = PHASE_DAMAGE_CAL;
		adjust_all();
		return FALSE;
	}
	case 24: {
		// PHASE_DAMAGE_CAL;
		calculate_battle_damage(0, 0, 0);
		raise_single_event(core.attacker, 0, EVENT_PRE_DAMAGE_CALCULATE, 0, 0, 0, 0, 0);
		if(core.attack_target)
			raise_single_event(core.attack_target, 0, EVENT_PRE_DAMAGE_CALCULATE, 0, 0, 0, 0, 1);
		raise_event((card*)0, EVENT_PRE_DAMAGE_CALCULATE, 0, 0, 0, 0, 0);
		process_single_event();
		process_instant_event();
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_EVENT);
		pduel->write_buffer8(0);
		pduel->write_buffer32(42);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_EVENT);
		pduel->write_buffer8(1);
		pduel->write_buffer32(42);
		core.hint_timing[infos.turn_player] = TIMING_DAMAGE_CAL;
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, TRUE);
		return FALSE;
	}
	case 25: {
		if(core.attacker->current.location != LOCATION_MZONE || core.attacker->fieldid_r != core.pre_field[0]
		        || core.attacker->current.controler != core.attacker->attack_controler
		        || (core.attack_target && (core.attack_target->current.location != LOCATION_MZONE
		                                || core.attack_target->current.controler != core.attack_target->attack_controler
		                                || core.attack_target->fieldid_r != core.pre_field[1]))) {
			reset_phase(PHASE_DAMAGE_CAL);
			infos.phase = PHASE_DAMAGE;
			core.units.begin()->step = 32;
			return FALSE;
		}
		raise_single_event(core.attacker, 0, EVENT_DAMAGE_CALCULATING, 0, 0, 0, 0, 0);
		if(core.attack_target)
			raise_single_event(core.attack_target, 0, EVENT_DAMAGE_CALCULATING, 0, 0, 0, 0, 1);
		raise_event((card*)0, EVENT_DAMAGE_CALCULATING, 0, 0, 0, 0, 0);
		process_single_event();
		process_instant_event();
		//this timing does not exist in Master Rule 3
		core.new_ochain.clear();
		core.new_fchain.clear();
		return FALSE;
	}
	case 26: {
		// Duel.CalculateDamage() goes here
		uint32 aa = core.attacker->get_attack(), ad = core.attacker->get_defense();
		uint32 da = 0, dd = 0;
		uint8 pa = core.attacker->current.controler, pd;
		core.attacker->q_cache.attack = aa;
		core.attacker->q_cache.defense = ad;
		core.attacker->set_status(STATUS_BATTLE_RESULT, FALSE);
		core.attacker->set_status(STATUS_BATTLE_DESTROYED, FALSE);
		if(core.attack_target) {
			da = core.attack_target->get_attack();
			dd = core.attack_target->get_defense();
			core.attack_target->q_cache.attack = da;
			core.attack_target->q_cache.defense = dd;
			core.attack_target->set_status(STATUS_BATTLE_RESULT, FALSE);
			core.attack_target->set_status(STATUS_BATTLE_DESTROYED, FALSE);
			pd = core.attack_target->current.controler;
			if(pa != pd) {
				core.attacker->set_status(STATUS_OPPO_BATTLE, TRUE);
				core.attack_target->set_status(STATUS_OPPO_BATTLE, TRUE);
			}
		}
		effect* damchange = 0;
		card* reason_card = 0;
		uint8 bd[2];
		calculate_battle_damage(&damchange, &reason_card, bd);
		if(bd[0]) {
			effect* indestructable_effect = core.attacker->is_affected_by_effect(EFFECT_INDESTRUCTABLE_BATTLE, core.attack_target);
			if(indestructable_effect) {
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_CARD);
				pduel->write_buffer8(0);
				pduel->write_buffer32(indestructable_effect->owner->data.code);
				bd[0] = FALSE;
			} else
				core.attacker->set_status(STATUS_BATTLE_RESULT, TRUE);
		}
		if(bd[1]) {
			effect* indestructable_effect = core.attack_target->is_affected_by_effect(EFFECT_INDESTRUCTABLE_BATTLE, core.attacker);
			if(indestructable_effect) {
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_CARD);
				pduel->write_buffer8(0);
				pduel->write_buffer32(indestructable_effect->owner->data.code);
				bd[1] = FALSE;
			} else
				core.attack_target->set_status(STATUS_BATTLE_RESULT, TRUE);
		}
		pduel->write_buffer8(MSG_BATTLE);
		pduel->write_buffer32(core.attacker->get_info_location());
		pduel->write_buffer32(aa);
		pduel->write_buffer32(ad);
		pduel->write_buffer8(bd[0]);
		if(core.attack_target) {
			pduel->write_buffer32(core.attack_target->get_info_location());
			pduel->write_buffer32(da);
			pduel->write_buffer32(dd);
			pduel->write_buffer8(bd[1]);
		} else {
			pduel->write_buffer32(0);
			pduel->write_buffer32(0);
			pduel->write_buffer32(0);
			pduel->write_buffer8(0);
		}
		core.units.begin()->peffect = damchange;
		if(reason_card)
			core.temp_var[0] = reason_card->current.controler;
		if(!reason_card)
			core.temp_var[1] = 0;
		else if(reason_card == core.attacker)
			core.temp_var[1] = 1;
		else core.temp_var[1] = 2;
		if(!damchange) {
			if(core.battle_damage[infos.turn_player]) {
				raise_single_event(core.attacker, 0, EVENT_PRE_BATTLE_DAMAGE, 0, 0, reason_card->current.controler, infos.turn_player, core.battle_damage[infos.turn_player]);
				if(core.attack_target)
					raise_single_event(core.attack_target, 0, EVENT_PRE_BATTLE_DAMAGE, 0, 0, reason_card->current.controler, infos.turn_player, core.battle_damage[infos.turn_player]);
				raise_event(reason_card, EVENT_PRE_BATTLE_DAMAGE, 0, 0, reason_card->current.controler, infos.turn_player, core.battle_damage[infos.turn_player]);
			}
			if(core.battle_damage[1 - infos.turn_player]) {
				raise_single_event(core.attacker, 0, EVENT_PRE_BATTLE_DAMAGE, 0, 0, reason_card->current.controler, 1 - infos.turn_player, core.battle_damage[1 - infos.turn_player]);
				if(core.attack_target)
					raise_single_event(core.attack_target, 0, EVENT_PRE_BATTLE_DAMAGE, 0, 0, reason_card->current.controler, 1 - infos.turn_player, core.battle_damage[1 - infos.turn_player]);
				raise_event(reason_card, EVENT_PRE_BATTLE_DAMAGE, 0, 0, reason_card->current.controler, 1 - infos.turn_player, core.battle_damage[1 - infos.turn_player]);
			}
		}
		process_single_event();
		process_instant_event();
		//this timing does not exist in Master Rule 3
		core.damage_calculated = TRUE;
		if(core.effect_damage_step)
			return TRUE;
		return FALSE;
	}
	case 27: {
		infos.phase = PHASE_DAMAGE;
		core.hint_timing[infos.turn_player] = 0;
		core.chain_attack = FALSE;
		core.attacker->battled_cards.addcard(core.attack_target);
		if(core.attack_target)
			core.attack_target->battled_cards.addcard(core.attacker);
		uint8 reason_player = core.temp_var[0];
		card* reason_card = 0;
		if(core.temp_var[1] == 1)
			reason_card = core.attacker;
		else if(core.temp_var[1] == 2)
			reason_card = core.attack_target;
		effect* damchange = core.units.begin()->peffect;
		if(!damchange) {
			if(core.battle_damage[0])
				damage(0, REASON_BATTLE, reason_player, reason_card, 0, core.battle_damage[0]);
			if(core.battle_damage[1])
				damage(0, REASON_BATTLE, reason_player, reason_card, 1, core.battle_damage[1]);
		} else {
			if(core.battle_damage[0])
				damage(damchange, REASON_EFFECT, reason_player, reason_card, 0, core.battle_damage[0]);
			if(core.battle_damage[1])
				damage(damchange, REASON_EFFECT, reason_player, reason_card, 1, core.battle_damage[1]);
		}
		reset_phase(PHASE_DAMAGE_CAL);
		adjust_all();
		return FALSE;
	}
	case 28: {
		card_set des;
		effect* peffect;
		if(core.attacker->is_status(STATUS_BATTLE_RESULT)
		        && core.attacker->current.location == LOCATION_MZONE && core.attacker->fieldid_r == core.pre_field[0]) {
			des.insert(core.attacker);
			core.attacker->temp.reason = core.attacker->current.reason;
			core.attacker->temp.reason_card = core.attacker->current.reason_card;
			core.attacker->temp.reason_effect = core.attacker->current.reason_effect;
			core.attacker->temp.reason_player = core.attacker->current.reason_player;
			core.attacker->current.reason_effect = 0;
			core.attacker->current.reason = REASON_BATTLE;
			core.attacker->current.reason_card = core.attack_target;
			core.attacker->current.reason_player = core.attack_target->current.controler;
			uint32 dest = LOCATION_GRAVE;
			uint32 seq = 0;
			if((peffect = core.attack_target->is_affected_by_effect(EFFECT_BATTLE_DESTROY_REDIRECT)) && (core.attacker->data.type & TYPE_MONSTER)) {
				dest = peffect->get_value(core.attacker);
				seq = dest >> 16;
				dest &= 0xffff;
			}
			core.attacker->sendto_param.set(core.attacker->owner, POS_FACEUP, dest, seq);
			core.attacker->set_status(STATUS_DESTROY_CONFIRMED, TRUE);
		}
		if(core.attack_target && core.attack_target->is_status(STATUS_BATTLE_RESULT)
		        && core.attack_target->current.location == LOCATION_MZONE && core.attack_target->fieldid_r == core.pre_field[1]) {
			des.insert(core.attack_target);
			core.attack_target->temp.reason = core.attack_target->current.reason;
			core.attack_target->temp.reason_card = core.attack_target->current.reason_card;
			core.attack_target->temp.reason_effect = core.attack_target->current.reason_effect;
			core.attack_target->temp.reason_player = core.attack_target->current.reason_player;
			core.attack_target->current.reason_effect = 0;
			core.attack_target->current.reason = REASON_BATTLE;
			core.attack_target->current.reason_card = core.attacker;
			core.attack_target->current.reason_player = core.attacker->current.controler;
			uint32 dest = LOCATION_GRAVE;
			uint32 seq = 0;
			if((peffect = core.attacker->is_affected_by_effect(EFFECT_BATTLE_DESTROY_REDIRECT)) && (core.attack_target->data.type & TYPE_MONSTER)) {
				dest = peffect->get_value(core.attack_target);
				seq = dest >> 16;
				dest &= 0xffff;
			}
			core.attack_target->sendto_param.set(core.attack_target->owner, POS_FACEUP, dest, seq);
			core.attack_target->set_status(STATUS_DESTROY_CONFIRMED, TRUE);
		}
		core.attacker->set_status(STATUS_BATTLE_RESULT, FALSE);
		if(core.attack_target)
			core.attack_target->set_status(STATUS_BATTLE_RESULT, FALSE);
		core.battle_destroy_rep.clear();
		core.desrep_chain.clear();
		if(des.size()) {
			group* ng = pduel->new_group();
			ng->container.swap(des);
			ng->is_readonly = TRUE;
			add_process(PROCESSOR_DESTROY, 10, 0, ng, REASON_BATTLE, PLAYER_NONE);
			core.units.begin()->ptarget = ng;
		}
		return FALSE;
	}
	case 29: {
		if(core.battle_destroy_rep.size())
			destroy(&core.battle_destroy_rep, 0, REASON_EFFECT | REASON_REPLACE, PLAYER_NONE);
		if(core.desrep_chain.size())
			add_process(PROCESSOR_OPERATION_REPLACE, 15, NULL, NULL, 0, 0);
		adjust_all();
		return FALSE;
	}
	case 30: {
		group* des = core.units.begin()->ptarget;
		if(des && des->container.size()) {
			for(auto cit = des->container.begin(); cit != des->container.end(); ++cit) {
				(*cit)->set_status(STATUS_BATTLE_DESTROYED, TRUE);
				(*cit)->set_status(STATUS_DESTROY_CONFIRMED, FALSE);
				(*cit)->filter_disable_related_cards();
			}
		}
		core.selfdes_disabled = FALSE;
		adjust_all();
		//EVENT_BATTLE_END was here, but this timing does not exist in Master Rule 3
		return FALSE;
	}
	case 31: {
		core.flip_delayed = FALSE;
		core.new_fchain.splice(core.new_fchain.begin(), core.new_fchain_b);
		core.new_ochain.splice(core.new_ochain.begin(), core.new_ochain_b);
		raise_single_event(core.attacker, 0, EVENT_BATTLED, 0, 0, PLAYER_NONE, 0, 0);
		if(core.attack_target)
			raise_single_event(core.attack_target, 0, EVENT_BATTLED, 0, 0, PLAYER_NONE, 0, 1);
		raise_event((card*)0, EVENT_BATTLED, 0, 0, PLAYER_NONE, 0, 0);
		process_single_event();
		process_instant_event();
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_EVENT);
		pduel->write_buffer8(0);
		pduel->write_buffer32(43);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_EVENT);
		pduel->write_buffer8(1);
		pduel->write_buffer32(43);
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, TRUE);
		return FALSE;
	}
	case 32: {
		group* des = core.units.begin()->ptarget;
		if(des) {
			for(auto cit = des->container.begin(); cit != des->container.end();) {
				auto rm = cit++;
				if((*rm)->current.location != LOCATION_MZONE || ((*rm)->fieldid_r != core.pre_field[0] && (*rm)->fieldid_r != core.pre_field[1]))
					des->container.erase(rm);
			}
			add_process(PROCESSOR_DESTROY, 3, 0, des, REASON_BATTLE, PLAYER_NONE);
		}
		adjust_all();
		return FALSE;
	}
	case 33: {
		core.units.begin()->ptarget = 0;
		// for unexpected end of damage step
		core.damage_calculated = TRUE;
		core.selfdes_disabled = FALSE;
		core.flip_delayed = FALSE;
		core.new_fchain.splice(core.new_fchain.begin(), core.new_fchain_b);
		core.new_ochain.splice(core.new_ochain.begin(), core.new_ochain_b);
		card_set ing;
		card_set ed;
		if(core.attacker->is_status(STATUS_BATTLE_DESTROYED) && (core.attacker->current.reason & REASON_BATTLE)) {
			raise_single_event(core.attack_target, 0, EVENT_BATTLE_DESTROYING, 0, core.attacker->current.reason, core.attack_target->current.controler, 0, 1);
			raise_single_event(core.attacker, 0, EVENT_BATTLE_DESTROYED, 0, core.attacker->current.reason, core.attack_target->current.controler, 0, 0);
			raise_single_event(core.attacker, 0, EVENT_DESTROYED, 0, core.attacker->current.reason, core.attack_target->current.controler, 0, 0);
			ing.insert(core.attack_target);
			ed.insert(core.attacker);
		}
		if(core.attack_target && core.attack_target->is_status(STATUS_BATTLE_DESTROYED) && (core.attack_target->current.reason & REASON_BATTLE)) {
			raise_single_event(core.attacker, 0, EVENT_BATTLE_DESTROYING, 0, core.attack_target->current.reason, core.attacker->current.controler, 0, 0);
			raise_single_event(core.attack_target, 0, EVENT_BATTLE_DESTROYED, 0, core.attack_target->current.reason, core.attacker->current.controler, 0, 1);
			raise_single_event(core.attack_target, 0, EVENT_DESTROYED, 0, core.attack_target->current.reason, core.attacker->current.controler, 0, 1);
			ing.insert(core.attacker);
			ed.insert(core.attack_target);
		}
		if(ing.size())
			raise_event(&ing, EVENT_BATTLE_DESTROYING, 0, 0, 0, 0, 0);
		if(ed.size()) {
			raise_event(&ed, EVENT_BATTLE_DESTROYED, 0, 0, 0, 0, 0);
			raise_event(&ed, EVENT_DESTROYED, 0, 0, 0, 0, 0);
		}
		raise_single_event(core.attacker, 0, EVENT_DAMAGE_STEP_END, 0, 0, 0, 0, 0);
		if(core.attack_target)
			raise_single_event(core.attack_target, 0, EVENT_DAMAGE_STEP_END, 0, 0, 0, 0, 1);
		raise_event((card*)0, EVENT_DAMAGE_STEP_END, 0, 0, 0, 0, 0);
		core.attacker->set_status(STATUS_BATTLE_DESTROYED, FALSE);
		if(core.attack_target)
			core.attack_target->set_status(STATUS_BATTLE_DESTROYED, FALSE);
		process_single_event();
		process_instant_event();
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_EVENT);
		pduel->write_buffer8(0);
		pduel->write_buffer32(44);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_EVENT);
		pduel->write_buffer8(1);
		pduel->write_buffer32(44);
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, TRUE);
		core.units.begin()->step = 38;
		return FALSE;
	}
	case 38: {
		return FALSE;
	}
	case 39: {
		//end of damage step
		core.attacker->set_status(STATUS_OPPO_BATTLE, FALSE);
		if(core.attack_target)
			core.attack_target->set_status(STATUS_OPPO_BATTLE, FALSE);
		core.units.begin()->step = -1;
		infos.phase = PHASE_BATTLE_STEP;
		pduel->write_buffer8(MSG_DAMAGE_STEP_END);
		reset_phase(PHASE_DAMAGE);
		adjust_all();
		if(core.effect_damage_step)
			return TRUE;
		return FALSE;
	}
	case 40: {
		for(auto it = core.chain_limit.begin(); it != core.chain_limit.end(); ++it)
			luaL_unref(pduel->lua->lua_state, LUA_REGISTRYINDEX, it->function);
		core.chain_limit.clear();
		if(core.current_chain.size()) {
			for(auto cait = core.current_chain.begin(); cait != core.current_chain.end(); ++cait)
				cait->triggering_effect->get_handler()->set_status(STATUS_CHAINING, FALSE);
			add_process(PROCESSOR_SOLVE_CHAIN, 0, 0, 0, FALSE, 0);
			core.units.begin()->step = -1;
			return FALSE;
		}
		reset_phase(PHASE_BATTLE_STEP);
		adjust_all();
		return FALSE;
	}
	case 41: {
		// normal end of battle step
		if(is_player_affected_by_effect(infos.turn_player, EFFECT_BP_TWICE))
			core.units.begin()->arg2 = 1;
		else
			core.units.begin()->arg2 = 0;
		infos.phase = PHASE_BATTLE;
		add_process(PROCESSOR_PHASE_EVENT, 0, 0, 0, PHASE_BATTLE, 0);
		adjust_all();
		return FALSE;
	}
	case 42: {
		core.attacker = 0;
		core.attack_target = 0;
		returns.ivalue[0] = core.units.begin()->arg1;
		returns.ivalue[1] = core.units.begin()->arg2;
		return TRUE;
	}
	}
	return TRUE;
}
// perform damage calculation by an effect
int32 field::process_damage_step(uint16 step, uint32 new_attack) {
	switch(step) {
	case 0: {
		core.effect_damage_step = 1;
		card* tmp = core.attacker;
		core.attacker = (card*)core.units.begin()->peffect;
		core.units.begin()->peffect = (effect*)tmp;
		tmp = core.attack_target;
		core.attack_target = (card*)core.units.begin()->ptarget;
		core.units.begin()->ptarget = (group*)tmp;
		core.units.begin()->arg1 = infos.phase;
		if(core.attacker->current.location != LOCATION_MZONE || core.attack_target && core.attack_target->current.location != LOCATION_MZONE) {
			core.units.begin()->step = 2;
			return FALSE;
		}
		if(new_attack) {
			core.attack_state_count[infos.turn_player]++;
			core.battled_count[infos.turn_player]++;
			check_card_counter(core.attacker, 5, infos.turn_player);
		}
		attack_all_target_check();
		pduel->write_buffer8(MSG_ATTACK);
		pduel->write_buffer32(core.attacker->get_info_location());
		if(core.attack_target)
			pduel->write_buffer32(core.attack_target->get_info_location());
		else
			pduel->write_buffer32(0);
		infos.phase = PHASE_DAMAGE;
		pduel->write_buffer8(MSG_DAMAGE_STEP_START);
		core.pre_field[0] = core.attacker->fieldid_r;
		core.attacker->attacked_count++;
		if(core.attack_target) {
			core.pre_field[1] = core.attack_target->fieldid_r;
			if(core.attack_target->is_position(POS_FACEDOWN)) {
				change_position(core.attack_target, 0, PLAYER_NONE, core.attack_target->current.position >> 1, 0, TRUE);
				adjust_all();
			}
		}
		else
			core.pre_field[1] = 0;
		return FALSE;
	}
	case 1: {
		infos.phase = PHASE_DAMAGE_CAL;
		raise_single_event(core.attacker, 0, EVENT_DAMAGE_CALCULATING, 0, 0, 0, 0, 0);
		if(core.attack_target)
			raise_single_event(core.attack_target, 0, EVENT_DAMAGE_CALCULATING, 0, 0, 0, 0, 1);
		raise_event((card*)0, EVENT_DAMAGE_CALCULATING, 0, 0, 0, 0, 0);
		process_single_event();
		process_instant_event();
		add_process(PROCESSOR_BATTLE_COMMAND, 26, 0, 0, 0, 0);
		core.units.begin()->step = 2;
		core.reserved = core.units.front();
		return TRUE;
	}
	case 2: {
		core.effect_damage_step = 2;
		add_process(PROCESSOR_BATTLE_COMMAND, 27, 0, 0, 0, 0);
		return FALSE;
	}
	case 3: {
		core.attacker = (card*)core.units.begin()->peffect;
		core.attack_target = (card*)core.units.begin()->ptarget;
		if(core.attacker)
			core.attacker->set_status(STATUS_ATTACK_CANCELED, TRUE);
		if(core.attack_target)
			core.attack_target->set_status(STATUS_ATTACK_CANCELED, TRUE);
		core.effect_damage_step = 0;
		infos.phase = core.units.begin()->arg1;
		return TRUE;
	}
	}
	return TRUE;
}
void field::calculate_battle_damage(effect** pdamchange, card** preason_card, uint8* battle_destroyed)
{
	uint32 aa = core.attacker->get_attack(), ad = core.attacker->get_defense();
	uint32 da = 0, dd = 0, a = aa, d;
	uint8 pa = core.attacker->current.controler, pd;
	uint8 damp = 0;
	effect* damchange = 0;
	card* reason_card = 0;
	uint8 bd[2] = {FALSE, FALSE};
	core.battle_damage[0] = core.battle_damage[1] = 0;
	if(core.attacker->is_position(POS_FACEUP_DEFENSE)) {
		effect* defattack = core.attacker->is_affected_by_effect(EFFECT_DEFENSE_ATTACK);
		if(defattack && defattack->get_value(core.attacker))
			a = ad;
	}
	if(core.attack_target) {
		da = core.attack_target->get_attack();
		dd = core.attack_target->get_defense();
		pd = core.attack_target->current.controler;
		if(core.attack_target->is_position(POS_ATTACK)) {
			d = da;
			if(a > d) {
				damchange = core.attacker->is_affected_by_effect(EFFECT_BATTLE_DAMAGE_TO_EFFECT);
				if(damchange) {
					damp = pd;
					core.battle_damage[damp] = a - d;
					reason_card = core.attacker;
				} else if(!core.attacker->is_affected_by_effect(EFFECT_NO_BATTLE_DAMAGE)
				          && !core.attack_target->is_affected_by_effect(EFFECT_AVOID_BATTLE_DAMAGE, core.attacker)
				          && !is_player_affected_by_effect(pd, EFFECT_AVOID_BATTLE_DAMAGE)) {
					if(core.attack_target->is_affected_by_effect(EFFECT_REFLECT_BATTLE_DAMAGE, core.attacker))
						damp = 1 - pd;
					else damp = pd;
					if(is_player_affected_by_effect(damp, EFFECT_REFLECT_BATTLE_DAMAGE))
						damp = 1 - damp;
					if(damp == pd || (!core.attacker->is_affected_by_effect(EFFECT_AVOID_BATTLE_DAMAGE, core.attack_target)
					        && !is_player_affected_by_effect(damp, EFFECT_AVOID_BATTLE_DAMAGE))) {
						core.battle_damage[damp] = a - d;
						reason_card = core.attacker;
					}
				}
				bd[1] = TRUE;
			} else if (a < d) {
				damchange = core.attack_target->is_affected_by_effect(EFFECT_BATTLE_DAMAGE_TO_EFFECT);
				if(damchange) {
					damp = pa;
					core.battle_damage[damp] = d - a;
					reason_card = core.attack_target;
				} else if(!core.attack_target->is_affected_by_effect(EFFECT_NO_BATTLE_DAMAGE)
				          && !core.attacker->is_affected_by_effect(EFFECT_AVOID_BATTLE_DAMAGE, core.attack_target)
				          && !is_player_affected_by_effect(pa, EFFECT_AVOID_BATTLE_DAMAGE)) {
					if(core.attacker->is_affected_by_effect(EFFECT_REFLECT_BATTLE_DAMAGE, core.attack_target))
						damp = 1 - pa;
					else damp = pa;
					if(is_player_affected_by_effect(damp, EFFECT_REFLECT_BATTLE_DAMAGE))
						damp = 1 - damp;
					if(damp == pa || (!core.attack_target->is_affected_by_effect(EFFECT_AVOID_BATTLE_DAMAGE, core.attacker)
					        && !is_player_affected_by_effect(damp, EFFECT_AVOID_BATTLE_DAMAGE))) {
						core.battle_damage[damp] = d - a;
						reason_card = core.attack_target;
					}
				}
				bd[0] = TRUE;
			} else {
				if(a != 0) {
					bd[0] = TRUE;
					bd[1] = TRUE;
				}
			}
		} else {
			d = dd;
			if(a > d) {
				effect_set eset;
				uint8 dp[2];
				dp[0] = dp[1] = 0;
				core.attacker->filter_effect(EFFECT_PIERCE, &eset);
				if(eset.size() && !core.attacker->is_affected_by_effect(EFFECT_NO_BATTLE_DAMAGE)
				        && !core.attack_target->is_affected_by_effect(EFFECT_AVOID_BATTLE_DAMAGE, core.attacker)) {
					for(int32 i = 0; i < eset.size(); ++i)
						dp[1 - eset[i]->get_handler_player()] = 1;
					if(dp[0] && is_player_affected_by_effect(0, EFFECT_AVOID_BATTLE_DAMAGE))
						dp[0] = 0;
					if(dp[1] && is_player_affected_by_effect(1, EFFECT_AVOID_BATTLE_DAMAGE))
						dp[1] = 0;
					if(dp[pd] && core.attack_target->is_affected_by_effect(EFFECT_REFLECT_BATTLE_DAMAGE, core.attacker)) {
						dp[pd] = 0;
						dp[1 - pd] = 1;
					}
					if(dp[pd] && is_player_affected_by_effect(pd, EFFECT_REFLECT_BATTLE_DAMAGE)) {
						dp[pd] = 0;
						dp[1 - pd] = 1;
					}
					if(dp[1 - pd] && is_player_affected_by_effect(1 - pd, EFFECT_REFLECT_BATTLE_DAMAGE)) {
						dp[pd] = 1;
						dp[1 - pd] = 0;
					}
					if(dp[pd] && !core.attack_target->is_affected_by_effect(EFFECT_AVOID_BATTLE_DAMAGE, core.attacker)
					        && !is_player_affected_by_effect(pd, EFFECT_AVOID_BATTLE_DAMAGE))
						core.battle_damage[pd] = a - d;
					if(dp[1 - pd] && !core.attacker->is_affected_by_effect(EFFECT_AVOID_BATTLE_DAMAGE, core.attack_target)
					        && !is_player_affected_by_effect(1 - pd, EFFECT_AVOID_BATTLE_DAMAGE))
						core.battle_damage[1 - pd] = a - d;
					reason_card = core.attacker;
				}
				bd[1] = TRUE;
			} else if (a < d) {
				damchange = core.attack_target->is_affected_by_effect(EFFECT_BATTLE_DAMAGE_TO_EFFECT);
				if(damchange) {
					damp = pa;
					core.battle_damage[damp] = d - a;
					reason_card = core.attack_target;
				} else if(!core.attack_target->is_affected_by_effect(EFFECT_NO_BATTLE_DAMAGE)
				          && !core.attacker->is_affected_by_effect(EFFECT_AVOID_BATTLE_DAMAGE, core.attack_target)
				          && !is_player_affected_by_effect(pa, EFFECT_AVOID_BATTLE_DAMAGE)) {
					if(core.attacker->is_affected_by_effect(EFFECT_REFLECT_BATTLE_DAMAGE, core.attack_target))
						damp = 1 - pa;
					else damp = pa;
					if(is_player_affected_by_effect(damp, EFFECT_REFLECT_BATTLE_DAMAGE))
						damp = 1 - damp;
					if(damp == pa || (!core.attack_target->is_affected_by_effect(EFFECT_AVOID_BATTLE_DAMAGE, core.attacker)
					        && !is_player_affected_by_effect(damp, EFFECT_AVOID_BATTLE_DAMAGE))) {
						core.battle_damage[damp] = d - a;
						reason_card = core.attack_target;
					}
				}
			}
		}
	} else {
		damchange = core.attacker->is_affected_by_effect(EFFECT_BATTLE_DAMAGE_TO_EFFECT);
		if(damchange) {
			damp = 1 - pa;
			core.battle_damage[damp] = a;
			reason_card = core.attacker;
		} else if(!core.attacker->is_affected_by_effect(EFFECT_NO_BATTLE_DAMAGE)
		          && !is_player_affected_by_effect(1 - pa, EFFECT_AVOID_BATTLE_DAMAGE)) {
			damp = 1 - pa;
			if(is_player_affected_by_effect(damp, EFFECT_REFLECT_BATTLE_DAMAGE))
				damp = 1 - damp;
			if(!is_player_affected_by_effect(damp, EFFECT_AVOID_BATTLE_DAMAGE)) {
				core.battle_damage[damp] = a;
				reason_card = core.attacker;
			}
		}
	}
	if(pdamchange)
		*pdamchange = damchange;
	if(preason_card)
		*preason_card = reason_card;
	if(battle_destroyed) {
		battle_destroyed[0] = bd[0];
		battle_destroyed[1] = bd[1];
	}
}
int32 field::process_turn(uint16 step, uint8 turn_player) {
	switch(step) {
	case 0: {
		//Pre Draw
		for(auto elit = core.used_event.begin(); elit != core.used_event.end(); ++elit) {
			if(elit->event_cards)
				pduel->delete_group(elit->event_cards);
		}
		core.used_event.clear();
		for(auto eit = core.reseted_effects.begin(); eit != core.reseted_effects.end(); ++eit) {
			pduel->delete_effect(*eit);
		}
		core.reseted_effects.clear();
		core.effect_count_code.clear();
		for(uint8 p = 0; p < 2; ++p) {
			for(auto cit = player[p].list_mzone.begin(); cit != player[p].list_mzone.end(); ++cit) {
				card* pcard = *cit;
				if(!pcard)
					continue;
				pcard->set_status(STATUS_SUMMON_TURN, FALSE);
				pcard->set_status(STATUS_FLIP_SUMMON_TURN, FALSE);
				pcard->set_status(STATUS_SPSUMMON_TURN, FALSE);
				pcard->set_status(STATUS_SET_TURN, FALSE);
				pcard->set_status(STATUS_FORM_CHANGED, FALSE);
				pcard->indestructable_effects.clear();
				pcard->announce_count = 0;
				pcard->attacked_count = 0;
				pcard->announced_cards.clear();
				pcard->attacked_cards.clear();
				pcard->battled_cards.clear();
				pcard->attack_all_target = TRUE;
			}
			for(auto cit = player[p].list_szone.begin(); cit != player[p].list_szone.end(); ++cit) {
				card* pcard = *cit;
				if(!pcard)
					continue;
				pcard->set_status(STATUS_SET_TURN, FALSE);
				pcard->indestructable_effects.clear();
			}
			core.summon_state_count[p] = 0;
			core.normalsummon_state_count[p] = 0;
			core.flipsummon_state_count[p] = 0;
			core.spsummon_state_count[p] = 0;
			core.spsummon_state_count_rst[p] = 0;
			core.attack_state_count[p] = 0;
			core.battle_phase_count[p] = 0;
			core.battled_count[p] = 0;
			core.summon_count[p] = 0;
			core.extra_summon[p] = 0;
			core.spsummon_once_map[p].clear();
			core.spsummon_once_map_rst[p].clear();
		}
		core.spsummon_rst = false;
		for(auto rit = effects.rechargeable.begin(); rit != effects.rechargeable.end(); ++rit)
			if(!(*rit)->is_flag(EFFECT_FLAG_NO_TURN_RESET))
				(*rit)->recharge();
		for(auto iter = core.summon_counter.begin(); iter != core.summon_counter.end(); ++iter)
			iter->second.second = 0;
		for(auto iter = core.normalsummon_counter.begin(); iter != core.normalsummon_counter.end(); ++iter)
			iter->second.second = 0;
		for(auto iter = core.spsummon_counter.begin(); iter != core.spsummon_counter.end(); ++iter)
			iter->second.second = 0;
		for(auto iter = core.flipsummon_counter.begin(); iter != core.flipsummon_counter.end(); ++iter)
			iter->second.second = 0;
		for(auto iter = core.attack_counter.begin(); iter != core.attack_counter.end(); ++iter)
			iter->second.second = 0;
		for(auto iter = core.chain_counter.begin(); iter != core.chain_counter.end(); ++iter)
			iter->second.second = 0;
		if(core.global_flag & GLOBALFLAG_SPSUMMON_COUNT) {
			for(auto iter = effects.spsummon_count_eff.begin(); iter != effects.spsummon_count_eff.end(); ++iter) {
				effect* peffect = *iter;
				card* pcard = peffect->get_handler();
				if(!peffect->is_flag(EFFECT_FLAG_NO_TURN_RESET)) {
					pcard->spsummon_counter[0] = pcard->spsummon_counter[1] = 0;
					pcard->spsummon_counter_rst[0] = pcard->spsummon_counter_rst[1] = 0;
				}
			}
		}
		infos.turn_id++;
		infos.turn_id_by_player[turn_player]++;
		infos.turn_player = turn_player;
		pduel->write_buffer8(MSG_NEW_TURN);
		pduel->write_buffer8(turn_player);
		if((core.duel_options & DUEL_TAG_MODE) && infos.turn_id != 1)
			tag_swap(turn_player);
		if(is_player_affected_by_effect(infos.turn_player, EFFECT_SKIP_TURN)) {
			core.units.begin()->step = 17;
			reset_phase(PHASE_DRAW);
			reset_phase(PHASE_STANDBY);
			reset_phase(PHASE_END);
			adjust_all();
			return FALSE;
		}
		infos.phase = PHASE_DRAW;
		core.phase_action = FALSE;
		core.hand_adjusted = FALSE;
		raise_event((card*)0, EVENT_PHASE_START + PHASE_DRAW, 0, 0, 0, turn_player, 0);
		process_instant_event();
		adjust_all();
		return FALSE;
	}
	case 1: {
		core.new_fchain.clear();
		core.new_ochain.clear();
		core.quick_f_chain.clear();
		core.delayed_quick_tmp.clear();
		if(is_player_affected_by_effect(infos.turn_player, EFFECT_SKIP_DP)) {
			core.units.begin()->step = 2;
			reset_phase(PHASE_DRAW);
			adjust_all();
			return FALSE;
		}
		pduel->write_buffer8(MSG_NEW_PHASE);
		pduel->write_buffer16(infos.phase);
		raise_event((card*)0, EVENT_PREDRAW, 0, 0, 0, turn_player, 0);
		process_instant_event();
		if(core.new_fchain.size() || core.new_ochain.size())
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, 0);
		return FALSE;
	}
	case 2: {
		// Draw, new ruling
		if((core.duel_rule <= 2) || (infos.turn_id > 1)) {
			int32 count = get_draw_count(infos.turn_player);
			if(count > 0) {
				draw(0, REASON_RULE, turn_player, turn_player, count);
				add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, 0);
			}
		}
		add_process(PROCESSOR_PHASE_EVENT, 0, 0, 0, PHASE_DRAW, 0);
		return FALSE;
	}
	case 3: {
		// EVENT_PHASE_PRESTART is removed
		return FALSE;
	}
	case 4: {
		//Standby Phase
		infos.phase = PHASE_STANDBY;
		core.phase_action = FALSE;
		core.new_fchain.clear();
		core.new_ochain.clear();
		core.quick_f_chain.clear();
		core.delayed_quick_tmp.clear();
		if(is_player_affected_by_effect(infos.turn_player, EFFECT_SKIP_SP)) {
			core.units.begin()->step = 5;
			reset_phase(PHASE_STANDBY);
			adjust_all();
			return FALSE;
		}
		pduel->write_buffer8(MSG_NEW_PHASE);
		pduel->write_buffer16(infos.phase);
		raise_event((card*)0, EVENT_PHASE_START + PHASE_STANDBY, 0, 0, 0, turn_player, 0);
		process_instant_event();
		return FALSE;
	}
	case 5: {
		// EVENT_PHASE_START + PHASE_STANDBY is a special case(c89642993)
		if(core.new_fchain.size() || core.new_ochain.size() || core.instant_event.back().event_code != EVENT_PHASE_START + PHASE_STANDBY)
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, 0);
		add_process(PROCESSOR_PHASE_EVENT, 0, 0, 0, PHASE_STANDBY, 0);
		return FALSE;
	}
	case 6: {
		//Main1
		infos.phase = PHASE_MAIN1;
		core.phase_action = FALSE;
		raise_event((card*)0, EVENT_PHASE_START + PHASE_MAIN1, 0, 0, 0, turn_player, 0);
		process_instant_event();
		adjust_all();
		return FALSE;
	}
	case 7: {
		return FALSE;
	}
	case 8: {
		core.new_fchain.clear();
		core.new_ochain.clear();
		core.quick_f_chain.clear();
		core.delayed_quick_tmp.clear();
		pduel->write_buffer8(MSG_NEW_PHASE);
		pduel->write_buffer16(infos.phase);
		add_process(PROCESSOR_IDLE_COMMAND, 0, 0, 0, 0, 0);
		return FALSE;
	}
	case 9: {
		if(returns.ivalue[0] == 7) { // End Phase
			core.units.begin()->step = 14;
			return FALSE;
		}
		infos.phase = PHASE_BATTLE_START;
		core.new_fchain.clear();
		core.new_ochain.clear();
		core.quick_f_chain.clear();
		core.delayed_quick_tmp.clear();
		core.phase_action = FALSE;
		core.battle_phase_count[infos.turn_player]++;
		pduel->write_buffer8(MSG_NEW_PHASE);
		pduel->write_buffer16(infos.phase);
		// Show the texts to indicate that BP is entered and skipped
		if(is_player_affected_by_effect(infos.turn_player, EFFECT_SKIP_BP)) {
			core.units.begin()->step = 14;
			reset_phase(PHASE_BATTLE_START);
			reset_phase(PHASE_BATTLE_STEP);
			reset_phase(PHASE_BATTLE);
			adjust_all();
			return FALSE;
		}
		raise_event((card*)0, EVENT_PHASE_START + PHASE_BATTLE_START, 0, 0, 0, turn_player, 0);
		process_instant_event();
		adjust_all();
		return FALSE;
	}
	case 10: {
		add_process(PROCESSOR_PHASE_EVENT, 0, 0, 0, PHASE_BATTLE_START, 0);
		return FALSE;
	}
	case 11: {
		infos.phase = PHASE_BATTLE_STEP;
		core.new_fchain.clear();
		core.new_ochain.clear();
		core.quick_f_chain.clear();
		core.delayed_quick_tmp.clear();
		core.phase_action = FALSE;
		core.chain_attack = FALSE;
		add_process(PROCESSOR_BATTLE_COMMAND, 0, 0, 0, 0, 0);
		return FALSE;
	}
	case 12: {
		if(core.units.begin()->arg2 == 0 && returns.ivalue[1]) { // 2nd Battle Phase
			core.units.begin()->arg2 = 1;
			core.units.begin()->step = 8;
			for(uint8 p = 0; p < 2; ++p) {
				for(auto cit = player[p].list_mzone.begin(); cit != player[p].list_mzone.end(); ++cit) {
					card* pcard = *cit;
					if(!pcard)
						continue;
					pcard->announce_count = 0;
					pcard->attacked_count = 0;
					pcard->announced_cards.clear();
					pcard->attacked_cards.clear();
					pcard->battled_cards.clear();
				}
			}
			return FALSE;
		}
		core.skip_m2 = FALSE;
		if(returns.ivalue[0] == 3) { // End Phase
			core.skip_m2 = TRUE;
		}
		//Main2
		infos.phase = PHASE_MAIN2;
		core.phase_action = FALSE;
		raise_event((card*)0, EVENT_PHASE_START + PHASE_MAIN2, 0, 0, 0, turn_player, 0);
		process_instant_event();
		adjust_all();
		return FALSE;
	}
	case 13: {
		if(core.new_fchain.size() || core.new_ochain.size())
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, 0);
		return FALSE;
	}
	case 14: {
		core.new_fchain.clear();
		core.new_ochain.clear();
		core.quick_f_chain.clear();
		core.delayed_quick_tmp.clear();
		pduel->write_buffer8(MSG_NEW_PHASE);
		pduel->write_buffer16(infos.phase);
		infos.can_shuffle = TRUE;
		add_process(PROCESSOR_IDLE_COMMAND, 0, 0, 0, 0, 0);
		return FALSE;
	}
	case 15: {
		//End Phase
		infos.phase = PHASE_END;
		core.phase_action = FALSE;
		pduel->write_buffer8(MSG_NEW_PHASE);
		pduel->write_buffer16(infos.phase);
		raise_event((card*)0, EVENT_PHASE_START + PHASE_END, 0, 0, 0, turn_player, 0);
		process_instant_event();
		adjust_all();
		return FALSE;
	}
	case 16: {
		if(core.new_fchain.size() || core.new_ochain.size())
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0, 0);
		return FALSE;
	}
	case 17: {
		core.new_fchain.clear();
		core.new_ochain.clear();
		core.quick_f_chain.clear();
		core.delayed_quick_tmp.clear();
		add_process(PROCESSOR_PHASE_EVENT, 0, 0, 0, PHASE_END, 0);
		return FALSE;
	}
	case 18: {
		raise_event((card*)0, EVENT_TURN_END, 0, 0, 0, turn_player, 0);
		process_instant_event();
		adjust_all();
		return FALSE;
	}
	case 19: {
		core.new_fchain.clear();
		core.new_ochain.clear();
		core.quick_f_chain.clear();
		core.delayed_quick_tmp.clear();
		core.units.begin()->step = -1;
		core.units.begin()->arg1 = 1 - core.units.begin()->arg1;
		return FALSE;
	}
	}
	return TRUE;
}
int32 field::add_chain(uint16 step) {
	switch (step) {
	case 0: {
		if (!core.new_chains.size())
			return TRUE;
		auto& clit = core.new_chains.front();
		effect* peffect = clit.triggering_effect;
		card* phandler = peffect->get_handler();
		effect_set eset;
		filter_player_effect(clit.triggering_player, EFFECT_ACTIVATE_COST, &eset);
		for(int32 i = 0; i < eset.size(); ++i) {
			pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
			pduel->lua->add_param(clit.triggering_effect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(clit.triggering_player, PARAM_TYPE_INT);
			if(!pduel->lua->check_condition(eset[i]->target, 3))
				continue;
			if(eset[i]->operation) {
				core.sub_solving_event.push_back(clit.evt);
				add_process(PROCESSOR_EXECUTE_OPERATION, 0, eset[i], 0, clit.triggering_player, 0);
			}
		}
		if(peffect->type & EFFECT_TYPE_ACTIVATE) {
			break_effect();
			int32 ecode = 0;
			if(phandler->current.location == LOCATION_HAND) {
				if(phandler->data.type & TYPE_TRAP)
					ecode = EFFECT_TRAP_ACT_IN_HAND;
				else if((phandler->data.type & TYPE_SPELL) && (phandler->data.type & TYPE_QUICKPLAY)
				        && infos.turn_player != phandler->current.controler)
					ecode = EFFECT_QP_ACT_IN_NTPHAND;
			} else if(phandler->current.location == LOCATION_SZONE) {
				if((phandler->data.type & TYPE_TRAP) && phandler->get_status(STATUS_SET_TURN))
					ecode = EFFECT_TRAP_ACT_IN_SET_TURN;
			}
			if(ecode) {
				eset.clear();
				phandler->filter_effect(ecode, &eset);
				effect* pactin = 0;
				for(int32 i = 0; i < eset.size(); ++i) {
					if(!eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
						pactin = eset[i];
						break;
					}
				}
				if(!pactin) {
					for(int32 i = 0; i < eset.size(); ++i) {
						if(eset[i]->check_count_limit(phandler->current.controler)) {
							eset[i]->dec_count(phandler->current.controler);
							break;
						}
					}
				}
			}
			if(phandler->current.location == LOCATION_HAND) {
				phandler->enable_field_effect(false);
				phandler->set_status(STATUS_ACT_FROM_HAND, TRUE);
				move_to_field(phandler, phandler->current.controler, phandler->current.controler, LOCATION_SZONE, POS_FACEUP);
			} else {
				phandler->set_status(STATUS_ACT_FROM_HAND, FALSE);
				change_position(phandler, 0, phandler->current.controler, POS_FACEUP, 0);
			}
		}
		if(phandler->current.location & (LOCATION_GRAVE | LOCATION_REMOVED))
			move_card(phandler->current.controler, phandler, phandler->current.location, 0);
		return FALSE;
	}
	case 1: {
		auto& clit = core.new_chains.front();
		effect* peffect = clit.triggering_effect;
		card* phandler = peffect->get_handler();
		if(peffect->type & EFFECT_TYPE_ACTIVATE) {
			clit.set_triggering_place(phandler);
		}
		pduel->write_buffer8(MSG_CHAINING);
		pduel->write_buffer32(phandler->data.code);
		pduel->write_buffer32(phandler->get_info_location());
		pduel->write_buffer8(clit.triggering_controler);
		pduel->write_buffer8((uint8)clit.triggering_location);
		pduel->write_buffer8(clit.triggering_sequence);
		pduel->write_buffer32(peffect->description);
		pduel->write_buffer8(core.current_chain.size() + 1);
		break_effect();
		for(auto it = core.chain_limit.begin(); it != core.chain_limit.end(); ++it)
			luaL_unref(pduel->lua->lua_state, LUA_REGISTRYINDEX, it->function);
		core.chain_limit.clear();
		peffect->card_type = phandler->get_type();
		if((peffect->card_type & 0x5) == 0x5)
			peffect->card_type -= TYPE_TRAP;
		peffect->active_type = peffect->card_type;
		peffect->active_handler = peffect->handler->overlay_target;
		clit.chain_count = core.current_chain.size() + 1;
		clit.target_cards = 0;
		clit.target_player = PLAYER_NONE;
		clit.target_param = 0;
		clit.disable_reason = 0;
		clit.disable_player = PLAYER_NONE;
		clit.replace_op = 0;
		if(phandler->current.location == LOCATION_HAND)
			clit.flag |= CHAIN_HAND_EFFECT;
		core.current_chain.push_back(clit);
		check_chain_counter(peffect, clit.triggering_player, clit.chain_count);
		// triggered events which are not caused by RaiseEvent create relation with the handler
		if(!peffect->is_flag(EFFECT_FLAG_FIELD_ONLY) && (!(peffect->type & 0x2a0) || (peffect->code & EVENT_PHASE) == EVENT_PHASE)) {
			phandler->create_relation(clit);
		}
		peffect->effect_owner = clit.triggering_player;
		// DISABLE_CHAIN should be check before cost
		effect* deffect;
		if(!peffect->is_flag(EFFECT_FLAG_FIELD_ONLY) && phandler->is_has_relation(clit) && (deffect = phandler->is_affected_by_effect(EFFECT_DISABLE_EFFECT))) {
			effect* negeff = pduel->new_effect();
			negeff->owner = deffect->owner;
			negeff->type = EFFECT_TYPE_SINGLE;
			negeff->code = EFFECT_DISABLE_CHAIN;
			negeff->value = clit.chain_id;
			negeff->reset_flag = RESET_CHAIN | RESET_EVENT | deffect->get_value();
			phandler->add_effect(negeff);
		}
		core.new_chains.pop_front();
		return FALSE;
	}
	case 2: {
		auto& clit = core.current_chain.back();
		int32 playerid = clit.triggering_player;
		effect* peffect = clit.triggering_effect;
		if(get_cteffect(peffect, playerid, TRUE)) {
			const bool damage_step = infos.phase == PHASE_DAMAGE && !peffect->is_flag(EFFECT_FLAG_DAMAGE_STEP);
			const bool damage_cal = infos.phase == PHASE_DAMAGE_CAL && !peffect->is_flag(EFFECT_FLAG_DAMAGE_CAL);
			if(damage_step || damage_cal) {
				returns.ivalue[0] = TRUE;
				return FALSE;
			}
			add_process(PROCESSOR_SELECT_EFFECTYN, 0, 0, (group*)peffect->get_handler(), playerid, 94);
		} else
			returns.ivalue[0] = FALSE;
		return FALSE;
	}
	case 3: {
		if(!returns.ivalue[0]) {
			core.select_chains.clear();
			core.select_options.clear();
			core.units.begin()->step = 4;
			return FALSE;
		}
		if(core.select_chains.size() > 1) {
			auto& clit = core.current_chain.back();
			add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, clit.triggering_player, 0);
		} else
			returns.ivalue[0] = 0;
		return FALSE;
	}
	case 4: {
		auto& clit = core.current_chain.back();
		chain& ch = core.select_chains[returns.ivalue[0]];
		int32 playerid = clit.triggering_player;
		effect* peffect = ch.triggering_effect;
		card* phandler = peffect->get_handler();
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_OPSELECTED);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(core.select_options[returns.ivalue[0]]);
		clit.triggering_effect = peffect;
		clit.evt = ch.evt;
		phandler->create_relation(clit);
		peffect->dec_count(playerid);
		if(!(peffect->type & EFFECT_TYPE_ACTIVATE)) {
			peffect->type |= EFFECT_TYPE_ACTIVATE;
			clit.flag |= CHAIN_ACTIVATING;
		}
		core.select_chains.clear();
		core.select_options.clear();
		effect* deffect = pduel->new_effect();
		deffect->owner = phandler;
		deffect->code = 0;
		deffect->type = EFFECT_TYPE_SINGLE;
		deffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_CLIENT_HINT;
		deffect->description = 65;
		deffect->reset_flag = RESET_CHAIN;
		phandler->add_effect(deffect);
		return FALSE;
	}
	case 5: {
		auto& clit = core.current_chain.back();
		effect* peffect = clit.triggering_effect;
		if(peffect->cost) {
			core.sub_solving_event.push_back(clit.evt);
			add_process(PROCESSOR_EXECUTE_COST, 0, peffect, 0, clit.triggering_player, 0);
		}
		return FALSE;
	}
	case 6: {
		auto& clit = core.current_chain.back();
		effect* peffect = clit.triggering_effect;
		if(peffect->target) {
			core.sub_solving_event.push_back(clit.evt);
			add_process(PROCESSOR_EXECUTE_TARGET, 0, peffect, 0, clit.triggering_player, 0);
		}
		return FALSE;
	}
	case 7: {
		break_effect();
		auto& clit = core.current_chain.back();
		effect* peffect = clit.triggering_effect;
		card* phandler = peffect->get_handler();
		if(clit.target_cards && clit.target_cards->container.size()) {
			if(peffect->is_flag(EFFECT_FLAG_CARD_TARGET)) {
				for(auto cit = clit.target_cards->container.begin(); cit != clit.target_cards->container.end(); ++cit)
					raise_single_event(*cit, 0, EVENT_BECOME_TARGET, peffect, 0, clit.triggering_player, 0, clit.chain_count);
				process_single_event();
				if(clit.target_cards->container.size())
					raise_event(&clit.target_cards->container, EVENT_BECOME_TARGET, peffect, 0, clit.triggering_player, clit.triggering_player, clit.chain_count);
			}
		}
		if(peffect->type & EFFECT_TYPE_ACTIVATE) {
			core.leave_confirmed.insert(phandler);
			if(!(phandler->data.type & (TYPE_CONTINUOUS + TYPE_FIELD + TYPE_EQUIP + TYPE_PENDULUM))
			        && !phandler->is_affected_by_effect(EFFECT_REMAIN_FIELD))
				phandler->set_status(STATUS_LEAVE_CONFIRMED, TRUE);
		}
		if((phandler->data.type & (TYPE_SPELL + TYPE_TRAP))
				&& (phandler->data.type & (TYPE_CONTINUOUS + TYPE_FIELD + TYPE_EQUIP + TYPE_PENDULUM))
				&& phandler->is_has_relation(clit) && phandler->current.location == LOCATION_SZONE
				&& !peffect->is_flag(EFFECT_FLAG_FIELD_ONLY))
			clit.flag |= CHAIN_CONTINUOUS_CARD;
		core.phase_action = TRUE;
		if(clit.opinfos.count(0x200) && clit.opinfos[0x200].op_count) {
			core.spsummon_rst = true;
			set_spsummon_counter(clit.triggering_player, true, true);
			if(clit.opinfos[0x200].op_player == PLAYER_ALL)
				set_spsummon_counter(1 - clit.triggering_player, true, true);
			if((core.global_flag & GLOBALFLAG_SPSUMMON_ONCE) && peffect->is_flag(EFFECT_FLAG_CARD_TARGET)) {
				auto& optarget = clit.opinfos[0x200];
				if(optarget.op_cards) {
					if(optarget.op_player == PLAYER_ALL) {
						auto opit = optarget.op_cards->container.begin();
						uint32 sumplayer = optarget.op_param;
						if((*opit)->spsummon_code) {
							core.spsummon_once_map[sumplayer][(*opit)->spsummon_code]++;
							core.spsummon_once_map_rst[sumplayer][(*opit)->spsummon_code]++;
						}
						++opit;
						if((*opit)->spsummon_code) {
							core.spsummon_once_map[1 - sumplayer][(*opit)->spsummon_code]++;
							core.spsummon_once_map_rst[1 - sumplayer][(*opit)->spsummon_code]++;
						}
					} else {
						uint32 sumplayer = clit.triggering_player;
						if(optarget.op_player == 1)
							sumplayer = 1 - sumplayer;
						for(auto opit = optarget.op_cards->container.begin(); opit != optarget.op_cards->container.end(); ++opit) {
							if((*opit)->spsummon_code) {
								core.spsummon_once_map[sumplayer][(*opit)->spsummon_code]++;
								core.spsummon_once_map_rst[sumplayer][(*opit)->spsummon_code]++;
							}
						}
					}
				}
			}
		}
		pduel->write_buffer8(MSG_CHAINED);
		pduel->write_buffer8(clit.chain_count);
		raise_event(phandler, EVENT_CHAINING, peffect, 0, clit.triggering_player, clit.triggering_player, clit.chain_count);
		process_instant_event();
		if(core.new_chains.size())
			add_process(PROCESSOR_ADD_CHAIN, 0, 0, 0, 0, 0);
		adjust_all();
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::sort_chain(uint16 step, uint8 tp) {
	switch(step) {
	case 0: {
		core.select_cards.clear();
		if(tp)
			for(auto clit = core.tpchain.begin(); clit != core.tpchain.end(); ++clit)
				core.select_cards.push_back(clit->triggering_effect->get_handler());
		else
			for(auto clit = core.ntpchain.begin(); clit != core.ntpchain.end(); ++clit)
				core.select_cards.push_back(clit->triggering_effect->get_handler());
		add_process(PROCESSOR_SORT_CARD, 0, 0, 0, tp ? infos.turn_player : (1 - infos.turn_player), 1);
		return FALSE;
	}
	case 1: {
		if(returns.bvalue[0] == -1)
			return TRUE;
		chain_list::iterator clit;
		int32 i = 0, count;
		if(tp) {
			count = core.tpchain.size();
			chain_array ch(count);
			for(i = 0, clit = core.tpchain.begin(); i < count; ++clit, ++i)
				ch[returns.bvalue[i]] = *clit;
			core.tpchain.clear();
			for(i = 0; i < count; ++i)
				core.tpchain.push_back(ch[i]);
		} else {
			count = core.ntpchain.size();
			chain_array ch(count);
			for(i = 0, clit = core.ntpchain.begin(); i < count; ++clit, ++i)
				ch[returns.bvalue[i]] = *clit;
			core.ntpchain.clear();
			for(i = 0; i < count; ++i)
				core.ntpchain.push_back(ch[i]);
		}
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::solve_continuous(uint16 step, effect * peffect, uint8 triggering_player) {
	switch(step) {
	case 0: {
		core.solving_event.splice(core.solving_event.begin(), core.sub_solving_event);
		if(!peffect->check_count_limit(triggering_player)) {
			core.solving_event.pop_front();
			return TRUE;
		}
		chain newchain;
		newchain.chain_id = 0;
		newchain.chain_count = 0;
		newchain.triggering_effect = peffect;
		newchain.triggering_player = triggering_player;
		newchain.evt = core.solving_event.front();
		newchain.target_cards = 0;
		newchain.target_player = PLAYER_NONE;
		newchain.target_param = 0;
		newchain.disable_player = PLAYER_NONE;
		newchain.disable_reason = 0;
		newchain.flag = 0;
		core.continuous_chain.push_back(newchain);
		if(peffect->is_flag(EFFECT_FLAG_DELAY) || !(peffect->code & 0x10030000) && (peffect->code & (EVENT_PHASE | EVENT_PHASE_START)))
			core.conti_solving = TRUE;
		core.units.begin()->ptarget = (group*)core.reason_effect;
		core.units.begin()->arg2 = core.reason_player;
		if(!peffect->target)
			return FALSE;
		core.sub_solving_event.push_back(core.solving_event.front());
		add_process(PROCESSOR_EXECUTE_TARGET, 0, peffect, 0, triggering_player, 0);
		return FALSE;
	}
	case 1: {
		return FALSE;
	}
	case 2: {
		if(!peffect->operation)
			return FALSE;
		peffect->dec_count(triggering_player);
		core.sub_solving_event.push_back(core.solving_event.front());
		add_process(PROCESSOR_EXECUTE_OPERATION, 0, peffect, 0, triggering_player, 0);
		return FALSE;
	}
	case 3: {
		core.reason_effect = (effect*)core.units.begin()->ptarget;
		core.reason_player = core.units.begin()->arg2;
		if(core.continuous_chain.back().target_cards)
			pduel->delete_group(core.continuous_chain.back().target_cards);
		for(auto oit = core.continuous_chain.back().opinfos.begin(); oit != core.continuous_chain.back().opinfos.end(); ++oit) {
			if(oit->second.op_cards)
				pduel->delete_group(oit->second.op_cards);
		}
		core.continuous_chain.pop_back();
		core.solving_event.pop_front();
		if(peffect->is_flag(EFFECT_FLAG_DELAY) || !(peffect->code & 0x10030000) && (peffect->code & (EVENT_PHASE | EVENT_PHASE_START))) {
			core.conti_solving = FALSE;
			adjust_all();
			return FALSE;
		}
		return TRUE;
	}
	case 4: {
		if(core.conti_player == PLAYER_NONE)
			core.conti_player = infos.turn_player;
		if(core.conti_player == infos.turn_player) {
			if(core.delayed_tp.size()) {
				core.sub_solving_event.push_back(core.delayed_tev.front());
				add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, core.delayed_tp.front(), 0, infos.turn_player, 0);
				core.delayed_tp.pop_front();
				core.delayed_tev.pop_front();
			} else
				core.conti_player = 1 - infos.turn_player;
		}
		if(core.conti_player == 1 - infos.turn_player) {
			if(core.delayed_ntp.size()) {
				core.sub_solving_event.push_back(core.delayed_ntev.front());
				add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, core.delayed_ntp.front(), 0, 1 - infos.turn_player, 0);
				core.delayed_ntp.pop_front();
				core.delayed_ntev.pop_front();
			} else if(core.delayed_tp.size()) {
				core.conti_player = infos.turn_player;
				core.sub_solving_event.push_back(core.delayed_tev.front());
				add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, core.delayed_tp.front(), 0, infos.turn_player, 0);
				core.delayed_tp.pop_front();
				core.delayed_tev.pop_front();
			} else
				core.conti_player = PLAYER_NONE;
		}
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::solve_chain(uint16 step, uint32 chainend_arg1, uint32 chainend_arg2) {
	if(core.current_chain.size() == 0 && step == 0)
		return TRUE;
	auto cait = core.current_chain.rbegin();
	switch(step) {
	case 0: {
		if(core.spsummon_rst) {
			set_spsummon_counter(0, false, true);
			set_spsummon_counter(1, false, true);
			for(int plr = 0; plr < 2; ++plr) {
				for(auto iter = core.spsummon_once_map[plr].begin(); iter != core.spsummon_once_map[plr].end(); ++iter) {
					auto spcode = iter->first;
					core.spsummon_once_map[plr][spcode] -= core.spsummon_once_map_rst[plr][spcode];
					core.spsummon_once_map_rst[plr][spcode] = 0;
				}
			}
			core.spsummon_rst = false;
		}
		pduel->write_buffer8(MSG_CHAIN_SOLVING);
		pduel->write_buffer8(cait->chain_count);
		add_to_disable_check_list(cait->triggering_effect->get_handler());
		adjust_instant();
		raise_event((card*)0, EVENT_CHAIN_ACTIVATING, cait->triggering_effect, 0, cait->triggering_player, cait->triggering_player, cait->chain_count);
		process_instant_event();
		return FALSE;
	}
	case 1: {
		effect* peffect = cait->triggering_effect;
		int32 activate = (peffect->type & EFFECT_TYPE_ACTIVATE);
		if(activate && (cait->flag & CHAIN_ACTIVATING))
			peffect->type &= ~EFFECT_TYPE_ACTIVATE;
		if(cait->flag & CHAIN_DISABLE_ACTIVATE && is_chain_negatable(cait->chain_count)) {
			remove_oath_effect(peffect);
			if(peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT) && (peffect->count_code & EFFECT_COUNT_CODE_OATH)) {
				dec_effect_code(peffect->count_code, cait->triggering_player);
			}
			check_chain_counter(peffect, cait->triggering_player, cait->chain_count, true);
			raise_event((card*)0, EVENT_CHAIN_NEGATED, peffect, 0, cait->triggering_player, cait->triggering_player, cait->chain_count);
			process_instant_event();
			core.units.begin()->step = 9;
			return FALSE;
		}
		for(auto oeit = effects.oath.begin(); oeit != effects.oath.end(); ++oeit)
			if(oeit->second == peffect)
				oeit->second = 0;
		break_effect();
		core.chain_solving = TRUE;
		card* pcard = peffect->get_handler();
		if(activate && pcard->is_has_relation(*cait)) {
			pcard->set_status(STATUS_ACTIVATED, TRUE);
			pcard->enable_field_effect(true);
			if(core.duel_rule <= 2) {
				if(pcard->data.type & TYPE_FIELD) {
					card* fscard = player[1 - pcard->current.controler].list_szone[5];
					if(fscard && fscard->is_position(POS_FACEUP))
						fscard->enable_field_effect(false);
				}
			}
			adjust_instant();
		}
		raise_event((card*)0, EVENT_CHAIN_SOLVING, peffect, 0, cait->triggering_player, cait->triggering_player, cait->chain_count);
		process_instant_event();
		return FALSE;
	}
	case 2: {
		core.spsummon_state_count_tmp[0] = core.spsummon_state_count[0];
		core.spsummon_state_count_tmp[1] = core.spsummon_state_count[1];
		card* pcard = cait->triggering_effect->get_handler();
		if(is_chain_disablable(cait->chain_count)) {
			if(is_chain_disabled(cait->chain_count) || (pcard->get_status(STATUS_DISABLED | STATUS_FORBIDDEN) && pcard->is_has_relation(*cait))) {
				if(!(cait->flag & CHAIN_DISABLE_EFFECT)) {
					pduel->write_buffer8(MSG_CHAIN_DISABLED);
					pduel->write_buffer8(cait->chain_count);
				}
				raise_event((card*)0, EVENT_CHAIN_DISABLED, cait->triggering_effect, 0, cait->triggering_player, cait->triggering_player, cait->chain_count);
				process_instant_event();
				core.units.begin()->step = 3;
				return FALSE;
			}
		}
		if(cait->replace_op) {
			core.units.begin()->arg4 = cait->triggering_effect->operation;
			cait->triggering_effect->operation = cait->replace_op;
		} else
			core.units.begin()->arg4 = 0;
		if((cait->flag & CHAIN_CONTINUOUS_CARD) && !pcard->is_has_relation(*cait))
			return FALSE;
		if(cait->triggering_effect->operation) {
			core.sub_solving_event.push_back(cait->evt);
			add_process(PROCESSOR_EXECUTE_OPERATION, 0, cait->triggering_effect, 0, cait->triggering_player, 0);
		}
		return FALSE;
	}
	case 3: {
		effect* peffect = cait->triggering_effect;
		if(core.units.begin()->arg4) {
			peffect->operation = core.units.begin()->arg4;
		}
		core.special_summoning.clear();
		core.equiping_cards.clear();
		return FALSE;
	}
	case 4: {
		if(core.units.begin()->arg4 == 0) {
			if(cait->opinfos.count(0x200) && cait->opinfos[0x200].op_count) {
				if(core.spsummon_state_count_tmp[cait->triggering_player] == core.spsummon_state_count[cait->triggering_player])
					set_spsummon_counter(cait->triggering_player);
				if(cait->opinfos[0x200].op_player == PLAYER_ALL && core.spsummon_state_count_tmp[1 - cait->triggering_player] == core.spsummon_state_count[1 - cait->triggering_player])
					set_spsummon_counter(1 - cait->triggering_player);
				//sometimes it may add twice, only works for once per turn
				if(cait->triggering_effect->is_flag(EFFECT_FLAG_CARD_TARGET)) {
					auto& optarget = cait->opinfos[0x200];
					if(optarget.op_cards) {
						if(optarget.op_player == PLAYER_ALL) {
							uint32 sumplayer = optarget.op_param;
							if(core.global_flag & GLOBALFLAG_SPSUMMON_ONCE) {
								auto opit = optarget.op_cards->container.begin();
								if((*opit)->spsummon_code)
									core.spsummon_once_map[sumplayer][(*opit)->spsummon_code]++;
								++opit;
								if((*opit)->spsummon_code)
									core.spsummon_once_map[1 - sumplayer][(*opit)->spsummon_code]++;
							}
							auto opit = optarget.op_cards->container.begin();
							check_card_counter(*opit, 3, sumplayer);
							++opit;
							check_card_counter(*opit, 3, 1 - sumplayer);
						} else {
							uint32 sumplayer = cait->triggering_player;
							if(optarget.op_player == 1)
								sumplayer = 1 - sumplayer;
							for(auto opit = optarget.op_cards->container.begin(); opit != optarget.op_cards->container.end(); ++opit) {
								if((core.global_flag & GLOBALFLAG_SPSUMMON_ONCE) && (*opit)->spsummon_code)
									core.spsummon_once_map[sumplayer][(*opit)->spsummon_code]++;
								check_card_counter(*opit, 3, sumplayer);
							}
						}
					}
				}
			}
		}
		core.spsummon_state_count_tmp[0] = 0;
		core.spsummon_state_count_tmp[1] = 0;
		core.chain_solving = FALSE;
		if(core.delayed_tp.size()) {
			core.conti_player = infos.turn_player;
			core.sub_solving_event.push_back(core.delayed_tev.front());
			add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, core.delayed_tp.front(), 0, infos.turn_player, 0);
			core.delayed_tp.pop_front();
			core.delayed_tev.pop_front();
		} else if(core.delayed_ntp.size()) {
			core.conti_player = 1 - infos.turn_player;
			core.sub_solving_event.push_back(core.delayed_ntev.front());
			add_process(PROCESSOR_SOLVE_CONTINUOUS, 0, core.delayed_ntp.front(), 0, 1 - infos.turn_player, 0);
			core.delayed_ntp.pop_front();
			core.delayed_ntev.pop_front();
		} else
			core.conti_player = PLAYER_NONE;
		pduel->write_buffer8(MSG_CHAIN_SOLVED);
		pduel->write_buffer8(cait->chain_count);
		raise_event((card*)0, EVENT_CHAIN_SOLVED, cait->triggering_effect, 0, cait->triggering_player, cait->triggering_player, cait->chain_count);
		adjust_disable_check_list();
		process_instant_event();
		core.units.begin()->step = 9;
		return FALSE;
	}
	case 10: {
		effect* peffect = cait->triggering_effect;
		card* pcard = peffect->get_handler();
		if((cait->flag & CHAIN_HAND_EFFECT) && !pcard->is_position(POS_FACEUP) && (pcard->current.location == LOCATION_HAND))
			shuffle(pcard->current.controler, LOCATION_HAND);
		if(cait->target_cards && cait->target_cards->container.size()) {
			for(auto cit = cait->target_cards->container.begin(); cit != cait->target_cards->container.end(); ++cit)
				(*cit)->release_relation(*cait);
		}
		if((pcard->data.type & TYPE_EQUIP) && (peffect->type & EFFECT_TYPE_ACTIVATE)
		        && !pcard->equiping_target && pcard->is_has_relation(*cait))
			destroy(pcard, 0, REASON_RULE, PLAYER_NONE);
		if(core.duel_rule <= 2) {
			if((pcard->data.type & TYPE_FIELD) && (peffect->type & EFFECT_TYPE_ACTIVATE)
					&& !pcard->is_status(STATUS_LEAVE_CONFIRMED) && pcard->is_has_relation(*cait)) {
				card* fscard = player[1 - pcard->current.controler].list_szone[5];
				if(fscard && fscard->is_position(POS_FACEUP))
					destroy(fscard, 0, REASON_RULE, 1 - pcard->current.controler);
			}
		}
		peffect->active_type = 0;
		peffect->active_handler = 0;
		pcard->release_relation(*cait);
		if(cait->target_cards)
			pduel->delete_group(cait->target_cards);
		for(auto oit = cait->opinfos.begin(); oit != cait->opinfos.end(); ++oit) {
			if(oit->second.op_cards)
				pduel->delete_group(oit->second.op_cards);
		}
		for(auto cit = core.delayed_enable_set.begin(); cit != core.delayed_enable_set.end(); ++cit) {
			if((*cit)->current.location == LOCATION_MZONE)
				(*cit)->enable_field_effect(true);
		}
		core.delayed_enable_set.clear();
		adjust_all();
		core.current_chain.pop_back();
		if(!core.current_chain.size()) {
			for(auto it = core.chain_limit.begin(); it != core.chain_limit.end(); ++it)
				luaL_unref(pduel->lua->lua_state, LUA_REGISTRYINDEX, it->function);
			core.chain_limit.clear();
			return FALSE;
		}
		core.units.begin()->step = -1;
		return FALSE;
	}
	case 11: {
		for(auto cit = core.leave_confirmed.begin(); cit != core.leave_confirmed.end();) {
			auto rm = cit++;
			if(!(*rm)->is_status(STATUS_LEAVE_CONFIRMED))
				core.leave_confirmed.erase(rm);
		}
		if(core.leave_confirmed.size())
			send_to(&core.leave_confirmed, 0, REASON_RULE, PLAYER_NONE, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
		return FALSE;
	}
	case 12: {
		core.used_event.splice(core.used_event.end(), core.point_event);
		pduel->write_buffer8(MSG_CHAIN_END);
		for(auto it = core.chain_limit_p.begin(); it != core.chain_limit_p.end(); ++it)
			luaL_unref(pduel->lua->lua_state, LUA_REGISTRYINDEX, it->function);
		core.chain_limit_p.clear();
		reset_chain();
		if(core.summoning_card || core.effect_damage_step == 1)
			core.subunits.push_back(core.reserved);
		return FALSE;
	}
	case 13: {
		raise_event((card*)0, EVENT_CHAIN_END, 0, 0, 0, 0, 0);
		process_instant_event();
		adjust_all();
		if(chainend_arg1 != 0x101 || chainend_arg2 != TRUE) {
			core.hint_timing[0] |= TIMING_CHAIN_END;
			core.hint_timing[1] |= TIMING_CHAIN_END;
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, chainend_arg1, chainend_arg2);
		}
		returns.ivalue[0] = TRUE;
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::break_effect() {
	core.hint_timing[0] &= TIMING_DAMAGE_STEP | TIMING_DAMAGE_CAL;
	core.hint_timing[1] &= TIMING_DAMAGE_STEP | TIMING_DAMAGE_CAL;
	for (auto chit = core.new_ochain.begin(); chit != core.new_ochain.end();) {
		auto rm = chit++;
		effect* peffect = rm->triggering_effect;
		if (!peffect->is_flag(EFFECT_FLAG_DELAY)) {
			if (peffect->is_flag(EFFECT_FLAG_FIELD_ONLY)
			        || !(peffect->type & EFFECT_TYPE_FIELD) || peffect->in_range(*rm)) {
				pduel->write_buffer8(MSG_MISSED_EFFECT);
				pduel->write_buffer32(peffect->get_handler()->get_info_location());
				pduel->write_buffer32(peffect->get_handler()->data.code);
			}
			core.new_ochain.erase(rm);
		}
	}
	core.delayed_quick_break.insert(core.delayed_quick_tmp.begin(), core.delayed_quick_tmp.end());
	core.delayed_quick_tmp.clear();
	core.used_event.splice(core.used_event.end(), core.instant_event);
	adjust_instant();
	return 0;
}
// adjust, type 1
// adjust disable, self_destroy
void field::adjust_instant() {
	adjust_disable_check_list();
	adjust_self_destroy_set();
}
// adjust, type 2 (including adjust_instant())
// adjust win, disable, control, self_destroy, equip, position, trap_monster
void field::adjust_all() {
	core.readjust_map.clear();
	add_process(PROCESSOR_ADJUST, 0, 0, 0, 0, 0);
}
void field::refresh_location_info_instant() {
	effect_set eset;
	uint32 value, p;
	uint32 dis1 = player[0].disabled_location | (player[1].disabled_location << 16);
	player[0].disabled_location = 0;
	player[1].disabled_location = 0;
	filter_field_effect(EFFECT_DISABLE_FIELD, &eset);
	for (int32 i = 0; i < eset.size(); ++i) {
		value = eset[i]->get_value();
		player[0].disabled_location |= value & 0x1f7f;
		player[1].disabled_location |= (value >> 16) & 0x1f7f;
	}
	eset.clear();
	filter_field_effect(EFFECT_USE_EXTRA_MZONE, &eset);
	for (int32 i = 0; i < eset.size(); ++i) {
		p = eset[i]->get_handler_player();
		value = eset[i]->get_value();
		player[p].disabled_location |= (value >> 16) & 0x1f;
	}
	eset.clear();
	filter_field_effect(EFFECT_USE_EXTRA_SZONE, &eset);
	for (int32 i = 0; i < eset.size(); ++i) {
		p = eset[i]->get_handler_player();
		value = eset[i]->get_value();
		player[p].disabled_location |= (value >> 8) & 0x1f00;
	}
	player[0].disabled_location |= (((player[1].disabled_location >> 5) & 1) << 6) | (((player[1].disabled_location >> 6) & 1) << 5);
	player[1].disabled_location |= (((player[0].disabled_location >> 5) & 1) << 6) | (((player[0].disabled_location >> 6) & 1) << 5);
	uint32 dis2 = player[0].disabled_location | (player[1].disabled_location << 16);
	if(dis1 != dis2) {
		pduel->write_buffer8(MSG_FIELD_DISABLED);
		pduel->write_buffer32(dis2);
	}
}
int32 field::refresh_location_info(uint16 step) {
	switch(step) {
	case 0: {
		effect_set eset;
		uint32 value;
		uint32 p;
		core.units.begin()->arg2 = player[0].disabled_location | (player[1].disabled_location << 16);
		player[0].disabled_location = 0;
		player[1].disabled_location = 0;
		core.disfield_effects.clear();
		core.extram_effects.clear();
		core.extras_effects.clear();
		filter_field_effect(EFFECT_DISABLE_FIELD, &eset);
		for (int32 i = 0; i < eset.size(); ++i) {
			value = eset[i]->get_value();
			if(value) {
				player[0].disabled_location |= value & 0x1f7f;
				player[1].disabled_location |= (value >> 16) & 0x1f7f;
			} else
				core.disfield_effects.add_item(eset[i]);
		}
		eset.clear();
		filter_field_effect(EFFECT_USE_EXTRA_MZONE, &eset);
		for (int32 i = 0; i < eset.size(); ++i) {
			p = eset[i]->get_handler_player();
			value = eset[i]->get_value();
			player[p].disabled_location |= (value >> 16) & 0x1f;
			if((uint32)field_used_count[(value >> 16) & 0x1f] < (value & 0xffff))
				core.extram_effects.add_item(eset[i]);
		}
		eset.clear();
		filter_field_effect(EFFECT_USE_EXTRA_SZONE, &eset);
		for (int32 i = 0; i < eset.size(); ++i) {
			p = eset[i]->get_handler_player();
			value = eset[i]->get_value();
			player[p].disabled_location |= (value >> 8) & 0x1f00;
			if((uint32)field_used_count[(value >> 16) & 0x1f] < (value & 0xffff))
				core.extras_effects.add_item(eset[i]);
		}
		return FALSE;
	}
	case 1: {
		if(core.disfield_effects.count == 0) {
			core.units.begin()->step = 2;
			return FALSE;
		}
		effect* peffect = core.disfield_effects[0];
		core.units.begin()->peffect = peffect;
		core.disfield_effects.remove_item(0);
		if(!peffect->operation) {
			peffect->value = 0x80;
			core.units.begin()->step = 0;
			return FALSE;
		}
		core.sub_solving_event.push_back(nil_event);
		add_process(PROCESSOR_EXECUTE_OPERATION, 0, peffect, 0, peffect->get_handler_player(), 0);
		return FALSE;
	}
	case 2: {
		returns.ivalue[0] &= 0x1f7f1f7f;
		if(returns.ivalue[0] == 0)
			returns.ivalue[0] = 0x80;
		if(core.units.begin()->peffect->get_handler_player() == 0) {
			core.units.begin()->peffect->value = returns.ivalue[0];
			player[0].disabled_location |= returns.ivalue[0] & 0x1f7f;
			player[1].disabled_location |= (returns.ivalue[0] >> 16) & 0x1f7f;
		} else {
			core.units.begin()->peffect->value = ((returns.ivalue[0] << 16) | (returns.ivalue[0] >> 16));
			player[1].disabled_location |= returns.ivalue[0] & 0x1f7f;
			player[0].disabled_location |= (returns.ivalue[0] >> 16) & 0x1f7f;
		}
		core.units.begin()->step = 0;
		return FALSE;
	}
	case 3: {
		// If the blocking number is not reached, we should block more slots.
		if(core.extram_effects.count == 0) {
			core.units.begin()->step = 4;
			return FALSE;
		}
		effect* peffect = core.extram_effects[0];
		core.units.begin()->peffect = peffect;
		core.extram_effects.remove_item(0);
		uint32 p = peffect->get_handler_player();
		uint32 mzone_flag = (player[p].disabled_location | player[p].used_location) & 0x1f;
		if(mzone_flag == 0x1f) {
			core.units.begin()->step = 4;
			return FALSE;
		}
		int32 val = peffect->get_value();
		int32 dis_count = (val & 0xffff) - field_used_count[(val >> 16) & 0x1f];
		int32 empty_count = 5 - field_used_count[mzone_flag];
		uint32 flag = mzone_flag | 0xffffffe0;
		if(dis_count > empty_count)
			dis_count = empty_count;
		core.units.begin()->arg1 = dis_count;
		add_process(PROCESSOR_SELECT_DISFIELD, 0, 0, 0, p, flag, dis_count);
		return FALSE;
	}
	case 4: {
		uint32 dis_count = core.units.begin()->arg1;
		uint32 mzone_flag = 0;
		uint8 s = 0, pt = 0;
		for(uint32 i = 0; i < dis_count; ++i) {
			s = returns.bvalue[pt + 2];
			mzone_flag |= 0x1u << s;
			pt += 3;
		}
		effect* peffect = core.units.begin()->peffect;
		player[peffect->get_handler_player()].disabled_location |= mzone_flag;
		peffect->value = (int32)(peffect->value | (mzone_flag << 16));
		core.units.begin()->step = 2;
		return FALSE;
	}
	case 5: {
		// EFFECT_USE_EXTRA_SZONE version
		if(core.extras_effects.count == 0) {
			core.units.begin()->step = 6;
			return FALSE;
		}
		effect* peffect = core.extras_effects[0];
		core.units.begin()->peffect = peffect;
		core.extras_effects.remove_item(0);
		uint32 p = peffect->get_handler_player();
		uint32 szone_flag = ((player[p].disabled_location | player[p].used_location) >> 8) & 0x1f;
		if(szone_flag == 0x1f) {
			core.units.begin()->step = 6;
			return FALSE;
		}
		int32 val = peffect->get_value();
		uint32 dis_count = (val & 0xffff) - field_used_count[(val >> 16) & 0x1f];
		uint32 empty_count = 5 - field_used_count[szone_flag];
		uint32 flag = (szone_flag << 8) | 0xffffe0ff;
		if(dis_count > empty_count)
			dis_count = empty_count;
		core.units.begin()->arg1 = dis_count;
		add_process(PROCESSOR_SELECT_DISFIELD, 0, 0, 0, p, flag, dis_count);
		return FALSE;
	}
	case 6: {
		uint32 dis_count = core.units.begin()->arg1;
		uint32 szone_flag = 0;
		uint8 s = 0, pt = 0;
		for(uint32 i = 0; i < dis_count; ++i) {
			s = returns.bvalue[pt + 2];
			szone_flag |= 0x1u << s;
			pt += 3;
		}
		effect* peffect = core.units.begin()->peffect;
		player[peffect->get_handler_player()].disabled_location |= szone_flag << 8;
		peffect->value = (int32)(peffect->value | (szone_flag << 16));
		core.units.begin()->step = 4;
		return FALSE;
	}
	case 7: {
		player[0].disabled_location |= (((player[1].disabled_location >> 5) & 1) << 6) | (((player[1].disabled_location >> 6) & 1) << 5);
		player[1].disabled_location |= (((player[0].disabled_location >> 5) & 1) << 6) | (((player[0].disabled_location >> 6) & 1) << 5);
		uint32 dis = player[0].disabled_location | (player[1].disabled_location << 16);
		if(dis != (uint32)core.units.begin()->arg2) {
			pduel->write_buffer8(MSG_FIELD_DISABLED);
			pduel->write_buffer32(dis);
		}
		return TRUE;
	}
	}
	return TRUE;
}
// adjust_all() goes here
int32 field::adjust_step(uint16 step) {
	switch(step) {
	case 0: {
		core.re_adjust = FALSE;
		return FALSE;
	}
	case 1: {
		//win check(deck=0 or lp=0)
		uint32 winp = 5, rea = 1;
		if(player[0].lp <= 0 && player[1].lp > 0) {
			winp = 1;
			rea = 1;
		}
		if(core.overdraw[0] && !core.overdraw[1]) {
			winp = 1;
			rea = 2;
		}
		if(player[1].lp <= 0 && player[0].lp > 0) {
			winp = 0;
			rea = 1;
		}
		if(core.overdraw[1] && !core.overdraw[0]) {
			winp = 0;
			rea = 2;
		}
		if(player[1].lp <= 0 && player[0].lp <= 0) {
			winp = PLAYER_NONE;
			rea = 1;
		}
		if(core.overdraw[1] && core.overdraw[0]) {
			winp = PLAYER_NONE;
			rea = 2;
		}
		if(winp != 5) {
			pduel->write_buffer8(MSG_WIN);
			pduel->write_buffer8(winp);
			pduel->write_buffer8(rea);
			core.overdraw[0] = core.overdraw[1] = FALSE;
			core.win_player = 5;
			core.win_reason = 0;
		} else if(core.win_player != 5) {
			pduel->write_buffer8(MSG_WIN);
			pduel->write_buffer8(core.win_player);
			pduel->write_buffer8(core.win_reason);
			core.win_player = 5;
			core.win_reason = 0;
			core.overdraw[0] = core.overdraw[1] = FALSE;
		}
		return FALSE;
	}
	case 2: {
		//disable check
		uint8 tp = infos.turn_player;
		for(uint8 p = 0; p < 2; ++p) {
			for(auto cit = player[tp].list_mzone.begin(); cit != player[tp].list_mzone.end(); ++cit) {
				card* pcard = *cit;
				if(pcard)
					add_to_disable_check_list(pcard);
			}
			for(auto cit = player[tp].list_szone.begin(); cit != player[tp].list_szone.end(); ++cit) {
				card* pcard = *cit;
				if(pcard)
					add_to_disable_check_list(pcard);
			}
			tp = 1 - tp;
		}
		adjust_disable_check_list();
		add_process(PROCESSOR_REFRESH_LOC, 0, 0, 0, 0, 0);
		return FALSE;
	}
	case 3: {
		//trap monster
		uint8 tp = infos.turn_player;
		for(uint8 p = 0; p < 2; ++p) {
			for(auto cit = player[tp].list_mzone.begin(); cit != player[tp].list_mzone.end(); ++cit) {
				card* pcard = *cit;
				if(!pcard) continue;
				if((pcard->get_type() & TYPE_TRAPMONSTER) && pcard->is_affected_by_effect(EFFECT_DISABLE_TRAPMONSTER)) {
					pcard->reset(RESET_TURN_SET, RESET_EVENT);
					refresh_location_info_instant();
					move_to_field(pcard, tp, tp, LOCATION_SZONE, pcard->current.position, FALSE, 2);
					core.re_adjust = TRUE;
				}
			}
			tp = 1 - tp;
		}
		return FALSE;
	}
	case 4: {
		//1-5 control
		core.control_adjust_set[0].clear();
		core.control_adjust_set[1].clear();
		for(uint8 p = 0; p < 2; ++p) {
			for(auto cit = player[p].list_mzone.begin(); cit != player[p].list_mzone.end(); ++cit) {
				card* pcard = *cit;
				if(!pcard) continue;
				uint8 cur = pcard->current.controler;
				uint8 ref = pcard->refresh_control_status();
				if(cur != ref && pcard->is_capable_change_control())
					core.control_adjust_set[p].insert(pcard);
			}
		}
		if(core.control_adjust_set[0].size() || core.control_adjust_set[1].size()) {
			core.re_adjust = TRUE;
			add_process(PROCESSOR_CONTROL_ADJUST, 0, 0, 0, 0, 0);
		}
		core.last_control_changed_id = infos.field_id;
		return FALSE;
	}
	case 5: {
		//remove brainwashing
		if(core.global_flag & GLOBALFLAG_BRAINWASHING_CHECK) {
			core.control_adjust_set[0].clear();
			core.control_adjust_set[1].clear();
			effect_set eset;
			filter_field_effect(EFFECT_REMOVE_BRAINWASHING, &eset, FALSE);
			uint32 res = eset.size() ? TRUE : FALSE;
			if(res) {
				for(uint8 p = 0; p < 2; ++p) {
					for(auto cit = player[p].list_mzone.begin(); cit != player[p].list_mzone.end(); ++cit) {
						card* pcard = *cit;
						// remove EFFECT_SET_CONTROL
						if(pcard && pcard->is_affected_by_effect(EFFECT_REMOVE_BRAINWASHING)) {
							pcard->reset(EFFECT_SET_CONTROL, RESET_CODE);
							if(p != pcard->owner && pcard->is_capable_change_control())
								core.control_adjust_set[p].insert(pcard);
						}
					}
				}
			}
			core.remove_brainwashing = res;
			if(core.control_adjust_set[0].size() || core.control_adjust_set[1].size()) {
				core.re_adjust = TRUE;
				add_process(PROCESSOR_CONTROL_ADJUST, 0, 0, 0, 0, 0);
			}
		}
		core.units.begin()->step = 7;
		return FALSE;
	}
	case 8: {
		if(adjust_grant_effect())
			core.re_adjust = TRUE;
		return FALSE;
	}
	case 9: {
		if(core.selfdes_disabled) {
			core.units.begin()->step = 10;
			return FALSE;
		}
		//self destroy
		adjust_self_destroy_set();
		return FALSE;
	}
	case 10: {
		//equip check
		uint8 tp = infos.turn_player;
		card_set destroy_set;
		for(uint8 p = 0; p < 2; ++p) {
			for(uint8 i = 0; i < 5; ++i) {
				card* pcard = player[tp].list_szone[i];
				if(pcard && pcard->equiping_target && !pcard->is_affected_by_effect(EFFECT_EQUIP_LIMIT, pcard->equiping_target))
					destroy_set.insert(pcard);
			}
			tp = 1 - tp;
		}
		if(destroy_set.size()) {
			core.re_adjust = TRUE;
			destroy(&destroy_set, 0, REASON_RULE, PLAYER_NONE);
		}
		return FALSE;
	}
	case 11: {
		//position
		uint32 tp = infos.turn_player, pos;
		card_set pos_adjust;
		effect_set eset;
		for(uint8 p = 0; p < 2; ++p) {
			for(auto cit = player[tp].list_mzone.begin(); cit != player[tp].list_mzone.end(); ++cit) {
				card* pcard = *cit;
				if(!pcard || (pcard->data.type & TYPE_LINK) || pcard->is_affected_by_effect(EFFECT_CANNOT_CHANGE_POS_E))
					continue;
				eset.clear();
				pcard->filter_effect(EFFECT_SET_POSITION, &eset);
				if(eset.size()) {
					pos = eset.get_last()->get_value();
					if((pos & 0xff) != pcard->current.position) {
						pos_adjust.insert(pcard);
						pcard->position_param = pos;
						if(pcard->is_status(STATUS_JUST_POS))
							pcard->set_status(STATUS_CONTINUOUS_POS, TRUE);
						else
							pcard->set_status(STATUS_CONTINUOUS_POS, FALSE);
					} else
						pcard->set_status(STATUS_CONTINUOUS_POS, FALSE);
					pcard->set_status(STATUS_JUST_POS, FALSE);
				}
			}
			tp = 1 - tp;
		}
		if(pos_adjust.size()) {
			core.re_adjust = TRUE;
			group* ng = pduel->new_group();
			ng->container.swap(pos_adjust);
			ng->is_readonly = TRUE;
			add_process(PROCESSOR_CHANGEPOS, 0, 0, ng, PLAYER_NONE, TRUE);
		}
		return FALSE;
	}
	case 12: {
		//shuffle check
		for(uint32 i = 0; i < player[0].list_hand.size(); ++i) {
			card* pcard = player[0].list_hand[i];
			effect* pub = pcard->is_affected_by_effect(EFFECT_PUBLIC);
			if(!pub && pcard->is_position(POS_FACEUP))
				core.shuffle_hand_check[0] = TRUE;
			pcard->current.position = pub ? POS_FACEUP : POS_FACEDOWN;
		}
		for(uint32 i = 0; i < player[1].list_hand.size(); ++i) {
			card* pcard = player[1].list_hand[i];
			effect* pub = pcard->is_affected_by_effect(EFFECT_PUBLIC);
			if(!pub && pcard->is_position(POS_FACEUP))
				core.shuffle_hand_check[1] = TRUE;
			pcard->current.position = pub ? POS_FACEUP : POS_FACEDOWN;
		}
		if(core.shuffle_hand_check[infos.turn_player])
			shuffle(infos.turn_player, LOCATION_HAND);
		if(core.shuffle_hand_check[1 - infos.turn_player])
			shuffle(1 - infos.turn_player, LOCATION_HAND);
		return FALSE;
	}
	case 13: {
		//reverse_deck
		effect_set eset;
		uint32 res = 0;
		if(core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK) {
			filter_field_effect(EFFECT_REVERSE_DECK, &eset, FALSE);
			res = eset.size() ? TRUE : FALSE;
			if(core.deck_reversed ^ res) {
				reverse_deck(0);
				reverse_deck(1);
				pduel->write_buffer8(MSG_REVERSE_DECK);
				if(res) {
					if(player[0].list_main.size()) {
						card* ptop = player[0].list_main.back();
						pduel->write_buffer8(MSG_DECK_TOP);
						pduel->write_buffer8(0);
						pduel->write_buffer8(0);
						if(ptop->current.position != POS_FACEUP_DEFENSE)
							pduel->write_buffer32(ptop->data.code);
						else
							pduel->write_buffer32(ptop->data.code | 0x80000000);
					}
					if(player[1].list_main.size()) {
						card* ptop = player[1].list_main.back();
						pduel->write_buffer8(MSG_DECK_TOP);
						pduel->write_buffer8(1);
						pduel->write_buffer8(0);
						if(ptop->current.position != POS_FACEUP_DEFENSE)
							pduel->write_buffer32(ptop->data.code);
						else
							pduel->write_buffer32(ptop->data.code | 0x80000000);
					}
				}
			}
			core.deck_reversed = res;
		}
		return FALSE;
	}
	case 14: {
		//attack cancel
		card* attacker = core.attacker;
		if(!attacker)
			return FALSE;
		if(attacker->is_affected_by_effect(EFFECT_CANNOT_ATTACK))
			attacker->set_status(STATUS_ATTACK_CANCELED, TRUE);
		if(core.attack_rollback)
			return FALSE;
		std::set<uint16> fidset;
		for(uint32 i = 0; i < player[1 - infos.turn_player].list_mzone.size(); ++i) {
			card* pcard = player[1 - infos.turn_player].list_mzone[i];
			if(pcard)
				fidset.insert(pcard->fieldid_r);
		}
		if(fidset != core.opp_mzone)
			core.attack_rollback = TRUE;
		return FALSE;
	}
	case 15: {
		raise_event((card*)0, EVENT_ADJUST, 0, 0, PLAYER_NONE, PLAYER_NONE, 0);
		process_instant_event();
		return FALSE;
	}
	case 16: {
		if(core.re_adjust) {
			core.units.begin()->step = -1;
			return FALSE;
		}
		if(core.shuffle_hand_check[0])
			shuffle(0, LOCATION_HAND);
		if(core.shuffle_hand_check[1])
			shuffle(1, LOCATION_HAND);
		if(core.shuffle_deck_check[0])
			shuffle(0, LOCATION_DECK);
		if(core.shuffle_deck_check[1])
			shuffle(1, LOCATION_DECK);
		return TRUE;
	}
	}
	return TRUE;
}
