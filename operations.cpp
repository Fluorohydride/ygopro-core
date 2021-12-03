/*
 * operations.cpp
 *
 *  Created on: 2010-9-18
 *      Author: Argon
 */
#include "field.h"
#include "duel.h"
#include "card.h"
#include "effect.h"
#include "group.h"
#include "interpreter.h"
#include <algorithm>

int32 field::negate_chain(uint8 chaincount) {
	if(core.current_chain.size() == 0)
		return FALSE;
	if(chaincount > core.current_chain.size() || chaincount < 1)
		chaincount = (uint8)core.current_chain.size();
	chain& pchain = core.current_chain[chaincount - 1];
	if(!(pchain.flag & CHAIN_DISABLE_ACTIVATE) && is_chain_negatable(pchain.chain_count)
	        && pchain.triggering_effect->handler->is_affect_by_effect(core.reason_effect) ) {
		pchain.flag |= CHAIN_DISABLE_ACTIVATE;
		pchain.disable_reason = core.reason_effect;
		pchain.disable_player = core.reason_player;
		if((pchain.triggering_effect->type & EFFECT_TYPE_ACTIVATE) && (pchain.triggering_effect->handler->current.location == LOCATION_SZONE)) {
			pchain.triggering_effect->handler->set_status(STATUS_LEAVE_CONFIRMED, TRUE);
			pchain.triggering_effect->handler->set_status(STATUS_ACTIVATE_DISABLED, TRUE);
		}
		pduel->write_buffer8(MSG_CHAIN_NEGATED);
		pduel->write_buffer8(chaincount);
		if(pchain.triggering_location == LOCATION_DECK
			|| core.duel_rule >= 5 && pchain.triggering_location == LOCATION_EXTRA && (pchain.triggering_position & POS_FACEDOWN))
			pchain.triggering_effect->handler->release_relation(pchain);
		return TRUE;
	}
	return FALSE;
}
int32 field::disable_chain(uint8 chaincount) {
	if(core.current_chain.size() == 0)
		return FALSE;
	if(chaincount > core.current_chain.size() || chaincount < 1)
		chaincount = (uint8)core.current_chain.size();
	chain& pchain = core.current_chain[chaincount - 1];
	if(!(pchain.flag & CHAIN_DISABLE_EFFECT) && is_chain_disablable(pchain.chain_count)
	        && pchain.triggering_effect->handler->is_affect_by_effect(core.reason_effect)) {
		core.current_chain[chaincount - 1].flag |= CHAIN_DISABLE_EFFECT;
		core.current_chain[chaincount - 1].disable_reason = core.reason_effect;
		core.current_chain[chaincount - 1].disable_player = core.reason_player;
		pduel->write_buffer8(MSG_CHAIN_DISABLED);
		pduel->write_buffer8(chaincount);
		if(pchain.triggering_location == LOCATION_DECK
			|| core.duel_rule >= 5 && pchain.triggering_location == LOCATION_EXTRA && (pchain.triggering_position & POS_FACEDOWN))
			pchain.triggering_effect->handler->release_relation(pchain);
		return TRUE;
	}
	return FALSE;
}
void field::change_chain_effect(uint8 chaincount, int32 rep_op) {
	if(core.current_chain.size() == 0)
		return;
	if(chaincount > core.current_chain.size() || chaincount < 1)
		chaincount = (uint8)core.current_chain.size();
	chain& pchain = core.current_chain[chaincount - 1];
	pchain.replace_op = rep_op;
	if((pchain.triggering_effect->type & EFFECT_TYPE_ACTIVATE) && (pchain.triggering_effect->handler->current.location == LOCATION_SZONE)) {
		pchain.triggering_effect->handler->set_status(STATUS_LEAVE_CONFIRMED, TRUE);
	}
}
void field::change_target(uint8 chaincount, group* targets) {
	if(core.current_chain.size() == 0)
		return;
	if(chaincount > core.current_chain.size() || chaincount < 1)
		chaincount = (uint8)core.current_chain.size();
	group* ot = core.current_chain[chaincount - 1].target_cards;
	if(ot) {
		effect* te = core.current_chain[chaincount - 1].triggering_effect;
		for(auto& pcard : ot->container)
			pcard->release_relation(core.current_chain[chaincount - 1]);
		ot->container = targets->container;
		for(auto& pcard : ot->container)
			pcard->create_relation(core.current_chain[chaincount - 1]);
		if(te->is_flag(EFFECT_FLAG_CARD_TARGET)) {
			for(auto& pcard : ot->container) {
				pduel->write_buffer8(MSG_BECOME_TARGET);
				pduel->write_buffer8(1);
				pduel->write_buffer32(pcard->get_info_location());
			}
		}
	}
}
void field::change_target_player(uint8 chaincount, uint8 playerid) {
	if(core.current_chain.size() == 0)
		return;
	if(chaincount > core.current_chain.size() || chaincount < 1)
		chaincount = (uint8)core.current_chain.size();
	core.current_chain[chaincount - 1].target_player = playerid;
}
void field::change_target_param(uint8 chaincount, int32 param) {
	if(core.current_chain.size() == 0)
		return;
	if(chaincount > core.current_chain.size() || chaincount < 1)
		chaincount = (uint8)core.current_chain.size();
	core.current_chain[chaincount - 1].target_param = param;
}
void field::remove_counter(uint32 reason, card* pcard, uint32 rplayer, uint32 s, uint32 o, uint32 countertype, uint32 count) {
	add_process(PROCESSOR_REMOVE_COUNTER, 0, NULL, (group*)pcard, (rplayer << 16) + (s << 8) + o, countertype, count, reason);
}
void field::remove_overlay_card(uint32 reason, card* pcard, uint32 rplayer, uint32 s, uint32 o, uint16 min, uint16 max) {
	add_process(PROCESSOR_REMOVE_OVERLAY, 0, NULL, (group*)pcard, (rplayer << 16) + (s << 8) + o, (max << 16) + min, reason);
}
void field::get_control(card_set* targets, effect* reason_effect, uint32 reason_player, uint32 playerid, uint32 reset_phase, uint32 reset_count, uint32 zone) {
	group* ng = pduel->new_group(*targets);
	ng->is_readonly = TRUE;
	add_process(PROCESSOR_GET_CONTROL, 0, reason_effect, ng, 0, (reason_player << 28) + (playerid << 24) + (reset_phase << 8) + reset_count, zone);
}
void field::get_control(card* target, effect* reason_effect, uint32 reason_player, uint32 playerid, uint32 reset_phase, uint32 reset_count, uint32 zone) {
	card_set tset;
	tset.insert(target);
	get_control(&tset, reason_effect, reason_player, playerid, reset_phase, reset_count, zone);
}
void field::swap_control(effect* reason_effect, uint32 reason_player, card_set* targets1, card_set* targets2, uint32 reset_phase, uint32 reset_count) {
	group* ng1 = pduel->new_group(*targets1);
	ng1->is_readonly = TRUE;
	group* ng2 = pduel->new_group(*targets2);
	ng2->is_readonly = TRUE;
	add_process(PROCESSOR_SWAP_CONTROL, 0, reason_effect, ng1, reason_player, reset_phase, reset_count, 0, ng2);
}
void field::swap_control(effect* reason_effect, uint32 reason_player, card* pcard1, card* pcard2, uint32 reset_phase, uint32 reset_count) {
	card_set tset1;
	tset1.insert(pcard1);
	card_set tset2;
	tset2.insert(pcard2);
	swap_control(reason_effect, reason_player, &tset1, &tset2, reset_phase, reset_count);
}
void field::equip(uint32 equip_player, card* equip_card, card* target, uint32 up, uint32 is_step) {
	add_process(PROCESSOR_EQUIP, 0, NULL, (group*)target, 0, equip_player + (up << 16) + (is_step << 24), 0, 0, equip_card);
}
void field::draw(effect* reason_effect, uint32 reason, uint32 reason_player, uint32 playerid, uint32 count) {
	add_process(PROCESSOR_DRAW, 0, reason_effect, 0, reason, (reason_player << 28) + (playerid << 24) + (count & 0xffffff));
}
void field::damage(effect* reason_effect, uint32 reason, uint32 reason_player, card* reason_card, uint32 playerid, uint32 amount, uint32 is_step) {
	uint32 arg2 = (is_step << 28) + (reason_player << 26) + (playerid << 24) + (amount & 0xffffff);
	if(reason & REASON_BATTLE)
		add_process(PROCESSOR_DAMAGE, 0, (effect*)reason_card, 0, reason, arg2);
	else
		add_process(PROCESSOR_DAMAGE, 0, reason_effect, 0, reason, arg2);
}
void field::recover(effect* reason_effect, uint32 reason, uint32 reason_player, uint32 playerid, uint32 amount, uint32 is_step) {
	add_process(PROCESSOR_RECOVER, 0, reason_effect, 0, reason, (is_step << 28) + (reason_player << 26) + (playerid << 24) + (amount & 0xffffff));
}
void field::summon(uint32 sumplayer, card* target, effect* proc, uint32 ignore_count, uint32 min_tribute, uint32 zone) {
	add_process(PROCESSOR_SUMMON_RULE, 0, proc, (group*)target, sumplayer + (ignore_count << 8) + (min_tribute << 16) + (zone << 24), 0);
}
void field::mset(uint32 setplayer, card* target, effect* proc, uint32 ignore_count, uint32 min_tribute, uint32 zone) {
	add_process(PROCESSOR_MSET, 0, proc, (group*)target, setplayer + (ignore_count << 8) + (min_tribute << 16) + (zone << 24), 0);
}
void field::special_summon_rule(uint32 sumplayer, card* target, uint32 summon_type) {
	add_process(PROCESSOR_SPSUMMON_RULE, 0, 0, (group*)target, sumplayer, summon_type);
}
void field::special_summon(card_set* target, uint32 sumtype, uint32 sumplayer, uint32 playerid, uint32 nocheck, uint32 nolimit, uint32 positions, uint32 zone) {
	if((positions & POS_FACEDOWN) && is_player_affected_by_effect(sumplayer, EFFECT_DIVINE_LIGHT))
		positions = (positions & POS_FACEUP) | ((positions & POS_FACEDOWN) >> 1);
	for(auto& pcard : *target) {
		pcard->temp.reason = pcard->current.reason;
		pcard->temp.reason_effect = pcard->current.reason_effect;
		pcard->temp.reason_player = pcard->current.reason_player;
		pcard->summon_info = (sumtype & 0xf00ffff) | SUMMON_TYPE_SPECIAL | ((uint32)pcard->current.location << 16);
		pcard->summon_player = sumplayer;
		pcard->current.reason = REASON_SPSUMMON;
		pcard->current.reason_effect = core.reason_effect;
		pcard->current.reason_player = core.reason_player;
		pcard->spsummon_param = (playerid << 24) + (nocheck << 16) + (nolimit << 8) + positions;
	}
	group* pgroup = pduel->new_group(*target);
	pgroup->is_readonly = TRUE;
	add_process(PROCESSOR_SPSUMMON, 0, core.reason_effect, pgroup, core.reason_player, zone);
}
void field::special_summon_step(card* target, uint32 sumtype, uint32 sumplayer, uint32 playerid, uint32 nocheck, uint32 nolimit, uint32 positions, uint32 zone) {
	if((positions & POS_FACEDOWN) && is_player_affected_by_effect(sumplayer, EFFECT_DIVINE_LIGHT))
		positions = (positions & POS_FACEUP) | ((positions & POS_FACEDOWN) >> 1);
	target->temp.reason = target->current.reason;
	target->temp.reason_effect = target->current.reason_effect;
	target->temp.reason_player = target->current.reason_player;
	target->summon_info = (sumtype & 0xf00ffff) | SUMMON_TYPE_SPECIAL | ((uint32)target->current.location << 16);
	target->summon_player = sumplayer;
	target->current.reason = REASON_SPSUMMON;
	target->current.reason_effect = core.reason_effect;
	target->current.reason_player = core.reason_player;
	target->spsummon_param = (playerid << 24) + (nocheck << 16) + (nolimit << 8) + positions;
	add_process(PROCESSOR_SPSUMMON_STEP, 0, core.reason_effect, NULL, zone, 0, 0, 0, target);
}
void field::special_summon_complete(effect* reason_effect, uint8 reason_player) {
	group* ng = pduel->new_group();
	ng->container.swap(core.special_summoning);
	ng->is_readonly = TRUE;
	core.hint_timing[reason_player] |= TIMING_SPSUMMON;
	add_process(PROCESSOR_SPSUMMON, 1, reason_effect, ng, reason_player, 0);
}
void field::destroy(card_set* targets, effect* reason_effect, uint32 reason, uint32 reason_player, uint32 playerid, uint32 destination, uint32 sequence) {
	for(auto cit = targets->begin(); cit != targets->end();) {
		card* pcard = *cit;
		if(pcard->is_status(STATUS_DESTROY_CONFIRMED)) {
			targets->erase(cit++);
			continue;
		}
		pcard->temp.reason = pcard->current.reason;
		pcard->current.reason = reason;
		if(reason_player != PLAYER_SELFDES) {
			pcard->temp.reason_effect = pcard->current.reason_effect;
			pcard->temp.reason_player = pcard->current.reason_player;
			if(reason_effect)
				pcard->current.reason_effect = reason_effect;
			pcard->current.reason_player = reason_player;
		}
		uint32 p = playerid;
		if(!(destination & (LOCATION_HAND | LOCATION_DECK | LOCATION_REMOVED)))
			destination = LOCATION_GRAVE;
		if(destination && p == PLAYER_NONE)
			p = pcard->owner;
		if(destination & (LOCATION_GRAVE | LOCATION_REMOVED))
			p = pcard->owner;
		pcard->set_status(STATUS_DESTROY_CONFIRMED, TRUE);
		pcard->sendto_param.set(p, POS_FACEUP, destination, sequence);
		++cit;
	}
	group* ng = pduel->new_group(*targets);
	ng->is_readonly = TRUE;
	add_process(PROCESSOR_DESTROY, 0, reason_effect, ng, reason, reason_player);
}
void field::destroy(card* target, effect* reason_effect, uint32 reason, uint32 reason_player, uint32 playerid, uint32 destination, uint32 sequence) {
	card_set tset;
	tset.insert(target);
	destroy(&tset, reason_effect, reason, reason_player, playerid, destination, sequence);
}
void field::release(card_set* targets, effect* reason_effect, uint32 reason, uint32 reason_player) {
	for(auto& pcard : *targets) {
		pcard->temp.reason = pcard->current.reason;
		pcard->temp.reason_effect = pcard->current.reason_effect;
		pcard->temp.reason_player = pcard->current.reason_player;
		pcard->current.reason = reason;
		pcard->current.reason_effect = reason_effect;
		pcard->current.reason_player = reason_player;
		pcard->sendto_param.set(pcard->owner, POS_FACEUP, LOCATION_GRAVE);
	}
	group* ng = pduel->new_group(*targets);
	ng->is_readonly = TRUE;
	add_process(PROCESSOR_RELEASE, 0, reason_effect, ng, reason, reason_player);
}
void field::release(card* target, effect* reason_effect, uint32 reason, uint32 reason_player) {
	card_set tset;
	tset.insert(target);
	release(&tset, reason_effect, reason, reason_player);
}
void field::send_to(card_set* targets, effect* reason_effect, uint32 reason, uint32 reason_player, uint32 playerid, uint32 destination, uint32 sequence, uint32 position) {
	if(destination & LOCATION_ONFIELD)
		return;
	for(auto& pcard : *targets) {
		pcard->temp.reason = pcard->current.reason;
		pcard->temp.reason_effect = pcard->current.reason_effect;
		pcard->temp.reason_player = pcard->current.reason_player;
		pcard->current.reason = reason;
		pcard->current.reason_effect = reason_effect;
		pcard->current.reason_player = reason_player;
		uint32 p = playerid;
		// send to hand from deck and playerid not given => send to the hand of controler
		if(p == PLAYER_NONE && (destination & LOCATION_HAND) && (pcard->current.location & LOCATION_DECK) && pcard->current.controler == reason_player)
			p = reason_player;
		if(destination & (LOCATION_GRAVE | LOCATION_REMOVED) || p == PLAYER_NONE)
			p = pcard->owner;
		if(destination == LOCATION_GRAVE && pcard->current.location == LOCATION_REMOVED)
			pcard->current.reason |= REASON_RETURN;
		uint32 pos = position;
		if(destination != LOCATION_REMOVED)
			pos = POS_FACEUP;
		else if(position == 0)
			pos = pcard->current.position;
		pcard->sendto_param.set(p, pos, destination, sequence);
	}
	group* ng = pduel->new_group(*targets);
	ng->is_readonly = TRUE;
	add_process(PROCESSOR_SENDTO, 0, reason_effect, ng, reason, reason_player);
}
void field::send_to(card* target, effect* reason_effect, uint32 reason, uint32 reason_player, uint32 playerid, uint32 destination, uint32 sequence, uint32 position) {
	card_set tset;
	tset.insert(target);
	send_to(&tset, reason_effect, reason, reason_player, playerid, destination, sequence, position);
}
void field::move_to_field(card* target, uint32 move_player, uint32 playerid, uint32 destination, uint32 positions, uint32 enable, uint32 ret, uint32 pzone, uint32 zone) {
	if(!(destination & LOCATION_ONFIELD) || !positions)
		return;
	if(destination == target->current.location && playerid == target->current.controler)
		return;
	target->to_field_param = (move_player << 24) + (playerid << 16) + (destination << 8) + positions;
	add_process(PROCESSOR_MOVETOFIELD, 0, 0, (group*)target, enable, ret + (pzone << 8), zone);
}
void field::change_position(card_set* targets, effect* reason_effect, uint32 reason_player, uint32 au, uint32 ad, uint32 du, uint32 dd, uint32 flag, uint32 enable) {
	group* ng = pduel->new_group(*targets);
	ng->is_readonly = TRUE;
	for(auto& pcard : *targets) {
		if(pcard->current.position == POS_FACEUP_ATTACK)
			pcard->position_param = au;
		else if(pcard->current.position == POS_FACEDOWN_DEFENSE)
			pcard->position_param = dd;
		else if(pcard->current.position == POS_FACEUP_DEFENSE)
			pcard->position_param = du;
		else
			pcard->position_param = ad;
		pcard->position_param |= flag;
	}
	add_process(PROCESSOR_CHANGEPOS, 0, reason_effect, ng, reason_player, enable);
}
void field::change_position(card* target, effect* reason_effect, uint32 reason_player, uint32 npos, uint32 flag, uint32 enable) {
	group* ng = pduel->new_group(target);
	ng->is_readonly = TRUE;
	target->position_param = npos;
	target->position_param |= flag;
	add_process(PROCESSOR_CHANGEPOS, 0, reason_effect, ng, reason_player, enable);
}
void field::operation_replace(int32 type, int32 step, group* targets) {
	int32 is_destroy = (type == EFFECT_DESTROY_REPLACE) ? TRUE : FALSE;
	auto pr = effects.continuous_effect.equal_range(type);
	std::vector<effect*> opp_effects;
	for(auto eit = pr.first; eit != pr.second;) {
		effect* reffect = eit->second;
		++eit;
		if(reffect->get_handler_player() == infos.turn_player)
			add_process(PROCESSOR_OPERATION_REPLACE, step, reffect, targets, is_destroy, 0);
		else
			opp_effects.push_back(reffect);
	}
	for(auto& peffect : opp_effects)
		add_process(PROCESSOR_OPERATION_REPLACE, step, peffect, targets, is_destroy, 0);
}
void field::select_tribute_cards(card* target, uint8 playerid, uint8 cancelable, int32 min, int32 max, uint8 toplayer, uint32 zone) {
	add_process(PROCESSOR_SELECT_TRIBUTE, 0, 0, (group*)target, playerid + ((uint32)cancelable << 16), min + (max << 16), toplayer, zone);
}
int32 field::draw(uint16 step, effect* reason_effect, uint32 reason, uint8 reason_player, uint8 playerid, uint32 count) {
	switch(step) {
	case 0: {
		card_vector cv;
		uint32 drawed = 0;
		uint32 public_count = 0;
		if(!(reason & REASON_RULE) && !is_player_can_draw(playerid)) {
			returns.ivalue[0] = 0;
			return TRUE;
		}
		core.overdraw[playerid] = FALSE;
		for(uint32 i = 0; i < count; ++i) {
			if(player[playerid].list_main.size() == 0) {
				core.overdraw[playerid] = TRUE;
				break;
			}
			drawed++;
			card* pcard = player[playerid].list_main.back();
			pcard->enable_field_effect(false);
			pcard->cancel_field_effect();
			player[playerid].list_main.pop_back();
			pcard->previous.controler = pcard->current.controler;
			pcard->previous.location = pcard->current.location;
			pcard->previous.sequence = pcard->current.sequence;
			pcard->previous.position = pcard->current.position;
			pcard->previous.pzone = pcard->current.pzone;
			pcard->current.controler = PLAYER_NONE;
			pcard->current.reason_effect = reason_effect;
			pcard->current.reason_player = reason_player;
			pcard->current.reason = reason | REASON_DRAW;
			pcard->current.location = 0;
			add_card(playerid, pcard, LOCATION_HAND, 0);
			pcard->enable_field_effect(true);
			effect* pub = pcard->is_affected_by_effect(EFFECT_PUBLIC);
			if(pub)
				public_count++;
			pcard->current.position = pub ? POS_FACEUP : POS_FACEDOWN;
			cv.push_back(pcard);
			pcard->reset(RESET_TOHAND, RESET_EVENT);
		}
		core.hint_timing[playerid] |= TIMING_DRAW + TIMING_TOHAND;
		adjust_instant();
		core.units.begin()->arg2 = (core.units.begin()->arg2 & 0xff000000) + drawed;
		card_set* drawed_set = new card_set;
		core.units.begin()->ptarget = (group*)drawed_set;
		drawed_set->insert(cv.begin(), cv.end());
		if(drawed) {
			if(core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK) {
				if(player[playerid].list_main.size()) {
					card* ptop = player[playerid].list_main.back();
					if(core.deck_reversed || (ptop->current.position == POS_FACEUP_DEFENSE)) {
						pduel->write_buffer8(MSG_DECK_TOP);
						pduel->write_buffer8(playerid);
						pduel->write_buffer8(drawed);
						if(ptop->current.position != POS_FACEUP_DEFENSE)
							pduel->write_buffer32(ptop->data.code);
						else
							pduel->write_buffer32(ptop->data.code | 0x80000000);
					}
				}
			}
			pduel->write_buffer8(MSG_DRAW);
			pduel->write_buffer8(playerid);
			pduel->write_buffer8(drawed);
			for(uint32 i = 0; i < drawed; ++i)
				pduel->write_buffer32(cv[i]->data.code | (cv[i]->is_position(POS_FACEUP) ? 0x80000000 : 0));
			if(core.deck_reversed && (public_count < drawed)) {
				pduel->write_buffer8(MSG_CONFIRM_CARDS);
				pduel->write_buffer8(1 - playerid);
				pduel->write_buffer8((uint8)drawed_set->size());
				for(auto& pcard : *drawed_set) {
					pduel->write_buffer32(pcard->data.code);
					pduel->write_buffer8(pcard->current.controler);
					pduel->write_buffer8(pcard->current.location);
					pduel->write_buffer8(pcard->current.sequence);
				}
				shuffle(playerid, LOCATION_HAND);
			}
			for (auto& pcard : *drawed_set) {
				if(pcard->owner != pcard->current.controler) {
					effect* deffect = pduel->new_effect();
					deffect->owner = pcard;
					deffect->code = 0;
					deffect->type = EFFECT_TYPE_SINGLE;
					deffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_CLIENT_HINT;
					deffect->description = 67;
					deffect->reset_flag = RESET_EVENT + 0x1fe0000;
					pcard->add_effect(deffect);
				}
				raise_single_event(pcard, 0, EVENT_DRAW, reason_effect, reason, reason_player, playerid, 0);
				raise_single_event(pcard, 0, EVENT_TO_HAND, reason_effect, reason, reason_player, playerid, 0);
				raise_single_event(pcard, 0, EVENT_MOVE, reason_effect, reason, reason_player, playerid, 0);
			}
			process_single_event();
			raise_event(drawed_set, EVENT_DRAW, reason_effect, reason, reason_player, playerid, drawed);
			raise_event(drawed_set, EVENT_TO_HAND, reason_effect, reason, reason_player, playerid, drawed);
			raise_event(drawed_set, EVENT_MOVE, reason_effect, reason, reason_player, playerid, drawed);
			process_instant_event();
		}
		return FALSE;
	}
	case 1: {
		card_set* drawed_set = (card_set*)core.units.begin()->ptarget;
		core.operated_set.swap(*drawed_set);
		delete drawed_set;
		returns.ivalue[0] = count;
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::damage(uint16 step, effect* reason_effect, uint32 reason, uint8 reason_player, card* reason_card, uint8 playerid, uint32 amount, uint32 is_step) {
	switch(step) {
	case 0: {
		effect_set eset;
		returns.ivalue[0] = amount;
		if(amount <= 0)
			return TRUE;
		if(!(reason & REASON_RDAMAGE)) {
			filter_player_effect(playerid, EFFECT_REVERSE_DAMAGE, &eset);
			for(int32 i = 0; i < eset.size(); ++i) {
				pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
				pduel->lua->add_param(reason, PARAM_TYPE_INT);
				pduel->lua->add_param(reason_player, PARAM_TYPE_INT);
				pduel->lua->add_param(reason_card, PARAM_TYPE_CARD);
				if(eset[i]->check_value_condition(4)) {
					recover(reason_effect, (reason & REASON_RRECOVER) | REASON_RDAMAGE | REASON_EFFECT, reason_player, playerid, amount, is_step);
					core.units.begin()->step = 2;
					return FALSE;
				}
			}
		}
		eset.clear();
		filter_player_effect(playerid, EFFECT_REFLECT_DAMAGE, &eset);
		for(int32 i = 0; i < eset.size(); ++i) {
			pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(amount, PARAM_TYPE_INT);
			pduel->lua->add_param(reason, PARAM_TYPE_INT);
			pduel->lua->add_param(reason_player, PARAM_TYPE_INT);
			pduel->lua->add_param(reason_card, PARAM_TYPE_CARD);
			if (eset[i]->check_value_condition(5)) {
				playerid = 1 - playerid;
				core.units.begin()->arg2 |= 1 << 29;
				break;
			}
		}
		uint32 val = amount;
		eset.clear();
		filter_player_effect(playerid, EFFECT_CHANGE_DAMAGE, &eset);
		for(int32 i = 0; i < eset.size(); ++i) {
			pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(val, PARAM_TYPE_INT);
			pduel->lua->add_param(reason, PARAM_TYPE_INT);
			pduel->lua->add_param(reason_player, PARAM_TYPE_INT);
			pduel->lua->add_param(reason_card, PARAM_TYPE_CARD);
			val = eset[i]->get_value(5);
			returns.ivalue[0] = val;
			if(val == 0)
				return TRUE;
		}
		core.units.begin()->arg2 = (core.units.begin()->arg2 & 0xff000000) | (val & 0xffffff);
		if(is_step) {
			core.units.begin()->step = 9;
			return TRUE;
		}
		return FALSE;
	}
	case 1: {
		uint32 is_reflect = (core.units.begin()->arg2 >> 29) & 1;
		if(is_reflect)
			playerid = 1 - playerid;
		if(is_reflect || (reason & REASON_RRECOVER))
			core.units.begin()->step = 2;
		core.hint_timing[playerid] |= TIMING_DAMAGE;
		player[playerid].lp -= amount;
		pduel->write_buffer8(MSG_DAMAGE);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(amount);
		raise_event(reason_card, EVENT_DAMAGE, reason_effect, reason, reason_player, playerid, amount);
		if(reason == REASON_BATTLE && reason_card) {
			if((player[playerid].lp <= 0) && (core.attack_target == 0) && reason_card->is_affected_by_effect(EFFECT_MATCH_KILL)) {
				pduel->write_buffer8(MSG_MATCH_KILL);
				pduel->write_buffer32(reason_card->data.code);
			}
			raise_single_event(reason_card, 0, EVENT_BATTLE_DAMAGE, 0, 0, reason_player, playerid, amount);
			raise_event(reason_card, EVENT_BATTLE_DAMAGE, 0, 0, reason_player, playerid, amount);
			process_single_event();
		}
		process_instant_event();
		return FALSE;
	}
	case 2: {
		returns.ivalue[0] = amount;
		return TRUE;
	}
	case 3: {
		returns.ivalue[0] = 0;
		return TRUE;
	}
	case 10: {
		//dummy
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::recover(uint16 step, effect* reason_effect, uint32 reason, uint8 reason_player, uint8 playerid, uint32 amount, uint32 is_step) {
	switch(step) {
	case 0: {
		effect_set eset;
		returns.ivalue[0] = amount;
		if(amount <= 0)
			return TRUE;
		if(!(reason & REASON_RRECOVER)) {
			filter_player_effect(playerid, EFFECT_REVERSE_RECOVER, &eset);
			for(int32 i = 0; i < eset.size(); ++i) {
				pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
				pduel->lua->add_param(reason, PARAM_TYPE_INT);
				pduel->lua->add_param(reason_player, PARAM_TYPE_INT);
				if(eset[i]->check_value_condition(3)) {
					damage(reason_effect, (reason & REASON_RDAMAGE) | REASON_RRECOVER | REASON_EFFECT, reason_player, 0, playerid, amount, is_step);
					core.units.begin()->step = 2;
					return FALSE;
				}
			}
		}
		if(is_step) {
			core.units.begin()->step = 9;
			return TRUE;
		}
		return FALSE;
	}
	case 1: {
		if(reason & REASON_RDAMAGE)
			core.units.begin()->step = 2;
		core.hint_timing[playerid] |= TIMING_RECOVER;
		player[playerid].lp += amount;
		pduel->write_buffer8(MSG_RECOVER);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(amount);
		raise_event((card*)0, EVENT_RECOVER, reason_effect, reason, reason_player, playerid, amount);
		process_instant_event();
		return FALSE;
	}
	case 2: {
		returns.ivalue[0] = amount;
		return TRUE;
	}
	case 3: {
		returns.ivalue[0] = 0;
		return TRUE;
	}
	case 10: {
		//dummy
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::pay_lp_cost(uint32 step, uint8 playerid, uint32 cost, uint32 must_pay) {
	switch(step) {
	case 0: {
		effect_set eset;
		int32 val = cost;
		filter_player_effect(playerid, EFFECT_LPCOST_CHANGE, &eset);
		for(int32 i = 0; i < eset.size(); ++i) {
			pduel->lua->add_param(core.reason_effect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			pduel->lua->add_param(val, PARAM_TYPE_INT);
			val = eset[i]->get_value(3);
		}
		if(val <= 0)
			return TRUE;
		core.units.begin()->arg2 = val;
		core.select_options.clear();
		core.select_effects.clear();
		if(val <= player[playerid].lp) {
			core.select_options.push_back(11);
			core.select_effects.push_back(0);
		}
		if(must_pay) {
			if(core.select_options.size() == 0)
				return TRUE;
			returns.ivalue[0] = 0;
			return FALSE;
		}
		tevent e;
		e.event_cards = 0;
		e.event_player = playerid;
		e.event_value = val;
		e.reason = 0;
		e.reason_effect = core.reason_effect;
		e.reason_player = playerid;
		auto pr = effects.continuous_effect.equal_range(EFFECT_LPCOST_REPLACE);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			if(peffect->is_activateable(peffect->get_handler_player(), e)) {
				core.select_options.push_back(peffect->description);
				core.select_effects.push_back(peffect);
			}
		}
		if(core.select_options.size() == 0)
			return TRUE;
		if(core.select_options.size() == 1)
			returns.ivalue[0] = 0;
		else if(core.select_effects[0] == 0 && core.select_effects.size() == 2)
			add_process(PROCESSOR_SELECT_EFFECTYN, 0, 0, (group*)core.select_effects[1]->handler, playerid, 218);
		else
			add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, playerid, 0);
		return FALSE;
	}
	case 1: {
		effect* peffect = core.select_effects[returns.ivalue[0]];
		if(!peffect) {
			player[playerid].lp -= cost;
			pduel->write_buffer8(MSG_PAY_LPCOST);
			pduel->write_buffer8(playerid);
			pduel->write_buffer32(cost);
			raise_event((card*)0, EVENT_PAY_LPCOST, core.reason_effect, 0, playerid, playerid, cost);
			process_instant_event();
			return TRUE;
		}
		tevent e;
		e.event_cards = 0;
		e.event_player = playerid;
		e.event_value = cost;
		e.reason = 0;
		e.reason_effect = core.reason_effect;
		e.reason_player = playerid;
		solve_continuous(playerid, peffect, e);
		return TRUE;
	}
	}
	return TRUE;
}
// rplayer rmoves counter from pcard or the field
// s,o: binary value indicating the available side
// from pcard: Card.RemoveCounter() -> here -> card::remove_counter() -> the script should raise EVENT_REMOVE_COUNTER if necessary
// from the field: Duel.RemoveCounter() -> here -> field::select_counter() -> the system raises EVENT_REMOVE_COUNTER automatically
int32 field::remove_counter(uint16 step, uint32 reason, card* pcard, uint8 rplayer, uint8 s, uint8 o, uint16 countertype, uint16 count) {
	switch(step) {
	case 0: {
		core.select_options.clear();
		core.select_effects.clear();
		if((pcard && pcard->get_counter(countertype) >= count) || (!pcard && get_field_counter(rplayer, s, o, countertype))) {
			core.select_options.push_back(10);
			core.select_effects.push_back(0);
		}
		auto pr = effects.continuous_effect.equal_range(EFFECT_RCOUNTER_REPLACE + countertype);
		tevent e;
		e.event_cards = 0;
		e.event_player = rplayer;
		e.event_value = count;
		e.reason = reason;
		e.reason_effect = core.reason_effect;
		e.reason_player = rplayer;
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			if(peffect->is_activateable(peffect->get_handler_player(), e)) {
				core.select_options.push_back(peffect->description);
				core.select_effects.push_back(peffect);
			}
		}
		returns.ivalue[0] = 0;
		if(core.select_options.size() == 0)
			return TRUE;
		if(core.select_options.size() == 1)
			returns.ivalue[0] = 0;
		else if(core.select_effects[0] == 0 && core.select_effects.size() == 2)
			add_process(PROCESSOR_SELECT_EFFECTYN, 0, 0, (group*)core.select_effects[1]->handler, rplayer, 220);
		else
			add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, rplayer, 0);
		return FALSE;
	}
	case 1: {
		effect* peffect = core.select_effects[returns.ivalue[0]];
		if(peffect) {
			tevent e;
			e.event_cards = 0;
			e.event_player = rplayer;
			e.event_value = count;
			e.reason = reason;
			e.reason_effect = core.reason_effect;
			e.reason_player = rplayer;
			solve_continuous(rplayer, peffect, e);
			core.units.begin()->step = 3;
			return FALSE;
		}
		if(pcard) {
			returns.ivalue[0] = pcard->remove_counter(countertype, count);
			core.units.begin()->step = 3;
			return FALSE;
		}
		add_process(PROCESSOR_SELECT_COUNTER, 0, NULL, NULL, rplayer, countertype, count, (s << 8) + o);
		return FALSE;
	}
	case 2: {
		for(uint32 i = 0; i < core.select_cards.size(); ++i)
			if(returns.svalue[i] > 0)
				core.select_cards[i]->remove_counter(countertype, returns.svalue[i]);
		return FALSE;
	}
	case 3: {
		raise_event((card*)0, EVENT_REMOVE_COUNTER + countertype, core.reason_effect, reason, rplayer, rplayer, count);
		process_instant_event();
		return FALSE;
	}
	case 4: {
		returns.ivalue[0] = 1;
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::remove_overlay_card(uint16 step, uint32 reason, card* pcard, uint8 rplayer, uint8 s, uint8 o, uint16 min, uint16 max) {
	switch(step) {
	case 0: {
		core.select_options.clear();
		core.select_effects.clear();
		if((pcard && pcard->xyz_materials.size() >= min) || (!pcard && get_overlay_count(rplayer, s, o) >= min)) {
			core.select_options.push_back(12);
			core.select_effects.push_back(0);
		}
		auto pr = effects.continuous_effect.equal_range(EFFECT_OVERLAY_REMOVE_REPLACE);
		tevent e;
		e.event_cards = 0;
		e.event_player = rplayer;
		e.event_value = min;
		e.reason = reason;
		e.reason_effect = core.reason_effect;
		e.reason_player = rplayer;
		for(auto eit = pr.first; eit != pr.second;) {
			effect* peffect = eit->second;
			++eit;
			if(peffect->is_activateable(peffect->get_handler_player(), e)) {
				core.select_options.push_back(peffect->description);
				core.select_effects.push_back(peffect);
			}
		}
		returns.ivalue[0] = 0;
		if(core.select_options.size() == 0)
			return TRUE;
		if(core.select_options.size() == 1)
			returns.ivalue[0] = 0;
		else if(core.select_effects[0] == 0 && core.select_effects.size() == 2)
			add_process(PROCESSOR_SELECT_EFFECTYN, 0, 0, (group*)core.select_effects[1]->handler, rplayer, 219);
		else
			add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, rplayer, 0);
		return FALSE;
	}
	case 1: {
		if(effect* peffect = core.select_effects[returns.ivalue[0]]) {
			tevent e;
			e.event_cards = 0;
			e.event_player = rplayer;
			e.event_value = min + (max << 16);
			e.reason = reason;
			e.reason_effect = core.reason_effect;
			e.reason_player = rplayer;
			solve_continuous(rplayer, peffect, e);
			core.units.begin()->peffect = peffect;
		}
		return FALSE;
	}
	case 2: {
		uint16 cancelable = FALSE;
		if(core.units.begin()->peffect) {
			int32 replace_count = returns.ivalue[0];
			if(replace_count >= max)
				return TRUE;
			min -= replace_count;
			max -= replace_count;
			if(min <= 0) {
				cancelable = TRUE;
				min = 0;
			}
			core.units.begin()->arg4 = replace_count;
		}
		core.select_cards.clear();
		if(pcard) {
			for(auto& mcard : pcard->xyz_materials)
				core.select_cards.push_back(mcard);
		} else {
			card_set cset;
			get_overlay_group(rplayer, s, o, &cset);
			for(auto& xcard : cset)
				core.select_cards.push_back(xcard);
		}
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(rplayer);
		pduel->write_buffer32(519);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, rplayer + (cancelable << 16), min + (max << 16));
		return FALSE;
	}
	case 3: {
		card_set cset;
		for(int32 i = 0; i < returns.bvalue[0]; ++i)
			cset.insert(core.select_cards[returns.bvalue[i + 1]]);
		send_to(&cset, core.reason_effect, reason, rplayer, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
		return FALSE;
	}
	case 4: {
		returns.ivalue[0] += (int32)core.units.begin()->arg4;
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::get_control(uint16 step, effect* reason_effect, uint8 reason_player, group* targets, uint8 playerid, uint16 reset_phase, uint8 reset_count, uint32 zone) {
	switch(step) {
	case 0: {
		card_set* destroy_set = new card_set;
		core.units.begin()->ptr1 = destroy_set;
		for(auto cit = targets->container.begin(); cit != targets->container.end();) {
			card* pcard = *cit++;
			pcard->filter_disable_related_cards();
			bool change = true;
			if(pcard->overlay_target)
				change = false;
			if(pcard->current.controler == playerid)
				change = false;
			if(pcard->current.controler == PLAYER_NONE)
				change = false;
			if(pcard->current.location != LOCATION_MZONE)
				change = false;
			if(!pcard->is_capable_change_control())
				change = false;
			if(!pcard->is_affect_by_effect(reason_effect))
				change = false;
			if(core.duel_rule <= 4 && (pcard->get_type() & TYPE_TRAPMONSTER) && get_useable_count(pcard, playerid, LOCATION_SZONE, reason_player, LOCATION_REASON_CONTROL) <= 0)
				change = false;
			if(!change)
				targets->container.erase(pcard);
		}
		int32 fcount = get_useable_count(NULL, playerid, LOCATION_MZONE, reason_player, LOCATION_REASON_CONTROL, zone);
		if(fcount <= 0) {
			destroy_set->swap(targets->container);
			core.units.begin()->step = 5;
			return FALSE;
		}
		if((int32)targets->container.size() > fcount) {
			core.select_cards.clear();
			for(auto& pcard : targets->container)
				core.select_cards.push_back(pcard);
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_SELECTMSG);
			pduel->write_buffer8(playerid);
			pduel->write_buffer32(502);
			uint32 ct = (uint32)targets->container.size() - fcount;
			add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid, ct + (ct << 16));
		} else
			core.units.begin()->step = 1;
		return FALSE;
	}
	case 1: {
		card_set* destroy_set = (card_set*)core.units.begin()->ptr1;
		for(int32 i = 0; i < returns.bvalue[0]; ++i) {
			card* pcard = core.select_cards[returns.bvalue[i + 1]];
			destroy_set->insert(pcard);
			targets->container.erase(pcard);
		}
		return FALSE;
	}
	case 2: {
		for(auto& pcard : targets->container) {
			if(pcard->unique_code && (pcard->unique_location & LOCATION_MZONE))
				remove_unique_card(pcard);
		}
		targets->it = targets->container.begin();
		return FALSE;
	}
	case 3: {
		if(targets->it == targets->container.end()) {
			adjust_instant();
			core.units.begin()->step = 4;
			return FALSE;
		}
		card* pcard = *targets->it;
		move_to_field(pcard, (reason_player != PLAYER_NONE) ? reason_player : playerid, playerid, LOCATION_MZONE, pcard->current.position, FALSE, 0, FALSE, zone);
		return FALSE;
	}
	case 4: {
		card* pcard = *targets->it;
		pcard->set_status(STATUS_ATTACK_CANCELED, TRUE);
		set_control(pcard, playerid, reset_phase, reset_count);
		pcard->reset(RESET_CONTROL, RESET_EVENT);
		pcard->filter_disable_related_cards();
		++targets->it;
		core.units.begin()->step = 2;
		return FALSE;
	}
	case 5: {
		for(auto cit = targets->container.begin(); cit != targets->container.end(); ) {
			card* pcard = *cit++;
			if(!(pcard->current.location & LOCATION_ONFIELD)) {
				targets->container.erase(pcard);
				continue;
			}
			if(pcard->unique_code && (pcard->unique_location & LOCATION_MZONE))
				add_unique_card(pcard);
			raise_single_event(pcard, 0, EVENT_CONTROL_CHANGED, reason_effect, REASON_EFFECT, reason_player, playerid, 0);
			raise_single_event(pcard, 0, EVENT_MOVE, reason_effect, REASON_EFFECT, reason_player, playerid, 0);
		}
		if(targets->container.size()) {
			raise_event(&targets->container, EVENT_CONTROL_CHANGED, reason_effect, REASON_EFFECT, reason_player, playerid, 0);
			raise_event(&targets->container, EVENT_MOVE, reason_effect, REASON_EFFECT, reason_player, playerid, 0);
		}
		process_single_event();
		process_instant_event();
		return FALSE;
	}
	case 6: {
		card_set* destroy_set = (card_set*)core.units.begin()->ptr1;
		if(destroy_set->size())
			destroy(destroy_set, 0, REASON_RULE, PLAYER_NONE);
		delete destroy_set;
		return FALSE;
	}
	case 7: {
		core.operated_set = targets->container;
		returns.ivalue[0] = (int32)targets->container.size();
		pduel->delete_group(targets);
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::swap_control(uint16 step, effect* reason_effect, uint8 reason_player, group* targets1, group* targets2, uint16 reset_phase, uint8 reset_count) {
	switch(step) {
	case 0: {
		core.units.begin()->step = 9;
		if(targets1->container.size() == 0)
			return FALSE;
		if(targets2->container.size() == 0)
			return FALSE;
		if(targets1->container.size() != targets2->container.size())
			return FALSE;
		for(auto& pcard : targets1->container)
			pcard->filter_disable_related_cards();
		for(auto& pcard : targets2->container)
			pcard->filter_disable_related_cards();
		auto cit1 = targets1->container.begin();
		auto cit2 = targets2->container.begin();
		uint8 p1 = (*cit1)->current.controler, p2 = (*cit2)->current.controler;
		if(p1 == p2 || p1 == PLAYER_NONE || p2 == PLAYER_NONE)
			return FALSE;
		for(auto& pcard : targets1->container) {
			if(pcard->overlay_target)
				return FALSE;
			if(pcard->current.controler != p1)
				return FALSE;
			if(pcard->current.location != LOCATION_MZONE)
				return FALSE;
			if(!pcard->is_capable_change_control())
				return FALSE;
			if(!pcard->is_affect_by_effect(reason_effect))
				return FALSE;
		}
		for(auto& pcard : targets2->container) {
			if(pcard->overlay_target)
				return FALSE;
			if(pcard->current.controler != p2)
				return FALSE;
			if(pcard->current.location != LOCATION_MZONE)
				return FALSE;
			if(!pcard->is_capable_change_control())
				return FALSE;
			if(!pcard->is_affect_by_effect(reason_effect))
				return FALSE;
		}
		int32 ct = get_useable_count(NULL, p1, LOCATION_MZONE, reason_player, LOCATION_REASON_CONTROL);
		for(auto& pcard : targets1->container) {
			if(pcard->current.sequence >= 5)
				ct--;
		}
		if(ct < 0)
			return FALSE;
		ct = get_useable_count(NULL, p2, LOCATION_MZONE, reason_player, LOCATION_REASON_CONTROL);
		for(auto& pcard : targets2->container) {
			if(pcard->current.sequence >= 5)
				ct--;
		}
		if(ct < 0)
			return FALSE;
		for(auto& pcard : targets1->container) {
			if(pcard->unique_code && (pcard->unique_location & LOCATION_MZONE))
				remove_unique_card(pcard);
		}
		for(auto& pcard : targets2->container) {
			if(pcard->unique_code && (pcard->unique_location & LOCATION_MZONE))
				remove_unique_card(pcard);
		}
		targets1->it = targets1->container.begin();
		targets2->it = targets2->container.begin();
		core.units.begin()->step = 0;
		return FALSE;
	}
	case 1: {
		if(targets1->it == targets1->container.end()) {
			core.units.begin()->step = 3;
			return FALSE;
		}
		card* pcard1 = *targets1->it;
		uint8 p1 = pcard1->current.controler;
		uint8 s1 = pcard1->current.sequence;
		uint32 flag;
		get_useable_count(NULL, p1, LOCATION_MZONE, reason_player, LOCATION_REASON_CONTROL, 0xff, &flag);
		if(reason_player == p1)
			flag = (flag & ~(1 << s1) & 0xff) | ~0x1f;
		else
			flag = ((flag & ~(1 << s1)) << 16 & 0xff0000) | ~0x1f0000;
		card* pcard2 = *targets2->it;
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(reason_player);
		pduel->write_buffer32(pcard2->data.code);
		add_process(PROCESSOR_SELECT_PLACE, 0, 0, 0, reason_player, flag, 1);
		return FALSE;
	}
	case 2: {
		core.units.begin()->arg4 = returns.bvalue[2];
		card* pcard2 = *targets2->it;
		uint8 p2 = pcard2->current.controler;
		uint8 s2 = pcard2->current.sequence;
		uint32 flag;
		get_useable_count(NULL, p2, LOCATION_MZONE, reason_player, LOCATION_REASON_CONTROL, 0xff, &flag);
		if(reason_player == p2)
			flag = (flag & ~(1 << s2) & 0xff) | ~0x1f;
		else
			flag = ((flag & ~(1 << s2)) << 16 & 0xff0000) | ~0x1f0000;
		card* pcard1 = *targets1->it;
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(reason_player);
		pduel->write_buffer32(pcard1->data.code);
		add_process(PROCESSOR_SELECT_PLACE, 0, 0, 0, reason_player, flag, 1);
		return FALSE;
	}
	case 3: {
		card* pcard1 = *targets1->it;
		card* pcard2 = *targets2->it;
		uint8 p1 = pcard1->current.controler, p2 = pcard2->current.controler;
		uint8 new_s1 = (uint8)core.units.begin()->arg4, new_s2 = returns.bvalue[2];
		swap_card(pcard1, pcard2, new_s1, new_s2);
		pcard1->reset(RESET_CONTROL, RESET_EVENT);
		pcard2->reset(RESET_CONTROL, RESET_EVENT);
		set_control(pcard1, p2, reset_phase, reset_count);
		set_control(pcard2, p1, reset_phase, reset_count);
		pcard1->set_status(STATUS_ATTACK_CANCELED, TRUE);
		pcard2->set_status(STATUS_ATTACK_CANCELED, TRUE);
		++targets1->it;
		++targets2->it;
		core.units.begin()->step = 0;
		return FALSE;
	}
	case 4: {
		targets1->container.insert(targets2->container.begin(), targets2->container.end());
		for(auto& pcard : targets1->container) {
			pcard->filter_disable_related_cards();
			if(pcard->unique_code && (pcard->unique_location & LOCATION_MZONE))
				add_unique_card(pcard);
			raise_single_event(pcard, 0, EVENT_CONTROL_CHANGED, reason_effect, REASON_EFFECT, reason_player, pcard->current.controler, 0);
			raise_single_event(pcard, 0, EVENT_MOVE, reason_effect, REASON_EFFECT, reason_player, pcard->current.controler, 0);
		}
		raise_event(&targets1->container, EVENT_CONTROL_CHANGED, reason_effect, REASON_EFFECT, reason_player, 0, 0);
		raise_event(&targets1->container, EVENT_MOVE, reason_effect, REASON_EFFECT, reason_player, 0, 0);
		process_single_event();
		process_instant_event();
		return FALSE;
	}
	case 5: {
		core.operated_set = targets1->container;
		returns.ivalue[0] = 1;
		pduel->delete_group(targets1);
		pduel->delete_group(targets2);
		return TRUE;
	}
	case 10: {
		core.operated_set.clear();
		returns.ivalue[0] = 0;
		pduel->delete_group(targets1);
		pduel->delete_group(targets2);
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::self_destroy(uint16 step, card* ucard, int32 p) {
	switch(step) {
	case 0: {
		if(core.unique_cards[p].find(ucard) == core.unique_cards[p].end()) {
			core.unique_destroy_set.erase(ucard);
			return TRUE;
		}
		card_set cset;
		ucard->get_unique_target(&cset, p);
		if(cset.size() == 0)
			ucard->unique_fieldid = 0;
		else if(cset.size() == 1) {
			auto cit = cset.begin();
			ucard->unique_fieldid = (*cit)->fieldid;
		} else {
			core.select_cards.clear();
			uint8 player = p;
			for(auto& pcard : cset) {
				if(pcard->current.controler == player && pcard->unique_fieldid != UINT_MAX)
					core.select_cards.push_back(pcard);
			}
			if(core.select_cards.size() == 0) {
				player = 1 - p;
				for(auto& pcard : cset) {
					if(pcard->current.controler == player && pcard->unique_fieldid != UINT_MAX)
						core.select_cards.push_back(pcard);
				}
			}
			if(core.select_cards.size() == 0) {
				player = p;
				for(auto& pcard : cset) {
					if(pcard->current.controler == player)
						core.select_cards.push_back(pcard);
				}
			}
			if(core.select_cards.size() == 0) {
				player = 1 - p;
				for(auto& pcard : cset) {
					if(pcard->current.controler == player)
						core.select_cards.push_back(pcard);
				}
			}
			if(core.select_cards.size() == 1) {
				returns.bvalue[1] = 0;
			}
			else {
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_SELECTMSG);
				pduel->write_buffer8(player);
				pduel->write_buffer32(534);
				add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, player, 0x10001);
			}
			return FALSE;
		}
		core.unique_destroy_set.erase(ucard);
		return TRUE;
	}
	case 1: {
		card_set cset;
		ucard->get_unique_target(&cset, p);
		card* mcard = core.select_cards[returns.bvalue[1]];
		ucard->unique_fieldid = mcard->fieldid;
		cset.erase(mcard);
		for(auto& pcard : cset) {
			pcard->temp.reason_effect = pcard->current.reason_effect;
			pcard->temp.reason_player = pcard->current.reason_player;
			pcard->current.reason_effect = ucard->unique_effect;
			pcard->current.reason_player = ucard->current.controler;
		}
		destroy(&cset, 0, REASON_RULE, PLAYER_SELFDES);
		return FALSE;
	}
	case 2: {
		core.unique_destroy_set.erase(ucard);
		return TRUE;
	}
	case 10: {
		if(core.self_destroy_set.empty())
			return FALSE;
		auto it = core.self_destroy_set.begin();
		card* pcard = *it;
		effect* peffect = pcard->is_affected_by_effect(EFFECT_SELF_DESTROY);
		if(peffect) {
			pcard->temp.reason_effect = pcard->current.reason_effect;
			pcard->temp.reason_player = pcard->current.reason_player;
			pcard->current.reason_effect = peffect;
			pcard->current.reason_player = peffect->get_handler_player();
			destroy(pcard, 0, REASON_EFFECT, PLAYER_SELFDES);
		}
		core.self_destroy_set.erase(it);
		core.units.begin()->step = 9;
		return FALSE;
	}
	case 11: {
		returns.ivalue[0] = 0;
		core.operated_set.clear();
		return TRUE;
	}
	case 20: {
		if(core.self_tograve_set.empty())
			return FALSE;
		auto it = core.self_tograve_set.begin();
		card* pcard = *it;
		effect* peffect = pcard->is_affected_by_effect(EFFECT_SELF_TOGRAVE);
		if(peffect) {
			pcard->temp.reason_effect = pcard->current.reason_effect;
			pcard->temp.reason_player = pcard->current.reason_player;
			pcard->current.reason_effect = peffect;
			pcard->current.reason_player = peffect->get_handler_player();
			send_to(pcard, 0, REASON_EFFECT, PLAYER_NONE, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
		}
		core.self_tograve_set.erase(it);
		core.units.begin()->step = 19;
		return FALSE;
	}
	case 21: {
		returns.ivalue[0] = 0;
		core.operated_set.clear();
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::trap_monster_adjust(uint16 step) {
	switch(step) {
	case 0: {
		if(core.duel_rule <= 4) {
			core.units.begin()->step = 3;
			return FALSE;
		}
		card_set* to_grave_set = new card_set;
		core.units.begin()->ptr1 = to_grave_set;
		return FALSE;
	}
	case 1: {
		card_set* to_grave_set = (card_set*)core.units.begin()->ptr1;
		uint8 check_player = infos.turn_player;
		if(core.units.begin()->arg1)
			check_player = 1 - infos.turn_player;
		refresh_location_info_instant();
		int32 fcount = get_useable_count(NULL, check_player, LOCATION_SZONE, check_player, 0);
		if(fcount <= 0) {
			for(auto& pcard : core.trap_monster_adjust_set[check_player]) {
				to_grave_set->insert(pcard);
				core.units.begin()->step = 2;
			}
			core.trap_monster_adjust_set[check_player].clear();
		} else if((int32)core.trap_monster_adjust_set[check_player].size() > fcount) {
			uint32 ct = (uint32)core.trap_monster_adjust_set[check_player].size() - fcount;
			core.select_cards.clear();
			for(auto& pcard : core.trap_monster_adjust_set[check_player])
				core.select_cards.push_back(pcard);
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_SELECTMSG);
			pduel->write_buffer8(check_player);
			pduel->write_buffer32(502);
			add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, check_player, ct + (ct << 16));
		} else
			core.units.begin()->step = 2;
		return FALSE;
	}
	case 2: {
		card_set* to_grave_set = (card_set*)core.units.begin()->ptr1;
		uint8 check_player = infos.turn_player;
		if(core.units.begin()->arg1)
			check_player = 1 - infos.turn_player;
		for(int32 i = 0; i < returns.bvalue[0]; ++i) {
			card* pcard = core.select_cards[returns.bvalue[i + 1]];
			to_grave_set->insert(pcard);
			core.trap_monster_adjust_set[check_player].erase(pcard);
		}
	}
	case 3: {
		if(!core.units.begin()->arg1) {
			core.units.begin()->arg1 = 1;
			core.units.begin()->step = 0;
		}
		return FALSE;
	}
	case 4: {
		uint8 tp = infos.turn_player;
		for(uint8 p = 0; p < 2; ++p) {
			for(auto& pcard : core.trap_monster_adjust_set[tp]) {
				pcard->reset(RESET_TURN_SET, RESET_EVENT);
				if(core.duel_rule <= 4)
					refresh_location_info_instant();
				move_to_field(pcard, tp, tp, LOCATION_SZONE, pcard->current.position, FALSE, 2);
			}
			tp = 1 - tp;
		}
		if(card_set* to_grave_set = (card_set*)core.units.begin()->ptr1) {
			if(to_grave_set->size())
				send_to(to_grave_set, 0, REASON_RULE, PLAYER_NONE, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
			delete to_grave_set;
		}
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::equip(uint16 step, uint8 equip_player, card * equip_card, card * target, uint32 up, uint32 is_step) {
	switch(step) {
	case 0: {
		returns.ivalue[0] = FALSE;
		if(!equip_card->is_affect_by_effect(core.reason_effect))
			return TRUE;
		if(equip_card == target)
			return TRUE;
		bool to_grave = false;
		if(target->current.location != LOCATION_MZONE || (target->current.position & POS_FACEDOWN))
			to_grave = true;
		if(equip_card->current.location != LOCATION_SZONE) {
			refresh_location_info_instant();
			if(get_useable_count(equip_card, equip_player, LOCATION_SZONE, equip_player, LOCATION_REASON_TOFIELD) <= 0)
				to_grave = true;
		}
		if(to_grave) {
			if(equip_card->current.location != LOCATION_GRAVE)
				send_to(equip_card, 0, REASON_RULE, PLAYER_NONE, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
			core.units.begin()->step = 2;
			return FALSE;
		}
		if(equip_card->equiping_target) {
			equip_card->effect_target_cards.erase(equip_card->equiping_target);
			equip_card->equiping_target->effect_target_owner.erase(equip_card);
			equip_card->unequip();
			equip_card->enable_field_effect(false);
			return FALSE;
		}
		if(equip_card->current.location == LOCATION_SZONE) {
			if(up && equip_card->is_position(POS_FACEDOWN))
				change_position(equip_card, 0, equip_player, POS_FACEUP, 0);
			return FALSE;
		}
		equip_card->enable_field_effect(false);
		move_to_field(equip_card, equip_player, equip_player, LOCATION_SZONE, (up || equip_card->is_position(POS_FACEUP)) ? POS_FACEUP : POS_FACEDOWN);
		return FALSE;
	}
	case 1: {
		equip_card->equip(target);
		if(!(equip_card->data.type & TYPE_EQUIP)) {
			effect* peffect = pduel->new_effect();
			peffect->owner = equip_card;
			peffect->handler = equip_card;
			peffect->type = EFFECT_TYPE_SINGLE;
			if(equip_card->get_type() & TYPE_TRAP) {
				peffect->code = EFFECT_ADD_TYPE;
				peffect->value = TYPE_EQUIP;
			} else if(equip_card->data.type & TYPE_UNION) {
				peffect->code = EFFECT_CHANGE_TYPE;
				peffect->value = TYPE_EQUIP + TYPE_SPELL + TYPE_UNION;
			} else {
				peffect->code = EFFECT_CHANGE_TYPE;
				peffect->value = TYPE_EQUIP + TYPE_SPELL;
			}
			peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
			peffect->reset_flag = RESET_EVENT + 0x17e0000;
			equip_card->add_effect(peffect);
		}
		equip_card->effect_target_cards.insert(target);
		target->effect_target_owner.insert(equip_card);
		if(!is_step) {
			if(equip_card->is_position(POS_FACEUP))
				equip_card->enable_field_effect(true);
			adjust_disable_check_list();
			card_set cset;
			cset.insert(equip_card);
			raise_single_event(target, &cset, EVENT_EQUIP, core.reason_effect, 0, core.reason_player, PLAYER_NONE, 0);
			raise_event(&cset, EVENT_EQUIP, core.reason_effect, 0, core.reason_player, PLAYER_NONE, 0);
			core.hint_timing[target->current.controler] |= TIMING_EQUIP;
			process_single_event();
			process_instant_event();
			return FALSE;
		} else {
			core.equiping_cards.insert(equip_card);
			returns.ivalue[0] = TRUE;
			return TRUE;
		}
	}
	case 2: {
		returns.ivalue[0] = TRUE;
		return TRUE;
	}
	case 3: {
		returns.ivalue[0] = FALSE;
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::summon(uint16 step, uint8 sumplayer, card* target, effect* proc, uint8 ignore_count, uint8 min_tribute, uint32 zone) {
	switch(step) {
	case 0: {
		if(!target->is_summonable_card())
			return TRUE;
		if(check_unique_onfield(target, sumplayer, LOCATION_MZONE))
			return TRUE;
		if(target->is_affected_by_effect(EFFECT_CANNOT_SUMMON))
			return TRUE;
		if(target->current.location == LOCATION_MZONE) {
			if(target->is_position(POS_FACEDOWN))
				return TRUE;
			if(!ignore_count && (core.extra_summon[sumplayer] || !target->is_affected_by_effect(EFFECT_EXTRA_SUMMON_COUNT))
			        && (core.summon_count[sumplayer] >= get_summon_count_limit(sumplayer)))
				return TRUE;
			if(!target->is_affected_by_effect(EFFECT_DUAL_SUMMONABLE))
				return TRUE;
			if(target->is_affected_by_effect(EFFECT_DUAL_STATUS))
				return TRUE;
			if(!is_player_can_summon(SUMMON_TYPE_DUAL, sumplayer, target, sumplayer))
				return TRUE;
		} else {
			effect_set eset;
			int32 res = target->filter_summon_procedure(sumplayer, &eset, ignore_count, min_tribute, zone);
			if(proc) {
				if(res < 0 || !target->check_summon_procedure(proc, sumplayer, ignore_count, min_tribute, zone))
					return TRUE;
			} else {
				if(res == -2)
					return TRUE;
				core.select_effects.clear();
				core.select_options.clear();
				if(res > 0) {
					core.select_effects.push_back(0);
					core.select_options.push_back(1);
				}
				for(int32 i = 0; i < eset.size(); ++i) {
					core.select_effects.push_back(eset[i]);
					core.select_options.push_back(eset[i]->description);
				}
				if(core.select_options.size() == 1)
					returns.ivalue[0] = 0;
				else
					add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, sumplayer, 0);
			}
		}
		if(core.summon_depth)
			core.summon_cancelable = FALSE;
		core.summon_depth++;
		target->material_cards.clear();
		return FALSE;
	}
	case 1: {
		effect_set eset;
		target->filter_effect(EFFECT_EXTRA_SUMMON_COUNT, &eset);
		if(target->current.location == LOCATION_MZONE) {
			core.units.begin()->step = 3;
			if(!ignore_count && !core.extra_summon[sumplayer]) {
				for(int32 i = 0; i < eset.size(); ++i) {
					core.units.begin()->ptr1 = eset[i];
					return FALSE;
				}
			}
			core.units.begin()->ptr1 = 0;
			return FALSE;
		}
		if(!proc) {
			proc = core.select_effects[returns.ivalue[0]];
			core.units.begin()->peffect = proc;
		}
		core.select_effects.clear();
		core.select_options.clear();
		if(ignore_count || core.summon_count[sumplayer] < get_summon_count_limit(sumplayer)) {
			core.select_effects.push_back(0);
			core.select_options.push_back(1);
		}
		if(!ignore_count && !core.extra_summon[sumplayer]) {
			for(int32 i = 0; i < eset.size(); ++i) {
				std::vector<int32> retval;
				eset[i]->get_value(target, 0, &retval);
				int32 new_min_tribute = retval.size() > 0 ? retval[0] : 0;
				int32 new_zone = retval.size() > 1 ? retval[1] : 0x1f;
				int32 releasable = retval.size() > 2 ? (retval[2] < 0 ? 0xff00ff + retval[2] : retval[2]) : 0xff00ff;
				new_zone &= zone;
				if(proc) {
					if(new_min_tribute < (int32)min_tribute)
						new_min_tribute = min_tribute;
					if(!target->is_summonable(proc, new_min_tribute, new_zone, releasable))
						continue;
				} else {
					int32 rcount = target->get_summon_tribute_count();
					int32 min = rcount & 0xffff;
					int32 max = (rcount >> 16) & 0xffff;
					if(!is_player_can_summon(SUMMON_TYPE_ADVANCE, sumplayer, target, sumplayer))
						max = 0;
					if(min < (int32)min_tribute)
						min = min_tribute;
					if(max < min)
						continue;
					if(min < new_min_tribute)
						min = new_min_tribute;
					if(!check_tribute(target, min, max, 0, target->current.controler, new_zone, releasable))
						continue;
				}
				core.select_effects.push_back(eset[i]);
				core.select_options.push_back(eset[i]->description);
			}
		}
		if(core.select_options.size() == 1)
			returns.ivalue[0] = 0;
		else
			add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, sumplayer, 0);
		return FALSE;
	}
	case 2: {
		effect* pextra = core.select_effects[returns.ivalue[0]];
		core.units.begin()->ptr1 = pextra;
		int32 releasable = 0xff00ff;
		if(pextra) {
			std::vector<int32> retval;
			pextra->get_value(target, 0, &retval);
			int32 new_min_tribute = retval.size() > 0 ? retval[0] : 0;
			int32 new_zone = retval.size() > 1 ? retval[1] : 0x1f;
			releasable = retval.size() > 2 ? (retval[2] < 0 ? 0xff00ff + retval[2] : retval[2]) : 0xff00ff;
			if((int32)min_tribute < new_min_tribute)
				min_tribute = new_min_tribute;
			zone &= new_zone;
			core.units.begin()->arg1 = sumplayer + (ignore_count << 8) + (min_tribute << 16) + (zone << 24);
			core.units.begin()->arg3 = releasable;
		}
		if(proc) {
			core.units.begin()->step = 3;
			return FALSE;
		}
		core.select_cards.clear();
		int32 required = target->get_summon_tribute_count();
		int32 min = required & 0xffff;
		int32 max = required >> 16;
		if(min < min_tribute) {
			min = min_tribute;
		}
		uint32 adv = is_player_can_summon(SUMMON_TYPE_ADVANCE, sumplayer, target, sumplayer);
		if(max == 0 || !adv) {
			returns.bvalue[0] = 0;
			core.units.begin()->step = 3;
		} else {
			core.release_cards.clear();
			core.release_cards_ex.clear();
			core.release_cards_ex_oneof.clear();
			int32 rcount = get_summon_release_list(target, &core.release_cards, &core.release_cards_ex, &core.release_cards_ex_oneof, NULL, 0, releasable);
			if(rcount == 0) {
				returns.bvalue[0] = 0;
				core.units.begin()->step = 3;
			} else {
				int32 ct = get_tofield_count(target, sumplayer, LOCATION_MZONE, sumplayer, LOCATION_REASON_TOFIELD, zone);
				int32 fcount = get_mzone_limit(sumplayer, sumplayer, LOCATION_REASON_TOFIELD);
				if(min == 0 && ct > 0 && fcount > 0) {
					add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, sumplayer, 90);
					core.units.begin()->arg2 = max;
				} else {
					if(min < -fcount + 1) {
						min = -fcount + 1;
					}
					select_tribute_cards(target, sumplayer, core.summon_cancelable, min, max, sumplayer, zone);
					core.units.begin()->step = 3;
				}
			}
		}
		return FALSE;
	}
	case 3: {
		if(returns.ivalue[0])
			returns.bvalue[0] = 0;
		else {
			int32 max = (int32)core.units.begin()->arg2;
			select_tribute_cards(target, sumplayer, core.summon_cancelable, 1, max, sumplayer, zone);
		}
		return FALSE;
	}
	case 4: {
		if(target->current.location == LOCATION_MZONE)
			core.units.begin()->step = 8;
		else if(proc)
			core.units.begin()->step = 5;
		else {
			if(returns.ivalue[0] == -1) {
				core.summon_depth--;
				return TRUE;
			}
			if(returns.bvalue[0]) {
				card_set* tributes = new card_set;
				for(int32 i = 0; i < returns.bvalue[0]; ++i)
					tributes->insert(core.select_cards[returns.bvalue[i + 1]]);
				core.units.begin()->peffect = (effect*)tributes;
			}
		}
		effect_set eset;
		target->filter_effect(EFFECT_SUMMON_COST, &eset);
		if(eset.size()) {
			for(int32 i = 0; i < eset.size(); ++i) {
				if(eset[i]->operation) {
					core.sub_solving_event.push_back(nil_event);
					add_process(PROCESSOR_EXECUTE_OPERATION, 0, eset[i], 0, sumplayer, 0);
				}
			}
			effect_set* peset = new effect_set;
			*peset = std::move(eset);
			core.units.begin()->ptr2 = peset;
		}
		return FALSE;
	}
	case 5: {
		card_set* tributes = (card_set*)proc;
		int32 min = 0;
		int32 level = target->get_level();
		if(level < 5)
			min = 0;
		else if(level < 7)
			min = 1;
		else
			min = 2;
		if(tributes)
			min -= (int32)tributes->size();
		if(min > 0) {
			std::vector<int32> duplicate;
			effect_set eset;
			target->filter_effect(EFFECT_DECREASE_TRIBUTE, &eset);
			for(int32 i = 0; i < eset.size(); ++i) {
				if(eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT))
					continue;
				std::vector<int32> retval;
				eset[i]->get_value(target, 0, &retval);
				int32 dec = retval.size() > 0 ? retval[0] : 0;
				int32 effect_code = retval.size() > 1 ? retval[1] : 0;
				if(effect_code > 0) {
					auto it = std::find(duplicate.begin(), duplicate.end(), effect_code);
					if(it == duplicate.end())
						duplicate.push_back(effect_code);
					else
						continue;
				}
				min -= dec & 0xffff;
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_CARD);
				pduel->write_buffer8(0);
				pduel->write_buffer32(eset[i]->handler->data.code);
			}
			for(int32 i = 0; i < eset.size() && min > 0; ++i) {
				if(!eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT) || eset[i]->count_limit == 0 || !eset[i]->target)
					continue;
				std::vector<int32> retval;
				eset[i]->get_value(target, 0, &retval);
				int32 dec = retval.size() > 0 ? retval[0] : 0;
				int32 effect_code = retval.size() > 1 ? retval[1] : 0;
				if(effect_code > 0) {
					auto it = std::find(duplicate.begin(), duplicate.end(), effect_code);
					if(it == duplicate.end())
						duplicate.push_back(effect_code);
					else
						continue;
				}
				min -= dec & 0xffff;
				eset[i]->dec_count();
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_CARD);
				pduel->write_buffer8(0);
				pduel->write_buffer32(eset[i]->handler->data.code);
			}
			for(int32 i = 0; i < eset.size() && min > 0; ++i) {
				if(!eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT) || eset[i]->count_limit == 0 || eset[i]->target)
					continue;
				std::vector<int32> retval;
				eset[i]->get_value(target, 0, &retval);
				int32 dec = retval.size() > 0 ? retval[0] : 0;
				int32 effect_code = retval.size() > 1 ? retval[1] : 0;
				if(effect_code > 0) {
					auto it = std::find(duplicate.begin(), duplicate.end(), effect_code);
					if(it == duplicate.end())
						duplicate.push_back(effect_code);
					else
						continue;
				}
				min -= dec & 0xffff;
				eset[i]->dec_count();
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_CARD);
				pduel->write_buffer8(0);
				pduel->write_buffer32(eset[i]->handler->data.code);
			}
		}
		if(tributes) {
			for(auto& pcard : *tributes)
				pcard->current.reason_card = target;
			target->set_material(tributes);
			release(tributes, 0, REASON_SUMMON | REASON_MATERIAL, sumplayer);
			target->summon_info = SUMMON_TYPE_NORMAL | SUMMON_TYPE_ADVANCE | (LOCATION_HAND << 16);
			delete tributes;
			core.units.begin()->peffect = 0;
			adjust_all();
		} else
			target->summon_info = SUMMON_TYPE_NORMAL | (LOCATION_HAND << 16);
		target->current.reason_effect = 0;
		target->current.reason_player = sumplayer;
		core.units.begin()->step = 6;
		return FALSE;
	}
	case 6: {
		target->summon_info = (proc->get_value(target) & 0xfffffff) | SUMMON_TYPE_NORMAL | (LOCATION_HAND << 16);
		target->current.reason_effect = proc;
		target->current.reason_player = sumplayer;
		int32 releasable = (int32)core.units.begin()->arg3;
		if(proc->operation) {
			pduel->lua->add_param(target, PARAM_TYPE_CARD);
			pduel->lua->add_param(min_tribute, PARAM_TYPE_INT);
			pduel->lua->add_param(zone, PARAM_TYPE_INT);
			pduel->lua->add_param(releasable, PARAM_TYPE_INT);
			pduel->game_field->core.limit_extra_summon_zone = zone;
			pduel->game_field->core.limit_extra_summon_releasable = releasable;
			core.sub_solving_event.push_back(nil_event);
			add_process(PROCESSOR_EXECUTE_OPERATION, 0, proc, 0, sumplayer, 0);
		}
		proc->dec_count(sumplayer);
		return FALSE;
	}
	case 7: {
		pduel->game_field->core.limit_extra_summon_zone = 0;
		pduel->game_field->core.limit_extra_summon_releasable = 0;
		core.summon_depth--;
		if(core.summon_depth)
			return TRUE;
		break_effect();
		if(ignore_count)
			return FALSE;
		effect* pextra = (effect*)core.units.begin()->ptr1;
		if(!pextra)
			core.summon_count[sumplayer]++;
		else {
			core.extra_summon[sumplayer] = TRUE;
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_CARD);
			pduel->write_buffer8(0);
			pduel->write_buffer32(pextra->handler->data.code);
			if(pextra->operation) {
				pduel->lua->add_param(target, PARAM_TYPE_CARD);
				core.sub_solving_event.push_back(nil_event);
				add_process(PROCESSOR_EXECUTE_OPERATION, 0, pextra, 0, sumplayer, 0);
			}
		}
		return FALSE;
	}
	case 8: {
		uint8 targetplayer = sumplayer;
		uint8 positions = POS_FACEUP_ATTACK;
		if(is_player_affected_by_effect(sumplayer, EFFECT_DIVINE_LIGHT))
			positions = POS_FACEUP;
		if(proc && proc->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
			positions = (uint8)proc->s_range & POS_FACEUP;
			if(proc->o_range)
				targetplayer = 1 - sumplayer;
		}
		target->enable_field_effect(false);
		move_to_field(target, sumplayer, targetplayer, LOCATION_MZONE, positions, FALSE, 0, FALSE, zone);
		core.units.begin()->step = 10;
		return FALSE;
	}
	case 9: {
		core.summon_depth--;
		if(core.summon_depth)
			return TRUE;
		target->enable_field_effect(false);
		target->summon_info |= SUMMON_TYPE_NORMAL;
		target->current.reason_effect = 0;
		target->current.reason_player = sumplayer;
		effect* deffect = pduel->new_effect();
		deffect->owner = target;
		deffect->code = EFFECT_DUAL_STATUS;
		deffect->type = EFFECT_TYPE_SINGLE;
		deffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_CLIENT_HINT;
		deffect->description = 64;
		deffect->reset_flag = RESET_EVENT + 0x1fe0000;
		target->add_effect(deffect);
		return FALSE;
	}
	case 10: {
		if(ignore_count)
			return FALSE;
		effect* pextra = (effect*)core.units.begin()->ptr1;
		if(!pextra)
			core.summon_count[sumplayer]++;
		else {
			core.extra_summon[sumplayer] = TRUE;
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_CARD);
			pduel->write_buffer8(0);
			pduel->write_buffer32(pextra->handler->data.code);
			if(pextra->operation) {
				pduel->lua->add_param(target, PARAM_TYPE_CARD);
				core.sub_solving_event.push_back(nil_event);
				add_process(PROCESSOR_EXECUTE_OPERATION, 0, pextra, 0, sumplayer, 0);
			}
		}
		return FALSE;
	}
	case 11: {
		set_control(target, target->current.controler, 0, 0);
		core.phase_action = TRUE;
		target->current.reason = REASON_SUMMON;
		target->summon_player = sumplayer;
		pduel->write_buffer8(MSG_SUMMONING);
		pduel->write_buffer32(target->data.code);
		pduel->write_buffer32(target->get_info_location());
		if (target->material_cards.size()) {
			for (auto& mcard : target->material_cards)
				raise_single_event(mcard, 0, EVENT_BE_PRE_MATERIAL, proc, REASON_SUMMON, sumplayer, sumplayer, 0);
			raise_event(&target->material_cards, EVENT_BE_PRE_MATERIAL, proc, REASON_SUMMON, sumplayer, sumplayer, 0);
		}
		process_single_event();
		process_instant_event();
		return FALSE;
	}
	case 12: {
		if(core.current_chain.size() == 0) {
			if(target->is_affected_by_effect(EFFECT_CANNOT_DISABLE_SUMMON))
				core.units.begin()->step = 14;
			return FALSE;
		} else {
			core.units.begin()->step = 14;
			return FALSE;
		}
		return FALSE;
	}
	case 13: {
		target->set_status(STATUS_SUMMONING, TRUE);
		target->set_status(STATUS_SUMMON_DISABLED, FALSE);
		raise_event(target, EVENT_SUMMON, proc, 0, sumplayer, sumplayer, 0);
		process_instant_event();
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0x101, TRUE);
		return FALSE;
	}
	case 14: {
		if(target->is_status(STATUS_SUMMONING)) {
			core.units.begin()->step = 14;
			return FALSE;
		}
		if(proc) {
			remove_oath_effect(proc);
			if(proc->is_flag(EFFECT_FLAG_COUNT_LIMIT) && (proc->count_code & EFFECT_COUNT_CODE_OATH)) {
				dec_effect_code(proc->count_code, sumplayer);
			}
		}
		if(effect_set* peset = (effect_set*)core.units.begin()->ptr2) {
			for(int32 i = 0; i < peset->size(); ++i)
				remove_oath_effect(peset->at(i));
			delete peset;
			core.units.begin()->ptr2 = 0;
		}
		if(target->current.location == LOCATION_MZONE)
			send_to(target, 0, REASON_RULE, sumplayer, sumplayer, LOCATION_GRAVE, 0, 0);
		adjust_instant();
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, 0);
		return TRUE;
	}
	case 15: {
		if(proc) {
			release_oath_relation(proc);
		}
		if(effect_set* peset = (effect_set*)core.units.begin()->ptr2) {
			for(int32 i = 0; i < peset->size(); ++i)
				release_oath_relation(peset->at(i));
			delete peset;
			core.units.begin()->ptr2 = 0;
		}
		target->set_status(STATUS_SUMMONING, FALSE);
		target->set_status(STATUS_SUMMON_TURN, TRUE);
		target->enable_field_effect(true);
		if(target->is_status(STATUS_DISABLED))
			target->reset(RESET_DISABLE, RESET_EVENT);
		return FALSE;
	}
	case 16: {
		pduel->write_buffer8(MSG_SUMMONED);
		adjust_instant();
		if(target->material_cards.size()) {
			for(auto& mcard : target->material_cards)
				raise_single_event(mcard, 0, EVENT_BE_MATERIAL, proc, REASON_SUMMON, sumplayer, sumplayer, 0);
			raise_event(&target->material_cards, EVENT_BE_MATERIAL, proc, REASON_SUMMON, sumplayer, sumplayer, 0);
		}
		process_single_event();
		process_instant_event();
		return FALSE;
	}
	case 17: {
		core.summon_state_count[sumplayer]++;
		core.normalsummon_state_count[sumplayer]++;
		check_card_counter(target, ACTIVITY_SUMMON, sumplayer);
		check_card_counter(target, ACTIVITY_NORMALSUMMON, sumplayer);
		raise_single_event(target, 0, EVENT_SUMMON_SUCCESS, proc, 0, sumplayer, sumplayer, 0);
		process_single_event();
		raise_event(target, EVENT_SUMMON_SUCCESS, proc, 0, sumplayer, sumplayer, 0);
		process_instant_event();
		if(core.current_chain.size() == 0) {
			adjust_all();
			core.hint_timing[sumplayer] |= TIMING_SUMMON;
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, 0);
		}
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::flip_summon(uint16 step, uint8 sumplayer, card * target) {
	switch(step) {
	case 0: {
		if(target->current.location != LOCATION_MZONE)
			return TRUE;
		if(!(target->current.position & POS_FACEDOWN))
			return TRUE;
		if(check_unique_onfield(target, sumplayer, LOCATION_MZONE))
			return TRUE;
		effect_set eset;
		target->filter_effect(EFFECT_FLIPSUMMON_COST, &eset);
		if(eset.size()) {
			for(int32 i = 0; i < eset.size(); ++i) {
				if(eset[i]->operation) {
					core.sub_solving_event.push_back(nil_event);
					add_process(PROCESSOR_EXECUTE_OPERATION, 0, eset[i], 0, sumplayer, 0);
				}
			}
			effect_set* peset = new effect_set;
			*peset = std::move(eset);
			core.units.begin()->ptr1 = peset;
		}
		return FALSE;
	}
	case 1: {
		target->previous.position = target->current.position;
		target->current.position = POS_FACEUP_ATTACK;
		target->summon_player = sumplayer;
		target->fieldid = infos.field_id++;
		core.phase_action = TRUE;
		pduel->write_buffer8(MSG_FLIPSUMMONING);
		pduel->write_buffer32(target->data.code);
		pduel->write_buffer32(target->get_info_location());
		if(target->is_affected_by_effect(EFFECT_CANNOT_DISABLE_FLIP_SUMMON))
			core.units.begin()->step = 2;
		else {
			target->set_status(STATUS_SUMMONING, TRUE);
			target->set_status(STATUS_SUMMON_DISABLED, FALSE);
			raise_event(target, EVENT_FLIP_SUMMON, 0, 0, sumplayer, sumplayer, 0);
			process_instant_event();
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0x101, TRUE);
		}
		return FALSE;
	}
	case 2: {
		if(target->is_status(STATUS_SUMMONING))
			return FALSE;
		if(effect_set* peset = (effect_set*)core.units.begin()->ptr1) {
			for(int32 i = 0; i < peset->size(); ++i)
				remove_oath_effect(peset->at(i));
			delete peset;
			core.units.begin()->ptr1 = 0;
		}
		if(target->current.location == LOCATION_MZONE)
			send_to(target, 0, REASON_RULE, sumplayer, sumplayer, LOCATION_GRAVE, 0, 0);
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, 0);
		return TRUE;
	}
	case 3: {
		if(effect_set* peset = (effect_set*)core.units.begin()->ptr1) {
			for(int32 i = 0; i < peset->size(); ++i)
				release_oath_relation(peset->at(i));
			delete peset;
			core.units.begin()->ptr1 = 0;
		}
		target->set_status(STATUS_SUMMONING, FALSE);
		target->enable_field_effect(true);
		if(target->is_status(STATUS_DISABLED))
			target->reset(RESET_DISABLE, RESET_EVENT);
		target->set_status(STATUS_FLIP_SUMMON_TURN, TRUE);
		return FALSE;
	}
	case 4: {
		pduel->write_buffer8(MSG_FLIPSUMMONED);
		core.flipsummon_state_count[sumplayer]++;
		check_card_counter(target, ACTIVITY_FLIPSUMMON, sumplayer);
		adjust_instant();
		raise_single_event(target, 0, EVENT_FLIP, 0, 0, sumplayer, sumplayer, 0);
		raise_single_event(target, 0, EVENT_FLIP_SUMMON_SUCCESS, 0, 0, sumplayer, sumplayer, 0);
		raise_single_event(target, 0, EVENT_CHANGE_POS, 0, 0, sumplayer, sumplayer, 0);
		process_single_event();
		raise_event(target, EVENT_FLIP, 0, 0, sumplayer, sumplayer, 0);
		raise_event(target, EVENT_FLIP_SUMMON_SUCCESS, 0, 0, sumplayer, sumplayer, 0);
		raise_event(target, EVENT_CHANGE_POS, 0, 0, sumplayer, sumplayer, 0);
		process_instant_event();
		adjust_all();
		if(core.current_chain.size() == 0) {
			core.hint_timing[sumplayer] |= TIMING_FLIPSUMMON;
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, 0);
		}
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::mset(uint16 step, uint8 setplayer, card* target, effect* proc, uint8 ignore_count, uint8 min_tribute, uint32 zone) {
	switch(step) {
	case 0: {
		if(!target->is_summonable_card())
			return TRUE;
		if(target->current.location != LOCATION_HAND)
			return TRUE;
		if(!(target->data.type & TYPE_MONSTER))
			return TRUE;
		if(target->is_affected_by_effect(EFFECT_CANNOT_MSET))
			return TRUE;
		effect_set eset;
		int32 res = target->filter_set_procedure(setplayer, &eset, ignore_count, min_tribute, zone);
		if(proc) {
			if(res < 0 || !target->check_set_procedure(proc, setplayer, ignore_count, min_tribute, zone))
				return TRUE;
		} else {
			if(res == -2)
				return TRUE;
			core.select_effects.clear();
			core.select_options.clear();
			if(res > 0) {
				core.select_effects.push_back(0);
				core.select_options.push_back(1);
			}
			for(int32 i = 0; i < eset.size(); ++i) {
				core.select_effects.push_back(eset[i]);
				core.select_options.push_back(eset[i]->description);
			}
			if(core.select_options.size() == 1)
				returns.ivalue[0] = 0;
			else
				add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, setplayer, 0);
		}
		target->material_cards.clear();
		return FALSE;
	}
	case 1: {
		effect_set eset;
		target->filter_effect(EFFECT_EXTRA_SET_COUNT, &eset);
		if(!proc) {
			proc = core.select_effects[returns.ivalue[0]];
			core.units.begin()->peffect = proc;
		}
		core.select_effects.clear();
		core.select_options.clear();
		if(ignore_count || core.summon_count[setplayer] < get_summon_count_limit(setplayer)) {
			core.select_effects.push_back(0);
			core.select_options.push_back(1);
		}
		if(!ignore_count && !core.extra_summon[setplayer]) {
			for(int32 i = 0; i < eset.size(); ++i) {
				std::vector<int32> retval;
				eset[i]->get_value(target, 0, &retval);
				int32 new_min_tribute = retval.size() > 0 ? retval[0] : 0;
				int32 new_zone = retval.size() > 1 ? retval[1] : 0x1f;
				int32 releasable = retval.size() > 2 ? (retval[2] < 0 ? 0xff00ff + retval[2] : retval[2]) : 0xff00ff;
				new_zone &= zone;
				if(proc) {
					if(new_min_tribute < (int32)min_tribute)
						new_min_tribute = min_tribute;
					if(!target->is_summonable(proc, new_min_tribute, new_zone, releasable))
						continue;
				} else {
					int32 rcount = target->get_set_tribute_count();
					int32 min = rcount & 0xffff;
					int32 max = (rcount >> 16) & 0xffff;
					if(!is_player_can_mset(SUMMON_TYPE_ADVANCE, setplayer, target, setplayer))
						max = 0;
					if(min < (int32)min_tribute)
						min = min_tribute;
					if(max < min)
						continue;
					if(min < new_min_tribute)
						min = new_min_tribute;
					if(!check_tribute(target, min, max, 0, target->current.controler, new_zone, releasable, POS_FACEDOWN_DEFENSE))
						continue;
				}
				core.select_effects.push_back(eset[i]);
				core.select_options.push_back(eset[i]->description);
			}
		}
		if(core.select_options.size() == 1)
			returns.ivalue[0] = 0;
		else
			add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, setplayer, 0);
		return FALSE;
	}
	case 2: {
		effect* pextra = core.select_effects[returns.ivalue[0]];
		core.units.begin()->ptr1 = pextra;
		int32 releasable = 0xff00ff;
		if(pextra) {
			std::vector<int32> retval;
			pextra->get_value(target, 0, &retval);
			int32 new_min_tribute = retval.size() > 0 ? retval[0] : 0;
			int32 new_zone = retval.size() > 1 ? retval[1] : 0x1f;
			releasable = retval.size() > 2 ? (retval[2] < 0 ? 0xff00ff + retval[2] : retval[2]) : 0xff00ff;
			if((int32)min_tribute < new_min_tribute)
				min_tribute = new_min_tribute;
			zone &= new_zone;
			core.units.begin()->arg1 = setplayer + (ignore_count << 8) + (min_tribute << 16) + (zone << 24);
		}
		if(proc) {
			core.units.begin()->step = 3;
			return FALSE;
		}
		core.select_cards.clear();
		int32 required = target->get_set_tribute_count();
		int32 min = required & 0xffff;
		int32 max = required >> 16;
		if(min < min_tribute) {
			min = min_tribute;
		}
		uint32 adv = is_player_can_mset(SUMMON_TYPE_ADVANCE, setplayer, target, setplayer);
		if(max == 0 || !adv) {
			returns.bvalue[0] = 0;
			core.units.begin()->step = 3;
		} else {
			core.release_cards.clear();
			core.release_cards_ex.clear();
			core.release_cards_ex_oneof.clear();
			int32 rcount = get_summon_release_list(target, &core.release_cards, &core.release_cards_ex, &core.release_cards_ex_oneof, NULL, 0, releasable, POS_FACEDOWN_DEFENSE);
			if(rcount == 0) {
				returns.bvalue[0] = 0;
				core.units.begin()->step = 3;
			} else {
				int32 ct = get_tofield_count(target, setplayer, LOCATION_MZONE, setplayer, LOCATION_REASON_TOFIELD, zone);
				int32 fcount = get_mzone_limit(setplayer, setplayer, LOCATION_REASON_TOFIELD);
				if(min == 0 && ct > 0 && fcount > 0) {
					add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, setplayer, 90);
					core.units.begin()->arg2 = max;
				} else {
					if(min < -fcount + 1) {
						min = -fcount + 1;
					}
					select_tribute_cards(target, setplayer, core.summon_cancelable, min, max, setplayer, zone);
					core.units.begin()->step = 3;
				}
			}
		}
		return FALSE;
	}
	case 3: {
		if(returns.ivalue[0])
			returns.bvalue[0] = 0;
		else {
			int32 max = (int32)core.units.begin()->arg2;
			select_tribute_cards(target, setplayer, core.summon_cancelable, 1, max, setplayer, zone);
		}
		return FALSE;
	}
	case 4: {
		if(proc)
			core.units.begin()->step = 5;
		else {
			if(returns.ivalue[0] == -1) {
				return TRUE;
			}
			if(returns.bvalue[0]) {
				card_set* tributes = new card_set;
				for(int32 i = 0; i < returns.bvalue[0]; ++i)
					tributes->insert(core.select_cards[returns.bvalue[i + 1]]);
				core.units.begin()->peffect = (effect*)tributes;
			}
		}
		effect_set eset;
		target->filter_effect(EFFECT_MSET_COST, &eset);
		for(int32 i = 0; i < eset.size(); ++i) {
			if(eset[i]->operation) {
				core.sub_solving_event.push_back(nil_event);
				add_process(PROCESSOR_EXECUTE_OPERATION, 0, eset[i], 0, setplayer, 0);
			}
		}
		return FALSE;
	}
	case 5: {
		card_set* tributes = (card_set*)proc;
		int32 min = 0;
		int32 level = target->get_level();
		if(level < 5)
			min = 0;
		else if(level < 7)
			min = 1;
		else
			min = 2;
		if(tributes)
			min -= (int32)tributes->size();
		if(min > 0) {
			std::vector<int32> duplicate;
			effect_set eset;
			target->filter_effect(EFFECT_DECREASE_TRIBUTE_SET, &eset);
			for(int32 i = 0; i < eset.size(); ++i) {
				if(eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT))
					continue;
				std::vector<int32> retval;
				eset[i]->get_value(target, 0, &retval);
				int32 dec = retval.size() > 0 ? retval[0] : 0;
				int32 effect_code = retval.size() > 1 ? retval[1] : 0;
				if(effect_code > 0) {
					auto it = std::find(duplicate.begin(), duplicate.end(), effect_code);
					if(it == duplicate.end())
						duplicate.push_back(effect_code);
					else
						continue;
				}
				min -= dec & 0xffff;
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_CARD);
				pduel->write_buffer8(0);
				pduel->write_buffer32(eset[i]->handler->data.code);
			}
			for(int32 i = 0; i < eset.size() && min > 0; ++i) {
				if(!eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT) || eset[i]->count_limit == 0 || !eset[i]->target)
					continue;
				std::vector<int32> retval;
				eset[i]->get_value(target, 0, &retval);
				int32 dec = retval.size() > 0 ? retval[0] : 0;
				int32 effect_code = retval.size() > 1 ? retval[1] : 0;
				if(effect_code > 0) {
					auto it = std::find(duplicate.begin(), duplicate.end(), effect_code);
					if(it == duplicate.end())
						duplicate.push_back(effect_code);
					else
						continue;
				}
				min -= dec & 0xffff;
				eset[i]->dec_count();
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_CARD);
				pduel->write_buffer8(0);
				pduel->write_buffer32(eset[i]->handler->data.code);
			}
			for(int32 i = 0; i < eset.size() && min > 0; ++i) {
				if(!eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT) || eset[i]->count_limit == 0 || eset[i]->target)
					continue;
				std::vector<int32> retval;
				eset[i]->get_value(target, 0, &retval);
				int32 dec = retval.size() > 0 ? retval[0] : 0;
				int32 effect_code = retval.size() > 1 ? retval[1] : 0;
				if(effect_code > 0) {
					auto it = std::find(duplicate.begin(), duplicate.end(), effect_code);
					if(it == duplicate.end())
						duplicate.push_back(effect_code);
					else
						continue;
				}
				min -= dec & 0xffff;
				eset[i]->dec_count();
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_CARD);
				pduel->write_buffer8(0);
				pduel->write_buffer32(eset[i]->handler->data.code);
			}
		}
		if(tributes) {
			for(auto& pcard : *tributes)
				pcard->current.reason_card = target;
			target->set_material(tributes);
			release(tributes, 0, REASON_SUMMON | REASON_MATERIAL, setplayer);
			target->summon_info = SUMMON_TYPE_NORMAL | SUMMON_TYPE_ADVANCE | (LOCATION_HAND << 16);
			delete tributes;
			core.units.begin()->peffect = 0;
			adjust_all();
		} else
			target->summon_info = SUMMON_TYPE_NORMAL | (LOCATION_HAND << 16);
		target->summon_player = setplayer;
		target->current.reason_effect = 0;
		target->current.reason_player = setplayer;
		core.units.begin()->step = 6;
		return FALSE;
	}
	case 6: {
		target->summon_info = (proc->get_value(target) & 0xfffffff) | SUMMON_TYPE_NORMAL | (LOCATION_HAND << 16);
		target->summon_player = setplayer;
		target->current.reason_effect = proc;
		target->current.reason_player = setplayer;
		pduel->lua->add_param(target, PARAM_TYPE_CARD);
		core.sub_solving_event.push_back(nil_event);
		add_process(PROCESSOR_EXECUTE_OPERATION, 0, proc, 0, setplayer, 0);
		proc->dec_count(setplayer);
		return FALSE;
	}
	case 7: {
		break_effect();
		if(ignore_count)
			return FALSE;
		effect* pextra = (effect*)core.units.begin()->ptr1;
		if(!pextra)
			core.summon_count[setplayer]++;
		else {
			core.extra_summon[setplayer] = TRUE;
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_CARD);
			pduel->write_buffer8(0);
			pduel->write_buffer32(pextra->handler->data.code);
			if(pextra->operation) {
				pduel->lua->add_param(target, PARAM_TYPE_CARD);
				core.sub_solving_event.push_back(nil_event);
				add_process(PROCESSOR_EXECUTE_OPERATION, 0, pextra, 0, setplayer, 0);
			}
		}
		return FALSE;
	}
	case 8: {
		uint8 targetplayer = setplayer;
		uint8 positions = POS_FACEDOWN_DEFENSE;
		if(proc && proc->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
			positions = (uint8)proc->s_range & POS_FACEDOWN;
			if(proc->o_range)
				targetplayer = 1 - setplayer;
		}
		target->enable_field_effect(false);
		move_to_field(target, setplayer, targetplayer, LOCATION_MZONE, positions, FALSE, 0, FALSE, zone);
		return FALSE;
	}
	case 9: {
		set_control(target, target->current.controler, 0, 0);
		core.phase_action = TRUE;
		core.normalsummon_state_count[setplayer]++;
		check_card_counter(target, ACTIVITY_NORMALSUMMON, setplayer);
		target->set_status(STATUS_SUMMON_TURN, TRUE);
		pduel->write_buffer8(MSG_SET);
		pduel->write_buffer32(target->data.code);
		pduel->write_buffer32(target->get_info_location());
		adjust_instant();
		raise_event(target, EVENT_MSET, proc, 0, setplayer, setplayer, 0);
		process_instant_event();
		if(core.current_chain.size() == 0) {
			adjust_all();
			core.hint_timing[setplayer] |= TIMING_MSET;
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, FALSE);
		}
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::sset(uint16 step, uint8 setplayer, uint8 toplayer, card * target, effect* reason_effect) {
	switch(step) {
	case 0: {
		if(!(target->data.type & TYPE_FIELD) && get_useable_count(target, toplayer, LOCATION_SZONE, setplayer, LOCATION_REASON_TOFIELD) <= 0)
			return TRUE;
		if(target->data.type & TYPE_MONSTER && !target->is_affected_by_effect(EFFECT_MONSTER_SSET))
			return TRUE;
		if(target->current.location == LOCATION_SZONE)
			return TRUE;
		if(!is_player_can_sset(setplayer, target))
			return TRUE;
		if(target->is_affected_by_effect(EFFECT_CANNOT_SSET))
			return TRUE;
		effect_set eset;
		target->filter_effect(EFFECT_SSET_COST, &eset);
		for(int32 i = 0; i < eset.size(); ++i) {
			if(eset[i]->operation) {
				core.sub_solving_event.push_back(nil_event);
				add_process(PROCESSOR_EXECUTE_OPERATION, 0, eset[i], 0, setplayer, 0);
			}
		}
		return FALSE;
	}
	case 1: {
		target->enable_field_effect(false);
		move_to_field(target, setplayer, toplayer, LOCATION_SZONE, POS_FACEDOWN, FALSE, 0, FALSE, (target->data.type & TYPE_FIELD) ? 0x1 << 5 : 0xff);
		return FALSE;
	}
	case 2: {
		core.phase_action = TRUE;
		target->set_status(STATUS_SET_TURN, TRUE);
		if(target->data.type & TYPE_MONSTER) {
			effect* peffect = target->is_affected_by_effect(EFFECT_MONSTER_SSET);
			int32 type_val = peffect->get_value();
			peffect = pduel->new_effect();
			peffect->owner = target;
			peffect->type = EFFECT_TYPE_SINGLE;
			peffect->code = EFFECT_CHANGE_TYPE;
			peffect->reset_flag = RESET_EVENT + 0x1fe0000;
			peffect->value = type_val;
			target->add_effect(peffect);
		}
		pduel->write_buffer8(MSG_SET);
		pduel->write_buffer32(target->data.code);
		pduel->write_buffer32(target->get_info_location());
		adjust_instant();
		raise_event(target, EVENT_SSET, reason_effect, 0, setplayer, setplayer, 0);
		process_instant_event();
		if(core.current_chain.size() == 0) {
			adjust_all();
			core.hint_timing[setplayer] |= TIMING_SSET;
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, FALSE);
		}
	}
	}
	return TRUE;
}
int32 field::sset_g(uint16 step, uint8 setplayer, uint8 toplayer, group* ptarget, uint8 confirm, effect* reason_effect) {
	switch(step) {
	case 0: {
		card_set* set_cards = new card_set;
		core.operated_set.clear();
		for(auto& target : ptarget->container) {
			if((!(target->data.type & TYPE_FIELD) && get_useable_count(target, toplayer, LOCATION_SZONE, setplayer, LOCATION_REASON_TOFIELD) <= 0)
			        || (target->data.type & TYPE_MONSTER && !target->is_affected_by_effect(EFFECT_MONSTER_SSET))
			        || (target->current.location == LOCATION_SZONE)
			        || (!is_player_can_sset(setplayer, target))
			        || (target->is_affected_by_effect(EFFECT_CANNOT_SSET))) {
				continue;
			}
			set_cards->insert(target);
		}
		if(set_cards->empty()) {
			delete set_cards;
			returns.ivalue[0] = 0;
			return TRUE;
		}
		effect_set eset;
 		for(auto& pcard : *set_cards) {
			eset.clear();
			pcard->filter_effect(EFFECT_SSET_COST, &eset);
			for(int32 i = 0; i < eset.size(); ++i) {
				if(eset[i]->operation) {
					core.sub_solving_event.push_back(nil_event);
					add_process(PROCESSOR_EXECUTE_OPERATION, 0, eset[i], 0, setplayer, 0);
				}
			}
		}
		core.set_group_pre_set.clear();
		core.set_group_set.clear();
		core.set_group_used_zones = 0;
		core.phase_action = TRUE;
		core.units.begin()->ptarget = (group*)set_cards;
		return FALSE;
	}
	case 1: {
		card_set* set_cards = (card_set*)ptarget;
		card* target = *set_cards->begin();
		if(target->data.type & TYPE_FIELD) {
			returns.bvalue[2] = 5;
			return FALSE;
		}
		uint32 flag;
		get_useable_count(target, toplayer, LOCATION_SZONE, setplayer, LOCATION_REASON_TOFIELD, 0xff, &flag);
		flag |= core.set_group_used_zones;
		if(setplayer == toplayer) {
			flag = ((flag & 0xff) << 8) | 0xffff00ff;
		} else {
			flag = ((flag & 0xff) << 24) | 0xffffff;
		}
		flag |= 0xe080e080;
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(setplayer);
		pduel->write_buffer32(target->data.code);
		add_process(PROCESSOR_SELECT_PLACE, 0, 0, 0, setplayer, flag, 1);
		return FALSE;
	}
	case 2: {
		card_set* set_cards = (card_set*)ptarget;
		card* target = *set_cards->begin();
		uint32 seq = returns.bvalue[2];
		core.set_group_seq[core.set_group_pre_set.size()] = seq;
		core.set_group_pre_set.insert(target);
		core.set_group_used_zones |= (1 << seq);
		set_cards->erase(target);
		if(!set_cards->empty())
			core.units.begin()->step = 0;
		else
			delete set_cards;
		return FALSE;
	}
	case 3: {
		card_set* set_cards = &core.set_group_pre_set;
		card* target = *set_cards->begin();
		target->enable_field_effect(false);
		uint32 zone;
		if(target->data.type & TYPE_FIELD) {
			zone = 1 << 5;
		} else {
			for(uint32 i = 0; i < 7; i++) {
				zone = 1 << i;
				if(core.set_group_used_zones & zone) {
					core.set_group_used_zones &= ~zone;
					break;
				}
			}
		}
		move_to_field(target, setplayer, toplayer, LOCATION_SZONE, POS_FACEDOWN, FALSE, 0, FALSE, zone);
		return FALSE;
	}
	case 4: {
		card_set* set_cards = &core.set_group_pre_set;
		card* target = *set_cards->begin();
		target->set_status(STATUS_SET_TURN, TRUE);
		if(target->data.type & TYPE_MONSTER) {
			effect* peffect = target->is_affected_by_effect(EFFECT_MONSTER_SSET);
			int32 type_val = peffect->get_value();
			peffect = pduel->new_effect();
			peffect->owner = target;
			peffect->type = EFFECT_TYPE_SINGLE;
			peffect->code = EFFECT_CHANGE_TYPE;
			peffect->reset_flag = RESET_EVENT + 0x1fe0000;
			peffect->value = type_val;
			target->add_effect(peffect);
		}
		pduel->write_buffer8(MSG_SET);
		pduel->write_buffer32(target->data.code);
		pduel->write_buffer32(target->get_info_location());
		core.set_group_set.insert(target);
		set_cards->erase(target);
		if(!set_cards->empty())
			core.units.begin()->step = 2;
		return FALSE;
	}
	case 5: {
		if(confirm) {
			pduel->write_buffer8(MSG_CONFIRM_CARDS);
			pduel->write_buffer8(toplayer);
			pduel->write_buffer8((uint8)core.set_group_set.size());
			for(auto& pcard : core.set_group_set) {
				pduel->write_buffer32(pcard->data.code);
				pduel->write_buffer8(pcard->current.controler);
				pduel->write_buffer8(pcard->current.location);
				pduel->write_buffer8(pcard->current.sequence);
			}
		}
		return FALSE;
	}
	case 6: {
		core.operated_set.clear();
		for(auto& pcard : core.set_group_set) {
			core.operated_set.insert(pcard);
		}
		uint8 ct = (uint8)core.operated_set.size();
		if(core.set_group_used_zones & (1 << 5))
			ct--;
		if(ct <= 1)
			return FALSE;
		pduel->write_buffer8(MSG_SHUFFLE_SET_CARD);
		pduel->write_buffer8(LOCATION_SZONE);
		pduel->write_buffer8(ct);
		uint8 i = 0;
		for(auto cit = core.operated_set.begin(); cit != core.operated_set.end(); ++cit) {
			card* pcard = *cit;
			uint8 seq = core.set_group_seq[i];
			i++;
			if(pcard->data.type & TYPE_FIELD)
				continue;
			pduel->write_buffer32(pcard->get_info_location());
			pduel->game_field->player[toplayer].list_szone[seq] = pcard;
			pcard->current.sequence = seq;
		}
		for(uint32 i = 0; i < ct; ++i) {
			pduel->write_buffer32(0);
		}
		return FALSE;
	}
	case 7: {
		adjust_instant();
		raise_event(&core.operated_set, EVENT_SSET, reason_effect, 0, setplayer, setplayer, 0);
		process_instant_event();
		if(core.current_chain.size() == 0) {
			adjust_all();
			core.hint_timing[setplayer] |= TIMING_SSET;
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, FALSE);
		}
		return FALSE;
	}
	case 8: {
		returns.ivalue[0] = (int32)core.operated_set.size();
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::special_summon_rule(uint16 step, uint8 sumplayer, card* target, uint32 summon_type) {
	switch(step) {
	case 0: {
		effect_set eset;
		material_info info;
		info.limit_tuner = core.limit_tuner;
		info.limit_syn = core.limit_syn;
		info.limit_syn_minc = core.limit_syn_minc;
		info.limit_syn_maxc = core.limit_syn_maxc;
		info.limit_xyz = core.limit_xyz;
		info.limit_xyz_minc = core.limit_xyz_minc;
		info.limit_xyz_maxc = core.limit_xyz_maxc;
		info.limit_link = core.limit_link;
		info.limit_link_card = core.limit_link_card;
		info.limit_link_minc = core.limit_link_minc;
		info.limit_link_maxc = core.limit_link_maxc;
		target->filter_spsummon_procedure(sumplayer, &eset, summon_type, info);
		target->filter_spsummon_procedure_g(sumplayer, &eset);
		if(!eset.size())
			return TRUE;
		core.select_effects.clear();
		core.select_options.clear();
		for(int32 i = 0; i < eset.size(); ++i) {
			core.select_effects.push_back(eset[i]);
			core.select_options.push_back(eset[i]->description);
		}
		if(core.select_options.size() == 1)
			returns.ivalue[0] = 0;
		else
			add_process(PROCESSOR_SELECT_OPTION, 0, 0, 0, sumplayer, 0);
		return FALSE;
	}
	case 1: {
		effect* peffect = core.select_effects[returns.ivalue[0]];
		core.units.begin()->peffect = peffect;
		if(peffect->code == EFFECT_SPSUMMON_PROC_G) {
			core.units.begin()->step = 19;
			return FALSE;
		}
		returns.ivalue[0] = TRUE;
		if(peffect->target) {
			pduel->lua->add_param(target, PARAM_TYPE_CARD);
			if(core.limit_tuner || core.limit_syn) {
				pduel->lua->add_param(core.limit_tuner, PARAM_TYPE_CARD);
				pduel->lua->add_param(core.limit_syn, PARAM_TYPE_GROUP);
				if(core.limit_syn_minc) {
					pduel->lua->add_param(core.limit_syn_minc, PARAM_TYPE_INT);
					pduel->lua->add_param(core.limit_syn_maxc, PARAM_TYPE_INT);
				}
			} else if(core.limit_xyz) {
				pduel->lua->add_param(core.limit_xyz, PARAM_TYPE_GROUP);
				if(core.limit_xyz_minc) {
					pduel->lua->add_param(core.limit_xyz_minc, PARAM_TYPE_INT);
					pduel->lua->add_param(core.limit_xyz_maxc, PARAM_TYPE_INT);
				}
			} else if(core.limit_link || core.limit_link_card) {
				pduel->lua->add_param(core.limit_link, PARAM_TYPE_GROUP);
				pduel->lua->add_param(core.limit_link_card, PARAM_TYPE_CARD);
				if(core.limit_link_minc) {
					pduel->lua->add_param(core.limit_link_minc, PARAM_TYPE_INT);
					pduel->lua->add_param(core.limit_link_maxc, PARAM_TYPE_INT);
				}
			}
			core.sub_solving_event.push_back(nil_event);
			add_process(PROCESSOR_EXECUTE_TARGET, 0, peffect, 0, sumplayer, 0);
		}
		return FALSE;
	}
	case 2: {
		if(!returns.ivalue[0])
			return TRUE;
		effect_set eset;
		target->filter_effect(EFFECT_SPSUMMON_COST, &eset);
		if(eset.size()) {
			for(int32 i = 0; i < eset.size(); ++i) {
				if(eset[i]->operation) {
					core.sub_solving_event.push_back(nil_event);
					add_process(PROCESSOR_EXECUTE_OPERATION, 0, eset[i], 0, sumplayer, 0);
				}
			}
			effect_set* peset = new effect_set;
			*peset = std::move(eset);
			core.units.begin()->ptr1 = peset;
		}
		return FALSE;
	}
	case 3: {
		effect* peffect = core.units.begin()->peffect;
		target->material_cards.clear();
		if(peffect->operation) {
			pduel->lua->add_param(target, PARAM_TYPE_CARD);
			if(core.limit_tuner || core.limit_syn) {
				pduel->lua->add_param(core.limit_tuner, PARAM_TYPE_CARD);
				pduel->lua->add_param(core.limit_syn, PARAM_TYPE_GROUP);
				core.limit_tuner = 0;
				if(core.limit_syn_minc) {
					pduel->lua->add_param(core.limit_syn_minc, PARAM_TYPE_INT);
					pduel->lua->add_param(core.limit_syn_maxc, PARAM_TYPE_INT);
					core.limit_syn_minc = 0;
					core.limit_syn_maxc = 0;
				}
			}
			if(core.limit_xyz) {
				pduel->lua->add_param(core.limit_xyz, PARAM_TYPE_GROUP);
				if(core.limit_xyz_minc) {
					pduel->lua->add_param(core.limit_xyz_minc, PARAM_TYPE_INT);
					pduel->lua->add_param(core.limit_xyz_maxc, PARAM_TYPE_INT);
					core.limit_xyz_minc = 0;
					core.limit_xyz_maxc = 0;
				}
			}
			if(core.limit_link || core.limit_link_card) {
				pduel->lua->add_param(core.limit_link, PARAM_TYPE_GROUP);
				pduel->lua->add_param(core.limit_link_card, PARAM_TYPE_CARD);
				core.limit_link_card = 0;
				if(core.limit_link_minc) {
					pduel->lua->add_param(core.limit_link_minc, PARAM_TYPE_INT);
					pduel->lua->add_param(core.limit_link_maxc, PARAM_TYPE_INT);
					core.limit_link_minc = 0;
					core.limit_link_maxc = 0;
				}
			}
			core.sub_solving_event.push_back(nil_event);
			add_process(PROCESSOR_EXECUTE_OPERATION, 0, peffect, 0, sumplayer, 0);
		}
		peffect->dec_count(sumplayer);
		return FALSE;
	}
	case 4: {
		if(core.limit_syn) {
			pduel->delete_group(core.limit_syn);
			core.limit_syn = 0;
		}
		if(core.limit_xyz) {
			pduel->delete_group(core.limit_xyz);
			core.limit_xyz = 0;
		}
		if(core.limit_link) {
			pduel->delete_group(core.limit_link);
			core.limit_link = 0;
		}
		effect* peffect = core.units.begin()->peffect;
		uint8 targetplayer = sumplayer;
		uint8 positions = POS_FACEUP;
		if(peffect->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
			positions = (uint8)peffect->s_range;
			if(peffect->o_range == 0)
				targetplayer = sumplayer;
			else
				targetplayer = 1 - sumplayer;
		}
		if(positions == 0)
			positions = POS_FACEUP_ATTACK;
		std::vector<int32> retval;
		peffect->get_value(target, 0, &retval);
		uint32 summon_info = retval.size() > 0 ? retval[0] : 0;
		uint32 zone = retval.size() > 1 ? retval[1] : 0xff;
		target->summon_info = (summon_info & 0xf00ffff) | SUMMON_TYPE_SPECIAL | ((uint32)target->current.location << 16);
		target->enable_field_effect(false);
		move_to_field(target, sumplayer, targetplayer, LOCATION_MZONE, positions, FALSE, 0, FALSE, zone);
		target->current.reason = REASON_SPSUMMON;
		target->current.reason_effect = peffect;
		target->current.reason_player = sumplayer;
		target->summon_player = sumplayer;
		break_effect();
		return FALSE;
	}
	case 5: {
		set_control(target, target->current.controler, 0, 0);
		core.phase_action = TRUE;
		target->current.reason_effect = core.units.begin()->peffect;
		pduel->write_buffer8(MSG_SPSUMMONING);
		pduel->write_buffer32(target->data.code);
		pduel->write_buffer32(target->get_info_location());
		return FALSE;
	}
	case 6: {
		effect* proc = core.units.begin()->peffect;
		int32 matreason = REASON_SPSUMMON;
		if(proc->value == SUMMON_TYPE_SYNCHRO)
			matreason = REASON_SYNCHRO;
		else if(proc->value == SUMMON_TYPE_XYZ)
			matreason = REASON_XYZ;
		else if(proc->value == SUMMON_TYPE_LINK)
			matreason = REASON_LINK;
		if (target->material_cards.size()) {
			for (auto& mcard : target->material_cards)
				raise_single_event(mcard, 0, EVENT_BE_PRE_MATERIAL, proc, matreason, sumplayer, sumplayer, 0);
		}
		raise_event(&target->material_cards, EVENT_BE_PRE_MATERIAL, proc, matreason, sumplayer, sumplayer, 0);
		process_single_event();
		process_instant_event();
		return FALSE;
	}
	case 7: {
		if (core.current_chain.size() == 0) {
			if (target->is_affected_by_effect(EFFECT_CANNOT_DISABLE_SPSUMMON))
				core.units.begin()->step = 14;
			else
				core.units.begin()->step = 9;
			return FALSE;
		} else {
			core.units.begin()->step = 14;
			return FALSE;
		}
		return FALSE;
	}
	case 10: {
		target->set_status(STATUS_SUMMONING, TRUE);
		target->set_status(STATUS_SUMMON_DISABLED, FALSE);
		raise_event(target, EVENT_SPSUMMON, core.units.begin()->peffect, 0, sumplayer, sumplayer, 0);
		process_instant_event();
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0x101, TRUE);
		return FALSE;
	}
	case 11: {
		if(target->is_status(STATUS_SUMMONING)) {
			core.units.begin()->step = 14;
			return FALSE;
		}
		effect* peffect = core.units.begin()->peffect;
		remove_oath_effect(peffect);
		if(peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT) && (peffect->count_code & EFFECT_COUNT_CODE_OATH)) {
			dec_effect_code(peffect->count_code, sumplayer);
		}
		if(effect_set* peset = (effect_set*)core.units.begin()->ptr1) {
			for(int32 i = 0; i < peset->size(); ++i)
				remove_oath_effect(peset->at(i));
			delete peset;
			core.units.begin()->ptr1 = 0;
		}
		if(target->current.location == LOCATION_MZONE)
			send_to(target, 0, REASON_RULE, sumplayer, sumplayer, LOCATION_GRAVE, 0, 0);
		adjust_instant();
		add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, 0);
		return TRUE;
	}
	case 15: {
		release_oath_relation(core.units.begin()->peffect);
		if(effect_set* peset = (effect_set*)core.units.begin()->ptr1) {
			for(int32 i = 0; i < peset->size(); ++i)
				release_oath_relation(peset->at(i));
			delete peset;
			core.units.begin()->ptr1 = 0;
		}
		target->set_status(STATUS_SUMMONING, FALSE);
		target->set_status(STATUS_PROC_COMPLETE | STATUS_SPSUMMON_TURN, TRUE);
		target->enable_field_effect(true);
		if(target->is_status(STATUS_DISABLED))
			target->reset(RESET_DISABLE, RESET_EVENT);
		return FALSE;
	}
	case 16: {
		pduel->write_buffer8(MSG_SPSUMMONED);
		adjust_instant();
		effect* proc = core.units.begin()->peffect;
		int32 matreason = REASON_SPSUMMON;
		if(proc->value == SUMMON_TYPE_SYNCHRO)
			matreason = REASON_SYNCHRO;
		else if(proc->value == SUMMON_TYPE_XYZ)
			matreason = REASON_XYZ;
		else if(proc->value == SUMMON_TYPE_LINK)
			matreason = REASON_LINK;
		if(target->material_cards.size()) {
			for(auto& mcard : target->material_cards)
				raise_single_event(mcard, 0, EVENT_BE_MATERIAL, proc, matreason, sumplayer, sumplayer, 0);
		}
		raise_event(&target->material_cards, EVENT_BE_MATERIAL, proc, matreason, sumplayer, sumplayer, 0);
		process_single_event();
		process_instant_event();
		return FALSE;
	}
	case 17: {
		set_spsummon_counter(sumplayer);
		check_card_counter(target, ACTIVITY_SPSUMMON, sumplayer);
		if(target->spsummon_code)
			core.spsummon_once_map[sumplayer][target->spsummon_code]++;
		raise_single_event(target, 0, EVENT_SPSUMMON_SUCCESS, core.units.begin()->peffect, 0, sumplayer, sumplayer, 0);
		process_single_event();
		raise_event(target, EVENT_SPSUMMON_SUCCESS, core.units.begin()->peffect, 0, sumplayer, sumplayer, 0);
		process_instant_event();
		if(core.current_chain.size() == 0) {
			adjust_all();
			core.hint_timing[sumplayer] |= TIMING_SPSUMMON;
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, 0);
		}
		return TRUE;
	}
	case 20: {
		effect* peffect = core.units.begin()->peffect;
		core.units.begin()->ptarget = pduel->new_group();
		if(peffect->operation) {
			core.sub_solving_event.push_back(nil_event);
			pduel->lua->add_param(target, PARAM_TYPE_CARD);
			pduel->lua->add_param(core.units.begin()->ptarget, PARAM_TYPE_GROUP);
			add_process(PROCESSOR_EXECUTE_OPERATION, 0, peffect, 0, sumplayer, 0);
		}
		peffect->dec_count(sumplayer);
		return FALSE;
	}
	case 21: {
		group* pgroup = core.units.begin()->ptarget;
		for(auto cit = pgroup->container.begin(); cit != pgroup->container.end(); ) {
			card* pcard = *cit++;
			if(!(pcard->data.type & TYPE_MONSTER)
			        || (pcard->current.location == LOCATION_MZONE)
			        || check_unique_onfield(pcard, sumplayer, LOCATION_MZONE)
			        || pcard->is_affected_by_effect(EFFECT_CANNOT_SPECIAL_SUMMON)) {
			    pgroup->container.erase(pcard);
			    continue;
			}
			effect_set eset;
			pcard->filter_effect(EFFECT_SPSUMMON_COST, &eset);
			if(eset.size()) {
				for(int32 i = 0; i < eset.size(); ++i) {
					if(eset[i]->operation) {
						core.sub_solving_event.push_back(nil_event);
						add_process(PROCESSOR_EXECUTE_OPERATION, 0, eset[i], 0, sumplayer, 0);
					}
				}
				effect_set* peset = new effect_set;
				*peset = std::move(eset);
				core.units.begin()->ptr1 = peset;
			}
		}
		return FALSE;
	}
	case 22: {
		group* pgroup = core.units.begin()->ptarget;
		if(pgroup->container.size() == 0)
			return TRUE;
		core.phase_action = TRUE;
		pgroup->it = pgroup->container.begin();
		return FALSE;
	}
	case 23: {
		effect* peffect = core.units.begin()->peffect;
		group* pgroup = core.units.begin()->ptarget;
		card* pcard = *pgroup->it;
		pcard->enable_field_effect(false);
		pcard->current.reason = REASON_SPSUMMON;
		pcard->current.reason_effect = peffect;
		pcard->current.reason_player = sumplayer;
		pcard->summon_player = sumplayer;
		pcard->summon_info = (peffect->get_value(pcard) & 0xff00ffff) | SUMMON_TYPE_SPECIAL | ((uint32)pcard->current.location << 16);
		uint32 zone = 0xff;
		uint32 flag1, flag2;
		int32 ct1 = get_tofield_count(pcard, sumplayer, LOCATION_MZONE, sumplayer, LOCATION_REASON_TOFIELD, zone, &flag1);
		int32 ct2 = get_spsummonable_count_fromex(pcard, sumplayer, sumplayer, zone, &flag2);
		for(auto it = pgroup->it; it != pgroup->container.end(); ++it) {
			if((*it)->current.location != LOCATION_EXTRA)
				ct1--;
			else
				ct2--;
		}
		if(pcard->current.location != LOCATION_EXTRA) {
			if(ct2 == 0)
				zone = flag2;
		} else {
			if(ct1 == 0)
				zone = flag1;
		}
		move_to_field(pcard, sumplayer, sumplayer, LOCATION_MZONE, POS_FACEUP, FALSE, 0, FALSE, zone);
		return FALSE;
	}
	case 24: {
		group* pgroup = core.units.begin()->ptarget;
		card* pcard = *pgroup->it++;
		pduel->write_buffer8(MSG_SPSUMMONING);
		pduel->write_buffer32(pcard->data.code);
		pduel->write_buffer8(pcard->current.controler);
		pduel->write_buffer8(pcard->current.location);
		pduel->write_buffer8(pcard->current.sequence);
		pduel->write_buffer8(pcard->current.position);
		set_control(pcard, pcard->current.controler, 0, 0);
		if(pgroup->it != pgroup->container.end())
			core.units.begin()->step = 22;
		return FALSE;
	}
	case 25: {
		group* pgroup = core.units.begin()->ptarget;
		card_set cset;
		for(auto& pcard : pgroup->container) {
			pcard->set_status(STATUS_SUMMONING, TRUE);
			if(!pcard->is_affected_by_effect(EFFECT_CANNOT_DISABLE_SPSUMMON)) {
				cset.insert(pcard);
			}
		}
		if(cset.size()) {
			raise_event(&cset, EVENT_SPSUMMON, core.units.begin()->peffect, 0, sumplayer, sumplayer, 0);
			process_instant_event();
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, 0x101, TRUE);
		}
		return FALSE;
	}
	case 26: {
		group* pgroup = core.units.begin()->ptarget;
		card_set cset;
		for(auto cit = pgroup->container.begin(); cit != pgroup->container.end(); ) {
			card* pcard = *cit++;
			if(!pcard->is_status(STATUS_SUMMONING)) {
				pgroup->container.erase(pcard);
				if(pcard->current.location == LOCATION_MZONE)
					cset.insert(pcard);
			}
		}
		if(cset.size()) {
			send_to(&cset, 0, REASON_RULE, sumplayer, sumplayer, LOCATION_GRAVE, 0, 0);
			adjust_instant();
		}
		if(pgroup->container.size() == 0) {
			effect* peffect = core.units.begin()->peffect;
			remove_oath_effect(peffect);
			if(peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT) && (peffect->count_code & EFFECT_COUNT_CODE_OATH)) {
				dec_effect_code(peffect->count_code, sumplayer);
			}
			if(effect_set* peset = (effect_set*)core.units.begin()->ptr1) {
				for(int32 i = 0; i < peset->size(); ++i)
					remove_oath_effect(peset->at(i));
				delete peset;
				core.units.begin()->ptr1 = 0;
			}
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, 0);
			return TRUE;
		}
		return FALSE;
	}
	case 27: {
		group* pgroup = core.units.begin()->ptarget;
		release_oath_relation(core.units.begin()->peffect);
		if(effect_set* peset = (effect_set*)core.units.begin()->ptr1) {
			for(int32 i = 0; i < peset->size(); ++i)
				release_oath_relation(peset->at(i));
			delete peset;
			core.units.begin()->ptr1 = 0;
		}
		for(auto& pcard : pgroup->container) {
			pcard->set_status(STATUS_SUMMONING, FALSE);
			pcard->set_status(STATUS_SPSUMMON_TURN, TRUE);
			pcard->enable_field_effect(true);
			if(pcard->is_status(STATUS_DISABLED))
				pcard->reset(RESET_DISABLE, RESET_EVENT);
		}
		return FALSE;
	}
	case 28: {
		group* pgroup = core.units.begin()->ptarget;
		pduel->write_buffer8(MSG_SPSUMMONED);
		set_spsummon_counter(sumplayer);
		check_card_counter(pgroup, ACTIVITY_SPSUMMON, sumplayer);
		std::set<uint32> spsummon_once_set;
		for(auto& pcard : pgroup->container) {
			if(pcard->spsummon_code)
				spsummon_once_set.insert(pcard->spsummon_code);
		}
		for(auto& cit : spsummon_once_set)
			core.spsummon_once_map[sumplayer][cit]++;
		for(auto& pcard : pgroup->container)
			raise_single_event(pcard, 0, EVENT_SPSUMMON_SUCCESS, pcard->current.reason_effect, 0, pcard->current.reason_player, pcard->summon_player, 0);
		process_single_event();
		raise_event(&pgroup->container, EVENT_SPSUMMON_SUCCESS, core.units.begin()->peffect, 0, sumplayer, sumplayer, 0);
		process_instant_event();
		if(core.current_chain.size() == 0) {
			adjust_all();
			core.hint_timing[sumplayer] |= TIMING_SPSUMMON;
			add_process(PROCESSOR_POINT_EVENT, 0, 0, 0, FALSE, 0);
		}
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::special_summon_step(uint16 step, group* targets, card* target, uint32 zone) {
	uint8 playerid = (target->spsummon_param >> 24) & 0xf;
	uint8 nocheck = (target->spsummon_param >> 16) & 0xff;
	uint8 nolimit = (target->spsummon_param >> 8) & 0xff;
	uint8 positions = target->spsummon_param & 0xff;
	switch(step) {
	case 0: {
		effect_set eset;
		if(target->is_affected_by_effect(EFFECT_REVIVE_LIMIT) && !target->is_status(STATUS_PROC_COMPLETE)) {
			if((!nolimit && (target->current.location & 0x38)) || (!nocheck && !nolimit && (target->current.location & 0x3))) {
				core.units.begin()->step = 4;
				return FALSE;
			}
		}
		if((target->current.location == LOCATION_MZONE)
				|| !(positions & POS_FACEDOWN) && check_unique_onfield(target, playerid, LOCATION_MZONE)
		        || !is_player_can_spsummon(core.reason_effect, target->summon_info & 0xff00ffff, positions, target->summon_player, playerid, target)
		        || (!nocheck && !(target->data.type & TYPE_MONSTER))) {
			core.units.begin()->step = 4;
			return FALSE;
		}
		if(get_useable_count(target, playerid, LOCATION_MZONE, target->summon_player, LOCATION_REASON_TOFIELD, zone) <= 0) {
			if(target->current.location != LOCATION_GRAVE)
				core.ss_tograve_set.insert(target);
			core.units.begin()->step = 4;
			return FALSE;
		}
		if(!nocheck) {
			target->filter_effect(EFFECT_SPSUMMON_CONDITION, &eset);
			for(int32 i = 0; i < eset.size(); ++i) {
				pduel->lua->add_param(core.reason_effect, PARAM_TYPE_EFFECT);
				pduel->lua->add_param(target->summon_player, PARAM_TYPE_INT);
				pduel->lua->add_param(target->summon_info & 0xff00ffff, PARAM_TYPE_INT);
				pduel->lua->add_param(positions, PARAM_TYPE_INT);
				pduel->lua->add_param(playerid, PARAM_TYPE_INT);
				if(!eset[i]->check_value_condition(5)) {
					core.units.begin()->step = 4;
					return FALSE;
				}
			}
		}
		eset.clear();
		target->filter_effect(EFFECT_SPSUMMON_COST, &eset);
		if(eset.size()) {
			for(int32 i = 0; i < eset.size(); ++i) {
				if(eset[i]->operation) {
					core.sub_solving_event.push_back(nil_event);
					add_process(PROCESSOR_EXECUTE_OPERATION, 0, eset[i], 0, target->summon_player, 0);
				}
			}
			effect_set* peset = new effect_set;
			*peset = std::move(eset);
			core.units.begin()->ptr2 = peset;
		}
		return FALSE;
	}
	case 1: {
		if(effect_set* peset = (effect_set*)core.units.begin()->ptr2) {
			for(int32 i = 0; i < peset->size(); ++i)
				release_oath_relation(peset->at(i));
			delete peset;
			core.units.begin()->ptr2 = 0;
		}
		if(!targets)
			core.special_summoning.insert(target);
		target->enable_field_effect(false);
		if(targets && core.duel_rule >= 4) {
			uint32 flag1, flag2;
			int32 ct1 = get_tofield_count(target, playerid, LOCATION_MZONE, target->summon_player, LOCATION_REASON_TOFIELD, zone, &flag1);
			int32 ct2 = get_spsummonable_count_fromex(target, playerid, target->summon_player, zone, &flag2);
			for(auto& pcard : targets->container) {
				if(pcard->current.location == LOCATION_MZONE)
					continue;
				if(pcard->current.location != LOCATION_EXTRA)
					ct1--;
				else
					ct2--;
			}
			if(target->current.location != LOCATION_EXTRA) {
				if(ct2 == 0)
					zone &= flag2;
			} else {
				if(ct1 == 0)
					zone &= flag1;
			}
		}
		move_to_field(target, target->summon_player, playerid, LOCATION_MZONE, positions, FALSE, 0, FALSE, zone);
		return FALSE;
	}
	case 2: {
		pduel->write_buffer8(MSG_SPSUMMONING);
		pduel->write_buffer32(target->data.code);
		pduel->write_buffer32(target->get_info_location());
		return FALSE;
	}
	case 3: {
		returns.ivalue[0] = TRUE;
		set_control(target, target->current.controler, 0, 0);
		target->set_status(STATUS_SPSUMMON_STEP, TRUE);
		return TRUE;
	}
	case 5: {
		returns.ivalue[0] = FALSE;
		target->current.reason = target->temp.reason;
		target->current.reason_effect = target->temp.reason_effect;
		target->current.reason_player = target->temp.reason_player;
		if(targets)
			targets->container.erase(target);
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::special_summon(uint16 step, effect* reason_effect, uint8 reason_player, group* targets, uint32 zone) {
	switch(step) {
	case 0: {
		card_vector cvs, cvo;
		for(auto& pcard : targets->container) {
			if(pcard->summon_player == infos.turn_player)
				cvs.push_back(pcard);
			else
				cvo.push_back(pcard);
		}
		if(!cvs.empty()) {
			if(cvs.size() > 1)
				std::sort(cvs.begin(), cvs.end(), card::card_operation_sort);
			core.hint_timing[infos.turn_player] |= TIMING_SPSUMMON;
			for(auto& pcard : cvs)
				add_process(PROCESSOR_SPSUMMON_STEP, 0, NULL, targets, zone, 0, 0, 0, pcard);
		}
		if(!cvo.empty()) {
			if(cvo.size() > 1)
				std::sort(cvo.begin(), cvo.end(), card::card_operation_sort);
			core.hint_timing[1 - infos.turn_player] |= TIMING_SPSUMMON;
			for(auto& pcard : cvo)
				add_process(PROCESSOR_SPSUMMON_STEP, 0, NULL, targets, zone, 0, 0, 0, pcard);
		}
		return FALSE;
	}
	case 1: {
		if(core.ss_tograve_set.size())
			send_to(&core.ss_tograve_set, 0, REASON_RULE, PLAYER_NONE, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
		return FALSE;
	}
	case 2: {
		core.ss_tograve_set.clear();
		if(targets->container.size() == 0) {
			returns.ivalue[0] = 0;
			core.operated_set.clear();
			pduel->delete_group(targets);
			return TRUE;
		}
		bool tp = false, ntp = false;
		std::set<uint32> spsummon_once_set[2];
		for(auto& pcard : targets->container) {
			if(pcard->summon_player == infos.turn_player)
				tp = true;
			else
				ntp = true;
			if(pcard->spsummon_code)
				spsummon_once_set[pcard->summon_player].insert(pcard->spsummon_code);
		}
		if(tp)
			set_spsummon_counter(infos.turn_player);
		if(ntp)
			set_spsummon_counter(1 - infos.turn_player);
		for(auto& cit : spsummon_once_set[0])
			core.spsummon_once_map[0][cit]++;
		for(auto& cit : spsummon_once_set[1])
			core.spsummon_once_map[1][cit]++;
		for(auto& pcard : targets->container) {
			pcard->set_status(STATUS_SPSUMMON_STEP, FALSE);
			pcard->set_status(STATUS_SPSUMMON_TURN, TRUE);
			if(pcard->is_position(POS_FACEUP))
				pcard->enable_field_effect(true);
		}
		adjust_instant();
		return FALSE;
	}
	case 3: {
		pduel->write_buffer8(MSG_SPSUMMONED);
		for(auto& pcard : targets->container) {
			check_card_counter(pcard, ACTIVITY_SPSUMMON, pcard->summon_player);
			if(!(pcard->current.position & POS_FACEDOWN))
				raise_single_event(pcard, 0, EVENT_SPSUMMON_SUCCESS, pcard->current.reason_effect, 0, pcard->current.reason_player, pcard->summon_player, 0);
			int32 summontype = pcard->summon_info & 0xff000000;
			if(summontype && pcard->material_cards.size() && !pcard->is_status(STATUS_FUTURE_FUSION)) {
				int32 matreason = 0;
				if(summontype == SUMMON_TYPE_FUSION)
					matreason = REASON_FUSION;
				else if(summontype == SUMMON_TYPE_RITUAL)
					matreason = REASON_RITUAL;
				else if(summontype == SUMMON_TYPE_XYZ)
					matreason = REASON_XYZ;
				else if(summontype == SUMMON_TYPE_LINK)
					matreason = REASON_LINK;
				for(auto& mcard : pcard->material_cards)
					raise_single_event(mcard, &targets->container, EVENT_BE_MATERIAL, pcard->current.reason_effect, matreason, pcard->current.reason_player, pcard->summon_player, 0);
				raise_event(&(pcard->material_cards), EVENT_BE_MATERIAL, reason_effect, matreason, reason_player, pcard->summon_player, 0);
			}
			pcard->set_status(STATUS_FUTURE_FUSION, FALSE);
		}
		process_single_event();
		process_instant_event();
		return FALSE;
	}
	case 4: {
		raise_event(&targets->container, EVENT_SPSUMMON_SUCCESS, reason_effect, 0, reason_player, PLAYER_NONE, 0);
		process_instant_event();
		return FALSE;
	}
	case 5: {
		core.operated_set.clear();
		core.operated_set = targets->container;
		returns.ivalue[0] = (int32)targets->container.size();
		pduel->delete_group(targets);
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::destroy_replace(uint16 step, group* targets, card* target, uint8 battle) {
	if(target->current.location & (LOCATION_GRAVE | LOCATION_REMOVED)) {
		target->current.reason = target->temp.reason;
		target->current.reason_effect = target->temp.reason_effect;
		target->current.reason_player = target->temp.reason_player;
		target->set_status(STATUS_DESTROY_CONFIRMED, FALSE);
		targets->container.erase(target);
		return TRUE;
	}
	if(targets->container.find(target) == targets->container.end())
		return TRUE;
	returns.ivalue[0] = FALSE;
	effect_set eset;
	target->filter_single_continuous_effect(EFFECT_DESTROY_REPLACE, &eset);
	if(!battle) {
		for (int32 i = 0; i < eset.size(); ++i)
			add_process(PROCESSOR_OPERATION_REPLACE, 0, eset[i], targets, 1, 0, 0, 0, target);
	} else {
		for (int32 i = 0; i < eset.size(); ++i)
			add_process(PROCESSOR_OPERATION_REPLACE, 10, eset[i], targets, 1, 0, 0, 0, target);
	}
	return TRUE;
}
int32 field::destroy(uint16 step, group * targets, effect * reason_effect, uint32 reason, uint8 reason_player) {
	switch (step) {
	case 0: {
		card_set extra;
		effect_set eset;
		card_set indestructable_set;
		std::set<effect*> indestructable_effect_set;
		for (auto cit = targets->container.begin(); cit != targets->container.end();) {
			auto rm = cit++;
			card* pcard = *rm;
			if(!pcard->is_destructable()) {
				indestructable_set.insert(pcard);
				continue;
			}
			if (!(pcard->current.reason & (REASON_RULE | REASON_COST))) {
				bool is_destructable = true;
				if (pcard->is_affect_by_effect(pcard->current.reason_effect)) {
					effect* indestructable_effect = pcard->check_indestructable_by_effect(pcard->current.reason_effect, pcard->current.reason_player);
					if (indestructable_effect) {
						if(reason_player != PLAYER_SELFDES)
							indestructable_effect_set.insert(indestructable_effect);
						is_destructable = false;
					}
				} else
					is_destructable = false;
				if (!is_destructable) {
					indestructable_set.insert(pcard);
					continue;
				}
			}
			eset.clear();
			pcard->filter_effect(EFFECT_INDESTRUCTABLE, &eset);
			if(eset.size()) {
				bool is_destructable = true;
				for(int32 i = 0; i < eset.size(); ++i) {
					pduel->lua->add_param(pcard->current.reason_effect, PARAM_TYPE_EFFECT);
					pduel->lua->add_param(pcard->current.reason, PARAM_TYPE_INT);
					pduel->lua->add_param(pcard->current.reason_player, PARAM_TYPE_INT);
					if(eset[i]->check_value_condition(3)) {
						if(reason_player != PLAYER_SELFDES)
							indestructable_effect_set.insert(eset[i]);
						is_destructable = false;
						break;
					}
				}
				if(!is_destructable) {
					indestructable_set.insert(pcard);
					continue;
				}
			}
			eset.clear();
			pcard->filter_effect(EFFECT_INDESTRUCTABLE_COUNT, &eset);
			if (eset.size()) {
				bool is_destructable = true;
				for(int32 i = 0; i < eset.size(); ++i) {
					if(eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
						if(eset[i]->count_limit == 0)
							continue;
						pduel->lua->add_param(pcard->current.reason_effect, PARAM_TYPE_EFFECT);
						pduel->lua->add_param(pcard->current.reason, PARAM_TYPE_INT);
						pduel->lua->add_param(pcard->current.reason_player, PARAM_TYPE_INT);
						if(eset[i]->check_value_condition(3)) {
							indestructable_effect_set.insert(eset[i]);
							is_destructable = false;
						}
					} else {
						pduel->lua->add_param(pcard->current.reason_effect, PARAM_TYPE_EFFECT);
						pduel->lua->add_param(pcard->current.reason, PARAM_TYPE_INT);
						pduel->lua->add_param(pcard->current.reason_player, PARAM_TYPE_INT);
						int32 ct;
						if(ct = eset[i]->get_value(3)) {
							auto it = pcard->indestructable_effects.emplace(eset[i]->id, 0);
							if(++it.first->second <= ct) {
								indestructable_effect_set.insert(eset[i]);
								is_destructable = false;
							}
						}
					}
				}
				if(!is_destructable) {
					indestructable_set.insert(pcard);
					continue;
				}
			}
			eset.clear();
			pcard->filter_effect(EFFECT_DESTROY_SUBSTITUTE, &eset);
			if (eset.size()) {
				bool sub = false;
				for (int32 i = 0; i < eset.size(); ++i) {
					pduel->lua->add_param(pcard->current.reason_effect, PARAM_TYPE_EFFECT);
					pduel->lua->add_param(pcard->current.reason, PARAM_TYPE_INT);
					pduel->lua->add_param(pcard->current.reason_player, PARAM_TYPE_INT);
					if(eset[i]->check_value_condition(3)) {
						extra.insert(eset[i]->handler);
						sub = true;
					}
				}
				if(sub) {
					pcard->current.reason = pcard->temp.reason;
					pcard->current.reason_effect = pcard->temp.reason_effect;
					pcard->current.reason_player = pcard->temp.reason_player;
					core.destroy_canceled.insert(pcard);
					targets->container.erase(pcard);
				}
			}
		}
		for (auto& pcard : indestructable_set) {
			pcard->current.reason = pcard->temp.reason;
			pcard->current.reason_effect = pcard->temp.reason_effect;
			pcard->current.reason_player = pcard->temp.reason_player;
			pcard->set_status(STATUS_DESTROY_CONFIRMED, FALSE);
			targets->container.erase(pcard);
		}
		for (auto& rep : extra) {
			if(targets->container.count(rep) == 0) {
				rep->temp.reason = rep->current.reason;
				rep->temp.reason_effect = rep->current.reason_effect;
				rep->temp.reason_player = rep->current.reason_player;
				rep->current.reason = REASON_EFFECT | REASON_DESTROY | REASON_REPLACE;
				rep->current.reason_effect = 0;
				rep->current.reason_player = rep->current.controler;
				rep->sendto_param.set(rep->owner, POS_FACEUP, LOCATION_GRAVE);
				targets->container.insert(rep);
			}
		}
		for (auto& peffect : indestructable_effect_set) {
			peffect->dec_count();
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_CARD);
			pduel->write_buffer8(0);
			pduel->write_buffer32(peffect->owner->data.code);
		}
		operation_replace(EFFECT_DESTROY_REPLACE, 5, targets);
		return FALSE;
	}
	case 1: {
		for (auto& pcard : targets->container) {
			add_process(PROCESSOR_DESTROY_REPLACE, 0, NULL, targets, 0, 0, 0, 0, pcard);
		}
		return FALSE;
	}
	case 2: {
		for (auto& pcard : core.destroy_canceled)
			pcard->set_status(STATUS_DESTROY_CONFIRMED, FALSE);
		core.destroy_canceled.clear();
		return FALSE;
	}
	case 3: {
		if(!targets->container.size()) {
			returns.ivalue[0] = 0;
			core.operated_set.clear();
			pduel->delete_group(targets);
			return TRUE;
		}
		card_vector cv(targets->container.begin(), targets->container.end());
		if(cv.size() > 1)
			std::sort(cv.begin(), cv.end(), card::card_operation_sort);
		for (auto& pcard : cv) {
			if(pcard->current.location & (LOCATION_GRAVE | LOCATION_REMOVED)) {
				pcard->current.reason = pcard->temp.reason;
				pcard->current.reason_effect = pcard->temp.reason_effect;
				pcard->current.reason_player = pcard->temp.reason_player;
				targets->container.erase(pcard);
				continue;
			}
			pcard->current.reason |= REASON_DESTROY;
			core.hint_timing[pcard->current.controler] |= TIMING_DESTROY;
			raise_single_event(pcard, 0, EVENT_DESTROY, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
		}
		adjust_instant();
		process_single_event();
		raise_event(&targets->container, EVENT_DESTROY, reason_effect, reason, reason_player, 0, 0);
		process_instant_event();
		return FALSE;
	}
	case 4: {
		group* sendtargets = pduel->new_group(targets->container);
		sendtargets->is_readonly = TRUE;
		for(auto& pcard : sendtargets->container) {
			pcard->set_status(STATUS_DESTROY_CONFIRMED, FALSE);
			uint32 dest = pcard->sendto_param.location;
			if(!dest)
				dest = LOCATION_GRAVE;
			if((dest == LOCATION_HAND && !pcard->is_capable_send_to_hand(reason_player))
			        || (dest == LOCATION_DECK && !pcard->is_capable_send_to_deck(reason_player))
			        || (dest == LOCATION_REMOVED && !pcard->is_removeable(reason_player, pcard->sendto_param.position, reason)))
				dest = LOCATION_GRAVE;
			pcard->sendto_param.location = dest;
		}
		operation_replace(EFFECT_SEND_REPLACE, 5, sendtargets);
		add_process(PROCESSOR_SENDTO, 1, reason_effect, sendtargets, reason | REASON_DESTROY, reason_player);
		return FALSE;
	}
	case 5: {
		core.operated_set.clear();
		core.operated_set = targets->container;
		for(auto cit = core.operated_set.begin(); cit != core.operated_set.end();) {
			if((*cit)->current.reason & REASON_REPLACE)
				core.operated_set.erase(cit++);
			else
				cit++;
		}
		returns.ivalue[0] = (int32)core.operated_set.size();
		pduel->delete_group(targets);
		return TRUE;
	}
	case 10: {
		effect_set eset;
		for (auto cit = targets->container.begin(); cit != targets->container.end();) {
			auto rm = cit++;
			card* pcard = *rm;
			if (!pcard->is_destructable()) {
				pcard->current.reason = pcard->temp.reason;
				pcard->current.reason_effect = pcard->temp.reason_effect;
				pcard->current.reason_player = pcard->temp.reason_player;
				pcard->set_status(STATUS_DESTROY_CONFIRMED, FALSE);
				targets->container.erase(pcard);
				continue;
			}
			eset.clear();
			pcard->filter_effect(EFFECT_INDESTRUCTABLE, &eset);
			if(eset.size()) {
				bool indes = false;
				for(int32 i = 0; i < eset.size(); ++i) {
					pduel->lua->add_param(pcard->current.reason_effect, PARAM_TYPE_EFFECT);
					pduel->lua->add_param(pcard->current.reason, PARAM_TYPE_INT);
					pduel->lua->add_param(pcard->current.reason_player, PARAM_TYPE_INT);
					if(eset[i]->check_value_condition(3)) {
						pduel->write_buffer8(MSG_HINT);
						pduel->write_buffer8(HINT_CARD);
						pduel->write_buffer8(0);
						pduel->write_buffer32(eset[i]->owner->data.code);
						indes = true;
						break;
					}
				}
				if(indes) {
					pcard->current.reason = pcard->temp.reason;
					pcard->current.reason_effect = pcard->temp.reason_effect;
					pcard->current.reason_player = pcard->temp.reason_player;
					pcard->set_status(STATUS_DESTROY_CONFIRMED, FALSE);
					targets->container.erase(pcard);
					continue;
				}
			}
			eset.clear();
			pcard->filter_effect(EFFECT_INDESTRUCTABLE_COUNT, &eset);
			if (eset.size()) {
				bool indes = false;
				for (int32 i = 0; i < eset.size(); ++i) {
					if(eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
						if(eset[i]->count_limit == 0)
							continue;
						pduel->lua->add_param(pcard->current.reason_effect, PARAM_TYPE_EFFECT);
						pduel->lua->add_param(pcard->current.reason, PARAM_TYPE_INT);
						pduel->lua->add_param(pcard->current.reason_player, PARAM_TYPE_INT);
						if(eset[i]->check_value_condition(3)) {
							eset[i]->dec_count();
							pduel->write_buffer8(MSG_HINT);
							pduel->write_buffer8(HINT_CARD);
							pduel->write_buffer8(0);
							pduel->write_buffer32(eset[i]->owner->data.code);
							indes = true;
						}
					} else {
						pduel->lua->add_param(pcard->current.reason_effect, PARAM_TYPE_EFFECT);
						pduel->lua->add_param(pcard->current.reason, PARAM_TYPE_INT);
						pduel->lua->add_param(pcard->current.reason_player, PARAM_TYPE_INT);
						int32 ct;
						if(ct = eset[i]->get_value(3)) {
							auto it = pcard->indestructable_effects.emplace(eset[i]->id, 0);
							if(++it.first->second <= ct) {
								pduel->write_buffer8(MSG_HINT);
								pduel->write_buffer8(HINT_CARD);
								pduel->write_buffer8(0);
								pduel->write_buffer32(eset[i]->owner->data.code);
								indes = true;
							}
						}
					}
				}
				if(indes) {
					pcard->current.reason = pcard->temp.reason;
					pcard->current.reason_effect = pcard->temp.reason_effect;
					pcard->current.reason_player = pcard->temp.reason_player;
					pcard->set_status(STATUS_DESTROY_CONFIRMED, FALSE);
					targets->container.erase(pcard);
					continue;
				}
			}
			eset.clear();
			pcard->filter_effect(EFFECT_DESTROY_SUBSTITUTE, &eset);
			if (eset.size()) {
				bool sub = false;
				for (int32 i = 0; i < eset.size(); ++i) {
					pduel->lua->add_param(pcard->current.reason_effect, PARAM_TYPE_EFFECT);
					pduel->lua->add_param(pcard->current.reason, PARAM_TYPE_INT);
					pduel->lua->add_param(pcard->current.reason_player, PARAM_TYPE_INT);
					if(eset[i]->check_value_condition(3)) {
						core.battle_destroy_rep.insert(eset[i]->handler);
						sub = true;
					}
				}
				if(sub) {
					pcard->current.reason = pcard->temp.reason;
					pcard->current.reason_effect = pcard->temp.reason_effect;
					pcard->current.reason_player = pcard->temp.reason_player;
					core.destroy_canceled.insert(pcard);
					targets->container.erase(pcard);
				}
			}
		}
		if(targets->container.size()) {
			operation_replace(EFFECT_DESTROY_REPLACE, 12, targets);
		}
		return FALSE;
	}
	case 11: {
		for (auto& pcard : targets->container) {
			add_process(PROCESSOR_DESTROY_REPLACE, 0, NULL, targets, 0, TRUE, 0, 0, pcard);
		}
		return FALSE;
	}
	case 12: {
		for (auto& pcard : core.destroy_canceled)
			pcard->set_status(STATUS_DESTROY_CONFIRMED, FALSE);
		core.destroy_canceled.clear();
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::release_replace(uint16 step, group* targets, card* target) {
	if(!(target->current.location & (LOCATION_ONFIELD | LOCATION_HAND))) {
		target->current.reason = target->temp.reason;
		target->current.reason_effect = target->temp.reason_effect;
		target->current.reason_player = target->temp.reason_player;
		targets->container.erase(target);
		return TRUE;
	}
	if(targets->container.find(target) == targets->container.end())
		return TRUE;
	if(!(target->current.reason & REASON_RULE)) {
		returns.ivalue[0] = FALSE;
		effect_set eset;
		target->filter_single_continuous_effect(EFFECT_RELEASE_REPLACE, &eset);
		for (int32 i = 0; i < eset.size(); ++i)
			add_process(PROCESSOR_OPERATION_REPLACE, 0, eset[i], targets, 0, 0, 0, 0, target);
	}
	return TRUE;
}
int32 field::release(uint16 step, group * targets, effect * reason_effect, uint32 reason, uint8 reason_player) {
	switch (step) {
	case 0: {
		for (auto cit = targets->container.begin(); cit != targets->container.end();) {
			auto rm = cit++;
			card* pcard = *rm;
			if (pcard->get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP)
			        || ((reason & REASON_SUMMON) && !pcard->is_releasable_by_summon(reason_player, pcard->current.reason_card))
			        || (!(pcard->current.reason & (REASON_RULE | REASON_SUMMON | REASON_COST))
			            && (!pcard->is_affect_by_effect(pcard->current.reason_effect) || !pcard->is_releasable_by_nonsummon(reason_player)))) {
				pcard->current.reason = pcard->temp.reason;
				pcard->current.reason_effect = pcard->temp.reason_effect;
				pcard->current.reason_player = pcard->temp.reason_player;
				targets->container.erase(rm);
				continue;
			}
		}
		if(reason & REASON_RULE)
			return FALSE;
		operation_replace(EFFECT_RELEASE_REPLACE, 5, targets);
		return FALSE;
	}
	case 1: {
		for (auto& pcard : targets->container) {
			add_process(PROCESSOR_RELEASE_REPLACE, 0, NULL, targets, 0, 0, 0, 0, pcard);
		}
		return FALSE;
	}
	case 2: {
		if(!targets->container.size()) {
			returns.ivalue[0] = 0;
			core.operated_set.clear();
			pduel->delete_group(targets);
			return TRUE;
		}
		card_vector cv(targets->container.begin(), targets->container.end());
		if(cv.size() > 1)
			std::sort(cv.begin(), cv.end(), card::card_operation_sort);
		for (auto& pcard : cv) {
			if(!(pcard->current.location & (LOCATION_ONFIELD | LOCATION_HAND))) {
				pcard->current.reason = pcard->temp.reason;
				pcard->current.reason_effect = pcard->temp.reason_effect;
				pcard->current.reason_player = pcard->temp.reason_player;
				targets->container.erase(pcard);
				continue;
			}
			pcard->current.reason |= REASON_RELEASE;
		}
		adjust_instant();
		return FALSE;
	}
	case 3: {
		group* sendtargets = pduel->new_group(targets->container);
		sendtargets->is_readonly = TRUE;
		operation_replace(EFFECT_SEND_REPLACE, 5, sendtargets);
		add_process(PROCESSOR_SENDTO, 1, reason_effect, sendtargets, reason | REASON_RELEASE, reason_player);
		return FALSE;
	}
	case 4: {
		for(auto& peffect : core.dec_count_reserve)
			peffect->dec_count();
		core.dec_count_reserve.clear();
		core.operated_set.clear();
		core.operated_set = targets->container;
		returns.ivalue[0] = (int32)targets->container.size();
		pduel->delete_group(targets);
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::send_replace(uint16 step, group * targets, card * target) {
	uint8 playerid = target->sendto_param.playerid;
	uint8 dest = target->sendto_param.location;
	if(targets->container.find(target) == targets->container.end())
		return TRUE;
	if(target->current.location == dest && target->current.controler == playerid) {
		target->current.reason = target->temp.reason;
		target->current.reason_effect = target->temp.reason_effect;
		target->current.reason_player = target->temp.reason_player;
		targets->container.erase(target);
		return TRUE;
	}
	if(!(target->current.reason & REASON_RULE)) {
		returns.ivalue[0] = FALSE;
		effect_set eset;
		target->filter_single_continuous_effect(EFFECT_SEND_REPLACE, &eset);
		for (int32 i = 0; i < eset.size(); ++i)
			add_process(PROCESSOR_OPERATION_REPLACE, 0, eset[i], targets, 0, 0, 0, 0, target);
	}
	return TRUE;
}
int32 field::send_to(uint16 step, group * targets, effect * reason_effect, uint32 reason, uint8 reason_player) {
	struct exargs {
		group* targets;
		card_set leave_field, leave_grave, detach;
		bool show_decktop[2];
		card_vector cv;
		card_vector::iterator cvit;
		effect* predirect;

		exargs()
			: targets(nullptr), show_decktop{ FALSE }, predirect(nullptr) {}
	} ;
	switch(step) {
	case 0: {
		for(auto cit = targets->container.begin(); cit != targets->container.end();) {
			auto rm = cit++;
			card* pcard = *rm;
			uint8 dest = pcard->sendto_param.location;
			if(!(reason & REASON_RULE) &&
			        (pcard->get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP)
			         || (!(pcard->current.reason & (REASON_COST | REASON_SUMMON | REASON_MATERIAL)) && !pcard->is_affect_by_effect(pcard->current.reason_effect))
			         || (dest == LOCATION_HAND && !pcard->is_capable_send_to_hand(core.reason_player))
			         || (dest == LOCATION_DECK && !pcard->is_capable_send_to_deck(core.reason_player))
			         || (dest == LOCATION_REMOVED && !pcard->is_removeable(core.reason_player, pcard->sendto_param.position, reason))
			         || (dest == LOCATION_GRAVE && !pcard->is_capable_send_to_grave(core.reason_player))
			         || (dest == LOCATION_EXTRA && !pcard->is_capable_send_to_extra(core.reason_player)))) {
				pcard->current.reason = pcard->temp.reason;
				pcard->current.reason_player = pcard->temp.reason_player;
				pcard->current.reason_effect = pcard->temp.reason_effect;
				targets->container.erase(rm);
				continue;
			}
		}
		if(reason & REASON_RULE)
			return FALSE;
		operation_replace(EFFECT_SEND_REPLACE, 5, targets);
		return FALSE;
	}
	case 1: {
		for(auto& pcard : targets->container) {
			add_process(PROCESSOR_SENDTO_REPLACE, 0, NULL, targets, 0, 0, 0, 0, pcard);
		}
		return FALSE;
	}
	case 2: {
		if(!targets->container.size()) {
			returns.ivalue[0] = 0;
			core.operated_set.clear();
			pduel->delete_group(targets);
			return TRUE;
		}
		card_set leave_p;
		for(auto& pcard : targets->container) {
			if((pcard->current.location & LOCATION_ONFIELD) && !pcard->is_status(STATUS_SUMMON_DISABLED) && !pcard->is_status(STATUS_ACTIVATE_DISABLED)) {
				raise_single_event(pcard, 0, EVENT_LEAVE_FIELD_P, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
				leave_p.insert(pcard);
			}
			if((pcard->current.location & LOCATION_ONFIELD)) {
				if(pcard->current.position & POS_FACEUP) {
					pcard->previous.code = pcard->get_code();
					pcard->previous.code2 = pcard->get_another_code();
					pcard->previous.type = pcard->get_type();
					if(pcard->current.location & LOCATION_MZONE) {
						pcard->previous.level = pcard->get_level();
						pcard->previous.rank = pcard->get_rank();
						pcard->previous.attribute = pcard->get_attribute();
						pcard->previous.race = pcard->get_race();
						pcard->previous.attack = pcard->get_attack();
						pcard->previous.defense = pcard->get_defense();
					}
				} else {
					effect_set eset;
					pcard->filter_effect(EFFECT_ADD_CODE, &eset);
					if(pcard->data.alias && !eset.size())
						pcard->previous.code = pcard->data.alias;
					else
						pcard->previous.code = pcard->data.code;
					if(eset.size())
						pcard->previous.code2 = eset.get_last()->get_value(pcard);
					else
						pcard->previous.code2 = 0;
					pcard->previous.type = pcard->data.type;
					pcard->previous.level = pcard->data.level;
					pcard->previous.rank = pcard->data.level;
					pcard->previous.attribute = pcard->data.attribute;
					pcard->previous.race = pcard->data.race;
					pcard->previous.attack = pcard->data.attack;
					pcard->previous.defense = pcard->data.defense;
				}
				effect_set eset;
				pcard->filter_effect(EFFECT_ADD_SETCODE, &eset);
				if(eset.size())
					pcard->previous.setcode = eset.get_last()->get_value(pcard);
				else
					pcard->previous.setcode = 0;
			}
		}
		if(leave_p.size())
			raise_event(&leave_p, EVENT_LEAVE_FIELD_P, reason_effect, reason, reason_player, 0, 0);
		process_single_event();
		process_instant_event();
		return FALSE;
	}
	case 3: {
		uint32 dest, redirect, redirect_seq, check_cb;
		for(auto& pcard : targets->container)
			pcard->enable_field_effect(false);
		adjust_disable_check_list();
		for(auto& pcard : targets->container) {
			dest = pcard->sendto_param.location;
			redirect = 0;
			redirect_seq = 0;
			check_cb = 0;
			if((dest & LOCATION_GRAVE) && pcard->is_affected_by_effect(EFFECT_TO_GRAVE_REDIRECT_CB))
				check_cb = 1;
			if((pcard->current.location & LOCATION_ONFIELD) && !pcard->is_status(STATUS_SUMMON_DISABLED) && !pcard->is_status(STATUS_ACTIVATE_DISABLED)) {
				redirect = pcard->leave_field_redirect(pcard->current.reason);
				redirect_seq = redirect >> 16;
				redirect &= 0xffff;
			}
			if(redirect) {
				pcard->current.reason &= ~REASON_TEMPORARY;
				pcard->current.reason |= REASON_REDIRECT;
				pcard->sendto_param.location = redirect;
				pcard->sendto_param.sequence = redirect_seq;
				dest = redirect;
				if(dest == LOCATION_REMOVED) {
					if(pcard->sendto_param.position & POS_FACEDOWN_ATTACK)
						pcard->sendto_param.position = (pcard->sendto_param.position & ~POS_FACEDOWN_ATTACK) | POS_FACEUP_ATTACK;
					if(pcard->sendto_param.position & POS_FACEDOWN_DEFENSE)
						pcard->sendto_param.position = (pcard->sendto_param.position & ~POS_FACEDOWN_DEFENSE) | POS_FACEUP_DEFENSE;
				}
			}
			redirect = pcard->destination_redirect(dest, pcard->current.reason);
			if(redirect) {
				redirect_seq = redirect >> 16;
				redirect &= 0xffff;
			}
			if(redirect && (pcard->current.location != redirect)) {
				pcard->current.reason |= REASON_REDIRECT;
				pcard->sendto_param.location = redirect;
				pcard->sendto_param.sequence = redirect_seq;
			}
			if(check_cb)
				pcard->sendto_param.playerid |= 0x1u << 4;
		}
		return FALSE;
	}
	case 4: {
		exargs* param = new exargs;
		core.units.begin()->ptarget = (group*)param;
		param->targets = targets;
		param->show_decktop[0] = false;
		param->show_decktop[1] = false;
		param->cv.assign(targets->container.begin(), targets->container.end());
		if(param->cv.size() > 1)
			std::sort(param->cv.begin(), param->cv.end(), card::card_operation_sort);
		if(core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK) {
			int32 d0 = (int32)player[0].list_main.size() - 1, s0 = d0;
			int32 d1 = (int32)player[1].list_main.size() - 1, s1 = d1;
			for(auto& pcard : param->cv) {
				if(pcard->current.location != LOCATION_DECK)
					continue;
				if((pcard->current.controler == 0) && (pcard->current.sequence == s0))
					s0--;
				if((pcard->current.controler == 1) && (pcard->current.sequence == s1))
					s1--;
			}
			if((s0 != d0) && (s0 > 0)) {
				card* ptop = player[0].list_main[s0];
				if(core.deck_reversed || (ptop->current.position == POS_FACEUP_DEFENSE)) {
					pduel->write_buffer8(MSG_DECK_TOP);
					pduel->write_buffer8(0);
					pduel->write_buffer8(d0 - s0);
					if(ptop->current.position != POS_FACEUP_DEFENSE)
						pduel->write_buffer32(ptop->data.code);
					else
						pduel->write_buffer32(ptop->data.code | 0x80000000);
				}
			}
			if((s1 != d1) && (s1 > 0)) {
				card* ptop = player[1].list_main[s1];
				if(core.deck_reversed || (ptop->current.position == POS_FACEUP_DEFENSE)) {
					pduel->write_buffer8(MSG_DECK_TOP);
					pduel->write_buffer8(1);
					pduel->write_buffer8(d1 - s1);
					if(ptop->current.position != POS_FACEUP_DEFENSE)
						pduel->write_buffer32(ptop->data.code);
					else
						pduel->write_buffer32(ptop->data.code | 0x80000000);
				}
			}
		}
		param->cvit = param->cv.begin();
		return FALSE;
	}
	case 5: {
		exargs* param = (exargs*)targets;
		if(param->cvit == param->cv.end()) {
			core.units.begin()->step = 8;
			return FALSE;
		}
		card* pcard = *param->cvit;
		param->predirect = 0;
		uint32 check_cb = pcard->sendto_param.playerid >> 4;
		if(check_cb)
			param->predirect = pcard->is_affected_by_effect(EFFECT_TO_GRAVE_REDIRECT_CB);
		pcard->enable_field_effect(false);
		if(pcard->data.type & TYPE_TOKEN) {
			pduel->write_buffer8(MSG_MOVE);
			pduel->write_buffer32(pcard->data.code);
			pduel->write_buffer8(pcard->current.controler);
			pduel->write_buffer8(pcard->current.location);
			pduel->write_buffer8(pcard->current.sequence);
			pduel->write_buffer8(pcard->current.position);
			pduel->write_buffer8(0);
			pduel->write_buffer8(0);
			pduel->write_buffer8(0);
			pduel->write_buffer8(0);
			pduel->write_buffer32(pcard->current.reason);
			pcard->previous.controler = pcard->current.controler;
			pcard->previous.location = pcard->current.location;
			pcard->previous.sequence = pcard->current.sequence;
			pcard->previous.position = pcard->current.position;
			pcard->previous.pzone = pcard->current.pzone;
			pcard->current.reason &= ~REASON_TEMPORARY;
			pcard->fieldid = infos.field_id++;
			pcard->fieldid_r = pcard->fieldid;
			pcard->reset(RESET_LEAVE, RESET_EVENT);
			pcard->clear_relate_effect();
			remove_card(pcard);
			param->leave_field.insert(pcard);
			++param->cvit;
			core.units.begin()->step = 4;
			return FALSE;
		}
		if(param->predirect && get_useable_count(pcard, pcard->current.controler, LOCATION_SZONE, pcard->current.controler, LOCATION_REASON_TOFIELD) > 0)
			add_process(PROCESSOR_SELECT_EFFECTYN, 0, 0, (group*)pcard, pcard->current.controler, 97);
		else
			returns.ivalue[0] = 0;
		return FALSE;
	}
	case 6: {
		if(returns.ivalue[0])
			return FALSE;
		exargs* param = (exargs*)targets;
		card* pcard = *param->cvit;
		uint8 oloc = pcard->current.location;
		uint8 playerid = pcard->sendto_param.playerid & 0x7;
		uint8 dest = pcard->sendto_param.location;
		uint8 seq = pcard->sendto_param.sequence;
		uint8 control_player = pcard->overlay_target ? pcard->overlay_target->current.controler : pcard->current.controler;
		if(dest == LOCATION_GRAVE) {
			core.hint_timing[control_player] |= TIMING_TOGRAVE;
		} else if(dest == LOCATION_HAND) {
			pcard->set_status(STATUS_PROC_COMPLETE, FALSE);
			core.hint_timing[control_player] |= TIMING_TOHAND;
		} else if(dest == LOCATION_DECK) {
			pcard->set_status(STATUS_PROC_COMPLETE, FALSE);
			core.hint_timing[control_player] |= TIMING_TODECK;
		} else if(dest == LOCATION_REMOVED) {
			core.hint_timing[control_player] |= TIMING_REMOVE;
		}
		if(pcard->current.controler != playerid || pcard->current.location != dest) {
			pduel->write_buffer8(MSG_MOVE);
			pduel->write_buffer32(pcard->data.code);
			pduel->write_buffer32(pcard->get_info_location());
			if(pcard->overlay_target) {
				param->detach.insert(pcard->overlay_target);
				pcard->overlay_target->xyz_remove(pcard);
			}
			move_card(playerid, pcard, dest, seq);
			pcard->current.position = pcard->sendto_param.position;
			pduel->write_buffer32(pcard->get_info_location());
			pduel->write_buffer32(pcard->current.reason);
		}
		if((core.deck_reversed && pcard->current.location == LOCATION_DECK) || (pcard->current.position == POS_FACEUP_DEFENSE))
			param->show_decktop[pcard->current.controler] = true;
		pcard->set_status(STATUS_LEAVE_CONFIRMED, FALSE);
		if(pcard->status & (STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED)) {
			pcard->set_status(STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED, FALSE);
			pcard->previous.location = 0;
		} else if(oloc & LOCATION_ONFIELD) {
			pcard->reset(RESET_LEAVE, RESET_EVENT);
			param->leave_field.insert(pcard);
		} else if(oloc == LOCATION_GRAVE) {
			param->leave_grave.insert(pcard);
		}
		if(pcard->previous.location == LOCATION_OVERLAY)
			pcard->previous.controler = control_player;
		++param->cvit;
		core.units.begin()->step = 4;
		return FALSE;
	}
	case 7: {
		// crystal beast redirection
		exargs* param = (exargs*)targets;
		card* pcard = *param->cvit;
		uint32 flag;
		get_useable_count(pcard, pcard->current.controler, LOCATION_SZONE, pcard->current.controler, LOCATION_REASON_TOFIELD, 0xff, &flag);
		flag = ((flag << 8) & 0xff00) | 0xffffe0ff;
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(pcard->current.controler);
		pduel->write_buffer32(pcard->data.code);
		add_process(PROCESSOR_SELECT_PLACE, 0, 0, 0, pcard->current.controler, flag, 1);
		return FALSE;
	}
	case 8: {
		exargs* param = (exargs*)targets;
		card* pcard = *param->cvit;
		uint8 oloc = pcard->current.location;
		uint8 seq = returns.bvalue[2];
		pduel->write_buffer8(MSG_MOVE);
		pduel->write_buffer32(pcard->data.code);
		pduel->write_buffer32(pcard->get_info_location());
		if(pcard->overlay_target) {
			param->detach.insert(pcard->overlay_target);
			pcard->overlay_target->xyz_remove(pcard);
		}
		move_card(pcard->current.controler, pcard, LOCATION_SZONE, seq);
		pcard->current.position = POS_FACEUP;
		pduel->write_buffer32(pcard->get_info_location());
		pduel->write_buffer32(pcard->current.reason);
		pcard->set_status(STATUS_LEAVE_CONFIRMED, FALSE);
		if(pcard->status & (STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED)) {
			pcard->set_status(STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED, FALSE);
			pcard->previous.location = 0;
		} else if(oloc & LOCATION_ONFIELD) {
			pcard->reset(RESET_LEAVE + RESET_MSCHANGE, RESET_EVENT);
			pcard->clear_card_target();
			param->leave_field.insert(pcard);
		}
		if(param->predirect->operation) {
			tevent e;
			e.event_cards = targets;
			e.event_player = pcard->current.controler;
			e.event_value = 0;
			e.reason = pcard->current.reason;
			e.reason_effect = reason_effect;
			e.reason_player = pcard->current.controler;
			core.sub_solving_event.push_back(e);
			add_process(PROCESSOR_EXECUTE_OPERATION, 0, param->predirect, 0, pcard->current.controler, 0);
		}
		++param->cvit;
		core.units.begin()->step = 4;
		return FALSE;
	}
	case 9: {
		exargs* param = (exargs*)targets;
		if(core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK) {
			if(param->show_decktop[0]) {
				card* ptop = *player[0].list_main.rbegin();
				pduel->write_buffer8(MSG_DECK_TOP);
				pduel->write_buffer8(0);
				pduel->write_buffer8(0);
				if(ptop->current.position != POS_FACEUP_DEFENSE)
					pduel->write_buffer32(ptop->data.code);
				else
					pduel->write_buffer32(ptop->data.code | 0x80000000);
			}
			if(param->show_decktop[1]) {
				card* ptop = *player[1].list_main.rbegin();
				pduel->write_buffer8(MSG_DECK_TOP);
				pduel->write_buffer8(1);
				pduel->write_buffer8(0);
				if(ptop->current.position != POS_FACEUP_DEFENSE)
					pduel->write_buffer32(ptop->data.code);
				else
					pduel->write_buffer32(ptop->data.code | 0x80000000);
			}
		}
		for(auto& pcard : param->targets->container) {
			if(!(pcard->data.type & TYPE_TOKEN))
				pcard->enable_field_effect(true);
			uint8 nloc = pcard->current.location;
			if(nloc == LOCATION_HAND)
				pcard->reset(RESET_TOHAND, RESET_EVENT);
			if(nloc == LOCATION_DECK || nloc == LOCATION_EXTRA)
				pcard->reset(RESET_TODECK, RESET_EVENT);
			if(nloc == LOCATION_GRAVE)
				pcard->reset(RESET_TOGRAVE, RESET_EVENT);
			if(nloc == LOCATION_REMOVED || ((pcard->data.type & TYPE_TOKEN) && pcard->sendto_param.location == LOCATION_REMOVED)) {
				if(pcard->current.reason & REASON_TEMPORARY)
					pcard->reset(RESET_TEMP_REMOVE, RESET_EVENT);
				else
					pcard->reset(RESET_REMOVE, RESET_EVENT);
			}
			pcard->refresh_disable_status();
		}
		for(auto& pcard : param->leave_field)
			raise_single_event(pcard, 0, EVENT_LEAVE_FIELD, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
		for(auto& pcard : param->leave_grave)
			raise_single_event(pcard, 0, EVENT_LEAVE_GRAVE, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
		if((core.global_flag & GLOBALFLAG_DETACH_EVENT) && param->detach.size()) {
			for(auto& pcard : param->detach) {
				if(pcard->current.location & LOCATION_MZONE)
					raise_single_event(pcard, 0, EVENT_DETACH_MATERIAL, reason_effect, reason, reason_player, 0, 0);
			}
		}
		process_single_event();
		if(param->leave_field.size())
			raise_event(&param->leave_field, EVENT_LEAVE_FIELD, reason_effect, reason, reason_player, 0, 0);
		if(param->leave_grave.size())
			raise_event(&param->leave_grave, EVENT_LEAVE_GRAVE, reason_effect, reason, reason_player, 0, 0);
		if((core.global_flag & GLOBALFLAG_DETACH_EVENT) && param->detach.size())
			raise_event(&param->detach, EVENT_DETACH_MATERIAL, reason_effect, reason, reason_player, 0, 0);
		process_instant_event();
		adjust_instant();
		return FALSE;
	}
	case 10: {
		exargs* param = (exargs*)targets;
		core.units.begin()->ptarget = param->targets;
		targets = param->targets;
		delete param;
		card_set tohand, todeck, tograve, remove, discard, released, destroyed;
		card_set equipings, overlays;
		for(auto& pcard : targets->container) {
			uint8 nloc = pcard->current.location;
			if(pcard->equiping_target)
				pcard->unequip();
			if(pcard->equiping_cards.size()) {
				for(auto csit = pcard->equiping_cards.begin(); csit != pcard->equiping_cards.end();) {
					card* equipc = *(csit++);
					equipc->unequip();
					if(equipc->current.location == LOCATION_SZONE)
						equipings.insert(equipc);
				}
			}
			pcard->clear_card_target();
			if(nloc == LOCATION_HAND) {
				if(pcard->owner != pcard->current.controler) {
					effect* deffect = pduel->new_effect();
					deffect->owner = pcard;
					deffect->code = 0;
					deffect->type = EFFECT_TYPE_SINGLE;
					deffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_CLIENT_HINT;
					deffect->description = 67;
					deffect->reset_flag = RESET_EVENT + 0x1fe0000;
					pcard->add_effect(deffect);
				}
				tohand.insert(pcard);
				raise_single_event(pcard, 0, EVENT_TO_HAND, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
			}
			if(nloc == LOCATION_DECK || nloc == LOCATION_EXTRA) {
				todeck.insert(pcard);
				raise_single_event(pcard, 0, EVENT_TO_DECK, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
			}
			if(nloc == LOCATION_GRAVE && !(pcard->current.reason & REASON_RETURN)) {
				tograve.insert(pcard);
				raise_single_event(pcard, 0, EVENT_TO_GRAVE, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
			}
			if(nloc == LOCATION_REMOVED || ((pcard->data.type & TYPE_TOKEN) && pcard->sendto_param.location == LOCATION_REMOVED)) {
				remove.insert(pcard);
				raise_single_event(pcard, 0, EVENT_REMOVE, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
			}
			if(pcard->current.reason & REASON_DISCARD) {
				discard.insert(pcard);
				raise_single_event(pcard, 0, EVENT_DISCARD, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
			}
			if(pcard->current.reason & REASON_RELEASE) {
				released.insert(pcard);
				raise_single_event(pcard, 0, EVENT_RELEASE, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
			}
			// non-battle destroy
			if(pcard->current.reason & REASON_DESTROY && !(pcard->current.reason & REASON_BATTLE)) {
				destroyed.insert(pcard);
				raise_single_event(pcard, 0, EVENT_DESTROYED, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
			}
			if(pcard->xyz_materials.size()) {
				for(auto& mcard : pcard->xyz_materials)
					overlays.insert(mcard);
			}
			raise_single_event(pcard, 0, EVENT_MOVE, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
		}
		if(tohand.size())
			raise_event(&tohand, EVENT_TO_HAND, reason_effect, reason, reason_player, 0, 0);
		if(todeck.size())
			raise_event(&todeck, EVENT_TO_DECK, reason_effect, reason, reason_player, 0, 0);
		if(tograve.size())
			raise_event(&tograve, EVENT_TO_GRAVE, reason_effect, reason, reason_player, 0, 0);
		if(remove.size())
			raise_event(&remove, EVENT_REMOVE, reason_effect, reason, reason_player, 0, 0);
		if(discard.size())
			raise_event(&discard, EVENT_DISCARD, reason_effect, reason, reason_player, 0, 0);
		if(released.size())
			raise_event(&released, EVENT_RELEASE, reason_effect, reason, reason_player, 0, 0);
		if(destroyed.size())
			raise_event(&destroyed, EVENT_DESTROYED, reason_effect, reason, reason_player, 0, 0);
		raise_event(&targets->container, EVENT_MOVE, reason_effect, reason, reason_player, 0, 0);
		process_single_event();
		process_instant_event();
		if(equipings.size())
			destroy(&equipings, 0, REASON_RULE + REASON_LOST_TARGET, PLAYER_NONE);
		if(overlays.size())
			send_to(&overlays, 0, REASON_RULE + REASON_LOST_OVERLAY, PLAYER_NONE, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
		adjust_instant();
		return FALSE;
	}
	case 11: {
		core.operated_set.clear();
		core.operated_set = targets->container;
		returns.ivalue[0] = (int32)targets->container.size();
		pduel->delete_group(targets);
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::discard_deck(uint16 step, uint8 playerid, uint8 count, uint32 reason) {
	switch(step) {
	case 0: {
		if(is_player_affected_by_effect(playerid, EFFECT_CANNOT_DISCARD_DECK)) {
			core.operated_set.clear();
			returns.ivalue[0] = 0;
			return TRUE;
		}
		int32 i = 0;
		for(auto cit = player[playerid].list_main.rbegin(); i < count && cit != player[playerid].list_main.rend(); ++cit, ++i) {
			uint32 dest = LOCATION_GRAVE;
			(*cit)->sendto_param.location = LOCATION_GRAVE;
			(*cit)->current.reason_effect = core.reason_effect;
			(*cit)->current.reason_player = core.reason_player;
			(*cit)->current.reason = reason;
			uint32 redirect = (*cit)->destination_redirect(dest, reason) & 0xffff;
			if(redirect) {
				(*cit)->sendto_param.location = redirect;
			}
		}
		if(core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK) {
			if(player[playerid].list_main.size() > count) {
				card* ptop = *(player[playerid].list_main.rbegin() + count);
				if(core.deck_reversed || (ptop->current.position == POS_FACEUP_DEFENSE)) {
					pduel->write_buffer8(MSG_DECK_TOP);
					pduel->write_buffer8(playerid);
					pduel->write_buffer8(count);
					if(ptop->current.position != POS_FACEUP_DEFENSE)
						pduel->write_buffer32(ptop->data.code);
					else
						pduel->write_buffer32(ptop->data.code | 0x80000000);
				}
			}
		}
		return FALSE;
	}
	case 1: {
		card_set tohand, todeck, tograve, remove;
		core.discarded_set.clear();
		for (int32 i = 0; i < count; ++i) {
			if(player[playerid].list_main.size() == 0)
				break;
			card* pcard = player[playerid].list_main.back();
			uint8 dest = pcard->sendto_param.location;
			if(dest == LOCATION_GRAVE)
				pcard->reset(RESET_TOGRAVE, RESET_EVENT);
			else if(dest == LOCATION_HAND) {
				pcard->reset(RESET_TOHAND, RESET_EVENT);
				pcard->set_status(STATUS_PROC_COMPLETE, FALSE);
			} else if(dest == LOCATION_DECK) {
				pcard->reset(RESET_TODECK, RESET_EVENT);
				pcard->set_status(STATUS_PROC_COMPLETE, FALSE);
			} else if(dest == LOCATION_REMOVED) {
				if(pcard->current.reason & REASON_TEMPORARY)
					pcard->reset(RESET_TEMP_REMOVE, RESET_EVENT);
				else
					pcard->reset(RESET_REMOVE, RESET_EVENT);
			}
			pduel->write_buffer8(MSG_MOVE);
			pduel->write_buffer32(pcard->data.code);
			pduel->write_buffer8(pcard->current.controler);
			pduel->write_buffer8(pcard->current.location);
			pduel->write_buffer8(pcard->current.sequence);
			pduel->write_buffer8(pcard->current.position);
			pcard->enable_field_effect(false);
			pcard->cancel_field_effect();
			player[playerid].list_main.pop_back();
			pcard->previous.controler = pcard->current.controler;
			pcard->previous.location = pcard->current.location;
			pcard->previous.sequence = pcard->current.sequence;
			pcard->previous.position = pcard->current.position;
			pcard->previous.pzone = pcard->current.pzone;
			pcard->current.controler = PLAYER_NONE;
			pcard->current.location = 0;
			add_card(pcard->owner, pcard, dest, 0);
			pcard->enable_field_effect(true);
			pcard->current.position = POS_FACEUP;
			pduel->write_buffer8(pcard->current.controler);
			pduel->write_buffer8(pcard->current.location);
			pduel->write_buffer8(pcard->current.sequence);
			pduel->write_buffer8(pcard->current.position);
			pduel->write_buffer32(pcard->current.reason);
			if(dest == LOCATION_HAND) {
				if(pcard->owner != pcard->current.controler) {
					effect* deffect = pduel->new_effect();
					deffect->owner = pcard;
					deffect->code = 0;
					deffect->type = EFFECT_TYPE_SINGLE;
					deffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE | EFFECT_FLAG_CLIENT_HINT;
					deffect->description = 67;
					deffect->reset_flag = RESET_EVENT + 0x1fe0000;
					pcard->add_effect(deffect);
				}
				tohand.insert(pcard);
				raise_single_event(pcard, 0, EVENT_TO_HAND, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
			} else if(dest == LOCATION_DECK || dest == LOCATION_EXTRA) {
				todeck.insert(pcard);
				raise_single_event(pcard, 0, EVENT_TO_DECK, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
			} else if(dest == LOCATION_GRAVE) {
				tograve.insert(pcard);
				raise_single_event(pcard, 0, EVENT_TO_GRAVE, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
			} else if(dest == LOCATION_REMOVED) {
				remove.insert(pcard);
				raise_single_event(pcard, 0, EVENT_REMOVE, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
			}
			raise_single_event(pcard, 0, EVENT_MOVE, pcard->current.reason_effect, pcard->current.reason, pcard->current.reason_player, 0, 0);
			core.discarded_set.insert(pcard);
		}
		if(tohand.size())
			raise_event(&tohand, EVENT_TO_HAND, core.reason_effect, reason, core.reason_player, 0, 0);
		if(todeck.size())
			raise_event(&todeck, EVENT_TO_DECK, core.reason_effect, reason, core.reason_player, 0, 0);
		if(tograve.size())
			raise_event(&tograve, EVENT_TO_GRAVE, core.reason_effect, reason, core.reason_player, 0, 0);
		if(remove.size())
			raise_event(&remove, EVENT_REMOVE, core.reason_effect, reason, core.reason_player, 0, 0);
		raise_event(&core.discarded_set, EVENT_MOVE, core.reason_effect, reason, core.reason_player, 0, 0);
		process_single_event();
		process_instant_event();
		adjust_instant();
		return FALSE;
	}
	case 2: {
		core.operated_set.clear();
		core.operated_set = core.discarded_set;
		returns.ivalue[0] = (int32)core.discarded_set.size();
		core.discarded_set.clear();
		return TRUE;
	}
	}
	return TRUE;
}
// move a card from anywhere to field, including sp_summon, Duel.MoveToField(), Duel.ReturnToField()
// ret: 0 = default, 1 = return after temporarily banished, 2 = trap_monster return to LOCATION_SZONE
// call move_card() in step 2
int32 field::move_to_field(uint16 step, card* target, uint32 enable, uint32 ret, uint32 pzone, uint32 zone) {
	uint32 move_player = (target->to_field_param >> 24) & 0xff;
	uint32 playerid = (target->to_field_param >> 16) & 0xff;
	uint32 location = (target->to_field_param >> 8) & 0xff;
	uint32 positions = (target->to_field_param) & 0xff;
	switch(step) {
	case 0: {
		returns.ivalue[0] = FALSE;
		if((ret == 1) && (!(target->current.reason & REASON_TEMPORARY) || (target->current.reason_effect->owner != core.reason_effect->owner)))
			return TRUE;
		if(location == LOCATION_SZONE && zone == 0x1 << 5 && (target->data.type & TYPE_FIELD) && (target->data.type & TYPE_SPELL)) {
			card* pcard = get_field_card(playerid, LOCATION_SZONE, 5);
			if(pcard) {
				if(core.duel_rule >= 3)
					send_to(pcard, 0, REASON_RULE, pcard->current.controler, PLAYER_NONE, LOCATION_GRAVE, 0, 0);
				else
					destroy(pcard, 0, REASON_RULE, pcard->current.controler);
				adjust_all();
			}
		} else if(pzone && location == LOCATION_SZONE && (target->data.type & TYPE_PENDULUM)) {
			uint32 flag = 0;
			if(is_location_useable(playerid, LOCATION_PZONE, 0))
				flag |= 0x1u << (core.duel_rule >= 4 ? 8 : 14);
			if(is_location_useable(playerid, LOCATION_PZONE, 1))
				flag |= 0x1u << (core.duel_rule >= 4 ? 12 : 15);
			if(!flag)
				return TRUE;
			if(move_player != playerid)
				flag = flag << 16;
			flag = ~flag;
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_SELECTMSG);
			pduel->write_buffer8(move_player);
			pduel->write_buffer32(target->data.code);
			add_process(PROCESSOR_SELECT_PLACE, 0, 0, 0, move_player, flag, 1);
		} else {
			uint32 flag;
			uint32 lreason = (target->current.location == LOCATION_MZONE) ? LOCATION_REASON_CONTROL : LOCATION_REASON_TOFIELD;
			int32 ct = get_useable_count(target, playerid, location, move_player, lreason, zone, &flag);
			if((ret == 1) && (ct <= 0 || target->is_status(STATUS_FORBIDDEN) || !(positions & POS_FACEDOWN) && check_unique_onfield(target, playerid, location))) {
				core.units.begin()->step = 3;
				send_to(target, core.reason_effect, REASON_RULE, core.reason_player, PLAYER_NONE, LOCATION_GRAVE, 0, 0);
				return FALSE;
			}
			if(ct <= 0)
				return TRUE;
			if((zone & zone - 1) == 0) {
				for(uint8 seq = 0; seq < 8; seq++) {
					if((1 << seq) & zone) {
						returns.bvalue[2] = seq;
						return FALSE;
					}
				}
			}
			if(ret == 2 && core.duel_rule <= 4) {
				returns.bvalue[2] = target->previous.sequence;
				return FALSE;
			}
			if(move_player == playerid) {
				if(location == LOCATION_SZONE)
					flag = ((flag & 0xff) << 8) | 0xffff00ff;
				else
					flag = (flag & 0xff) | 0xffffff00;
			} else {
				if(location == LOCATION_SZONE)
					flag = ((flag & 0xff) << 24) | 0xffffff;
				else
					flag = ((flag & 0xff) << 16) | 0xff00ffff;
			}
			flag |= 0xe080e080;
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_SELECTMSG);
			pduel->write_buffer8(move_player);
			pduel->write_buffer32(target->data.code);
			add_process(PROCESSOR_SELECT_PLACE, 0, 0, 0, move_player, flag, 1);
		}
		return FALSE;
	}
	case 1: {
		uint32 seq = returns.bvalue[2];
		if(location == LOCATION_SZONE && zone == 0x1 << 5 && (target->data.type & TYPE_FIELD) && (target->data.type & TYPE_SPELL))
			seq = 5;
		if(ret != 1) {
			if(location != target->current.location) {
				uint32 resetflag = 0;
				if(location & LOCATION_ONFIELD)
					resetflag |= RESET_TOFIELD;
				if(target->current.location & LOCATION_ONFIELD)
					resetflag |= RESET_LEAVE;
				effect* peffect = target->is_affected_by_effect(EFFECT_PRE_MONSTER);
				if((location & LOCATION_ONFIELD) && (target->current.location & LOCATION_ONFIELD)
						&& !(peffect && (peffect->value & TYPE_TRAP)) && ret != 2)
					resetflag |= RESET_MSCHANGE;
				target->reset(resetflag, RESET_EVENT);
				target->clear_card_target();
			}
			if(!(target->current.location & LOCATION_ONFIELD))
				target->clear_relate_effect();
		}
		if(ret == 1)
			target->current.reason &= ~REASON_TEMPORARY;
		if(ret == 0 && location != target->current.location
			|| ret == 1 && target->turnid != infos.turn_id) {
			target->set_status(STATUS_SUMMON_TURN, FALSE);
			target->set_status(STATUS_FLIP_SUMMON_TURN, FALSE);
			target->set_status(STATUS_SPSUMMON_TURN, FALSE);
			target->set_status(STATUS_SET_TURN, FALSE);
			target->set_status(STATUS_FORM_CHANGED, FALSE);
		}
		target->temp.sequence = seq;
		if(location != LOCATION_MZONE) {
			returns.ivalue[0] = positions;
			return FALSE;
		}
		if(target->data.type & TYPE_LINK) {
			returns.ivalue[0] = POS_FACEUP_ATTACK;
			return FALSE;
		}
		add_process(PROCESSOR_SELECT_POSITION, 0, 0, 0, (positions << 16) + move_player, target->data.code);
		return FALSE;
	}
	case 2: {
		if(core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK) {
			if(target->current.location == LOCATION_DECK) {
				uint32 curp = target->current.controler;
				uint32 curs = target->current.sequence;
				if(curs > 0 && (curs == player[curp].list_main.size() - 1)) {
					card* ptop = player[curp].list_main[curs - 1];
					if(core.deck_reversed || (ptop->current.position == POS_FACEUP_DEFENSE)) {
						pduel->write_buffer8(MSG_DECK_TOP);
						pduel->write_buffer8(curp);
						pduel->write_buffer8(1);
						if(ptop->current.position != POS_FACEUP_DEFENSE)
							pduel->write_buffer32(ptop->data.code);
						else
							pduel->write_buffer32(ptop->data.code | 0x80000000);
					}
				}
			}
		}
		pduel->write_buffer8(MSG_MOVE);
		pduel->write_buffer32(target->data.code);
		pduel->write_buffer32(target->get_info_location());
		if(target->overlay_target)
			target->overlay_target->xyz_remove(target);
		move_card(playerid, target, location, target->temp.sequence, pzone);
		target->current.position = returns.ivalue[0];
		target->set_status(STATUS_LEAVE_CONFIRMED, FALSE);
		pduel->write_buffer32(target->get_info_location());
		pduel->write_buffer32(target->current.reason);
		if((target->current.location != LOCATION_MZONE)) {
			if(target->equiping_cards.size()) {
				destroy(&target->equiping_cards, 0, REASON_LOST_TARGET + REASON_RULE, PLAYER_NONE);
				for(auto csit = target->equiping_cards.begin(); csit != target->equiping_cards.end();) {
					auto rm = csit++;
					(*rm)->unequip();
				}
			}
			if(target->xyz_materials.size()) {
				card_set overlays;
				overlays.insert(target->xyz_materials.begin(), target->xyz_materials.end());
				send_to(&overlays, 0, REASON_LOST_OVERLAY + REASON_RULE, PLAYER_NONE, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
			}
		}
		if((target->previous.location == LOCATION_SZONE) && target->equiping_target)
			target->unequip();
		if(target->current.location == LOCATION_MZONE) {
			effect_set eset;
			filter_player_effect(0, EFFECT_MUST_USE_MZONE, &eset, FALSE);
			filter_player_effect(1, EFFECT_MUST_USE_MZONE, &eset, FALSE);
			target->filter_effect(EFFECT_MUST_USE_MZONE, &eset);
			for(int32 i = 0; i < eset.size(); i++) {
				effect* peffect = eset[i];
				if(peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT) && peffect->count_limit == 0)
					continue;
				uint32 value = 0x1f;
				if(peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET))
					value = peffect->get_value();
				else {
					uint32 lreason = (target->current.location == LOCATION_MZONE) ? LOCATION_REASON_CONTROL : LOCATION_REASON_TOFIELD;
					pduel->lua->add_param(target->current.controler, PARAM_TYPE_INT);
					pduel->lua->add_param(move_player, PARAM_TYPE_INT);
					pduel->lua->add_param(lreason, PARAM_TYPE_INT);
					value = peffect->get_value(target, 3);
				}
				if(peffect->get_handler_player() != target->current.controler)
					value = value >> 16;
				if(value & (0x1 << target->current.sequence)) {
					peffect->dec_count();
				}
			}
			if(effect* teffect = target->is_affected_by_effect(EFFECT_PRE_MONSTER)) {
				uint32 type = teffect->value;
				if(type & TYPE_TRAP)
					type |= TYPE_TRAPMONSTER | target->data.type;
				target->reset(EFFECT_PRE_MONSTER, RESET_CODE);
				effect* peffect = pduel->new_effect();
				peffect->owner = target;
				peffect->type = EFFECT_TYPE_SINGLE;
				peffect->code = EFFECT_CHANGE_TYPE;
				peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
				peffect->reset_flag = RESET_EVENT + 0x1fc0000;
				peffect->value = TYPE_MONSTER | type;
				target->add_effect(peffect);
				if(core.duel_rule <= 4 && (type & TYPE_TRAPMONSTER)) {
					peffect = pduel->new_effect();
					peffect->owner = target;
					peffect->type = EFFECT_TYPE_FIELD;
					peffect->range = LOCATION_MZONE;
					peffect->code = EFFECT_USE_EXTRA_SZONE;
					peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
					peffect->reset_flag = RESET_EVENT + 0x1fe0000;
					peffect->value = 1 + (0x10000 << target->previous.sequence);
					target->add_effect(peffect);
				}
			}
		}
		if(enable || ((ret == 1) && target->is_position(POS_FACEUP)))
			target->enable_field_effect(true);
		if(ret == 1 && target->current.location == LOCATION_MZONE && !(target->data.type & TYPE_MONSTER))
			send_to(target, 0, REASON_RULE, PLAYER_NONE, PLAYER_NONE, LOCATION_GRAVE, 0, 0);
		else {
			if(target->previous.location == LOCATION_GRAVE) {
				raise_single_event(target, 0, EVENT_LEAVE_GRAVE, target->current.reason_effect, target->current.reason, move_player, 0, 0);
				raise_event(target, EVENT_LEAVE_GRAVE, target->current.reason_effect, target->current.reason, move_player, 0, 0);
			}
			raise_single_event(target, 0, EVENT_MOVE, target->current.reason_effect, target->current.reason, target->current.reason_player, 0, 0);
			raise_event(target, EVENT_MOVE, target->current.reason_effect, target->current.reason, target->current.reason_player, 0, 0);
			process_single_event();
			process_instant_event();
		}
		adjust_disable_check_list();
		return FALSE;
	}
	case 3: {
		returns.ivalue[0] = TRUE;
		return TRUE;
	}
	case 4: {
		returns.ivalue[0] = FALSE;
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::change_position(uint16 step, group * targets, effect * reason_effect, uint8 reason_player, uint32 enable) {
	switch(step) {
	case 0: {
		for(auto cit = targets->container.begin(); cit != targets->container.end();) {
			card* pcard = *cit++;
			uint8 npos = pcard->position_param & 0xff;
			uint8 opos = pcard->current.position;
			if((pcard->current.location != LOCATION_MZONE && pcard->current.location != LOCATION_SZONE)
				|| (pcard->data.type & TYPE_LINK)
				|| pcard->get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP)
				|| !pcard->is_affect_by_effect(reason_effect) || npos == opos
				|| (!(pcard->data.type & TYPE_TOKEN) && (opos & POS_FACEUP) && (npos & POS_FACEDOWN) && !pcard->is_capable_turn_set(reason_player))
				|| (reason_effect && pcard->is_affected_by_effect(EFFECT_CANNOT_CHANGE_POS_E))) {
				targets->container.erase(pcard);
				continue;
			}
		}
		card_set* to_grave_set = new card_set;
		core.units.begin()->ptr1 = to_grave_set;
		return FALSE;
	}
	case 1: {
		card_set* to_grave_set = (card_set*)core.units.begin()->ptr1;
		uint8 playerid = reason_player;
		if(core.units.begin()->arg3)
			playerid = 1 - reason_player;
		card_set ssets;
		for(auto& pcard : targets->container) {
			uint8 npos = pcard->position_param & 0xff;
			uint8 opos = pcard->current.position;
			if((opos & POS_FACEUP) && (npos & POS_FACEDOWN)) {
				if(pcard->get_type() & TYPE_TRAPMONSTER) {
					if(pcard->current.controler == playerid)
						ssets.insert(pcard);
				}
			}
		}
		if(ssets.size()) {
			refresh_location_info_instant();
			int32 fcount = get_useable_count(NULL, playerid, LOCATION_SZONE, playerid, 0);
			if(fcount <= 0) {
				for(auto& pcard : ssets) {
					to_grave_set->insert(pcard);
					targets->container.erase(pcard);
				}
				core.units.begin()->step = 2;
			} else if((int32)ssets.size() > fcount) {
				core.select_cards.clear();
				for(auto& pcard : ssets)
					core.select_cards.push_back(pcard);
				uint32 ct = (uint32)ssets.size() - fcount;
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_SELECTMSG);
				pduel->write_buffer8(reason_player);
				pduel->write_buffer32(502);
				add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, reason_player, ct + (ct << 16));
			}
		} else
			core.units.begin()->step = 2;
		return FALSE;
	}
	case 2: {
		card_set* to_grave_set = (card_set*)core.units.begin()->ptr1;
		for(int32 i = 0; i < returns.bvalue[0]; ++i) {
			card* pcard = core.select_cards[returns.bvalue[i + 1]];
			to_grave_set->insert(pcard);
			targets->container.erase(pcard);
		}
		return FALSE;
	}
	case 3: {
		if(!core.units.begin()->arg3) {
			core.units.begin()->arg3 = 1;
			core.units.begin()->step = 0;
		}
		return FALSE;
	}
	case 4: {
		card_set equipings;
		card_set flips;
		card_set ssets;
		card_set pos_changed;
		card_vector cv(targets->container.begin(), targets->container.end());
		if(cv.size() > 1)
			std::sort(cv.begin(), cv.end(), card::card_operation_sort);
		for(auto& pcard : cv) {
			uint8 npos = pcard->position_param & 0xff;
			uint8 opos = pcard->current.position;
			uint8 flag = pcard->position_param >> 16;
			if((pcard->data.type & TYPE_TOKEN) && (npos & POS_FACEDOWN))
				npos = POS_FACEUP_DEFENSE;
			pcard->previous.position = opos;
			pcard->current.position = npos;
			if((npos & POS_DEFENSE) && !pcard->is_affected_by_effect(EFFECT_DEFENSE_ATTACK))
				pcard->set_status(STATUS_ATTACK_CANCELED, TRUE);
			pcard->set_status(STATUS_JUST_POS, TRUE);
			pduel->write_buffer8(MSG_POS_CHANGE);
			pduel->write_buffer32(pcard->data.code);
			pduel->write_buffer8(pcard->current.controler);
			pduel->write_buffer8(pcard->current.location);
			pduel->write_buffer8(pcard->current.sequence);
			pduel->write_buffer8(pcard->previous.position);
			pduel->write_buffer8(pcard->current.position);
			core.hint_timing[pcard->current.controler] |= TIMING_POS_CHANGE;
			if((opos & POS_FACEDOWN) && (npos & POS_FACEUP)) {
				pcard->fieldid = infos.field_id++;
				if(check_unique_onfield(pcard, pcard->current.controler, pcard->current.location))
					pcard->unique_fieldid = UINT_MAX;
				if(pcard->current.location == LOCATION_MZONE) {
					raise_single_event(pcard, 0, EVENT_FLIP, reason_effect, 0, reason_player, 0, flag);
					flips.insert(pcard);
				}
				if(enable) {
					if(!reason_effect || !(reason_effect->type & 0x7f0) || pcard->current.location != LOCATION_MZONE)
						pcard->enable_field_effect(true);
					else
						core.delayed_enable_set.insert(pcard);
				} else
					pcard->refresh_disable_status();
			}
			if(pcard->current.location == LOCATION_MZONE) {
				raise_single_event(pcard, 0, EVENT_CHANGE_POS, reason_effect, 0, reason_player, 0, 0);
				pos_changed.insert(pcard);
			}
			bool trapmonster = false;
			if((opos & POS_FACEUP) && (npos & POS_FACEDOWN)) {
				if(pcard->get_type() & TYPE_TRAPMONSTER)
					trapmonster = true;
				if(pcard->status & (STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED))
					pcard->set_status(STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED, FALSE);
				pcard->reset(RESET_TURN_SET, RESET_EVENT);
				pcard->clear_card_target();
				pcard->set_status(STATUS_SET_TURN, TRUE);
				pcard->enable_field_effect(false);
				pcard->previous.location = 0;
				pcard->summon_info &= 0xdf00ffff;
				if((pcard->summon_info & SUMMON_TYPE_PENDULUM) == SUMMON_TYPE_PENDULUM)
					pcard->summon_info &= 0xf000ffff;
				pcard->spsummon_counter[0] = pcard->spsummon_counter[1] = 0;
			}
			if((npos & POS_FACEDOWN) && pcard->equiping_cards.size()) {
				for(auto csit = pcard->equiping_cards.begin(); csit != pcard->equiping_cards.end();) {
					auto erm = csit++;
					equipings.insert(*erm);
					(*erm)->unequip();
				}
			}
			if((npos & POS_FACEDOWN) && pcard->equiping_target)
				pcard->unequip();
			if(trapmonster) {
				refresh_location_info_instant();
				move_to_field(pcard, pcard->current.controler, pcard->current.controler, LOCATION_SZONE, POS_FACEDOWN, FALSE, 2);
				raise_single_event(pcard, 0, EVENT_SSET, reason_effect, 0, reason_player, 0, 0);
				ssets.insert(pcard);
			}
		}
		adjust_instant();
		process_single_event();
		if(flips.size())
			raise_event(&flips, EVENT_FLIP, reason_effect, 0, reason_player, 0, 0);
		if(ssets.size())
			raise_event(&ssets, EVENT_SSET, reason_effect, 0, reason_player, 0, 0);
		if(pos_changed.size())
			raise_event(&pos_changed, EVENT_CHANGE_POS, reason_effect, 0, reason_player, 0, 0);
		process_instant_event();
		if(equipings.size())
			destroy(&equipings, 0, REASON_LOST_TARGET + REASON_RULE, PLAYER_NONE);
		card_set* to_grave_set = (card_set*)core.units.begin()->ptr1;
		if(to_grave_set->size()) {
			send_to(to_grave_set, 0, REASON_RULE, PLAYER_NONE, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
		}
		delete to_grave_set;
		return FALSE;
	}
	case 5: {
		core.operated_set.clear();
		core.operated_set = targets->container;
		returns.ivalue[0] = (int32)targets->container.size();
		pduel->delete_group(targets);
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::operation_replace(uint16 step, effect* replace_effect, group* targets, card* target, int32 is_destroy) {
	switch (step) {
	case 0: {
		if(returns.ivalue[0])
			return TRUE;
		if(!replace_effect->target)
			return TRUE;
		tevent e;
		e.event_cards = targets;
		e.event_player = replace_effect->get_handler_player();
		e.event_value = 0;
		e.reason = target->current.reason;
		e.reason_effect = target->current.reason_effect;
		e.reason_player = target->current.reason_player;
		if(!replace_effect->is_activateable(replace_effect->get_handler_player(), e))
			return TRUE;
		chain newchain;
		newchain.chain_id = 0;
		newchain.chain_count = 0;
		newchain.triggering_effect = replace_effect;
		newchain.triggering_player = e.event_player;
		newchain.evt = e;
		newchain.target_cards = 0;
		newchain.target_player = PLAYER_NONE;
		newchain.target_param = 0;
		newchain.disable_player = PLAYER_NONE;
		newchain.disable_reason = 0;
		newchain.flag = 0;
		core.solving_event.push_front(e);
		core.sub_solving_event.push_back(e);
		core.continuous_chain.push_back(newchain);
		add_process(PROCESSOR_EXECUTE_TARGET, 0, replace_effect, 0, replace_effect->get_handler_player(), 0);
		return FALSE;
	}
	case 1: {
		if (returns.ivalue[0]) {
			if(!(target->current.reason_effect && target->current.reason_effect->is_self_destroy_related())) {
				targets->container.erase(target);
				target->current.reason = target->temp.reason;
				target->current.reason_effect = target->temp.reason_effect;
				target->current.reason_player = target->temp.reason_player;
				if(is_destroy)
					core.destroy_canceled.insert(target);
			}
			replace_effect->dec_count(replace_effect->get_handler_player());
		} else
			core.units.begin()->step = 2;
		return FALSE;
	}
	case 2: {
		if(!replace_effect->operation)
			return FALSE;
		core.sub_solving_event.push_back(*core.solving_event.begin());
		add_process(PROCESSOR_EXECUTE_OPERATION, 0, replace_effect, 0, replace_effect->get_handler_player(), 0);
		return FALSE;
	}
	case 3: {
		if(core.continuous_chain.rbegin()->target_cards)
			pduel->delete_group(core.continuous_chain.rbegin()->target_cards);
		for(auto& oit : core.continuous_chain.rbegin()->opinfos) {
			if(oit.second.op_cards)
				pduel->delete_group(oit.second.op_cards);
		}
		core.continuous_chain.pop_back();
		core.solving_event.pop_front();
		return TRUE;
	}
	case 5: {
		tevent e;
		e.event_cards = targets;
		e.event_player = replace_effect->get_handler_player();
		e.event_value = 0;
		if(targets->container.size() == 0)
			return TRUE;
		card* pc = *targets->container.begin();
		e.reason = pc->current.reason;
		e.reason_effect = pc->current.reason_effect;
		e.reason_player = pc->current.reason_player;
		if(!replace_effect->is_activateable(replace_effect->get_handler_player(), e) || !replace_effect->value)
			return TRUE;
		chain newchain;
		newchain.chain_id = 0;
		newchain.chain_count = 0;
		newchain.triggering_effect = replace_effect;
		newchain.triggering_player = e.event_player;
		newchain.evt = e;
		newchain.target_cards = 0;
		newchain.target_player = PLAYER_NONE;
		newchain.target_param = 0;
		newchain.disable_player = PLAYER_NONE;
		newchain.disable_reason = 0;
		newchain.flag = 0;
		core.solving_event.push_front(e);
		core.sub_solving_event.push_back(e);
		core.continuous_chain.push_back(newchain);
		add_process(PROCESSOR_EXECUTE_TARGET, 0, replace_effect, 0, replace_effect->get_handler_player(), 0);
		return FALSE;
	}
	case 6: {
		if (returns.ivalue[0]) {
			for (auto cit = targets->container.begin(); cit != targets->container.end();) {
				auto rm = cit++;
				if(replace_effect->get_value(*rm) && !((*rm)->current.reason_effect && (*rm)->current.reason_effect->is_self_destroy_related())) {
					(*rm)->current.reason = (*rm)->temp.reason;
					(*rm)->current.reason_effect = (*rm)->temp.reason_effect;
					(*rm)->current.reason_player = (*rm)->temp.reason_player;
					if(is_destroy)
						core.destroy_canceled.insert(*rm);
					targets->container.erase(rm);
				}
			}
			replace_effect->dec_count(replace_effect->get_handler_player());
		} else
			core.units.begin()->step = 7;
		return FALSE;
	}
	case 7: {
		if(!replace_effect->operation)
			return FALSE;
		core.sub_solving_event.push_back(*core.solving_event.begin());
		add_process(PROCESSOR_EXECUTE_OPERATION, 0, replace_effect, 0, replace_effect->get_handler_player(), 0);
		return FALSE;
	}
	case 8: {
		if(core.continuous_chain.rbegin()->target_cards)
			pduel->delete_group(core.continuous_chain.rbegin()->target_cards);
		for(auto& oit : core.continuous_chain.rbegin()->opinfos) {
			if(oit.second.op_cards)
				pduel->delete_group(oit.second.op_cards);
		}
		core.continuous_chain.pop_back();
		core.solving_event.pop_front();
		return TRUE;
	}
	case 10: {
		if(returns.ivalue[0])
			return TRUE;
		if(!replace_effect->target)
			return TRUE;
		tevent e;
		e.event_cards = targets;
		e.event_player = replace_effect->get_handler_player();
		e.event_value = 0;
		e.reason = target->current.reason;
		e.reason_effect = target->current.reason_effect;
		e.reason_player = target->current.reason_player;
		if(!replace_effect->is_activateable(replace_effect->get_handler_player(), e))
			return TRUE;
		chain newchain;
		newchain.chain_id = 0;
		newchain.chain_count = 0;
		newchain.triggering_effect = replace_effect;
		newchain.triggering_player = e.event_player;
		newchain.evt = e;
		newchain.target_cards = 0;
		newchain.target_player = PLAYER_NONE;
		newchain.target_param = 0;
		newchain.disable_player = PLAYER_NONE;
		newchain.disable_reason = 0;
		newchain.flag = 0;
		core.sub_solving_event.push_back(e);
		core.continuous_chain.push_back(newchain);
		add_process(PROCESSOR_EXECUTE_TARGET, 0, replace_effect, 0, replace_effect->get_handler_player(), 0);
		return FALSE;
	}
	case 11: {
		if (returns.ivalue[0]) {
			targets->container.erase(target);
			target->current.reason = target->temp.reason;
			target->current.reason_effect = target->temp.reason_effect;
			target->current.reason_player = target->temp.reason_player;
			if(is_destroy)
				core.destroy_canceled.insert(target);
			replace_effect->dec_count(replace_effect->get_handler_player());
			core.desrep_chain.push_back(core.continuous_chain.front());
		}
		core.continuous_chain.pop_front();
		return TRUE;
	}
	case 12: {
		tevent e;
		e.event_cards = targets;
		e.event_player = replace_effect->get_handler_player();
		e.event_value = 0;
		if(targets->container.size() == 0)
			return TRUE;
		card* pc = *targets->container.begin();
		e.reason = pc->current.reason;
		e.reason_effect = pc->current.reason_effect;
		e.reason_player = pc->current.reason_player;
		if(!replace_effect->is_activateable(replace_effect->get_handler_player(), e) || !replace_effect->value)
			return TRUE;
		chain newchain;
		newchain.chain_id = 0;
		newchain.chain_count = 0;
		newchain.triggering_effect = replace_effect;
		newchain.triggering_player = e.event_player;
		newchain.evt = e;
		newchain.target_cards = 0;
		newchain.target_player = PLAYER_NONE;
		newchain.target_param = 0;
		newchain.disable_player = PLAYER_NONE;
		newchain.disable_reason = 0;
		newchain.flag = 0;
		core.continuous_chain.push_back(newchain);
		core.sub_solving_event.push_back(e);
		add_process(PROCESSOR_EXECUTE_TARGET, 0, replace_effect, 0, replace_effect->get_handler_player(), 0);
		return FALSE;
	}
	case 13: {
		if (returns.ivalue[0]) {
			for (auto cit = targets->container.begin(); cit != targets->container.end();) {
				auto rm = cit++;
				if (replace_effect->get_value(*rm)) {
					(*rm)->current.reason = (*rm)->temp.reason;
					(*rm)->current.reason_effect = (*rm)->temp.reason_effect;
					(*rm)->current.reason_player = (*rm)->temp.reason_player;
					if(is_destroy)
						core.destroy_canceled.insert(*rm);
					targets->container.erase(rm);
				}
			}
			replace_effect->dec_count(replace_effect->get_handler_player());
			core.desrep_chain.push_back(core.continuous_chain.front());
		}
		core.continuous_chain.pop_front();
		return TRUE;
	}
	case 15: {
		if(core.desrep_chain.size() == 0)
			return TRUE;
		core.continuous_chain.push_back(core.desrep_chain.front());
		core.desrep_chain.pop_front();
		effect* reffect = core.continuous_chain.back().triggering_effect;
		if(!reffect->operation)
			return FALSE;
		core.sub_solving_event.push_back(core.continuous_chain.back().evt);
		add_process(PROCESSOR_EXECUTE_OPERATION, 0, reffect, 0, reffect->get_handler_player(), 0);
		return FALSE;
	}
	case 16: {
		if(core.continuous_chain.rbegin()->target_cards)
			pduel->delete_group(core.continuous_chain.rbegin()->target_cards);
		for(auto& oit : core.continuous_chain.rbegin()->opinfos) {
			if(oit.second.op_cards)
				pduel->delete_group(oit.second.op_cards);
		}
		core.continuous_chain.pop_back();
		core.units.begin()->step = 14;
		return FALSE;
	}
	}
	return TRUE;
}
int32 field::activate_effect(uint16 step, effect* peffect) {
	switch(step) {
	case 0: {
		card* phandler = peffect->get_handler();
		int32 playerid = phandler->current.controler;
		nil_event.event_code = EVENT_FREE_CHAIN;
		if(!peffect->is_activateable(playerid, nil_event))
			return TRUE;
		chain newchain;
		newchain.flag = 0;
		newchain.chain_id = infos.field_id++;
		newchain.evt.event_code = peffect->code;
		newchain.evt.event_player = PLAYER_NONE;
		newchain.evt.event_value = 0;
		newchain.evt.event_cards = 0;
		newchain.evt.reason = 0;
		newchain.evt.reason_effect = 0;
		newchain.evt.reason_player = PLAYER_NONE;
		newchain.triggering_effect = peffect;
		newchain.set_triggering_state(phandler);
		newchain.triggering_player = playerid;
		core.new_chains.push_back(newchain);
		phandler->set_status(STATUS_CHAINING, TRUE);
		peffect->dec_count(playerid);
		add_process(PROCESSOR_ADD_CHAIN, 0, 0, 0, 0, 0);
		add_process(PROCESSOR_QUICK_EFFECT, 0, 0, 0, FALSE, 1 - playerid);
		infos.priorities[0] = 0;
		infos.priorities[1] = 0;
		return FALSE;
	}
	case 1: {
		for(auto& ch_lim : core.chain_limit)
			luaL_unref(pduel->lua->lua_state, LUA_REGISTRYINDEX, ch_lim.function);
		core.chain_limit.clear();
		for(auto& ch : core.current_chain)
			ch.triggering_effect->get_handler()->set_status(STATUS_CHAINING, FALSE);
		add_process(PROCESSOR_SOLVE_CHAIN, 0, 0, 0, FALSE, 0);
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::select_synchro_material(int16 step, uint8 playerid, card* pcard, int32 min, int32 max, card* smat, group* mg) {
	switch(step) {
	case 0: {
		core.select_cards.clear();
		if(core.global_flag & GLOBALFLAG_MUST_BE_SMATERIAL) {
			effect_set eset;
			filter_player_effect(pcard->current.controler, EFFECT_MUST_BE_SMATERIAL, &eset);
			if(eset.size() && (!mg || mg->has_card(eset[0]->handler))) {
				core.select_cards.push_back(eset[0]->handler);
				pduel->restore_assumes();
				pduel->write_buffer8(MSG_HINT);
				pduel->write_buffer8(HINT_SELECTMSG);
				pduel->write_buffer8(playerid);
				pduel->write_buffer32(512);
				add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + ((uint32)core.summon_cancelable << 16), 0x10001);
				return FALSE;
			}
		}
		if(mg) {
			for(auto& pm : mg->container) {
				if(check_tuner_material(pcard, pm, -3, -2, min, max, smat, mg))
					core.select_cards.push_back(pm);
			}
		} else {
			for(uint8 p = 0; p < 2; ++p) {
				for(auto& tuner : player[p].list_mzone) {
					if(check_tuner_material(pcard, tuner, -3, -2, min, max, smat, mg))
						core.select_cards.push_back(tuner);
				}
			}
		}
		if(core.select_cards.size() == 0)
			return TRUE;
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(512);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + ((uint32)core.summon_cancelable << 16), 0x10001);
		return FALSE;
	}
	case 1: {
		if(returns.ivalue[0] == -1) {
			lua_pop(pduel->lua->current_state, 3);
			pduel->lua->add_param((void*)0, PARAM_TYPE_GROUP);
			core.limit_tuner = 0;
			return TRUE;
		}
		card* tuner = core.select_cards[returns.bvalue[1]];
		effect* pcheck = tuner->is_affected_by_effect(EFFECT_SYNCHRO_CHECK);
		if(pcheck)
			pcheck->get_value(tuner);
		core.limit_tuner = tuner;
		effect* peffect;
		if((peffect = tuner->is_affected_by_effect(EFFECT_SYNCHRO_MATERIAL_CUSTOM, pcard))) {
			if(!peffect->operation)
				return FALSE;
			core.synchro_materials.clear();
			pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
			pduel->lua->add_param(-2, PARAM_TYPE_INDEX);
			pduel->lua->add_param(min, PARAM_TYPE_INT);
			pduel->lua->add_param(max, PARAM_TYPE_INT);
			core.sub_solving_event.push_back(nil_event);
			add_process(PROCESSOR_EXECUTE_OPERATION, 0, peffect, 0, playerid, 0);
		} else {
			core.units.begin()->step = 2;
		}
		return FALSE;
	}
	case 2: {
		lua_pop(pduel->lua->current_state, 3);
		group* pgroup = pduel->new_group(core.synchro_materials);
		pgroup->container.insert(core.limit_tuner);
		pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
		pduel->restore_assumes();
		core.limit_tuner = 0;
		return TRUE;
	}
	case 3: {
		card* tuner = core.limit_tuner;
		effect* ptuner = tuner->is_affected_by_effect(EFFECT_TUNER_MATERIAL_LIMIT);
		if(ptuner && ptuner->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
			if(ptuner->s_range && ptuner->s_range > min)
				min = ptuner->s_range;
			if(ptuner->o_range && ptuner->o_range < max)
				max = ptuner->o_range;
			core.units.begin()->arg2 = min + (max << 16);
		}
		int32 mzone_limit = get_mzone_limit(playerid, playerid, LOCATION_REASON_TOFIELD);
		if(mzone_limit < 0) {
			int32 ft = -mzone_limit;
			if(ft > min)
				min = ft;
			core.units.begin()->arg2 = min + (max << 16);
		}
		if(!smat)
			return FALSE;
		min--;
		max--;
		core.units.begin()->arg2 = min + (max << 16);
		effect* pcheck = tuner->is_affected_by_effect(EFFECT_SYNCHRO_CHECK);
		if(pcheck)
			pcheck->get_value(smat);
		if(min == 0) {
			int32 lv = pcard->get_level();
			card_vector nsyn;
			nsyn.push_back(tuner);
			nsyn.push_back(smat);
			tuner->sum_param = tuner->get_synchro_level(pcard);
			smat->sum_param = smat->get_synchro_level(pcard);
			if(check_with_sum_limit_m(nsyn, lv, 0, 0, 0, 0xffff, 2))
				core.units.begin()->step = 8;
		}
		return FALSE;
	}
	case 4: {
		card* tuner = core.limit_tuner;
		int32 location = LOCATION_MZONE;
		effect* ptuner = tuner->is_affected_by_effect(EFFECT_TUNER_MATERIAL_LIMIT);
		if(ptuner) {
			if(ptuner->value)
				location = ptuner->value;
		}
		effect* pcheck = tuner->is_affected_by_effect(EFFECT_SYNCHRO_CHECK);
		core.must_select_cards.clear();
		core.must_select_cards.push_back(tuner);
		tuner->sum_param = tuner->get_synchro_level(pcard);
		if(smat) {
			core.must_select_cards.push_back(smat);
			smat->sum_param = smat->get_synchro_level(pcard);
		}
		card_vector nsyn(core.must_select_cards);
		int32 mcount = (int32)nsyn.size();
		if(mg) {
			for(auto& pm : mg->container) {
				if(pm == tuner || pm == smat || !pm->is_can_be_synchro_material(pcard, tuner))
					continue;
				if(ptuner && ptuner->target) {
					pduel->lua->add_param(ptuner, PARAM_TYPE_EFFECT);
					pduel->lua->add_param(pm, PARAM_TYPE_CARD);
					if(!pduel->lua->get_function_value(ptuner->target, 2))
						continue;
				}
				if(pcheck)
					pcheck->get_value(pm);
				if(pm->current.location == LOCATION_MZONE && !pm->is_position(POS_FACEUP))
					continue;
				if(!pduel->lua->check_matching(pm, -2, 1))
					continue;
				nsyn.push_back(pm);
				pm->sum_param = pm->get_synchro_level(pcard);
			}
		} else {
			card_vector cv;
			if(location & LOCATION_MZONE) {
				cv.insert(cv.end(), player[0].list_mzone.begin(), player[0].list_mzone.end());
				cv.insert(cv.end(), player[1].list_mzone.begin(), player[1].list_mzone.end());
			}
			if(location & LOCATION_HAND)
				cv.insert(cv.end(), player[playerid].list_hand.begin(), player[playerid].list_hand.end());
			for(auto& pm : cv) {
				if(!pm || pm == tuner || pm == smat || !pm->is_can_be_synchro_material(pcard, tuner))
					continue;
				if(ptuner && ptuner->target) {
					pduel->lua->add_param(ptuner, PARAM_TYPE_EFFECT);
					pduel->lua->add_param(pm, PARAM_TYPE_CARD);
					if(!pduel->lua->get_function_value(ptuner->target, 2))
						continue;
				}
				if(pcheck)
					pcheck->get_value(pm);
				if(pm->current.location == LOCATION_MZONE && !pm->is_position(POS_FACEUP))
					continue;
				if(!pduel->lua->check_matching(pm, -2, 1))
					continue;
				nsyn.push_back(pm);
				pm->sum_param = pm->get_synchro_level(pcard);
			}
		}
		int32 lv = pcard->get_level();
		core.select_cards.clear();
		auto start = nsyn.begin() + mcount;
		for(auto cit = start; cit != nsyn.end(); ++cit) {
			card* pm = *cit;
			if(start != cit)
				std::iter_swap(start, cit);
			if(check_other_synchro_material(nsyn, lv, min - 1, max - 1, mcount + 1))
				core.select_cards.push_back(pm);
			if(start != cit)
				std::iter_swap(start, cit);
		}
		return FALSE;
	}
	case 5: {
		card* tuner = core.limit_tuner;
		int32 playerid = pcard->current.controler;
		int32 ct = get_spsummonable_count(pcard, playerid);
		if(ct > 0) {
			core.units.begin()->step = 6;
			return FALSE;
		}
		card_set handover_zone_cards;
		uint32 must_use_zone_flag = 0;
		filter_must_use_mzone(playerid, playerid, LOCATION_REASON_TOFIELD, pcard, &must_use_zone_flag);
		uint32 handover_zone = get_rule_zone_fromex(playerid, pcard) & ~must_use_zone_flag;
		get_cards_in_zone(&handover_zone_cards, handover_zone, playerid, LOCATION_MZONE);
		if(handover_zone_cards.find(tuner) != handover_zone_cards.end())
			ct++;
		if(smat && handover_zone_cards.find(smat) != handover_zone_cards.end())
			ct++;
		if(ct > 0) {
			core.units.begin()->step = 6;
			return FALSE;
		}
		card_vector* select_cards = new card_vector;
		for(auto& pm : core.select_cards) {
			if(handover_zone_cards.find(pm) != handover_zone_cards.end())
				select_cards->push_back(pm);
		}
		if(select_cards->size() == core.select_cards.size()) {
			delete select_cards;
			core.units.begin()->step = 6;
			return FALSE;
		}
		select_cards->swap(core.select_cards);
		card_vector* must_select_cards = new card_vector;
		must_select_cards->swap(core.must_select_cards);
		core.units.begin()->ptr1 = select_cards;
		core.units.begin()->ptr2 = must_select_cards;
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(512);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid, 0x10001);
		return FALSE;
	}
	case 6: {
		card* pcard = core.select_cards[returns.bvalue[1]];
		card_vector* select_cards = (card_vector*)core.units.begin()->ptr1;
		auto it = std::find(select_cards->begin(), select_cards->end(), pcard);
		select_cards->erase(it);
		card_vector* must_select_cards = (card_vector*)core.units.begin()->ptr2;
		must_select_cards->push_back(pcard);
		select_cards->swap(core.select_cards);
		must_select_cards->swap(core.must_select_cards);
		delete select_cards;
		delete must_select_cards;
		min--;
		max--;
		core.units.begin()->arg2 = min + (max << 16);
		return FALSE;
	}
	case 7: {
		int32 lv = pcard->get_level();
		if(core.global_flag & GLOBALFLAG_SCRAP_CHIMERA) {
			effect* peffect = 0;
			for(auto& pm : core.select_cards) {
				peffect = pm->is_affected_by_effect(EFFECT_SCRAP_CHIMERA);
				if(peffect)
					break;
			}
			if(peffect) {
				card_vector nsyn(core.must_select_cards);
				nsyn.insert(nsyn.end(), core.select_cards.begin(), core.select_cards.end());
				card_vector nsyn_filtered;
				for(auto& pm : nsyn) {
					if(!peffect->get_value(pm))
						nsyn_filtered.push_back(pm);
				}
				if(nsyn_filtered.size() < nsyn.size()) {
					card_vector nsyn_removed;
					for(auto& pm : nsyn) {
						if(!pm->is_affected_by_effect(EFFECT_SCRAP_CHIMERA))
							nsyn_removed.push_back(pm);
					}
					bool mfiltered = true;
					bool mremoved = true;
					int32 mcount = (int32)core.must_select_cards.size();
					for(int32 i = 0; i < mcount; ++i) {
						if(peffect->get_value(nsyn[i]))
							mfiltered = false;
						if(nsyn[i]->is_affected_by_effect(EFFECT_SCRAP_CHIMERA))
							mremoved = false;
					}
					if(mfiltered && check_with_sum_limit_m(nsyn_filtered, lv, 0, min, max, 0xffff, mcount)) {
						if(mremoved && check_with_sum_limit_m(nsyn_removed, lv, 0, min, max, 0xffff, mcount)) {
							add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, playerid, peffect->description);
							core.units.begin()->step = 9;
							return FALSE;
						} else
							core.select_cards.assign(nsyn_filtered.begin() + mcount, nsyn_filtered.end());
					} else
						core.select_cards.assign(nsyn_removed.begin() + mcount, nsyn_removed.end());
				}
			}
		}
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(512);
		add_process(PROCESSOR_SELECT_SUM, 0, 0, 0, lv, playerid + (min << 16) + (max << 24));
		return FALSE;
	}
	case 8: {
		lua_pop(pduel->lua->current_state, 3);
		group* pgroup = pduel->new_group();
		int32 mcount = (int32)core.must_select_cards.size();
		for(int32 i = mcount; i < returns.bvalue[0]; ++i) {
			card* pcard = core.select_cards[returns.bvalue[i + 1]];
			pgroup->container.insert(pcard);
		}
		pgroup->container.insert(core.must_select_cards.begin(), core.must_select_cards.end());
		core.must_select_cards.clear();
		pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
		pduel->restore_assumes();
		core.limit_tuner = 0;
		return TRUE;
	}
	case 9: {
		lua_pop(pduel->lua->current_state, 3);
		group* pgroup = pduel->new_group();
		pgroup->container.insert(core.limit_tuner);
		pgroup->container.insert(smat);
		pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
		pduel->restore_assumes();
		core.limit_tuner = 0;
		return TRUE;
	}
	case 10: {
		int32 lv = pcard->get_level();
		if(returns.ivalue[0]) {
			effect* peffect = 0;
			for(auto& pm : core.select_cards) {
				peffect = pm->is_affected_by_effect(EFFECT_SCRAP_CHIMERA);
				if(peffect)
					break;
			}
			card_vector nsyn_filtered;
			for(auto& pm : core.select_cards) {
				if(!peffect->get_value(pm))
					nsyn_filtered.push_back(pm);
			}
			core.select_cards.swap(nsyn_filtered);
		} else {
			card_vector nsyn_removed;
			for(auto& pm : core.select_cards) {
				if(!pm->is_affected_by_effect(EFFECT_SCRAP_CHIMERA))
					nsyn_removed.push_back(pm);
			}
			core.select_cards.swap(nsyn_removed);
		}
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(512);
		add_process(PROCESSOR_SELECT_SUM, 0, 0, 0, lv, playerid + (min << 16) + (max << 24));
		core.units.begin()->step = 7;
		return FALSE;
	}
	}
	return TRUE;
}
int32 field::select_xyz_material(int16 step, uint8 playerid, uint32 lv, card* scard, int32 min, int32 max) {
	switch(step) {
	case 0: {
		core.operated_set.clear();
		int32 mzone_limit = get_mzone_limit(playerid, playerid, LOCATION_REASON_TOFIELD);
		if(mzone_limit <= 0) {
			int32 ft = -mzone_limit + 1;
			if(ft > min) {
				min = ft;
				core.units.begin()->arg2 = min + (max << 16);
			}
		}
		effect_set eset;
		filter_player_effect(playerid, EFFECT_MUST_BE_XMATERIAL, &eset);
		core.select_cards.clear();
		for(int i = 0; i < eset.size(); ++i)
			core.select_cards.push_back(eset[i]->handler);
		int32 mct = (int32)core.select_cards.size();
		if(mct == 0) {
			returns.ivalue[0] = 1;
			core.units.begin()->step = 1;
			return FALSE;
		}
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(513);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + ((uint32)core.summon_cancelable << 16), mct + (mct << 16));
		return FALSE;
	}
	case 1: {
		if(returns.ivalue[0] == -1) {
			pduel->lua->add_param((void*)0, PARAM_TYPE_GROUP);
			return TRUE;
		}
		int32 pv = 0;
		for(int32 i = 0; i < returns.bvalue[0]; ++i) {
			card* pcard = core.select_cards[returns.bvalue[i + 1]];
			core.operated_set.insert(pcard);
			for(auto iter = core.xmaterial_lst.begin(); iter != core.xmaterial_lst.end(); ++iter) {
				if(iter->second == pcard) {
					if(pv < iter->first)
						pv = iter->first;
					core.xmaterial_lst.erase(iter);
					break;
				}
			}
		}
		max -= returns.bvalue[0];
		if(max == 0 || core.xmaterial_lst.size() == 0) {
			group* pgroup = pduel->new_group(core.operated_set);
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			return TRUE;
		}
		min -= returns.bvalue[0];
		if(min < 0)
			min = 0;
		if(pv - (int32)core.operated_set.size() > min)
			min = pv - (int32)core.operated_set.size();
		core.units.begin()->arg2 = min + (max << 16);
		if(min == 0) {
			add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, playerid, 93);
			return FALSE;
		}
		returns.ivalue[0] = 1;
		return FALSE;
	}
	case 2: {
		if(!returns.ivalue[0]) {
			group* pgroup = pduel->new_group(core.operated_set);
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			return TRUE;
		}
		if(!(core.global_flag & GLOBALFLAG_TUNE_MAGICIAN))
			return FALSE;
		int32 ct = get_spsummonable_count(scard, playerid);
		card_set handover_zone_cards;
		if(ct <= 0) {
			uint32 must_use_zone_flag = 0;
			filter_must_use_mzone(playerid, playerid, LOCATION_REASON_TOFIELD, scard, &must_use_zone_flag);
			uint32 handover_zone = get_rule_zone_fromex(playerid, scard) & ~must_use_zone_flag;
			get_cards_in_zone(&handover_zone_cards, handover_zone, playerid, LOCATION_MZONE);
		}
		for(auto& pcard : core.operated_set) {
			effect* peffect = pcard->is_affected_by_effect(EFFECT_TUNE_MAGICIAN_X);
			if(peffect) {
				for(auto cit = core.xmaterial_lst.begin(); cit != core.xmaterial_lst.end();) {
					if(peffect->get_value(cit->second))
						cit = core.xmaterial_lst.erase(cit);
					else
						++cit;
				}
			}
		}
		for(auto cit = core.xmaterial_lst.begin(); cit != core.xmaterial_lst.end(); ++cit)
			cit->second->sum_param = 0;
		int32 digit = 1;
		for(auto cit = core.xmaterial_lst.begin(); cit != core.xmaterial_lst.end();) {
			card* pcard = cit->second;
			effect* peffect = pcard->is_affected_by_effect(EFFECT_TUNE_MAGICIAN_X);
			if(peffect) {
				if(std::any_of(core.operated_set.begin(), core.operated_set.end(),
					[=](card* pcard) { return !!peffect->get_value(pcard); })) {
					cit = core.xmaterial_lst.erase(cit);
					continue;
				}
				digit <<= 1;
				for(auto mit = core.xmaterial_lst.begin(); mit != core.xmaterial_lst.end(); ++mit) {
					if(!peffect->get_value(mit->second))
						mit->second->sum_param |= digit;
				}
				pcard->sum_param |= digit;
			} else
				pcard->sum_param |= 1;
			++cit;
		}
		int32 selectable = 0;
		std::multimap<int32, card*, std::greater<int32>> mat;
		for(int32 icheck = 1; icheck <= digit; icheck <<= 1) {
			mat.clear();
			for(auto cit = core.xmaterial_lst.begin(); cit != core.xmaterial_lst.end(); ++cit) {
				if(cit->second->sum_param & icheck)
					mat.insert(*cit);
			}
			if(core.global_flag & GLOBALFLAG_XMAT_COUNT_LIMIT) {
				int32 maxc = std::min(max, (int32)mat.size()) + (int32)core.operated_set.size();
				auto iter = mat.lower_bound(maxc);
				mat.erase(mat.begin(), iter);
			}
			if(ct <= 0) {
				int32 ft = ct;
				for(auto cit = mat.begin(); cit != mat.end(); ++cit) {
					card* pcard = cit->second;
					if(handover_zone_cards.find(pcard) != handover_zone_cards.end())
						ft++;
				}
				if(ft <= 0)
					continue;
			}
			if((int32)mat.size() >= min)
				selectable |= icheck;
		}
		int32 acc = selectable;
		for(auto cit = core.xmaterial_lst.begin(); cit != core.xmaterial_lst.end();) {
			if(!(cit->second->sum_param & selectable))
				cit = core.xmaterial_lst.erase(cit);
			else {
				acc &= cit->second->sum_param;
				++cit;
			}
		}
		if(acc)
			return FALSE;
		core.units.begin()->arg3 = selectable;
		core.units.begin()->step = 19;
		return FALSE;
	}
	case 3: {
		int32 ct = get_spsummonable_count(scard, playerid);
		if(ct > 0) {
			returns.ivalue[0] = 1;
			core.units.begin()->step = 4;
			return FALSE;
		}
		card_set handover_zone_cards;
		uint32 must_use_zone_flag = 0;
		filter_must_use_mzone(playerid, playerid, LOCATION_REASON_TOFIELD, scard, &must_use_zone_flag);
		uint32 handover_zone = get_rule_zone_fromex(playerid, scard) & ~must_use_zone_flag;
		get_cards_in_zone(&handover_zone_cards, handover_zone, playerid, LOCATION_MZONE);
		int32 ft = ct + (int32)std::count_if(core.operated_set.begin(), core.operated_set.end(),
			[=](card* pcard) { return handover_zone_cards.find(pcard) != handover_zone_cards.end(); });
		if(ft > 0) {
			returns.ivalue[0] = 1;
			core.units.begin()->step = 4;
			return FALSE;
		}
		int32 mmax = 0;
		core.select_cards.clear();
		for(auto iter = core.xmaterial_lst.begin(); iter != core.xmaterial_lst.end(); ++iter) {
			card* pcard = iter->second;
			if(handover_zone_cards.find(pcard) != handover_zone_cards.end())
				core.select_cards.push_back(pcard);
			else
				mmax++;
		}
		if(min > mmax) {
			returns.ivalue[0] = 1;
			core.units.begin()->step = 4;
			return FALSE;
		}
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(513);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + ((uint32)core.summon_cancelable << 16), 0x10001);
		return FALSE;
	}
	case 4: {
		if(returns.ivalue[0] == -1) {
			pduel->lua->add_param((void*)0, PARAM_TYPE_GROUP);
			return TRUE;
		}
		card* pcard = core.select_cards[returns.bvalue[1]];
		core.operated_set.insert(pcard);
		int32 pv = 0;
		for(auto iter = core.xmaterial_lst.begin(); iter != core.xmaterial_lst.end(); ++iter) {
			if(iter->second == pcard) {
				if(pv < iter->first)
					pv = iter->first;
				core.xmaterial_lst.erase(iter);
				break;
			}
		}
		max--;
		if(max == 0 || core.xmaterial_lst.size() == 0) {
			group* pgroup = pduel->new_group(core.operated_set);
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			return TRUE;
		}
		if(min > 0)
			min--;
		if(pv - (int32)core.operated_set.size() > min)
			min = pv - (int32)core.operated_set.size();
		core.units.begin()->arg2 = min + (max << 16);
		if(min == 0) {
			add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, playerid, 93);
			return FALSE;
		}
		returns.ivalue[0] = 1;
		return FALSE;
	}
	case 5: {
		if(!returns.ivalue[0]) {
			group* pgroup = pduel->new_group(core.operated_set);
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			return TRUE;
		}
		int32 maxv = 0;
		if(core.xmaterial_lst.size())
			maxv = core.xmaterial_lst.begin()->first;
		if(min >= maxv) {
			core.select_cards.clear();
			for(auto iter = core.xmaterial_lst.begin(); iter != core.xmaterial_lst.end(); ++iter)
				core.select_cards.push_back(iter->second);
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_SELECTMSG);
			pduel->write_buffer8(playerid);
			pduel->write_buffer32(513);
			add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + ((uint32)core.summon_cancelable << 16), min + (max << 16));
		} else
			core.units.begin()->step = 6;
		return FALSE;
	}
	case 6: {
		if(returns.ivalue[0] == -1) {
			pduel->lua->add_param((void*)0, PARAM_TYPE_GROUP);
			return TRUE;
		}
		group* pgroup = pduel->new_group(core.operated_set);
		for(int32 i = 0; i < returns.bvalue[0]; ++i) {
			card* pcard = core.select_cards[returns.bvalue[i + 1]];
			pgroup->container.insert(pcard);
		}
		pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
		return TRUE;
	}
	case 7: {
		core.select_cards.clear();
		for(auto iter = core.xmaterial_lst.begin(); iter != core.xmaterial_lst.end(); ++iter)
			core.select_cards.push_back(iter->second);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(513);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + ((uint32)core.summon_cancelable << 16), min + (min << 16));
		return FALSE;
	}
	case 8: {
		if(returns.ivalue[0] == -1) {
			pduel->lua->add_param((void*)0, PARAM_TYPE_GROUP);
			return TRUE;
		}
		int32 pv = 0;
		for(int32 i = 0; i < returns.bvalue[0]; ++i) {
			card* pcard = core.select_cards[returns.bvalue[i + 1]];
			core.operated_set.insert(pcard);
			for(auto iter = core.xmaterial_lst.begin(); iter != core.xmaterial_lst.end(); ++iter) {
				if(iter->second == pcard) {
					if(pv < iter->first)
						pv = iter->first;
					core.xmaterial_lst.erase(iter);
					break;
				}
			}
		}
		max -= returns.bvalue[0];
		if(max == 0 || core.xmaterial_lst.size() == 0) {
			group* pgroup = pduel->new_group(core.operated_set);
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			return TRUE;
		}
		min = 0;
		if(pv - (int32)core.operated_set.size() > min)
			min = pv - (int32)core.operated_set.size();
		core.units.begin()->arg2 = min + (max << 16);
		if(min == 0) {
			add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, playerid, 93);
			return FALSE;
		}
		returns.ivalue[0] = 1;
		return FALSE;
	}
	case 9: {
		if(!returns.ivalue[0]) {
			group* pgroup = pduel->new_group(core.operated_set);
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			return TRUE;
		}
		core.select_cards.clear();
		for(auto iter = core.xmaterial_lst.begin(); iter != core.xmaterial_lst.end(); ++iter)
			core.select_cards.push_back(iter->second);
		int32 maxv = core.xmaterial_lst.begin()->first;
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(513);
		if(min == 0)
			min = 1;
		if(min + (int32)core.operated_set.size() >= maxv)
			add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid, min + (max << 16));
		else {
			add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid, min + (min << 16));
			core.units.begin()->step = 7;
		}
		return FALSE;
	}
	case 10: {
		group* pgroup = pduel->new_group(core.operated_set);
		for(int32 i = 0; i < returns.bvalue[0]; ++i) {
			card* pcard = core.select_cards[returns.bvalue[i + 1]];
			pgroup->container.insert(pcard);
		}
		pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
		return TRUE;
	}
	case 20: {
		int32 ct = get_spsummonable_count(scard, playerid);
		if(ct > 0)
			return FALSE;
		card_set handover_zone_cards;
		uint32 must_use_zone_flag = 0;
		filter_must_use_mzone(playerid, playerid, LOCATION_REASON_TOFIELD, scard, &must_use_zone_flag);
		uint32 handover_zone = get_rule_zone_fromex(playerid, scard) & ~must_use_zone_flag;
		get_cards_in_zone(&handover_zone_cards, handover_zone, playerid, LOCATION_MZONE);
		int32 ft = ct + (int32)std::count_if(core.operated_set.begin(), core.operated_set.end(),
			[=](card* pcard) { return handover_zone_cards.find(pcard) != handover_zone_cards.end(); });
		if(ft > 0)
			return FALSE;
		core.select_cards.clear();
		for(auto iter = core.xmaterial_lst.begin(); iter != core.xmaterial_lst.end(); ++iter) {
			card* pcard = iter->second;
			if(handover_zone_cards.find(pcard) != handover_zone_cards.end())
				core.select_cards.push_back(pcard);
		}
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(513);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + ((uint32)core.summon_cancelable << 16), 0x10001);
		core.units.begin()->step = 21;
		return FALSE;
	}
	case 21: {
		core.select_cards.clear();
		for(auto iter = core.xmaterial_lst.begin(); iter != core.xmaterial_lst.end(); ++iter)
			core.select_cards.push_back(iter->second);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(513);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, playerid + ((uint32)core.summon_cancelable << 16), 0x10001);
		return FALSE;
	}
	case 22: {
		if(returns.ivalue[0] == -1) {
			pduel->lua->add_param((void*)0, PARAM_TYPE_GROUP);
			return TRUE;
		}
		int32 pv = 0;
		card* pcard = core.select_cards[returns.bvalue[1]];
		core.operated_set.insert(pcard);
		int32 selectable = core.units.begin()->arg3 & pcard->sum_param;
		for(auto iter = core.xmaterial_lst.begin(); iter != core.xmaterial_lst.end();) {
			if(iter->second == pcard) {
				pv = iter->first;
				iter = core.xmaterial_lst.erase(iter);
			} else if(!(iter->second->sum_param & selectable))
				iter = core.xmaterial_lst.erase(iter);
			else
				++iter;
		}
		max--;
		if(max == 0 || core.xmaterial_lst.size() == 0) {
			group* pgroup = pduel->new_group(core.operated_set);
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			return TRUE;
		}
		if(min > 0)
			min--;
		if(pv - (int32)core.operated_set.size() > min)
			min = pv - (int32)core.operated_set.size();
		core.units.begin()->arg2 = min + (max << 16);
		core.units.begin()->arg3 = selectable;
		if(min == 0) {
			add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, playerid, 93);
			return FALSE;
		}
		core.units.begin()->step = 20;
		return FALSE;
	}
	case 23: {
		if(!returns.ivalue[0]) {
			group* pgroup = pduel->new_group(core.operated_set);
			pduel->lua->add_param(pgroup, PARAM_TYPE_GROUP);
			return TRUE;
		}
		core.units.begin()->step = 20;
		return FALSE;
	}
	}
	return TRUE;
}
int32 field::select_release_cards(int16 step, uint8 playerid, uint8 cancelable, int32 min, int32 max) {
	switch(step) {
	case 0: {
		core.operated_set.clear();
		if(core.release_cards_ex.empty()) {
			core.units.begin()->step = 1;
			return FALSE;
		}
		core.select_cards.clear();
		for(auto& pcard : core.release_cards_ex)
			core.select_cards.push_back(pcard);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(500);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, ((uint32)cancelable << 16) + playerid, (max << 16) + min);
		return FALSE;
	}
	case 1: {
		int32 count = returns.bvalue[0];
		max -= count;
		if(max == 0 || core.release_cards.size() + core.release_cards_ex_oneof.size() == 0)
			return TRUE;
		for(int32 i = 0; i < count; ++i)
			core.operated_set.insert(core.select_cards[returns.bvalue[i + 1]]);
		min -= count;
		if(min < 0)
			min = 0;
		core.units.begin()->arg2 = (max << 16) + min;
		return FALSE;
	}
	case 2: {
		if(!core.release_cards_ex_oneof.empty()) {
			if((int32)core.release_cards.size() < min)
				returns.ivalue[0] = TRUE;
			else
				add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, playerid, 98);
		} else
			returns.ivalue[0] = FALSE;
		return FALSE;
	}
	case 3: {
		if(!returns.ivalue[0]) {
			core.units.begin()->step = 4;
			return FALSE;
		}
		core.select_cards.clear();
		for(auto& pcard : core.release_cards_ex_oneof)
			core.select_cards.push_back(pcard);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(500);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, ((uint32)cancelable << 16) + playerid, 0x10001);
		return FALSE;
	}
	case 4: {
		card* pcard = core.select_cards[returns.bvalue[1]];
		core.operated_set.insert(pcard);
		effect* peffect = pcard->is_affected_by_effect(EFFECT_EXTRA_RELEASE_NONSUM);
		core.dec_count_reserve.push_back(peffect);
		max--;
		if(max == 0 || core.release_cards.empty()) {
			core.units.begin()->step = 6;
			return FALSE;
		}
		min--;
		if(min < 0)
			min = 0;
		core.units.begin()->arg2 = (max << 16) + min;
		return FALSE;
	}
	case 5: {
		core.select_cards.clear();
		for(auto& pcard : core.release_cards)
			core.select_cards.push_back(pcard);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(500);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, ((uint32)cancelable << 16) + playerid, (max << 16) + min);
		return FALSE;
	}
	case 6: {
		for(int32 i = 0; i < returns.bvalue[0]; ++i)
			core.operated_set.insert(core.select_cards[returns.bvalue[i + 1]]);
		return FALSE;
	}
	case 7: {
		core.select_cards.clear();
		returns.bvalue[0] = (int8)core.operated_set.size();
		int32 i = 0;
		for(auto cit = core.operated_set.begin(); cit != core.operated_set.end(); ++cit, ++i) {
			core.select_cards.push_back(*cit);
			returns.bvalue[i + 1] = i;
		}
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::select_tribute_cards(int16 step, card* target, uint8 playerid, uint8 cancelable, int32 min, int32 max, uint8 toplayer, uint32 zone) {
	switch(step) {
	case 0: {
		core.operated_set.clear();
		zone &= 0x1f;
		int32 ct = get_tofield_count(target, toplayer, LOCATION_MZONE, playerid, LOCATION_REASON_TOFIELD, zone);
		if(ct > 0) {
			returns.ivalue[0] = TRUE;
			core.units.begin()->step = 1;
			return FALSE;
		}
		int32 rmax = 0;
		core.select_cards.clear();
		for(auto& pcard : core.release_cards) {
			if(pcard->current.location == LOCATION_MZONE && pcard->current.controler == toplayer && ((zone >> pcard->current.sequence) & 1))
				core.select_cards.push_back(pcard);
			else
				rmax += pcard->release_param;
		}
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(500);
		if(core.release_cards_ex.empty() && core.release_cards_ex_oneof.empty() && min > rmax) {
			if(rmax > 0) {
				core.select_cards.clear();
				for(auto& pcard : core.release_cards)
					core.select_cards.push_back(pcard);
			}
			add_process(PROCESSOR_SELECT_TRIBUTE_P, 0, 0, 0, ((uint32)cancelable << 16) + playerid, (max << 16) + min);
			return TRUE;
		}
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, ((uint32)cancelable << 16) + playerid, 0x10001);
		return FALSE;
	}
	case 1: {
		if(returns.ivalue[0] == -1)
			return TRUE;
		card* pcard = core.select_cards[returns.bvalue[1]];
		core.operated_set.insert(pcard);
		core.release_cards.erase(pcard);
		if(min <= (int32)pcard->release_param) {
			if(max > 1 && core.release_cards_ex.empty() && (!core.release_cards.empty() || !core.release_cards_ex_oneof.empty()))
				add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, playerid, 210);
			else if(max > 1 && !core.release_cards_ex.empty())
				returns.ivalue[0] = TRUE;
			else
				core.units.begin()->step = 8;
		} else
			returns.ivalue[0] = TRUE;
		return FALSE;
	}
	case 2: {
		if(!returns.ivalue[0]) {
			core.units.begin()->step = 8;
			return FALSE;
		}
		int32 fcount = get_mzone_limit(toplayer, playerid, LOCATION_REASON_TOFIELD);
		if(!core.operated_set.empty()) {
			min -= (*core.operated_set.begin())->release_param;
			max--;
			fcount++;
		}
		if(core.release_cards_ex.size() + core.release_cards_ex_oneof.size() == 0
		        || (fcount <= 0 && min < 2)) {
			core.select_cards.clear();
			for(auto& pcard : core.release_cards)
				core.select_cards.push_back(pcard);
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_SELECTMSG);
			pduel->write_buffer8(playerid);
			pduel->write_buffer32(500);
			add_process(PROCESSOR_SELECT_TRIBUTE_P, 0, 0, 0, ((uint32)cancelable << 16) + playerid, (max << 16) + min);
			core.units.begin()->step = 7;
			return FALSE;
		}
		if(core.release_cards_ex.size() >= (uint32)max) {
			core.select_cards.clear();
			for(auto& pcard : core.release_cards_ex)
				core.select_cards.push_back(pcard);
			pduel->write_buffer8(MSG_HINT);
			pduel->write_buffer8(HINT_SELECTMSG);
			pduel->write_buffer8(playerid);
			pduel->write_buffer32(500);
			add_process(PROCESSOR_SELECT_TRIBUTE_P, 0, 0, 0, ((uint32)cancelable << 16) + playerid, (max << 16) + min);
			core.units.begin()->step = 7;
			return FALSE;
		}
		int32 rmax = 0;
		for(auto& pcard : core.release_cards)
			rmax += pcard->release_param;
		for(auto& pcard : core.release_cards_ex)
			rmax += pcard->release_param;
		core.temp_var[0] = 0;
		if(rmax < min) {
			returns.ivalue[0] = TRUE;
			if(rmax == 0 && min == 2)
				core.temp_var[0] = 1;
		} else if(!core.release_cards_ex_oneof.empty())
			add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, playerid, 92);
		else
			core.units.begin()->step = 4;
		return FALSE;
	}
	case 3: {
		if(!returns.ivalue[0]) {
			core.units.begin()->step = 4;
			return FALSE;
		}
		core.select_cards.clear();
		if(core.temp_var[0] == 0)
			for(auto& pcard : core.release_cards_ex_oneof)
				core.select_cards.push_back(pcard);
		else
			for(auto& pcard : core.release_cards_ex_oneof)
				if(pcard->release_param == 2)
					core.select_cards.push_back(pcard);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(500);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, ((uint32)cancelable << 16) + playerid, 0x10001);
		return FALSE;
	}
	case 4: {
		if(returns.ivalue[0] == -1)
			return TRUE;
		card* pcard = core.select_cards[returns.bvalue[1]];
		core.operated_set.insert(pcard);
		core.units.begin()->peffect = pcard->is_affected_by_effect(EFFECT_EXTRA_RELEASE_SUM);
		return FALSE;
	}
	case 5: {
		core.select_cards.clear();
		for(auto& pcard : core.release_cards_ex)
			core.select_cards.push_back(pcard);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(500);
		add_process(PROCESSOR_SELECT_CARD, 0, 0, 0, ((uint32)cancelable << 16) + playerid, (core.release_cards_ex.size() << 16) + core.release_cards_ex.size());
		return FALSE;
	}
	case 6: {
		if(returns.ivalue[0] == -1)
			return TRUE;
		for(int32 i = 0; i < returns.bvalue[0]; ++i)
			core.operated_set.insert(core.select_cards[returns.bvalue[i + 1]]);
		uint32 rmin = (uint32)core.operated_set.size();
		uint32 rmax = 0;
		for(auto& pcard : core.operated_set)
			rmax += pcard->release_param;
		min -= rmax;
		max -= rmin;
		min = min > 0 ? min : 0;
		max = max > 0 ? max : 0;
		core.units.begin()->arg2 = (max << 16) + min;
		if(min <= 0) {
			if(max > 0 && !core.release_cards.empty())
				add_process(PROCESSOR_SELECT_YESNO, 0, 0, 0, playerid, 210);
			else
				core.units.begin()->step = 8;
		} else
			returns.ivalue[0] = TRUE;
		return FALSE;
	}
	case 7: {
		if(!returns.ivalue[0]) {
			core.units.begin()->step = 8;
			return FALSE;
		}
		core.select_cards.clear();
		for(auto& pcard : core.release_cards)
			core.select_cards.push_back(pcard);
		pduel->write_buffer8(MSG_HINT);
		pduel->write_buffer8(HINT_SELECTMSG);
		pduel->write_buffer8(playerid);
		pduel->write_buffer32(500);
		add_process(PROCESSOR_SELECT_TRIBUTE_P, 0, 0, 0, ((uint32)cancelable << 16) + playerid, (max << 16) + min);
		return FALSE;
	}
	case 8: {
		if(returns.ivalue[0] == -1)
			return TRUE;
		for(int32 i = 0; i < returns.bvalue[0]; ++i)
			core.operated_set.insert(core.select_cards[returns.bvalue[i + 1]]);
		return FALSE;
	}
	case 9: {
		core.select_cards.clear();
		returns.bvalue[0] = (int8)core.operated_set.size();
		int32 i = 0;
		for(auto cit = core.operated_set.begin(); cit != core.operated_set.end(); ++cit, ++i) {
			core.select_cards.push_back(*cit);
			returns.bvalue[i + 1] = i;
		}
		effect* peffect = core.units.begin()->peffect;
		if(peffect)
			peffect->dec_count();
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::toss_coin(uint16 step, effect * reason_effect, uint8 reason_player, uint8 playerid, uint8 count) {
	switch(step) {
	case 0: {
		effect_set eset;
		effect* peffect = 0;
		tevent e;
		e.event_cards = 0;
		e.event_player = playerid;
		e.event_value = count;
		e.reason = 0;
		e.reason_effect = reason_effect;
		e.reason_player = reason_player;
		for(uint8 i = 0; i < 5; ++i)
			core.coin_result[i] = 0;
		auto pr = effects.continuous_effect.equal_range(EFFECT_TOSS_COIN_REPLACE);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* pe = eit->second;
			++eit;
			if(pe->is_activateable(pe->get_handler_player(), e)) {
				peffect = pe;
				break;
			}
		}
		if(!peffect) {
			pduel->write_buffer8(MSG_TOSS_COIN);
			pduel->write_buffer8(playerid);
			pduel->write_buffer8(count);
			for(int32 i = 0; i < count; ++i) {
				core.coin_result[i] = pduel->get_next_integer(0, 1);
				pduel->write_buffer8(core.coin_result[i]);
			}
			raise_event((card*)0, EVENT_TOSS_COIN_NEGATE, reason_effect, 0, reason_player, playerid, count);
			process_instant_event();
		} else {
			solve_continuous(peffect->get_handler_player(), peffect, e);
			//call Duel.SetCoinResult in operation if necessary
			return TRUE;
		}
		return FALSE;
	}
	case 1: {
		raise_event((card*)0, EVENT_TOSS_COIN, reason_effect, 0, reason_player, playerid, count);
		process_instant_event();
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::toss_dice(uint16 step, effect * reason_effect, uint8 reason_player, uint8 playerid, uint8 count1, uint8 count2) {
	switch(step) {
	case 0: {
		effect_set eset;
		effect* peffect = 0;
		tevent e;
		e.event_cards = 0;
		e.event_player = playerid;
		e.event_value = count1 + (count2 << 16);
		e.reason = 0;
		e.reason_effect = reason_effect;
		e.reason_player = reason_player;
		for(int32 i = 0; i < 5; ++i)
			core.dice_result[i] = 0;
		auto pr = effects.continuous_effect.equal_range(EFFECT_TOSS_DICE_REPLACE);
		for(auto eit = pr.first; eit != pr.second;) {
			effect* pe = eit->second;
			++eit;
			if(pe->is_activateable(pe->get_handler_player(), e)) {
				peffect = pe;
				break;
			}
		}
		if(!peffect) {
			pduel->write_buffer8(MSG_TOSS_DICE);
			pduel->write_buffer8(playerid);
			pduel->write_buffer8(count1);
			for(int32 i = 0; i < count1; ++i) {
				core.dice_result[i] = pduel->get_next_integer(1, 6);
				pduel->write_buffer8(core.dice_result[i]);
			}
			if(count2 > 0) {
				pduel->write_buffer8(MSG_TOSS_DICE);
				pduel->write_buffer8(1 - playerid);
				pduel->write_buffer8(count2);
				for(int32 i = 0; i < count2; ++i) {
					core.dice_result[count1 + i] = pduel->get_next_integer(1, 6);
					pduel->write_buffer8(core.dice_result[count1 + i]);
				}
			}
			raise_event((card*)0, EVENT_TOSS_DICE_NEGATE, reason_effect, 0, reason_player, playerid, count1 + (count2 << 16));
			process_instant_event();
		} else {
			solve_continuous(peffect->get_handler_player(), peffect, e);
			//call Duel.SetDiceResult in operation if necessary
			return TRUE;
		}
		return FALSE;
	}
	case 1: {
		raise_event((card*)0, EVENT_TOSS_DICE, reason_effect, 0, reason_player, playerid, count1 + (count2 << 16));
		process_instant_event();
		return TRUE;
	}
	}
	return TRUE;
}
int32 field::rock_paper_scissors(uint16 step, uint8 repeat) {
	switch (step) {
	case 0: {
		pduel->write_buffer8(MSG_ROCK_PAPER_SCISSORS);
		pduel->write_buffer8(0);
		return FALSE;
	}
	case 1: {
		core.units.begin()->arg2 = returns.ivalue[0];
		pduel->write_buffer8(MSG_ROCK_PAPER_SCISSORS);
		pduel->write_buffer8(1);
		return FALSE;
	}
	case 2: {
		int32 hand0 = (int32)core.units.begin()->arg2;
		int32 hand1 = returns.ivalue[0];
		pduel->write_buffer8(MSG_HAND_RES);
		pduel->write_buffer8(hand0 + (hand1 << 2));
		if(hand0 == hand1) {
			if(repeat) {
				core.units.begin()->step = -1;
				return FALSE;
			} else
				returns.ivalue[0] = PLAYER_NONE;
		} else if((hand0 == 1 && hand1 == 2) || (hand0 == 2 && hand1 == 3) || (hand0 == 3 && hand1 == 1)) {
			returns.ivalue[0] = 1;
		} else {
			returns.ivalue[0] = 0;
		}
		return TRUE;
	}
	}
	return TRUE;
}
