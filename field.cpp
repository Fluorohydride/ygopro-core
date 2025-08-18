/*
 * field.cpp
 *
 *  Created on: 2010-7-21
 *      Author: Argon
 */

#include "field.h"
#include "duel.h"
#include "card.h"
#include "group.h"
#include "effect.h"
#include "interpreter.h"
#include <cstring>
#include <algorithm>

int32_t field::field_used_count[32] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5};

bool chain::chain_operation_sort(const chain& c1, const chain& c2) {
	auto e1 = c1.triggering_effect;
	auto e2 = c2.triggering_effect;
	if (e1 && e2) {
		if (e1->handler == e2->handler && e1->description != e2->description)
			return e1->description < e2->description;
		if (e1->id != e2->id)
			return e1->id < e2->id;
	}
	return c1.chain_id < c2.chain_id;
}
void chain::set_triggering_state(card* pcard) {
	triggering_controler = pcard->current.controler;
	if(pcard->current.is_location(LOCATION_FZONE))
		triggering_location = LOCATION_SZONE | LOCATION_FZONE;
	else if(pcard->current.is_location(LOCATION_PZONE))
		triggering_location = LOCATION_SZONE | LOCATION_PZONE;
	else
		triggering_location = pcard->current.location;
	triggering_sequence = pcard->current.sequence;
	triggering_position = pcard->current.position;
	triggering_state.code = pcard->get_code();
	triggering_state.code2 = pcard->get_another_code();
	triggering_state.level = pcard->get_level();
	triggering_state.rank = pcard->get_rank();
	triggering_state.attribute = pcard->get_attribute();
	triggering_state.race = pcard->get_race();
	std::pair<int32_t, int32_t> atk_def = pcard->get_atk_def();
	triggering_state.attack = atk_def.first;
	triggering_state.defense = atk_def.second;
}
bool tevent::operator< (const tevent& v) const {
	return std::memcmp(this, &v, sizeof(tevent)) < 0;
}
field::field(duel* pd)
	: pduel(pd) {
	for (int32_t i = 0; i < 2; ++i) {
		player[i].list_mzone.resize(7, 0);
		player[i].list_szone.resize(8, 0);
		player[i].list_main.reserve(60);
		player[i].list_hand.reserve(60);
		player[i].list_grave.reserve(75);
		player[i].list_remove.reserve(75);
		player[i].list_extra.reserve(30);
	}
}
void field::reload_field_info() {
	pduel->write_buffer8(MSG_RELOAD_FIELD);
	pduel->write_buffer8(core.duel_rule);
	for(int32_t playerid = 0; playerid < 2; ++playerid) {
		pduel->write_buffer32(player[playerid].lp);
		for(auto& pcard : player[playerid].list_mzone) {
			if(pcard) {
				pduel->write_buffer8(1);
				pduel->write_buffer8(pcard->current.position);
				pduel->write_buffer8((uint8_t)pcard->xyz_materials.size());
			} else {
				pduel->write_buffer8(0);
			}
		}
		for(auto& pcard : player[playerid].list_szone) {
			if(pcard) {
				pduel->write_buffer8(1);
				pduel->write_buffer8(pcard->current.position);
			} else {
				pduel->write_buffer8(0);
			}
		}
		pduel->write_buffer8((uint8_t)player[playerid].list_main.size());
		pduel->write_buffer8((uint8_t)player[playerid].list_hand.size());
		pduel->write_buffer8((uint8_t)player[playerid].list_grave.size());
		pduel->write_buffer8((uint8_t)player[playerid].list_remove.size());
		pduel->write_buffer8((uint8_t)player[playerid].list_extra.size());
		pduel->write_buffer8((uint8_t)player[playerid].extra_p_count);
	}
	pduel->write_buffer8((uint8_t)core.current_chain.size());
	for(const auto& ch : core.current_chain) {
		effect* peffect = ch.triggering_effect;
		pduel->write_buffer32(peffect->get_handler()->data.code);
		pduel->write_buffer32(peffect->get_handler()->get_info_location());
		pduel->write_buffer8(ch.triggering_controler);
		pduel->write_buffer8((uint8_t)ch.triggering_location);
		pduel->write_buffer8(ch.triggering_sequence);
		pduel->write_buffer32(peffect->description);
	}
}
void field::add_card(uint8_t playerid, card* pcard, uint8_t location, uint8_t sequence, uint8_t pzone) {
	if (pcard->current.location != 0)
		return;
	if (!is_location_useable(playerid, location, sequence))
		return;
	if(pcard->is_extra_deck_monster() && (location & (LOCATION_HAND | LOCATION_DECK))) {
		location = LOCATION_EXTRA;
		pcard->sendto_param.position = POS_FACEDOWN_DEFENSE;
	}
	pcard->current.controler = playerid;
	pcard->current.location = location;
	switch (location) {
	case LOCATION_MZONE: {
		player[playerid].list_mzone[sequence] = pcard;
		pcard->current.sequence = sequence;
		break;
	}
	case LOCATION_SZONE: {
		player[playerid].list_szone[sequence] = pcard;
		pcard->current.sequence = sequence;
		break;
	}
	case LOCATION_DECK: {
		if (sequence == SEQ_DECKTOP) {
			player[playerid].list_main.push_back(pcard);
			pcard->current.sequence = (uint8_t)player[playerid].list_main.size() - 1;
		} else if (sequence == SEQ_DECKBOTTOM) {
			player[playerid].list_main.insert(player[playerid].list_main.begin(), pcard);
			reset_sequence(playerid, LOCATION_DECK);
		} else { // SEQ_DECKSHUFFLE
			if(core.duel_options & DUEL_RETURN_DECK_TOP) {
				player[playerid].list_main.push_back(pcard);
				pcard->current.sequence = (uint8_t)player[playerid].list_main.size() - 1;
			}
			else {
				player[playerid].list_main.insert(player[playerid].list_main.begin(), pcard);
				reset_sequence(playerid, LOCATION_DECK);
			}
			if(!core.shuffle_check_disabled)
				core.shuffle_deck_check[playerid] = TRUE;
		}
		pcard->sendto_param.position = POS_FACEDOWN;
		break;
	}
	case LOCATION_HAND: {
		player[playerid].list_hand.push_back(pcard);
		pcard->current.sequence = (uint8_t)player[playerid].list_hand.size() - 1;
		uint32_t pos = pcard->is_affected_by_effect(EFFECT_PUBLIC) ? POS_FACEUP : POS_FACEDOWN;
		pcard->sendto_param.position = pos;
		if(!(pcard->current.reason & REASON_DRAW) && !core.shuffle_check_disabled)
			core.shuffle_hand_check[playerid] = TRUE;
		break;
	}
	case LOCATION_GRAVE: {
		player[playerid].list_grave.push_back(pcard);
		pcard->current.sequence = (uint8_t)player[playerid].list_grave.size() - 1;
		break;
	}
	case LOCATION_REMOVED: {
		player[playerid].list_remove.push_back(pcard);
		pcard->current.sequence = (uint8_t)player[playerid].list_remove.size() - 1;
		break;
	}
	case LOCATION_EXTRA: {
		if(player[playerid].extra_p_count == 0 || (pcard->data.type & TYPE_PENDULUM) && (pcard->sendto_param.position & POS_FACEUP))
			player[playerid].list_extra.push_back(pcard);
		else
			player[playerid].list_extra.insert(player[playerid].list_extra.end() - player[playerid].extra_p_count, pcard);
		if((pcard->data.type & TYPE_PENDULUM) && (pcard->sendto_param.position & POS_FACEUP))
			++player[playerid].extra_p_count;
		reset_sequence(playerid, LOCATION_EXTRA);
		break;
	}
	}
	if(pzone)
		pcard->current.pzone = true;
	else
		pcard->current.pzone = false;
	pcard->apply_field_effect();
	pcard->fieldid = infos.field_id++;
	pcard->fieldid_r = pcard->fieldid;
	pcard->activate_count_id = pcard->fieldid;
	if(check_unique_onfield(pcard, pcard->current.controler, pcard->current.location))
		pcard->unique_fieldid = UINT_MAX;
	pcard->turnid = infos.turn_id;
	refresh_player_info(playerid);
}
void field::remove_card(card* pcard) {
	if (!check_playerid(pcard->current.controler) || pcard->current.location == 0)
		return;
	uint8_t playerid = pcard->current.controler;
	switch (pcard->current.location) {
	case LOCATION_MZONE:
		player[playerid].list_mzone[pcard->current.sequence] = 0;
		break;
	case LOCATION_SZONE:
		player[playerid].list_szone[pcard->current.sequence] = 0;
		break;
	case LOCATION_DECK:
		player[playerid].list_main.erase(player[playerid].list_main.begin() + pcard->current.sequence);
		reset_sequence(playerid, LOCATION_DECK);
		if(!core.shuffle_check_disabled)
			core.shuffle_deck_check[playerid] = TRUE;
		break;
	case LOCATION_HAND:
		pcard->set_status(STATUS_TO_HAND_WITHOUT_CONFIRM, FALSE);
		player[playerid].list_hand.erase(player[playerid].list_hand.begin() + pcard->current.sequence);
		reset_sequence(playerid, LOCATION_HAND);
		break;
	case LOCATION_GRAVE:
		player[playerid].list_grave.erase(player[playerid].list_grave.begin() + pcard->current.sequence);
		reset_sequence(playerid, LOCATION_GRAVE);
		break;
	case LOCATION_REMOVED:
		player[playerid].list_remove.erase(player[playerid].list_remove.begin() + pcard->current.sequence);
		reset_sequence(playerid, LOCATION_REMOVED);
		break;
	case LOCATION_EXTRA:
		player[playerid].list_extra.erase(player[playerid].list_extra.begin() + pcard->current.sequence);
		reset_sequence(playerid, LOCATION_EXTRA);
		if((pcard->data.type & TYPE_PENDULUM) && (pcard->current.position & POS_FACEUP))
			--player[playerid].extra_p_count;
		break;
	}
	pcard->cancel_field_effect();
	refresh_player_info(playerid);
	pcard->previous.controler = pcard->current.controler;
	pcard->previous.location = pcard->current.location;
	pcard->previous.sequence = pcard->current.sequence;
	pcard->previous.position = pcard->current.position;
	pcard->previous.pzone = pcard->current.pzone;
	pcard->current.controler = PLAYER_NONE;
	pcard->current.location = 0;
	pcard->current.sequence = 0;
}
void field::move_card(uint8_t playerid, card* pcard, uint8_t location, uint8_t sequence, uint8_t pzone) {
	if (!is_location_useable(playerid, location, sequence))
		return;
	uint8_t preplayer = pcard->current.controler;
	uint8_t presequence = pcard->current.sequence;
	if(pcard->is_extra_deck_monster() && (location & (LOCATION_HAND | LOCATION_DECK))) {
		location = LOCATION_EXTRA;
		pcard->sendto_param.position = POS_FACEDOWN_DEFENSE;
	}
	if (pcard->current.location) {
		if (pcard->current.location == location && pcard->current.pzone == !!pzone) {
			if (pcard->current.location == LOCATION_DECK) {
				if(preplayer == playerid) {
					pduel->write_buffer8(MSG_MOVE);
					pduel->write_buffer32(pcard->data.code);
					pduel->write_buffer32(pcard->get_info_location());
					player[preplayer].list_main.erase(player[preplayer].list_main.begin() + pcard->current.sequence);
					if (sequence == SEQ_DECKTOP) {
						player[playerid].list_main.push_back(pcard);
					} else if (sequence == SEQ_DECKBOTTOM) {
						player[playerid].list_main.insert(player[playerid].list_main.begin(), pcard);
					} else { // SEQ_DECKSHUFFLE
						if(core.duel_options & DUEL_RETURN_DECK_TOP)
							player[playerid].list_main.push_back(pcard);
						else
							player[playerid].list_main.insert(player[playerid].list_main.begin(), pcard);
						if(!core.shuffle_check_disabled)
							core.shuffle_deck_check[playerid] = TRUE;
					}
					reset_sequence(playerid, LOCATION_DECK);
					pcard->previous.controler = preplayer;
					pcard->current.controler = playerid;
					pduel->write_buffer32(pcard->get_info_location());
					pduel->write_buffer32(pcard->current.reason);
					return;
				} else
					remove_card(pcard);
			} else if(location & LOCATION_ONFIELD) {
				if (playerid == preplayer && sequence == presequence)
					return;
				if(location == LOCATION_MZONE) {
					if(sequence >= (int32_t)player[playerid].list_mzone.size() || player[playerid].list_mzone[sequence])
						return;
				} else {
					if(sequence >= player[playerid].szone_size || player[playerid].list_szone[sequence])
						return;
				}
				if(preplayer == playerid) {
					pduel->write_buffer8(MSG_MOVE);
					pduel->write_buffer32(pcard->data.code);
					pduel->write_buffer32(pcard->get_info_location());
				}
				pcard->previous.controler = pcard->current.controler;
				pcard->previous.location = pcard->current.location;
				pcard->previous.sequence = pcard->current.sequence;
				pcard->previous.position = pcard->current.position;
				pcard->previous.pzone = pcard->current.pzone;
				if (location == LOCATION_MZONE) {
					player[preplayer].list_mzone[presequence] = 0;
					player[playerid].list_mzone[sequence] = pcard;
					pcard->current.controler = playerid;
					pcard->current.sequence = sequence;
				} else {
					player[preplayer].list_szone[presequence] = 0;
					player[playerid].list_szone[sequence] = pcard;
					pcard->current.controler = playerid;
					pcard->current.sequence = sequence;
				}
				if(preplayer == playerid) {
					refresh_player_info(playerid);
					pduel->write_buffer32(pcard->get_info_location());
					pduel->write_buffer32(pcard->current.reason);
				} else {
					refresh_player_info(preplayer);
					refresh_player_info(playerid);
					pcard->fieldid = infos.field_id++;
					if(check_unique_onfield(pcard, pcard->current.controler, pcard->current.location))
						pcard->unique_fieldid = UINT_MAX;
				}
				return;
			} else if(location == LOCATION_HAND) {
				if(preplayer == playerid)
					return;
				remove_card(pcard);
			} else {
				if(location == LOCATION_GRAVE) {
					if(pcard->current.sequence == player[pcard->current.controler].list_grave.size() - 1)
						return;
					pduel->write_buffer8(MSG_MOVE);
					pduel->write_buffer32(pcard->data.code);
					pduel->write_buffer32(pcard->get_info_location());
					player[pcard->current.controler].list_grave.erase(player[pcard->current.controler].list_grave.begin() + pcard->current.sequence);
					player[pcard->current.controler].list_grave.push_back(pcard);
					reset_sequence(pcard->current.controler, LOCATION_GRAVE);
					pduel->write_buffer32(pcard->get_info_location());
					pduel->write_buffer32(pcard->current.reason);
				} else if(location == LOCATION_REMOVED) {
					if(pcard->current.sequence == player[pcard->current.controler].list_remove.size() - 1)
						return;
					pduel->write_buffer8(MSG_MOVE);
					pduel->write_buffer32(pcard->data.code);
					pduel->write_buffer32(pcard->get_info_location());
					player[pcard->current.controler].list_remove.erase(player[pcard->current.controler].list_remove.begin() + pcard->current.sequence);
					player[pcard->current.controler].list_remove.push_back(pcard);
					reset_sequence(pcard->current.controler, LOCATION_REMOVED);
					pduel->write_buffer32(pcard->get_info_location());
					pduel->write_buffer32(pcard->current.reason);
				} else {
					pduel->write_buffer8(MSG_MOVE);
					pduel->write_buffer32(pcard->data.code);
					pduel->write_buffer32(pcard->get_info_location());
					player[pcard->current.controler].list_extra.erase(player[pcard->current.controler].list_extra.begin() + pcard->current.sequence);
					player[pcard->current.controler].list_extra.push_back(pcard);
					reset_sequence(pcard->current.controler, LOCATION_EXTRA);
					pduel->write_buffer32(pcard->get_info_location());
					pduel->write_buffer32(pcard->current.reason);
				}
				return;
			}
		} else {
			if((pcard->data.type & TYPE_PENDULUM) && (location == LOCATION_GRAVE)
			        && pcard->is_capable_send_to_extra(playerid)
			        && (((pcard->current.location == LOCATION_MZONE) && !pcard->is_status(STATUS_SUMMON_DISABLED))
			        || ((pcard->current.location == LOCATION_SZONE) && !pcard->is_status(STATUS_ACTIVATE_DISABLED)))) {
				location = LOCATION_EXTRA;
				pcard->sendto_param.position = POS_FACEUP_DEFENSE;
			}
			remove_card(pcard);
		}
	}
	add_card(playerid, pcard, location, sequence, pzone);
}
void field::swap_card(card* pcard1, card* pcard2, uint8_t new_sequence1, uint8_t new_sequence2) {
	uint8_t p1 = pcard1->current.controler, p2 = pcard2->current.controler;
	uint8_t l1 = pcard1->current.location, l2 = pcard2->current.location;
	uint8_t s1 = pcard1->current.sequence, s2 = pcard2->current.sequence;
	uint32_t info1 = pcard1->get_info_location(), info2 = pcard2->get_info_location();
	if(!(l1 & LOCATION_ONFIELD) || !(l2 & LOCATION_ONFIELD))
		return;
	if(new_sequence1 != s1 && !is_location_useable(p1, l1, new_sequence1)
		|| new_sequence2 != s2 && !is_location_useable(p2, l2, new_sequence2))
		return;
	if(p1 == p2 && l1 == l2 && (new_sequence1 == s2 || new_sequence2 == s1))
		return;
	if(l1 == l2) {
		pcard1->previous.controler = p1;
		pcard1->previous.location = l1;
		pcard1->previous.sequence = s1;
		pcard1->previous.position = pcard1->current.position;
		pcard1->previous.pzone = pcard1->current.pzone;
		pcard1->current.controler = p2;
		pcard1->current.location = l2;
		pcard1->current.sequence = new_sequence2;
		pcard2->previous.controler = p2;
		pcard2->previous.location = l2;
		pcard2->previous.sequence = s2;
		pcard2->previous.position = pcard2->current.position;
		pcard2->previous.pzone = pcard2->current.pzone;
		pcard2->current.controler = p1;
		pcard2->current.location = l1;
		pcard2->current.sequence = new_sequence1;
		if(p1 != p2) {
			pcard1->fieldid = infos.field_id++;
			pcard2->fieldid = infos.field_id++;
			if(check_unique_onfield(pcard1, pcard1->current.controler, pcard1->current.location))
				pcard1->unique_fieldid = UINT_MAX;
			if(check_unique_onfield(pcard2, pcard2->current.controler, pcard2->current.location))
				pcard2->unique_fieldid = UINT_MAX;
		}
		if(l1 == LOCATION_MZONE) {
			player[p1].list_mzone[s1] = 0;
			player[p2].list_mzone[s2] = 0;
			player[p2].list_mzone[new_sequence2] = pcard1;
			player[p1].list_mzone[new_sequence1] = pcard2;
			refresh_player_info(p1);
			refresh_player_info(p2);
		} else if(l1 == LOCATION_SZONE) {
			player[p1].list_szone[s1] = 0;
			player[p2].list_szone[s2] = 0;
			player[p2].list_szone[new_sequence2] = pcard1;
			player[p1].list_szone[new_sequence1] = pcard2;
			refresh_player_info(p1);
			refresh_player_info(p2);
		}
	} else {
		remove_card(pcard1);
		remove_card(pcard2);
		add_card(p2, pcard1, l2, new_sequence2);
		add_card(p1, pcard2, l1, new_sequence1);
	}
	if(s1 == new_sequence1 && s2 == new_sequence2) {
		pduel->write_buffer8(MSG_SWAP);
		pduel->write_buffer32(pcard1->data.code);
		pduel->write_buffer32(info1);
		pduel->write_buffer32(pcard2->data.code);
		pduel->write_buffer32(info2);
	} else if(s1 == new_sequence1) {
		pduel->write_buffer8(MSG_MOVE);
		pduel->write_buffer32(pcard1->data.code);
		pduel->write_buffer32(info1);
		pduel->write_buffer32(pcard1->get_info_location());
		pduel->write_buffer32(0);
		pduel->write_buffer8(MSG_MOVE);
		pduel->write_buffer32(pcard2->data.code);
		pduel->write_buffer32(info2);
		pduel->write_buffer32(pcard2->get_info_location());
		pduel->write_buffer32(0);
	} else {
		pduel->write_buffer8(MSG_MOVE);
		pduel->write_buffer32(pcard2->data.code);
		pduel->write_buffer32(info2);
		pduel->write_buffer32(pcard2->get_info_location());
		pduel->write_buffer32(0);
		pduel->write_buffer8(MSG_MOVE);
		pduel->write_buffer32(pcard1->data.code);
		pduel->write_buffer32(info1);
		pduel->write_buffer32(pcard1->get_info_location());
		pduel->write_buffer32(0);
	}
}
void field::swap_card(card* pcard1, card* pcard2) {
	return swap_card(pcard1, pcard2, pcard1->current.sequence, pcard2->current.sequence);
}
void field::set_control(card* pcard, uint8_t playerid, uint16_t reset_phase, uint8_t reset_count) {
	if((core.remove_brainwashing && pcard->is_affected_by_effect(EFFECT_REMOVE_BRAINWASHING)) || std::get<uint8_t>(pcard->refresh_control_status()) == playerid)
		return;
	effect* peffect = pduel->new_effect();
	if(core.reason_effect)
		peffect->owner = core.reason_effect->get_handler();
	else
		peffect->owner = pcard;
	peffect->handler = pcard;
	peffect->type = EFFECT_TYPE_SINGLE;
	peffect->code = EFFECT_SET_CONTROL;
	peffect->value = playerid;
	peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
	peffect->reset_flag = RESET_EVENT | 0xc6c0000;
	if(reset_count) {
		peffect->reset_flag |= RESET_PHASE | reset_phase;
		if(!(peffect->reset_flag & (RESET_SELF_TURN | RESET_OPPO_TURN)))
			peffect->reset_flag |= (RESET_SELF_TURN | RESET_OPPO_TURN);
		peffect->reset_count = reset_count;
	}
	pcard->add_effect(peffect);
	pcard->current.controler = playerid;
}

int32_t field::get_pzone_sequence(uint8_t pseq) const {
	if (core.duel_rule >= NEW_MASTER_RULE) {
		if(!pseq)
			return 0;
		else
			return 4;
	}
	else {
		if (!pseq)
			return 6;
		else
			return 7;
	}
}
card* field::get_field_card(uint8_t playerid, uint32_t general_location, uint8_t sequence) const {
	if (!check_playerid(playerid))
		return nullptr;
	switch(general_location) {
	case LOCATION_MZONE: {
		if(sequence < (int32_t)player[playerid].list_mzone.size())
			return player[playerid].list_mzone[sequence];
		else
			return nullptr;
		break;
	}
	case LOCATION_SZONE: {
		if(sequence < player[playerid].szone_size)
			return player[playerid].list_szone[sequence];
		else
			return nullptr;
		break;
	}
	case LOCATION_FZONE: {
		if(sequence == 0)
			return player[playerid].list_szone[5];
		else
			return nullptr;
		break;
	}
	case LOCATION_PZONE: {
		if(sequence == 0 || sequence == 1) {
			card* pcard = player[playerid].list_szone[get_pzone_sequence(sequence)];
			return (pcard && pcard->current.pzone) ? pcard : nullptr;
		} else
			return nullptr;
		break;
	}
	case LOCATION_DECK: {
		if(sequence < player[playerid].list_main.size())
			return player[playerid].list_main[sequence];
		else
			return nullptr;
		break;
	}
	case LOCATION_HAND: {
		if(sequence < player[playerid].list_hand.size())
			return player[playerid].list_hand[sequence];
		else
			return nullptr;
		break;
	}
	case LOCATION_GRAVE: {
		if(sequence < player[playerid].list_grave.size())
			return player[playerid].list_grave[sequence];
		else
			return nullptr;
		break;
	}
	case LOCATION_REMOVED: {
		if(sequence < player[playerid].list_remove.size())
			return player[playerid].list_remove[sequence];
		else
			return nullptr;
		break;
	}
	case LOCATION_EXTRA: {
		if(sequence < player[playerid].list_extra.size())
			return player[playerid].list_extra[sequence];
		else
			return nullptr;
		break;
	}
	}
	return nullptr;
}
int32_t field::is_location_useable(uint8_t playerid, uint32_t general_location, uint8_t sequence) const {
	if (!check_playerid(playerid))
		return FALSE;
	uint32_t flag = player[playerid].disabled_location | player[playerid].used_location;
	if (general_location == LOCATION_MZONE) {
		if (sequence >= (int32_t)player[playerid].list_mzone.size())
			return FALSE;
		if(flag & (0x1U << sequence))
			return FALSE;
		if(sequence >= 5) {
			uint32_t oppo = player[1 - playerid].disabled_location | player[1 - playerid].used_location;
			if(oppo & (0x1U << (11 - sequence)))
				return FALSE;
		}
	} else if (general_location == LOCATION_SZONE) {
		if (sequence >= player[playerid].szone_size)
			return FALSE;
		if(flag & (0x100U << sequence))
			return FALSE;
	} else if (general_location == LOCATION_FZONE) {
		if (sequence >= 1)
			return FALSE;
		if(flag & (0x100U << (5 + sequence)))
			return FALSE;
	} else if (general_location == LOCATION_PZONE) {
		if (sequence >= 2)
			return FALSE;
		if (flag & (0x100U << get_pzone_sequence(sequence)))
			return FALSE;
	}
	return TRUE;
}
/**
* Return usable count in `zone` when `uplayer` moves `pcard` to the MZONE or SZONE(0~4) of `playerid` (can be negative).
* For LOCATION_MZONE, "usable" means not used, not disabled, satisfying EFFECT_MUST_USE_MZONE, satisfying EFFECT_MAX_MZONE.
* For LOCATION_SZONE, "usable" means not used, not disabled, satisfying EFFECT_MAX_SZONE.
*
* @param pcard the card about to move
* @param playerid target player
* @param location LOCATION_MZONE or LOCATION_SZONE
* @param uplayer request player, PLAYER_NONE means ignoring EFFECT_MUST_USE_MZONE of uplayer, ignoring EFFECT_MAX_MZONE, EFFECT_MAX_SZONE of playerid
* @param reason location reason
* @param zone specified zones, default: 0xff 
* @param list storing unavailable or unspecified zones
*
* @return usable count in the MZONE or SZONE(0~4) of `playerid` (can be negative)
*/
int32_t field::get_useable_count(card* pcard, uint8_t playerid, uint8_t location, uint8_t uplayer, uint32_t reason, uint32_t zone, uint32_t* list) {
	if(location == LOCATION_MZONE && pcard && pcard->current.location == LOCATION_EXTRA)
		return get_useable_count_fromex(pcard, playerid, uplayer, zone, list);
	else
		return get_useable_count_other(pcard, playerid, location, uplayer, reason, zone, list);
}
/**
* @param pcard the card about to move from Extra Deck (nullptr means any card in Extra Deck)
* @return usable count in the MZONE of `playerid`
*/
int32_t field::get_useable_count_fromex(card* pcard, uint8_t playerid, uint8_t uplayer, uint32_t zone, uint32_t* list) {
	bool use_temp_card = false;
	if(!pcard) {
		use_temp_card = true;
		pcard = temp_card;
		pcard->current.location = LOCATION_EXTRA;
	}
	int32_t useable_count = 0;
	if(core.duel_rule >= NEW_MASTER_RULE)
		useable_count = get_useable_count_fromex_rule4(pcard, playerid, uplayer, zone, list);
	else
		useable_count = get_useable_count_other(pcard, playerid, LOCATION_MZONE, uplayer, LOCATION_REASON_TOFIELD, zone, list);
	if(use_temp_card)
		pcard->current.location = 0;
	return useable_count;
}
/**
* @return the number of available slots in `zone` when `pcard` is sp_summoned to the MZONE of `playerid`
* For LOCATION_MZONE, "available" means not used, not disabled, satisfying EFFECT_MUST_USE_MZONE.
*/
int32_t field::get_spsummonable_count(card* pcard, uint8_t playerid, uint32_t zone, uint32_t* list) {
	if(pcard->current.location == LOCATION_EXTRA)
		return get_spsummonable_count_fromex(pcard, playerid, playerid, zone, list);
	else
		return get_tofield_count(pcard, playerid, LOCATION_MZONE, playerid, LOCATION_REASON_TOFIELD, zone, list);
}
/**
* @param pcard the card about to move from Extra Deck (nullptr means any card in Extra Deck)
* @return the number of available slots in `zone` when `pcard` is sp_summoned to the MZONE of `playerid` by `uplayer`
*/
int32_t field::get_spsummonable_count_fromex(card* pcard, uint8_t playerid, uint8_t uplayer, uint32_t zone, uint32_t* list) {
	bool use_temp_card = false;
	if(!pcard) {
		use_temp_card = true;
		pcard = temp_card;
		pcard->current.location = LOCATION_EXTRA;
	}
	int32_t spsummonable_count = 0;
	if(core.duel_rule >= NEW_MASTER_RULE)
		spsummonable_count = get_spsummonable_count_fromex_rule4(pcard, playerid, uplayer, zone, list);
	else
		spsummonable_count = get_tofield_count(pcard, playerid, LOCATION_MZONE, uplayer, LOCATION_REASON_TOFIELD, zone, list);
	if(use_temp_card)
		pcard->current.location = 0;
	return spsummonable_count;
}
/**
* @return usable count in `zone` of Main MZONE or SZONE(0~4)
*/
int32_t field::get_useable_count_other(card* pcard, uint8_t playerid, uint8_t location, uint8_t uplayer, uint32_t reason, uint32_t zone, uint32_t* list) {
	int32_t count = get_tofield_count(pcard, playerid, location, uplayer, reason, zone, list);
	int32_t limit;
	if(location == LOCATION_MZONE)
		limit = get_mzone_limit(playerid, uplayer, reason);
	else
		limit = get_szone_limit(playerid, uplayer, reason);
	if(count > limit)
		count = limit;
	return count;
}
/**
* @return the number of available slots in `zone` of Main MZONE or SZONE(0~4)
* for LOCATION_MZONE, "available" means not used, not disabled, satisfying EFFECT_MUST_USE_MZONE
* for LOCATION_SZONE, "available" means not used, not disabled
*/
int32_t field::get_tofield_count(card* pcard, uint8_t playerid, uint8_t location, uint32_t uplayer, uint32_t reason, uint32_t zone, uint32_t* list) {
	if (location != LOCATION_MZONE && location != LOCATION_SZONE)
		return 0;
	uint32_t flag = player[playerid].disabled_location | player[playerid].used_location;
	if(location == LOCATION_MZONE && (reason & LOCATION_REASON_TOFIELD)) {
		filter_must_use_mzone(playerid, uplayer, reason, pcard, &flag);
	}
	if (location == LOCATION_MZONE)
		flag = (flag | ~zone) & 0x1f;
	else
		flag = ((flag >> 8) | ~zone) & 0x1f;
	int32_t count = 5 - field_used_count[flag];
	if(location == LOCATION_MZONE)
		flag |= (1u << 5) | (1u << 6);
	if(list)
		*list = flag;
	return count;
}
int32_t field::get_useable_count_fromex_rule4(card* pcard, uint8_t playerid, uint8_t uplayer, uint32_t zone, uint32_t* list) {
	int32_t count = get_spsummonable_count_fromex_rule4(pcard, playerid, uplayer, zone, list);
	int32_t limit = get_mzone_limit(playerid, uplayer, LOCATION_REASON_TOFIELD);
	if(count > limit)
		count = limit;
	return count;
}
int32_t field::get_spsummonable_count_fromex_rule4(card* pcard, uint8_t playerid, uint8_t uplayer, uint32_t zone, uint32_t* list) {
	uint32_t flag = player[playerid].disabled_location | player[playerid].used_location;
	filter_must_use_mzone(playerid, uplayer, LOCATION_REASON_TOFIELD, pcard, &flag);
	if(player[playerid].list_mzone[5] && is_location_useable(playerid, LOCATION_MZONE, 6)
		&& check_extra_link(playerid, pcard, 6)) {
		flag |= 1u << 5;
	} else if(player[playerid].list_mzone[6] && is_location_useable(playerid, LOCATION_MZONE, 5)
		&& check_extra_link(playerid, pcard, 5)) {
		flag |= 1u << 6;
	} else if(player[playerid].list_mzone[5] || player[playerid].list_mzone[6]) {
		flag |= (1u << 5) | (1u << 6);
	} else {
		if(!is_location_useable(playerid, LOCATION_MZONE, 5))
			flag |= 1u << 5;
		if(!is_location_useable(playerid, LOCATION_MZONE, 6))
			flag |= 1u << 6;
	}
	uint32_t rule_zone = get_rule_zone_fromex(playerid, pcard);
	flag = flag | ~zone | ~rule_zone;
	if(list)
		*list = flag & 0x7f;
	int32_t count = 5 - field_used_count[flag & 0x1f];
	if(~flag & ((1u << 5) | (1u << 6)))
		++count;
	return count;
}
/**
* @param playerid target player
* @param uplayer request player, PLAYER_NONE means ignoring EFFECT_MAX_MZONE
* @param reason location reason
*
* @return the remaining count in the MZONE of `playerid` after applying EFFECT_MAX_MZONE (can be negative).
*/
int32_t field::get_mzone_limit(uint8_t playerid, uint8_t uplayer, uint32_t reason) {
	uint32_t used_flag = player[playerid].used_location;
	used_flag = used_flag & 0x1f;
	int32_t max = 5;
	int32_t used_count = field_used_count[used_flag];
	if(core.duel_rule >= NEW_MASTER_RULE) {
		max = 7;
		if(player[playerid].list_mzone[5])
			++used_count;
		if(player[playerid].list_mzone[6])
			++used_count;
	}
	effect_set eset;
	if(uplayer < 2)
		filter_player_effect(playerid, EFFECT_MAX_MZONE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(uplayer, PARAM_TYPE_INT);
		pduel->lua->add_param(reason, PARAM_TYPE_INT);
		int32_t v = eset[i]->get_value(3);
		if(max > v)
			max = v;
	}
	int32_t limit = max - used_count;
	return limit;
}
/**
* @param playerid target player
* @param uplayer request player, PLAYER_NONE means ignoring EFFECT_MAX_SZONE
* @param reason location reason
*
* @return the remaining count in the SZONE(0~4) of `playerid` after applying EFFECT_MAX_SZONE.
*/
int32_t field::get_szone_limit(uint8_t playerid, uint8_t uplayer, uint32_t reason) {
	uint32_t used_flag = player[playerid].used_location;
	used_flag = (used_flag >> 8) & 0x1f;
	effect_set eset;
	if(uplayer < 2)
		filter_player_effect(playerid, EFFECT_MAX_SZONE, &eset);
	int32_t max = 5;
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(uplayer, PARAM_TYPE_INT);
		pduel->lua->add_param(reason, PARAM_TYPE_INT);
		int32_t v = eset[i]->get_value(3);
		if(max > v)
			max = v;
	}
	int32_t limit = max - field_used_count[used_flag];
	return limit;
}
uint32_t field::get_linked_zone(int32_t playerid) {
	uint32_t zones = 0;
	for(auto& pcard : player[playerid].list_mzone) {
		if(pcard)
			zones |= pcard->get_linked_zone() & 0xffff;
	}
	for(auto& pcard : player[1 - playerid].list_mzone) {
		if(pcard)
			zones |= pcard->get_linked_zone() >> 16;
	}
	return zones;
}
uint32_t field::get_rule_zone_fromex(int32_t playerid, card* pcard) {
	if(core.duel_rule >= NEW_MASTER_RULE) {
		if(core.duel_rule >= MASTER_RULE_2020 && pcard && (pcard->data.type & (TYPE_FUSION | TYPE_SYNCHRO | TYPE_XYZ))
			&& (pcard->is_position(POS_FACEDOWN) || !(pcard->data.type & TYPE_PENDULUM)))
			return 0x7f;
		else
			return get_linked_zone(playerid) | (1u << 5) | (1u << 6);
	} else {
		return 0x1f;
	}
}
/**
* @param playerid target player
* @param uplayer request player, PLAYER_NONE means ignoring EFFECT_MUST_USE_MZONE of uplayer
* @param reason location reason
* @param pcard the card about to move
* @param flag storing the zones in MZONE blocked by EFFECT_MUST_USE_MZONE
*/
void field::filter_must_use_mzone(uint8_t playerid, uint8_t uplayer, uint32_t reason, card* pcard, uint32_t* flag) {
	effect_set eset;
	if(uplayer < 2)
		filter_player_effect(uplayer, EFFECT_MUST_USE_MZONE, &eset);
	if(pcard)
		pcard->filter_effect(EFFECT_MUST_USE_MZONE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT) && eset[i]->count_limit == 0)
			continue;
		uint32_t value = 0x1f;
		if(eset[i]->is_flag(EFFECT_FLAG_PLAYER_TARGET))
			value = eset[i]->get_value();
		else {
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			pduel->lua->add_param(uplayer, PARAM_TYPE_INT);
			pduel->lua->add_param(reason, PARAM_TYPE_INT);
			value = eset[i]->get_value(pcard, 3);
		}
		if(eset[i]->get_handler_player() == playerid)
			*flag |= ~value & 0x7f;
		else
			*flag |= ~(value >> 16) & 0x7f;
	}
}
void field::get_linked_cards(uint8_t self, uint8_t s, uint8_t o, card_set* cset) {
	cset->clear();
	uint8_t c = s;
	for(int32_t p = 0; p < 2; ++p) {
		if(c) {
			uint32_t linked_zone = get_linked_zone(self);
			get_cards_in_zone(cset, linked_zone, self, LOCATION_MZONE);
		}
		self = 1 - self;
		c = o;
	}
}
int32_t field::check_extra_link(int32_t playerid) {
	if(!player[playerid].list_mzone[5] || !player[playerid].list_mzone[6])
		return FALSE;
	card* pcard = player[playerid].list_mzone[5];
	uint32_t checked = 0x1u << 5;
	uint32_t linked_zone = pcard->get_mutual_linked_zone();
	while(true) {
		if((linked_zone >> 6) & 0x1U)
			return TRUE;
		uint32_t checking = linked_zone & ~checked;
		if(!checking)
			return FALSE;
		uint32_t rightmost = checking & (~checking + 1);
		checked |= rightmost;
		if(rightmost < 0x10000U) {
			for(int32_t i = 0; i < 7; ++i) {
				if(rightmost & 0x1U) {
					pcard = player[playerid].list_mzone[i];
					linked_zone |= pcard->get_mutual_linked_zone();
					break;
				}
				rightmost >>= 1;
			}
		} else {
			rightmost >>= 16;
			for(int32_t i = 0; i < 7; ++i) {
				if(rightmost & 0x1U) {
					pcard = player[1 - playerid].list_mzone[i];
					uint32_t zone = pcard->get_mutual_linked_zone();
					linked_zone |= (zone << 16) | (zone >> 16);
					break;
				}
				rightmost >>= 1;
			}
		}
	}
	return FALSE;
}
int32_t field::check_extra_link(int32_t playerid, card* pcard, int32_t sequence) {
	if(!pcard)
		return FALSE;
	if(player[playerid].list_mzone[sequence])
		return FALSE;
	uint8_t cur_controler = pcard->current.controler;
	uint8_t cur_location = pcard->current.location;
	uint8_t cur_sequence = pcard->current.sequence;
	player[playerid].list_mzone[sequence] = pcard;
	pcard->current.controler = playerid;
	pcard->current.location = LOCATION_MZONE;
	pcard->current.sequence = sequence;
	int32_t ret = check_extra_link(playerid);
	player[playerid].list_mzone[sequence] = 0;
	pcard->current.controler = cur_controler;
	pcard->current.location = cur_location;
	pcard->current.sequence = cur_sequence;
	return ret;
}
void field::get_cards_in_zone(card_set* cset, uint32_t zone, int32_t playerid, int32_t location) {
	if(!(location & LOCATION_ONFIELD))
		return;
	card_vector& svector = (location == LOCATION_MZONE) ? player[playerid].list_mzone : player[playerid].list_szone;
	uint32_t icheck = 0x1;
	for(auto& pcard : svector) {
		if(zone & icheck) {
			if(pcard)
				cset->insert(pcard);
		}
		icheck <<= 1;
	}
}
void field::shuffle(uint8_t playerid, uint8_t location) {
	if(!(location & (LOCATION_HAND | LOCATION_DECK | LOCATION_EXTRA)))
		return;
	card_vector& svector = (location == LOCATION_HAND) ? player[playerid].list_hand : (location == LOCATION_DECK) ? player[playerid].list_main : player[playerid].list_extra;
	if(svector.size() == 0)
		return;
	if(location == LOCATION_HAND) {
		bool shuffle = false;
		for(auto& pcard : svector)
			if(!pcard->is_position(POS_FACEUP))
				shuffle = true;
		if(!shuffle) {
			core.shuffle_hand_check[playerid] = FALSE;
			return;
		}
	}
	if(location == LOCATION_HAND || !(core.duel_options & DUEL_PSEUDO_SHUFFLE)) {
		int32_t s = (int32_t)svector.size();
		if(location == LOCATION_EXTRA)
			s = s - (int32_t)player[playerid].extra_p_count;
		if(s > 1) {
			pduel->random.shuffle_vector(svector, 0, s, pduel->rng_version);
			reset_sequence(playerid, location);
		}
	}
	if(location == LOCATION_HAND || location == LOCATION_EXTRA) {
		pduel->write_buffer8((location == LOCATION_HAND) ? MSG_SHUFFLE_HAND : MSG_SHUFFLE_EXTRA);
		pduel->write_buffer8(playerid);
		pduel->write_buffer8((uint8_t)svector.size());
		for(auto& pcard : svector)
			pduel->write_buffer32(pcard->data.code);
		if(location == LOCATION_HAND) {
			core.shuffle_hand_check[playerid] = FALSE;
			for(auto& pcard : svector) {
				for(auto& i : pcard->indexer) {
					effect* peffect = i.first;
					if(peffect->is_flag(EFFECT_FLAG_CLIENT_HINT) && !peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET)) {
						pduel->write_buffer8(MSG_CARD_HINT);
						pduel->write_buffer32(pcard->get_info_location());
						pduel->write_buffer8(CHINT_DESC_ADD);
						pduel->write_buffer32(peffect->description);
					}
				}
			}
		}
	} else {
		pduel->write_buffer8(MSG_SHUFFLE_DECK);
		pduel->write_buffer8(playerid);
		core.shuffle_deck_check[playerid] = FALSE;
		if(core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK) {
			card* ptop = svector.back();
			if(core.deck_reversed || (ptop->current.position == POS_FACEUP_DEFENSE)) {
				pduel->write_buffer8(MSG_DECK_TOP);
				pduel->write_buffer8(playerid);
				pduel->write_buffer8(0);
				if(ptop->current.position != POS_FACEUP_DEFENSE)
					pduel->write_buffer32(ptop->data.code);
				else
					pduel->write_buffer32(ptop->data.code | 0x80000000);
			}
		}
	}
}
void field::reset_sequence(uint8_t playerid, uint8_t location) {
	if(location & (LOCATION_ONFIELD))
		return;
	int32_t i = 0;
	switch(location) {
	case LOCATION_DECK:
		for(auto& pcard : player[playerid].list_main)
			pcard->current.sequence = i++;
		break;
	case LOCATION_HAND:
		for(auto& pcard : player[playerid].list_hand)
			pcard->current.sequence = i++;
		break;
	case LOCATION_EXTRA:
		for(auto& pcard : player[playerid].list_extra)
			pcard->current.sequence = i++;
		break;
	case LOCATION_GRAVE:
		for(auto& pcard : player[playerid].list_grave)
			pcard->current.sequence = i++;
		break;
	case LOCATION_REMOVED:
		for(auto& pcard : player[playerid].list_remove)
			pcard->current.sequence = i++;
		break;
	}
}
void field::swap_deck_and_grave(uint8_t playerid) {
	for(auto& pcard : player[playerid].list_grave) {
		pcard->previous.location = LOCATION_GRAVE;
		pcard->previous.sequence = pcard->current.sequence;
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	for(auto& pcard : player[playerid].list_main) {
		pcard->previous.location = LOCATION_DECK;
		pcard->previous.sequence = pcard->current.sequence;
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	player[playerid].list_grave.swap(player[playerid].list_main);
	card_vector ex;
	for(auto clit = player[playerid].list_main.begin(); clit != player[playerid].list_main.end(); ) {
		if((*clit)->is_extra_deck_monster()) {
			ex.push_back(*clit);
			clit = player[playerid].list_main.erase(clit);
		} else
			++clit;
	}
	for(auto& pcard : player[playerid].list_grave) {
		pcard->current.position = POS_FACEUP;
		pcard->current.location = LOCATION_GRAVE;
		pcard->current.reason = REASON_EFFECT;
		pcard->current.reason_effect = core.reason_effect;
		pcard->current.reason_player = core.reason_player;
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
		pcard->reset(RESET_TOGRAVE, RESET_EVENT);
	}
	for(auto& pcard : player[playerid].list_main) {
		pcard->current.position = POS_FACEDOWN_DEFENSE;
		pcard->current.location = LOCATION_DECK;
		pcard->current.reason = REASON_EFFECT;
		pcard->current.reason_effect = core.reason_effect;
		pcard->current.reason_player = core.reason_player;
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
		pcard->reset(RESET_TODECK, RESET_EVENT);
		pcard->set_status(STATUS_PROC_COMPLETE, FALSE);
	}
	for(auto& pcard : ex) {
		pcard->current.position = POS_FACEDOWN_DEFENSE;
		pcard->current.location = LOCATION_EXTRA;
		pcard->current.reason = REASON_EFFECT;
		pcard->current.reason_effect = core.reason_effect;
		pcard->current.reason_player = core.reason_player;
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
		pcard->reset(RESET_TODECK, RESET_EVENT);
		pcard->set_status(STATUS_PROC_COMPLETE, FALSE);
	}
	player[playerid].list_extra.insert(player[playerid].list_extra.end() - player[playerid].extra_p_count, ex.begin(), ex.end());
	reset_sequence(playerid, LOCATION_GRAVE);
	reset_sequence(playerid, LOCATION_DECK);
	reset_sequence(playerid, LOCATION_EXTRA);
	pduel->write_buffer8(MSG_SWAP_GRAVE_DECK);
	pduel->write_buffer8(playerid);
	shuffle(playerid, LOCATION_DECK);
}
void field::reverse_deck(uint8_t playerid) {
	int32_t count = (int32_t)player[playerid].list_main.size();
	if(count == 0)
		return;
	for(int32_t i = 0; i < count / 2; ++i) {
		player[playerid].list_main[i]->current.sequence = count - 1 - i;
		player[playerid].list_main[count - 1 - i]->current.sequence = i;
		std::swap(player[playerid].list_main[i], player[playerid].list_main[count - 1 - i]);
	}
}
void field::refresh_player_info(uint8_t playerid) {
	if (!check_playerid(playerid))
		return;
	uint32_t used_flag = 0;
	for (int32_t i = 0; i < (int32_t)player[playerid].list_mzone.size(); ++i) {
		if (player[playerid].list_mzone[i])
			used_flag |= 0x1U << i;
	}
	for (int32_t i = 0; i < player[playerid].szone_size; ++i) {
		if (player[playerid].list_szone[i])
			used_flag |= 0x100U << i;
	}
	player[playerid].used_location = used_flag;
}
void field::tag_swap(uint8_t playerid) {
	//main
	for(auto& pcard : player[playerid].list_main) {
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	std::swap(player[playerid].list_main, player[playerid].tag_list_main);
	for(auto& pcard : player[playerid].list_main) {
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
	}
	//hand
	for(auto& pcard : player[playerid].list_hand) {
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	std::swap(player[playerid].list_hand, player[playerid].tag_list_hand);
	for(auto& pcard : player[playerid].list_hand) {
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
	}
	//extra
	for(auto& pcard : player[playerid].list_extra) {
		pcard->enable_field_effect(false);
		pcard->cancel_field_effect();
	}
	std::swap(player[playerid].list_extra, player[playerid].tag_list_extra);
	std::swap(player[playerid].extra_p_count, player[playerid].tag_extra_p_count);
	for(auto& pcard : player[playerid].list_extra) {
		pcard->apply_field_effect();
		pcard->enable_field_effect(true);
	}
	pduel->write_buffer8(MSG_TAG_SWAP);
	pduel->write_buffer8(playerid);
	pduel->write_buffer8((uint8_t)player[playerid].list_main.size());
	pduel->write_buffer8((uint8_t)player[playerid].list_extra.size());
	pduel->write_buffer8((uint8_t)player[playerid].extra_p_count);
	pduel->write_buffer8((uint8_t)player[playerid].list_hand.size());
	if(core.deck_reversed && player[playerid].list_main.size())
		pduel->write_buffer32(player[playerid].list_main.back()->data.code);
	else
		pduel->write_buffer32(0);
	for(auto& pcard : player[playerid].list_hand)
		pduel->write_buffer32(pcard->data.code | (pcard->is_position(POS_FACEUP) ? 0x80000000 : 0));
	for(auto& pcard : player[playerid].list_extra)
		pduel->write_buffer32(pcard->data.code | (pcard->is_position(POS_FACEUP) ? 0x80000000 : 0));
}
void field::add_effect(effect* peffect, uint8_t owner_player) {
	if (!peffect)
		return;
	if (effects.indexer.find(peffect) != effects.indexer.end())
		return;
	if (peffect->type & EFFECT_TYPES_CHAIN_LINK && peffect->owner == temp_card)
		return;
	effect_container::iterator it;
	if (!(peffect->type & EFFECT_TYPE_ACTIONS)) {
		it = effects.aura_effect.emplace(peffect->code, peffect);
		if(peffect->code == EFFECT_SPSUMMON_COUNT_LIMIT)
			effects.spsummon_count_eff.insert(peffect);
		if(peffect->type & EFFECT_TYPE_GRANT)
			effects.grant_effect.emplace(peffect, field_effect::gain_effects());
	} else {
		if (peffect->type & EFFECT_TYPE_IGNITION)
			it = effects.ignition_effect.emplace(peffect->code, peffect);
		else if (peffect->type & EFFECT_TYPE_TRIGGER_O)
			it = effects.trigger_o_effect.emplace(peffect->code, peffect);
		else if (peffect->type & EFFECT_TYPE_TRIGGER_F)
			it = effects.trigger_f_effect.emplace(peffect->code, peffect);
		else if (peffect->type & EFFECT_TYPE_QUICK_O)
			it = effects.quick_o_effect.emplace(peffect->code, peffect);
		else if (peffect->type & EFFECT_TYPE_QUICK_F)
			it = effects.quick_f_effect.emplace(peffect->code, peffect);
		else if (peffect->type & EFFECT_TYPE_ACTIVATE)
			it = effects.activate_effect.emplace(peffect->code, peffect);
		else if (peffect->type & EFFECT_TYPE_CONTINUOUS)
			it = effects.continuous_effect.emplace(peffect->code, peffect);
		else
			return;
	}
	if (!peffect->handler) {
		peffect->flag[0] |= EFFECT_FLAG_FIELD_ONLY;
		peffect->handler = peffect->owner;
		peffect->effect_owner = owner_player;
		peffect->id = infos.field_id++;
	}
	peffect->card_type = peffect->owner->data.type;
	effects.indexer.emplace(peffect, it);
	if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY)) {
		if(peffect->is_disable_related())
			update_disable_check_list(peffect);
		if(peffect->is_flag(EFFECT_FLAG_OATH))
			effects.oath.emplace(peffect, core.reason_effect);
		if(peffect->reset_flag & RESET_PHASE)
			effects.pheff.insert(peffect);
		if(peffect->reset_flag & RESET_CHAIN)
			effects.cheff.insert(peffect);
		if(peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT))
			effects.rechargeable.insert(peffect);
		if(peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET) && peffect->is_flag(EFFECT_FLAG_CLIENT_HINT)) {
			bool target_player[2] = {false, false};
			if(peffect->s_range)
				target_player[owner_player] = true;
			if(peffect->o_range)
				target_player[1 - owner_player] = true;
			if(target_player[0]) {
				pduel->write_buffer8(MSG_PLAYER_HINT);
				pduel->write_buffer8(0);
				pduel->write_buffer8(PHINT_DESC_ADD);
				pduel->write_buffer32(peffect->description);
			}
			if(target_player[1]) {
				pduel->write_buffer8(MSG_PLAYER_HINT);
				pduel->write_buffer8(1);
				pduel->write_buffer8(PHINT_DESC_ADD);
				pduel->write_buffer32(peffect->description);
			}
		}
	}
}
void field::remove_effect(effect* peffect) {
	auto eit = effects.indexer.find(peffect);
	if (eit == effects.indexer.end())
		return;
	auto& it = eit->second;
	if (!(peffect->type & EFFECT_TYPE_ACTIONS)) {
		effects.aura_effect.erase(it);
		if(peffect->code == EFFECT_SPSUMMON_COUNT_LIMIT)
			effects.spsummon_count_eff.erase(peffect);
		if(peffect->type & EFFECT_TYPE_GRANT)
			erase_grant_effect(peffect);
	} else {
		if (peffect->type & EFFECT_TYPE_IGNITION)
			effects.ignition_effect.erase(it);
		else if (peffect->type & EFFECT_TYPE_TRIGGER_O)
			effects.trigger_o_effect.erase(it);
		else if (peffect->type & EFFECT_TYPE_TRIGGER_F)
			effects.trigger_f_effect.erase(it);
		else if (peffect->type & EFFECT_TYPE_QUICK_O)
			effects.quick_o_effect.erase(it);
		else if (peffect->type & EFFECT_TYPE_QUICK_F)
			effects.quick_f_effect.erase(it);
		else if (peffect->type & EFFECT_TYPE_ACTIVATE)
			effects.activate_effect.erase(it);
		else if (peffect->type & EFFECT_TYPE_CONTINUOUS)
			effects.continuous_effect.erase(it);
	}
	effects.indexer.erase(peffect);
	if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY)) {
		if(peffect->is_disable_related())
			update_disable_check_list(peffect);
		if(peffect->is_flag(EFFECT_FLAG_OATH))
			effects.oath.erase(peffect);
		if(peffect->reset_flag & RESET_PHASE)
			effects.pheff.erase(peffect);
		if(peffect->reset_flag & RESET_CHAIN)
			effects.cheff.erase(peffect);
		if(peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT))
			effects.rechargeable.erase(peffect);
		core.reseted_effects.insert(peffect);
		if(peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET) && peffect->is_flag(EFFECT_FLAG_CLIENT_HINT)) {
			bool target_player[2] = {false, false};
			if(peffect->s_range)
				target_player[peffect->effect_owner] = true;
			if(peffect->o_range)
				target_player[1 - peffect->effect_owner] = true;
			if(target_player[0]) {
				pduel->write_buffer8(MSG_PLAYER_HINT);
				pduel->write_buffer8(0);
				pduel->write_buffer8(PHINT_DESC_REMOVE);
				pduel->write_buffer32(peffect->description);
			}
			if(target_player[1]) {
				pduel->write_buffer8(MSG_PLAYER_HINT);
				pduel->write_buffer8(1);
				pduel->write_buffer8(PHINT_DESC_REMOVE);
				pduel->write_buffer32(peffect->description);
			}
		}
	}
}
void field::remove_oath_effect(effect* reason_effect) {
	for(auto oeit = effects.oath.begin(); oeit != effects.oath.end();) {
		auto rm = oeit++;
		if(rm->second == reason_effect) {
			effect* peffect = rm->first;
			if(peffect->is_flag(EFFECT_FLAG_FIELD_ONLY))
				remove_effect(peffect);
			else
				peffect->handler->remove_effect(peffect);
		}
	}
}
void field::release_oath_relation(effect* reason_effect) {
	for(auto& oeit : effects.oath)
		if(oeit.second == reason_effect)
			oeit.second = 0;
}
void field::reset_phase(uint32_t phase) {
	for(auto eit = effects.pheff.begin(); eit != effects.pheff.end();) {
		auto rm = eit++;
		if((*rm)->reset(phase, RESET_PHASE)) {
			if((*rm)->is_flag(EFFECT_FLAG_FIELD_ONLY))
				remove_effect(*rm);
			else
				(*rm)->handler->remove_effect((*rm));
		}
	}
}
void field::reset_chain() {
	for(auto eit = effects.cheff.begin(); eit != effects.cheff.end();) {
		auto rm = eit++;
		if((*rm)->is_flag(EFFECT_FLAG_FIELD_ONLY))
			remove_effect(*rm);
		else
			(*rm)->handler->remove_effect((*rm));
	}
}
void field::add_effect_code(uint32_t code, int32_t playerid) {
	if (playerid < 0 || playerid > PLAYER_NONE)
		return;
	auto count_map = &core.effect_count_code[playerid];
	if (code & EFFECT_COUNT_CODE_DUEL)
		count_map = &core.effect_count_code_duel[playerid];
	else if (code & EFFECT_COUNT_CODE_CHAIN)
		count_map = &core.effect_count_code_chain[playerid];
	(*count_map)[code]++;
}
int32_t field::get_effect_code(uint32_t code, int32_t playerid) {
	if (playerid < 0 || playerid > PLAYER_NONE)
		return 0;
	auto count_map = &core.effect_count_code[playerid];
	if(code & EFFECT_COUNT_CODE_DUEL)
		count_map = &core.effect_count_code_duel[playerid];
	else if(code & EFFECT_COUNT_CODE_CHAIN)
		count_map = &core.effect_count_code_chain[playerid];
	auto iter = count_map->find(code);
	if(iter == count_map->end())
		return 0;
	return iter->second;
}
void field::dec_effect_code(uint32_t code, int32_t playerid) {
	if (playerid < 0 || playerid > PLAYER_NONE)
		return;
	auto count_map = &core.effect_count_code[playerid];
	if (code & EFFECT_COUNT_CODE_DUEL)
		count_map = &core.effect_count_code_duel[playerid];
	else if (code & EFFECT_COUNT_CODE_CHAIN)
		count_map = &core.effect_count_code_chain[playerid];
	auto iter = count_map->find(code);
	if(iter == count_map->end())
		return;
	if (iter->second > 0)
		iter->second--;
}
void field::filter_field_effect(uint32_t code, effect_set* eset, uint8_t sort) {
	auto rg = effects.aura_effect.equal_range(code);
	for (; rg.first != rg.second; ) {
		effect* peffect = rg.first->second;
		++rg.first;
		if (peffect->is_available())
			eset->push_back(peffect);
	}
	if(sort)
		std::sort(eset->begin(), eset->end(), effect_sort_id);
}
//Get all cards in the target range of a EFFECT_TYPE_FIELD effect
void field::filter_affected_cards(effect* peffect, card_set* cset) {
	if ((peffect->type & EFFECT_TYPE_ACTIONS) || !(peffect->type & EFFECT_TYPE_FIELD))
		return;
	if (peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET | EFFECT_FLAG_SPSUM_PARAM))
		return;
	uint8_t self = peffect->get_handler_player();
	if (!check_playerid(self))
		return;
	std::vector<card_vector*> cvec;
	uint16_t range = peffect->s_range;
	for(uint32_t p = 0; p < 2; ++p) {
		if(range & LOCATION_MZONE)
			cvec.push_back(&player[self].list_mzone);
		if(range & LOCATION_SZONE)
			cvec.push_back(&player[self].list_szone);
		if(range & LOCATION_GRAVE)
			cvec.push_back(&player[self].list_grave);
		if(range & LOCATION_REMOVED)
			cvec.push_back(&player[self].list_remove);
		if(range & LOCATION_HAND)
			cvec.push_back(&player[self].list_hand);
		if(range & LOCATION_DECK)
			cvec.push_back(&player[self].list_main);
		if(range & LOCATION_EXTRA)
			cvec.push_back(&player[self].list_extra);
		range = peffect->o_range;
		self = 1 - self;
	}
	for(auto& cvit : cvec) {
		for(auto& pcard : *cvit) {
			if(pcard && peffect->is_target(pcard))
				cset->insert(pcard);
		}
	}
}
void field::filter_inrange_cards(effect* peffect, card_set* cset) {
	if(peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET | EFFECT_FLAG_SPSUM_PARAM))
		return;
	uint8_t self = peffect->get_handler_player();
	if (!check_playerid(self))
		return;
	uint16_t range = peffect->s_range;
	std::vector<card_vector*> cvec;
	for(int32_t p = 0; p < 2; ++p) {
		if(range & LOCATION_MZONE)
			cvec.push_back(&player[self].list_mzone);
		if(range & LOCATION_SZONE)
			cvec.push_back(&player[self].list_szone);
		if(range & LOCATION_GRAVE)
			cvec.push_back(&player[self].list_grave);
		if(range & LOCATION_REMOVED)
			cvec.push_back(&player[self].list_remove);
		if(range & LOCATION_HAND)
			cvec.push_back(&player[self].list_hand);
		if(range & LOCATION_DECK)
			cvec.push_back(&player[self].list_main);
		if(range & LOCATION_EXTRA)
			cvec.push_back(&player[self].list_extra);
		range = peffect->o_range;
		self = 1 - self;
	}
	for(auto& cvit : cvec) {
		for(auto& pcard : *cvit) {
			if (pcard && (!(pcard->current.location & LOCATION_ONFIELD) || !pcard->is_treated_as_not_on_field()) && peffect->is_fit_target_function(pcard))
				cset->insert(pcard);
		}
	}
}
void field::filter_player_effect(uint8_t playerid, uint32_t code, effect_set* eset, uint8_t sort) {
	auto rg = effects.aura_effect.equal_range(code);
	for (; rg.first != rg.second; ++rg.first) {
		effect* peffect = rg.first->second;
		if (peffect->is_target_player(playerid) && peffect->is_available())
			eset->push_back(peffect);
	}
	if(sort)
		std::sort(eset->begin(), eset->end(), effect_sort_id);
}
int32_t field::filter_matching_card(lua_State* L, int32_t findex, uint8_t self, uint32_t location1, uint32_t location2, group* pgroup, card* pexception, group* pexgroup, uint32_t extraargs, card** pret, int32_t fcount, int32_t is_target) {
	if(self != 0 && self != 1)
		return FALSE;
	card_set result;
	uint32_t location = location1;
	for(uint32_t p = 0; p < 2; ++p) {
		if(location & LOCATION_MZONE) {
			for(auto& pcard : player[self].list_mzone) {
				if(pcard && !pcard->is_treated_as_not_on_field()
						&& pcard != pexception && !(pexgroup && pexgroup->has_card(pcard))
						&& pduel->lua->check_filter(L, pcard, findex, extraargs)
						&& (!is_target || pcard->is_capable_be_effect_target(core.reason_effect, core.reason_player))) {
					if(pret) {
						*pret = pcard;
						return TRUE;
					}
					result.insert(pcard);
					if(fcount && (int32_t)result.size() >= fcount)
						return TRUE;
				}
			}
		}
		if(location & LOCATION_SZONE) {
			for(auto& pcard : player[self].list_szone) {
				if(pcard && !pcard->is_treated_as_not_on_field()
				        && pcard != pexception && !(pexgroup && pexgroup->has_card(pcard))
				        && pduel->lua->check_filter(L, pcard, findex, extraargs)
				        && (!is_target || pcard->is_capable_be_effect_target(core.reason_effect, core.reason_player))) {
					if(pret) {
						*pret = pcard;
						return TRUE;
					}
					result.insert(pcard);
					if(fcount && (int32_t)result.size() >= fcount)
						return TRUE;
				}
			}
		}
		if(location & LOCATION_FZONE) {
			card* pcard = player[self].list_szone[5];
			if(pcard && !pcard->is_treated_as_not_on_field()
			        && pcard != pexception && !(pexgroup && pexgroup->has_card(pcard))
			        && pduel->lua->check_filter(L, pcard, findex, extraargs)
			        && (!is_target || pcard->is_capable_be_effect_target(core.reason_effect, core.reason_player))) {
				if(pret) {
					*pret = pcard;
					return TRUE;
				}
				result.insert(pcard);
				if (fcount && (int32_t)result.size() >= fcount)
					return TRUE;
			}
		}
		if(location & LOCATION_PZONE) {
			for(int32_t i = 0; i < 2; ++i) {
				card* pcard = player[self].list_szone[get_pzone_sequence(i)];
				if(pcard && pcard->current.pzone && !pcard->is_treated_as_not_on_field()
				        && pcard != pexception && !(pexgroup && pexgroup->has_card(pcard))
				        && pduel->lua->check_filter(L, pcard, findex, extraargs)
				        && (!is_target || pcard->is_capable_be_effect_target(core.reason_effect, core.reason_player))) {
					if(pret) {
						*pret = pcard;
						return TRUE;
					}
					result.insert(pcard);
					if(fcount && (int32_t)result.size() >= fcount)
						return TRUE;
				}
			}
		}
		if(location & LOCATION_DECK) {
			for(auto cit = player[self].list_main.rbegin(); cit != player[self].list_main.rend(); ++cit) {
				if(*cit != pexception && !(pexgroup && pexgroup->has_card(*cit))
				        && pduel->lua->check_filter(L, *cit, findex, extraargs)
				        && (!is_target || (*cit)->is_capable_be_effect_target(core.reason_effect, core.reason_player))) {
					if(pret) {
						*pret = *cit;
						return TRUE;
					}
					result.insert(*cit);
					if(fcount && (int32_t)result.size() >= fcount)
						return TRUE;
				}
			}
		}
		if(location & LOCATION_EXTRA) {
			for(auto cit = player[self].list_extra.rbegin(); cit != player[self].list_extra.rend(); ++cit) {
				if(*cit != pexception && !(pexgroup && pexgroup->has_card(*cit))
				        && pduel->lua->check_filter(L, *cit, findex, extraargs)
				        && (!is_target || (*cit)->is_capable_be_effect_target(core.reason_effect, core.reason_player))) {
					if(pret) {
						*pret = *cit;
						return TRUE;
					}
					result.insert(*cit);
					if(fcount && (int32_t)result.size() >= fcount)
						return TRUE;
				}
			}
		}
		if(location & LOCATION_HAND) {
			for(auto& pcard : player[self].list_hand) {
				if(pcard != pexception && !(pexgroup && pexgroup->has_card(pcard))
				        && pduel->lua->check_filter(L, pcard, findex, extraargs)
				        && (!is_target || pcard->is_capable_be_effect_target(core.reason_effect, core.reason_player))) {
					if(pret) {
						*pret = pcard;
						return TRUE;
					}
					result.insert(pcard);
					if(fcount &&  (int32_t)result.size() >= fcount)
						return TRUE;
				}
			}
		}
		if(location & LOCATION_GRAVE) {
			for(auto cit = player[self].list_grave.rbegin(); cit != player[self].list_grave.rend(); ++cit) {
				if(*cit != pexception && !(pexgroup && pexgroup->has_card(*cit))
				        && pduel->lua->check_filter(L, *cit, findex, extraargs)
				        && (!is_target || (*cit)->is_capable_be_effect_target(core.reason_effect, core.reason_player))) {
					if(pret) {
						*pret = *cit;
						return TRUE;
					}
					result.insert(*cit);
					if(fcount && (int32_t)result.size() >= fcount)
						return TRUE;
				}
			}
		}
		if(location & LOCATION_REMOVED) {
			for(auto cit = player[self].list_remove.rbegin(); cit != player[self].list_remove.rend(); ++cit) {
				if(*cit != pexception && !(pexgroup && pexgroup->has_card(*cit))
				        && pduel->lua->check_filter(L, *cit, findex, extraargs)
				        && (!is_target || (*cit)->is_capable_be_effect_target(core.reason_effect, core.reason_player))) {
					if(pret) {
						*pret = *cit;
						return TRUE;
					}
					result.insert(*cit);
					if(fcount && (int32_t)result.size() >= fcount)
						return TRUE;
				}
			}
		}
		location = location2;
		self = 1 - self;
	}
	if (pgroup)
		pgroup->container.insert(result.begin(), result.end());
	return FALSE;
}
// Duel.GetFieldGroup(), Duel.GetFieldGroupCount()
int32_t field::filter_field_card(uint8_t self, uint32_t location1, uint32_t location2, group* pgroup) {
	if(self != 0 && self != 1)
		return 0;
	uint32_t location = location1;
	card_set result;
	for(uint32_t p = 0; p < 2; ++p) {
		if(location & LOCATION_MZONE) {
			for(auto& pcard : player[self].list_mzone) {
				if(pcard && !pcard->get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP)) {
					result.insert(pcard);
				}
			}
		}
		if(location & LOCATION_SZONE) {
			for(auto& pcard : player[self].list_szone) {
				if(pcard) {
					result.insert(pcard);
				}
			}
		}
		if(location & LOCATION_FZONE) {
			card* pcard = player[self].list_szone[5];
			if(pcard) {
				result.insert(pcard);
			}
		}
		if(location & LOCATION_PZONE) {
			for(int32_t i = 0; i < 2; ++i) {
				card* pcard = player[self].list_szone[get_pzone_sequence(i)];
				if(pcard && pcard->current.pzone) {
					result.insert(pcard);
				}
			}
		}
		if(location & LOCATION_HAND) {
			result.insert(player[self].list_hand.begin(), player[self].list_hand.end());
		}
		if(location & LOCATION_DECK) {
			result.insert(player[self].list_main.rbegin(), player[self].list_main.rend());
		}
		if(location & LOCATION_EXTRA) {
			result.insert(player[self].list_extra.rbegin(), player[self].list_extra.rend());
		}
		if(location & LOCATION_GRAVE) {
			result.insert(player[self].list_grave.rbegin(), player[self].list_grave.rend());
		}
		if(location & LOCATION_REMOVED) {
			result.insert(player[self].list_remove.rbegin(), player[self].list_remove.rend());
		}
		location = location2;
		self = 1 - self;
	}
	if (pgroup)
		pgroup->container.insert(result.begin(), result.end());
	return (int32_t)result.size();
}
effect* field::is_player_affected_by_effect(uint8_t playerid, uint32_t code) {
	auto rg = effects.aura_effect.equal_range(code);
	for (; rg.first != rg.second; ++rg.first) {
		effect* peffect = rg.first->second;
		if (peffect->is_target_player(playerid) && peffect->is_available())
			return peffect;
	}
	return nullptr;
}
int32_t field::get_release_list(lua_State* L, uint8_t playerid, card_set* release_list, card_set* ex_list, card_set* ex_list_oneof, int32_t use_con, int32_t use_hand, int32_t fun, int32_t exarg, card* exc, group* exg, uint32_t reason) {
	uint32_t rcount = 0;
	effect* re = core.reason_effect;
	for(auto& pcard : player[playerid].list_mzone) {
		if(pcard && pcard != exc && !(exg && exg->has_card(pcard)) && pcard->is_releasable_by_nonsummon(playerid, reason)
		        && (reason != REASON_EFFECT || pcard->is_releasable_by_effect(playerid, re))
		        && (!use_con || pduel->lua->check_filter(L, pcard, fun, exarg))) {
			if(release_list)
				release_list->insert(pcard);
			pcard->release_param = 1;
			++rcount;
		}
	}
	if(use_hand) {
		for(auto& pcard : player[playerid].list_hand) {
			if(pcard && pcard != exc && !(exg && exg->has_card(pcard)) && pcard->is_releasable_by_nonsummon(playerid, reason)
				    && (reason != REASON_EFFECT || pcard->is_releasable_by_effect(playerid, re))
			        && (!use_con || pduel->lua->check_filter(L, pcard, fun, exarg))) {
				if(release_list)
					release_list->insert(pcard);
				pcard->release_param = 1;
				++rcount;
			}
		}
	}
	int32_t ex_oneof_max = 0;
	for(auto& pcard : player[1 - playerid].list_mzone) {
		if(pcard && pcard != exc && !(exg && exg->has_card(pcard)) && (pcard->is_position(POS_FACEUP) || !use_con)
		        && pcard->is_releasable_by_nonsummon(playerid, reason)
			    && (reason != REASON_EFFECT || pcard->is_releasable_by_effect(playerid, re))
			    && (!use_con || pduel->lua->check_filter(L, pcard, fun, exarg))) {
			pcard->release_param = 1;
			if(pcard->is_affected_by_effect(EFFECT_EXTRA_RELEASE)) {
				if(ex_list)
					ex_list->insert(pcard);
				++rcount;
			} else {
				effect* peffect = pcard->is_affected_by_effect(EFFECT_EXTRA_RELEASE_NONSUM);
				if(!peffect || (peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT) && peffect->count_limit == 0))
					continue;
				pduel->lua->add_param(core.reason_effect, PARAM_TYPE_EFFECT);
				pduel->lua->add_param(reason, PARAM_TYPE_INT);
				pduel->lua->add_param(core.reason_player, PARAM_TYPE_INT);
				if(!peffect->check_value_condition(3))
					continue;
				if(ex_list_oneof)
					ex_list_oneof->insert(pcard);
				ex_oneof_max = 1;
			}
		}
	}
	return rcount + ex_oneof_max;
}
int32_t field::check_release_list(lua_State* L, uint8_t playerid, int32_t count, int32_t use_con, int32_t use_hand, int32_t fun, int32_t exarg, card* exc, group* exg, uint32_t reason) {
	card_set relcard;
	card_set relcard_oneof;
	get_release_list(L, playerid, &relcard, &relcard, &relcard_oneof, use_con, use_hand, fun, exarg, exc, exg, reason);
	bool has_oneof = false;
	for(auto& pcard : core.must_select_cards) {
		auto it = relcard.find(pcard);
		if(it != relcard.end())
			relcard.erase(it);
		else if(!has_oneof && relcard_oneof.find(pcard) != relcard_oneof.end())
			has_oneof = true;
		else
			return FALSE;
	}
	int32_t rcount = (int32_t)relcard.size();
	if(!has_oneof && !relcard_oneof.empty())
		++rcount;
	return (rcount >= count) ? TRUE : FALSE;
}
// return: the max release count of mg or all monsters on field
int32_t field::get_summon_release_list(card* target, card_set* release_list, card_set* ex_list, card_set* ex_list_oneof, group* mg, uint32_t ex, uint32_t releasable, uint32_t pos) {
	uint8_t p = target->current.controler;
	card_set ex_tribute;
	effect_set eset;
	target->filter_effect(EFFECT_ADD_EXTRA_TRIBUTE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(eset[i]->get_value() & pos)
			filter_inrange_cards(eset[i], &ex_tribute);
	}
	uint32_t rcount = 0;
	for(auto& pcard : player[p].list_mzone) {
		if(pcard && ((releasable >> pcard->current.sequence) & 1) && pcard->is_releasable_by_summon(p, target)) {
			if(mg && !mg->has_card(pcard))
				continue;
			if(release_list)
				release_list->insert(pcard);
			if(pcard->is_affected_by_effect(EFFECT_DOUBLE_TRIBUTE, target))
				pcard->release_param = 2;
			else
				pcard->release_param = 1;
			rcount += pcard->release_param;
		}
	}
	uint32_t ex_oneof_max = 0;
	for(auto& pcard : player[1 - p].list_mzone) {
		if(!pcard || !((releasable >> (pcard->current.sequence + 16)) & 1) || !pcard->is_releasable_by_summon(p, target))
			continue;
		if(mg && !mg->has_card(pcard))
			continue;
		if(pcard->is_affected_by_effect(EFFECT_DOUBLE_TRIBUTE, target))
			pcard->release_param = 2;
		else
			pcard->release_param = 1;
		if(ex || ex_tribute.find(pcard) != ex_tribute.end()) {
			if(release_list)
				release_list->insert(pcard);
			rcount += pcard->release_param;
		} else if(pcard->is_affected_by_effect(EFFECT_EXTRA_RELEASE)) {
			if(ex_list)
				ex_list->insert(pcard);
			rcount += pcard->release_param;
		} else {
			effect* peffect = pcard->is_affected_by_effect(EFFECT_EXTRA_RELEASE_SUM);
			if(!peffect || (peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT) && peffect->count_limit == 0))
				continue;
			if(ex_list_oneof)
				ex_list_oneof->insert(pcard);
			if(ex_oneof_max < pcard->release_param)
				ex_oneof_max = pcard->release_param;
		}
	}
	for(auto& pcard : ex_tribute) {
		if(pcard->current.location == LOCATION_MZONE || !pcard->is_releasable_by_summon(p, target))
			continue;
		if(release_list)
			release_list->insert(pcard);
		if(pcard->is_affected_by_effect(EFFECT_DOUBLE_TRIBUTE, target))
			pcard->release_param = 2;
		else
			pcard->release_param = 1;
		rcount += pcard->release_param;
	}
	return rcount + ex_oneof_max;
}
int32_t field::get_summon_count_limit(uint8_t playerid) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_SET_SUMMON_COUNT_LIMIT, &eset);
	int32_t count = 1;
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		int32_t c = eset[i]->get_value();
		if(c > count)
			count = c;
	}
	return count;
}
int32_t field::get_draw_count(uint8_t playerid) {
	if ((core.duel_rule >= 3) && (infos.turn_id == 1) && (infos.turn_player == playerid)) {
		return 0;
	}
	effect_set eset;
	filter_player_effect(playerid, EFFECT_DRAW_COUNT, &eset);
	int32_t count = player[playerid].draw_count;
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		int32_t c = eset[i]->get_value();
		if(c == 0)
			return 0;
		if(c > count)
			count = c;
	}
	return count;
}
void field::get_ritual_material(uint8_t playerid, effect* peffect, card_set* material, uint8_t no_level) {
	for(auto& pcard : player[playerid].list_mzone) {
		if(pcard && pcard->is_affect_by_effect(peffect)
		        && pcard->is_releasable_by_nonsummon(playerid, REASON_EFFECT) && pcard->is_releasable_by_effect(playerid, peffect)
				&& (no_level || pcard->get_level() > 0 || pcard->is_affected_by_effect(EFFECT_RITUAL_LEVEL_EX)))
			material->insert(pcard);
		if(pcard && pcard->is_affected_by_effect(EFFECT_OVERLAY_RITUAL_MATERIAL))
			for(auto& mcard : pcard->xyz_materials)
				if (no_level || mcard->get_level() > 0 || pcard->is_affected_by_effect(EFFECT_RITUAL_LEVEL_EX))
					material->insert(mcard);
	}
	for(auto& pcard : player[1 - playerid].list_mzone) {
		if(pcard && pcard->is_affect_by_effect(peffect)
		        && pcard->is_affected_by_effect(EFFECT_EXTRA_RELEASE) && pcard->is_position(POS_FACEUP)
		        && pcard->is_releasable_by_nonsummon(playerid, REASON_EFFECT) && pcard->is_releasable_by_effect(playerid, peffect)
				&& (no_level || pcard->get_level() > 0 || pcard->is_affected_by_effect(EFFECT_RITUAL_LEVEL_EX)))
			material->insert(pcard);
	}
	for(auto& pcard : player[playerid].list_hand)
		if((pcard->data.type & TYPE_MONSTER) && pcard->is_releasable_by_nonsummon(playerid, REASON_EFFECT))
			material->insert(pcard);
	for(auto& pcard : player[playerid].list_grave)
		if((pcard->data.type & TYPE_MONSTER)
				&& pcard->is_affected_by_effect(EFFECT_EXTRA_RITUAL_MATERIAL) && pcard->is_removeable(playerid, POS_FACEUP, REASON_EFFECT)
				&& (no_level || pcard->get_level() > 0 || pcard->is_affected_by_effect(EFFECT_RITUAL_LEVEL_EX)))
			material->insert(pcard);
	for(auto& pcard : player[playerid].list_extra)
		if(pcard->is_affected_by_effect(EFFECT_EXTRA_RITUAL_MATERIAL)
				&& (no_level || pcard->get_level() > 0 || pcard->is_affected_by_effect(EFFECT_RITUAL_LEVEL_EX)))
			material->insert(pcard);
}
void field::get_fusion_material(uint8_t playerid, card_set* material_all, card_set* material_base, uint32_t location) {
	if(location & LOCATION_MZONE) {
		for(auto& pcard : player[playerid].list_mzone) {
			if(pcard && !pcard->is_treated_as_not_on_field())
				material_base->insert(pcard);
		}
	}
	if(location & LOCATION_HAND) {
		for(auto& pcard : player[playerid].list_hand) {
			if(pcard->data.type & TYPE_MONSTER)
				material_base->insert(pcard);
		}
	}
	if(location & LOCATION_GRAVE) {
		for(auto& pcard : player[playerid].list_grave) {
			if(pcard->data.type & TYPE_MONSTER)
				material_base->insert(pcard);
		}
	}
	if(location & LOCATION_REMOVED) {
		for(auto& pcard : player[playerid].list_remove) {
			if(pcard->data.type & TYPE_MONSTER)
				material_base->insert(pcard);
		}
	}
	if(location & LOCATION_DECK) {
		for(auto& pcard : player[playerid].list_main) {
			if(pcard->data.type & TYPE_MONSTER)
				material_base->insert(pcard);
		}
	}
	if(location & LOCATION_EXTRA) {
		for(auto& pcard : player[playerid].list_extra) {
			if(pcard->data.type & TYPE_MONSTER)
				material_base->insert(pcard);
		}
	}
	if(location & LOCATION_SZONE) {
		for(auto& pcard : player[playerid].list_szone) {
			if(pcard && pcard->data.type & TYPE_MONSTER && !pcard->is_treated_as_not_on_field())
				material_base->insert(pcard);
		}
	}
	if(location & LOCATION_PZONE) {
		for(auto& pcard : player[playerid].list_szone) {
			if(pcard && pcard->current.pzone && pcard->data.type & TYPE_MONSTER && !pcard->is_treated_as_not_on_field())
				material_base->insert(pcard);
		}
	}
	for(auto& pcard : player[playerid].list_szone) {
		if(pcard && pcard->is_affected_by_effect(EFFECT_EXTRA_FUSION_MATERIAL))
			material_all->insert(pcard);
	}
	for(auto& pcard : player[playerid].list_grave) {
		if(pcard->is_affected_by_effect(EFFECT_EXTRA_FUSION_MATERIAL))
			material_all->insert(pcard);
	}
	material_all->insert(material_base->begin(), material_base->end());
}
void field::ritual_release(const card_set& material) {
	card_set rel;
	card_set rem;
	card_set tgy;
	for(auto& pcard : material) {
		if(pcard->current.location == LOCATION_GRAVE)
			rem.insert(pcard);
		else if(pcard->current.location == LOCATION_OVERLAY || pcard->current.location == LOCATION_EXTRA)
			tgy.insert(pcard);
		else
			rel.insert(pcard);
	}
	send_to(tgy, core.reason_effect, REASON_RITUAL + REASON_EFFECT + REASON_MATERIAL, core.reason_player, PLAYER_NONE, LOCATION_GRAVE, 0, POS_FACEUP);
	release(rel, core.reason_effect, REASON_RITUAL + REASON_EFFECT + REASON_MATERIAL, core.reason_player);
	send_to(rem, core.reason_effect, REASON_RITUAL + REASON_EFFECT + REASON_MATERIAL, core.reason_player, PLAYER_NONE, LOCATION_REMOVED, 0, POS_FACEUP);
}
void field::get_xyz_material(lua_State* L, card* scard, int32_t findex, uint32_t lv, int32_t maxc, group* mg) {
	core.xmaterial_lst.clear();
	uint32_t xyz_level;
	if(mg) {
		for (auto& pcard : mg->container) {
			if(pcard->is_can_be_xyz_material(scard) && (xyz_level = pcard->check_xyz_level(scard, lv))
					&& (findex == 0 || pduel->lua->check_filter(L, pcard, findex, 0)))
				core.xmaterial_lst.emplace((xyz_level >> 12) & 0xf, pcard);
		}
	} else {
		int32_t playerid = scard->current.controler;
		for(auto& pcard : player[playerid].list_mzone) {
			if(pcard && pcard->is_position(POS_FACEUP) && !pcard->is_treated_as_not_on_field()
					&& pcard->is_can_be_xyz_material(scard) && (xyz_level = pcard->check_xyz_level(scard, lv))
					&& (findex == 0 || pduel->lua->check_filter(L, pcard, findex, 0)))
				core.xmaterial_lst.emplace((xyz_level >> 12) & 0xf, pcard);
		}
		for(auto& pcard : player[1 - playerid].list_mzone) {
			if(pcard && pcard->is_position(POS_FACEUP) && !pcard->is_treated_as_not_on_field()
					&& pcard->is_can_be_xyz_material(scard) && (xyz_level = pcard->check_xyz_level(scard, lv))
			        && pcard->is_affected_by_effect(EFFECT_XYZ_MATERIAL) && (findex == 0 || pduel->lua->check_filter(L, pcard, findex, 0)))
				core.xmaterial_lst.emplace((xyz_level >> 12) & 0xf, pcard);
		}
	}
	if(core.global_flag & GLOBALFLAG_XMAT_COUNT_LIMIT) {
		if(maxc > (int32_t)core.xmaterial_lst.size())
			maxc = (int32_t)core.xmaterial_lst.size();
		auto iter = core.xmaterial_lst.lower_bound(maxc);
		core.xmaterial_lst.erase(core.xmaterial_lst.begin(), iter);
	}
}
void field::get_overlay_group(uint8_t self, uint8_t s, uint8_t o, card_set* pset) {
	if (!check_playerid(self))
		return;
	uint8_t c = s;
	for(int32_t p = 0; p < 2; ++p) {
		if(c) {
			for(auto& pcard : player[self].list_mzone) {
				if(pcard && !pcard->is_treated_as_not_on_field() && pcard->xyz_materials.size())
					pset->insert(pcard->xyz_materials.begin(), pcard->xyz_materials.end());
			}
		}
		self = 1 - self;
		c = o;
	}
}
int32_t field::get_overlay_count(uint8_t self, uint8_t s, uint8_t o) {
	if (!check_playerid(self))
		return 0;
	uint8_t c = s;
	int32_t count = 0;
	for(int32_t p = 0; p < 2; ++p) {
		if(c) {
			for(auto& pcard : player[self].list_mzone) {
				if(pcard && !pcard->is_treated_as_not_on_field())
					count += (int32_t)pcard->xyz_materials.size();
			}
		}
		self = 1 - self;
		c = o;
	}
	return count;
}
// put all cards in the target of peffect into effects.disable_check_list
void field::update_disable_check_list(effect* peffect) {
	card_set cset;
	filter_affected_cards(peffect, &cset);
	for (auto& pcard : cset)
		add_to_disable_check_list(pcard);
}
void field::add_to_disable_check_list(card* pcard) {
	auto result = effects.disable_check_set.insert(pcard);
	if(!result.second)
		return;
	effects.disable_check_list.push_back(pcard);
}
void field::adjust_disable_check_list() {
	if(!effects.disable_check_list.size())
		return;
	do {
		card_set checked;
		while(effects.disable_check_list.size()) {
			card* checking = effects.disable_check_list.front();
			effects.disable_check_list.pop_front();
			effects.disable_check_set.erase(checking);
			checked.insert(checking);
			if(checking->is_status(STATUS_TO_ENABLE | STATUS_TO_DISABLE)) // prevent loop
				continue;
			int32_t pre_disable = checking->get_status(STATUS_DISABLED | STATUS_FORBIDDEN);
			checking->refresh_disable_status();
			int32_t new_disable = checking->get_status(STATUS_DISABLED | STATUS_FORBIDDEN);
			if(pre_disable != new_disable && checking->is_status(STATUS_EFFECT_ENABLED)) {
				checking->filter_disable_related_cards(); // change effects.disable_check_list
				if(pre_disable)
					checking->set_status(STATUS_TO_ENABLE, TRUE);
				else
					checking->set_status(STATUS_TO_DISABLE, TRUE);
			}
		}
		for(auto& pcard : checked) {
			if(pcard->is_status(STATUS_DISABLED) && pcard->is_status(STATUS_TO_DISABLE) && !pcard->is_status(STATUS_TO_ENABLE))
				pcard->reset(RESET_DISABLE, RESET_EVENT);
			pcard->set_status(STATUS_TO_ENABLE | STATUS_TO_DISABLE, FALSE);
		}
	} while(effects.disable_check_list.size());
}
// adjust SetUniqueOnField(), EFFECT_SELF_DESTROY, EFFECT_SELF_TOGRAVE
void field::adjust_self_destroy_set() {
	if(core.selfdes_disabled || !core.unique_destroy_set.empty() || !core.self_destroy_set.empty() || !core.self_tograve_set.empty())
		return;
	int32_t check_player = infos.turn_player;
	for(int32_t i = 0; i < 2; ++i) {
		std::vector<card*> uniq_set;
		for(auto& ucard : core.unique_cards[check_player]) {
			if(ucard->is_position(POS_FACEUP) && ucard->get_status(STATUS_EFFECT_ENABLED)
					&& !ucard->get_status(STATUS_DISABLED | STATUS_FORBIDDEN)) {
				card_set cset;
				ucard->get_unique_target(&cset, check_player);
				if(cset.size() == 0)
					ucard->unique_fieldid = 0;
				else if(cset.size() == 1) {
					auto cit = cset.begin();
					ucard->unique_fieldid = (*cit)->fieldid;
				} else
					uniq_set.push_back(ucard);
			}
		}
		std::sort(uniq_set.begin(), uniq_set.end(), [](card* lhs, card* rhs) { return lhs->fieldid < rhs->fieldid; });
		for(auto& pcard : uniq_set) {
			add_process(PROCESSOR_SELF_DESTROY, 0, 0, 0, check_player, 0, 0, 0, pcard);
			core.unique_destroy_set.insert(pcard);
		}
		check_player = 1 - check_player;
	}
	card_set cset;
	for(int32_t p = 0; p < 2; ++p) {
		for(auto& pcard : player[p].list_mzone) {
			if(pcard && pcard->is_position(POS_FACEUP) && !pcard->is_status(STATUS_BATTLE_DESTROYED))
				cset.insert(pcard);
		}
		for(auto& pcard : player[p].list_szone) {
			if(pcard && pcard->is_position(POS_FACEUP))
				cset.insert(pcard);
		}
	}
	for(auto& pcard : cset) {
		effect* peffect = pcard->is_affected_by_effect(EFFECT_SELF_DESTROY);
		if(peffect) {
			core.self_destroy_set.insert(pcard);
		}
	}
	if(core.global_flag & GLOBALFLAG_SELF_TOGRAVE) {
		for(auto& pcard : cset) {
			effect* peffect = pcard->is_affected_by_effect(EFFECT_SELF_TOGRAVE);
			if(peffect) {
				core.self_tograve_set.insert(pcard);
			}
		}
	}
	if(!core.self_destroy_set.empty())
		add_process(PROCESSOR_SELF_DESTROY, 10, 0, 0, 0, 0);
	if(!core.self_tograve_set.empty())
		add_process(PROCESSOR_SELF_DESTROY, 20, 0, 0, 0, 0);
}
void field::erase_grant_effect(effect* peffect) {
	auto eit = effects.grant_effect.find(peffect);
	if (eit == effects.grant_effect.end())
		return;
	for(auto& it : eit->second)
		it.first->remove_effect(it.second);
	effects.grant_effect.erase(eit);
}
int32_t field::adjust_grant_effect() {
	int32_t adjusted = FALSE;
	for(auto& eit : effects.grant_effect) {
		effect* peffect = eit.first;
		if (peffect->object_type != PARAM_TYPE_EFFECT)
			continue;
		effect* geffect = (effect*)peffect->get_label_object();
		if (geffect->type & EFFECT_TYPE_GRANT)
			continue;
		if (geffect->code == EFFECT_UNIQUE_CHECK)
			continue;
		card_set cset;
		if(peffect->is_available())
			filter_affected_cards(peffect, &cset);
		card_set add_set;
		for(auto& pcard : cset) {
			if(pcard->is_affect_by_effect(peffect) && !eit.second.count(pcard))
				add_set.insert(pcard);
		}
		card_set remove_set;
		for(auto& cit : eit.second) {
			card* pcard = cit.first;
			if(!pcard->is_affect_by_effect(peffect) || !cset.count(pcard))
				remove_set.insert(pcard);
		}
		for(auto& pcard : add_set) {
			effect* ceffect = geffect->clone();
			ceffect->owner = pcard;
			pcard->add_effect(ceffect);
			eit.second.emplace(pcard, ceffect);
		}
		for(auto& pcard : remove_set) {
			auto it = eit.second.find(pcard);
			pcard->remove_effect(it->second);
			eit.second.erase(it);
		}
		if(!add_set.empty() || !remove_set.empty())
			adjusted = TRUE;
	}
	return adjusted;
}
void field::add_unique_card(card* pcard) {
	uint8_t con = pcard->current.controler;
	if(pcard->unique_pos[0])
		core.unique_cards[con].insert(pcard);
	if(pcard->unique_pos[1])
		core.unique_cards[1 - con].insert(pcard);
	pcard->unique_fieldid = 0;
}
void field::remove_unique_card(card* pcard) {
	uint8_t con = pcard->current.controler;
	if (!check_playerid(con))
		return;
	if(pcard->unique_pos[0])
		core.unique_cards[con].erase(pcard);
	if(pcard->unique_pos[1])
		core.unique_cards[1 - con].erase(pcard);
}
// return: pcard->unique_effect or 0
effect* field::check_unique_onfield(card* pcard, uint8_t controler, uint8_t location, card* icard) {
	for(auto& ucard : core.unique_cards[controler]) {
		if((ucard != pcard) && (ucard != icard) && ucard->is_position(POS_FACEUP) && ucard->get_status(STATUS_EFFECT_ENABLED)
			&& !ucard->get_status(STATUS_DISABLED | STATUS_FORBIDDEN)
			&& ucard->unique_fieldid && ucard->check_unique_code(pcard) && (ucard->unique_location & location))
			return ucard->unique_effect;
	}
	if(!pcard->unique_code || !(pcard->unique_location & location) || pcard->get_status(STATUS_DISABLED | STATUS_FORBIDDEN))
		return 0;
	card_set cset;
	pcard->get_unique_target(&cset, controler, icard);
	if(pcard->check_unique_code(pcard))
		cset.insert(pcard);
	if(cset.size() >= 2)
		return pcard->unique_effect;
	return nullptr;
}
int32_t field::check_spsummon_once(card* pcard, uint8_t playerid) {
	if(pcard->spsummon_code == 0)
		return TRUE;
	auto iter = core.spsummon_once_map[playerid].find(pcard->spsummon_code);
	return (iter == core.spsummon_once_map[playerid].end()) || (iter->second == 0);
}
// increase the binary custom counter 1~5
void field::check_card_counter(card* pcard, int32_t counter_type, int32_t playerid) {
	auto& counter_map = (counter_type == ACTIVITY_SUMMON) ? core.summon_counter :
						(counter_type == ACTIVITY_NORMALSUMMON) ? core.normalsummon_counter :
						(counter_type == ACTIVITY_SPSUMMON) ? core.spsummon_counter :
						(counter_type == ACTIVITY_FLIPSUMMON) ? core.flipsummon_counter : core.attack_counter;
	for(auto& iter : counter_map) {
		auto& info = iter.second;
		if((playerid == 0) && (info.second & 0xffff) != 0)
			continue;
		if((playerid == 1) && (info.second & 0xffff0000) != 0)
			continue;
		if(info.first) {
			pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
			if(!pduel->lua->check_condition(info.first, 1)) {
				if(playerid == 0)
					info.second += 0x1;
				else
					info.second += 0x10000;
			}
		}
	}
}
void field::check_card_counter(group* pgroup, int32_t counter_type, int32_t playerid) {
	auto& counter_map = (counter_type == ACTIVITY_SUMMON) ? core.summon_counter :
						(counter_type == ACTIVITY_NORMALSUMMON) ? core.normalsummon_counter :
						(counter_type == ACTIVITY_SPSUMMON) ? core.spsummon_counter :
						(counter_type == ACTIVITY_FLIPSUMMON) ? core.flipsummon_counter : core.attack_counter;
	for(auto& iter : counter_map) {
		auto& info = iter.second;
		if((playerid == 0) && (info.second & 0xffff) != 0)
			continue;
		if((playerid == 1) && (info.second & 0xffff0000) != 0)
			continue;
		if(info.first) {
			for(auto& pcard : pgroup->container) {
				pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
				if(!pduel->lua->check_condition(info.first, 1)) {
					if(playerid == 0)
						info.second += 0x1;
					else
						info.second += 0x10000;
					break;
				}
			}
		}
	}
}

void field::check_chain_counter(effect* peffect, int32_t playerid, int32_t chainid, bool cancel) {
	for(auto& iter : core.chain_counter) {
		auto& info = iter.second;
		if(info.first) {
			pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			pduel->lua->add_param(chainid, PARAM_TYPE_INT);
			if(!pduel->lua->check_condition(info.first, 3)) {
				if(playerid == 0) {
					if(!cancel)
						info.second += 0x1;
					else if(info.second & 0xffff)
						info.second -= 0x1;
				} else {
					if(!cancel)
						info.second += 0x10000;
					else if(info.second & 0xffff0000)
						info.second -= 0x10000;
				}
			}
		}
	}
}
void field::set_spsummon_counter(uint8_t playerid) {
	++core.spsummon_state_count[playerid];
	if(core.global_flag & GLOBALFLAG_SPSUMMON_COUNT) {
		for(auto& peffect : effects.spsummon_count_eff) {
			card* pcard = peffect->get_handler();
			if(peffect->limit_counter_is_available()) {
				if(((playerid == pcard->current.controler) && peffect->s_range) || ((playerid != pcard->current.controler) && peffect->o_range)) {
					++pcard->spsummon_counter[playerid];
				}
			}
		}
	}
}
int32_t field::check_spsummon_counter(uint8_t playerid, uint8_t ct) {
	if(core.global_flag & GLOBALFLAG_SPSUMMON_COUNT) {
		for(auto& peffect : effects.spsummon_count_eff) {
			card* pcard = peffect->get_handler();
			uint16_t val = (uint16_t)peffect->value;
			if(peffect->is_available()) {
				if(pcard->spsummon_counter[playerid] + ct > val)
					return FALSE;
			}
		}
	}
	return TRUE;
}
bool field::is_select_hide_deck_sequence(uint8_t playerid) {
	return !core.select_deck_sequence_revealed && !(core.duel_options & DUEL_REVEAL_DECK_SEQ) && playerid == core.selecting_player;
}
int32_t field::check_lp_cost(uint8_t playerid, int32_t cost, uint32_t must_pay) {
	effect_set eset;
	int32_t val = cost;
	filter_player_effect(playerid, EFFECT_LPCOST_CHANGE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(core.reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(val, PARAM_TYPE_INT);
		val = eset[i]->get_value(3);
	}
	if(val <= 0) {
		if(must_pay)
			return FALSE;
		return TRUE;
	}
	if(!must_pay) {
		tevent e;
		e.event_cards = 0;
		e.event_player = playerid;
		e.event_value = cost;
		e.reason = 0;
		e.reason_effect = core.reason_effect;
		e.reason_player = playerid;
		if(effect_replace_check(EFFECT_LPCOST_REPLACE, e))
			return TRUE;
	}
	//cost[playerid].amount += val;
	if(val <= player[playerid].lp)
		return TRUE;
	return FALSE;
}
/*
void field::save_lp_cost() {
	for(uint8_t playerid = 0; playerid < 2; ++playerid) {
		if(cost[playerid].count < 8)
			cost[playerid].lpstack[cost[playerid].count] = cost[playerid].amount;
		++cost[playerid].count;
	}
}
void field::restore_lp_cost() {
	for(uint8_t playerid = 0; playerid < 2; ++playerid) {
		--cost[playerid].count;
		if(cost[playerid].count < 8)
			cost[playerid].amount = cost[playerid].lpstack[cost[playerid].count];
	}
}
*/
uint32_t field::get_field_counter(uint8_t self, uint8_t s, uint8_t o, uint16_t countertype) {
	uint8_t c = s;
	uint32_t count = 0;
	for(int32_t p = 0; p < 2; ++p) {
		if(c) {
			for(auto& pcard : player[self].list_mzone) {
				if(pcard)
					count += pcard->get_counter(countertype);
			}
			for(auto& pcard : player[self].list_szone) {
				if(pcard)
					count += pcard->get_counter(countertype);
			}
		}
		self = 1 - self;
		c = o;
	}
	return count;
}
int32_t field::effect_replace_check(uint32_t code, const tevent& e) {
	auto pr = effects.continuous_effect.equal_range(code);
	for(auto eit = pr.first; eit != pr.second;) {
		effect* peffect = eit->second;
		++eit;
		if(peffect->is_activateable(peffect->get_handler_player(), e))
			return TRUE;
	}
	return FALSE;
}
int32_t field::get_attack_target(card* pcard, card_vector* v, uint8_t chain_attack, bool select_target) {
	pcard->direct_attackable = 0;
	uint8_t p = pcard->current.controler;
	card_vector auto_attack, only_attack, must_attack;
	for(auto& atarget : player[1 - p].list_mzone) {
		if(atarget) {
			if(atarget->is_affected_by_effect(EFFECT_ONLY_BE_ATTACKED))
				auto_attack.push_back(atarget);
			if(pcard->is_affected_by_effect(EFFECT_ONLY_ATTACK_MONSTER, atarget))
				only_attack.push_back(atarget);
			if(pcard->is_affected_by_effect(EFFECT_MUST_ATTACK_MONSTER, atarget))
				must_attack.push_back(atarget);
		}
	}
	card_vector* pv = nullptr;
	int32_t atype = 0;
	if(auto_attack.size()) {
		atype = 1;
		if(auto_attack.size() == 1)
			pv = &auto_attack;
		else
			return atype;
	} else if(pcard->is_affected_by_effect(EFFECT_ONLY_ATTACK_MONSTER)) {
		atype = 2;
		if(only_attack.size() == 1)
			pv = &only_attack;
		else
			return atype;
	} else if(pcard->is_affected_by_effect(EFFECT_MUST_ATTACK_MONSTER)) {
		atype = 3;
		if(must_attack.size())
			pv = &must_attack;
		else
			return atype;
	} else {
		atype = 4;
		pv = &player[1 - p].list_mzone;
	}
	int32_t extra_count = 0;
	effect_set eset;
	pcard->filter_effect(EFFECT_EXTRA_ATTACK, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		int32_t val = eset[i]->get_value(pcard);
		if(val > extra_count)
			extra_count = val;
	}
	int32_t extra_count_m = 0;
	eset.clear();
	pcard->filter_effect(EFFECT_EXTRA_ATTACK_MONSTER, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		int32_t val = eset[i]->get_value(pcard);
		if(val > extra_count_m)
			extra_count_m = val;
	}
	if(!chain_attack && pcard->announce_count > extra_count
		&& (!extra_count_m || pcard->announce_count > extra_count_m || pcard->announced_cards.findcard(0))) {
		effect* peffect;
		if((peffect = pcard->is_affected_by_effect(EFFECT_ATTACK_ALL)) && pcard->attack_all_target) {
			for(auto& atarget : *pv) {
				if(!atarget)
					continue;
				pduel->lua->add_param(atarget, PARAM_TYPE_CARD);
				if(!peffect->check_value_condition(1))
					continue;
				if(pcard->announced_cards.findcard(atarget) >= (uint32_t)peffect->get_value(atarget))
					continue;
				if(atype >= 2 && atarget->is_affected_by_effect(EFFECT_IGNORE_BATTLE_TARGET, pcard))
					continue;
				if(select_target && (atype == 2 || atype == 4)) {
					if(atarget->is_affected_by_effect(EFFECT_CANNOT_BE_BATTLE_TARGET, pcard))
						continue;
					if(pcard->is_affected_by_effect(EFFECT_CANNOT_SELECT_BATTLE_TARGET, atarget))
						continue;
				}
				v->push_back(atarget);
			}
		}
		return atype;
	}
	//chain attack or announce count check passed
	uint32_t mcount = 0;
	for(auto& atarget : *pv) {
		if(!atarget)
			continue;
		if(atype >= 2 && atarget->is_affected_by_effect(EFFECT_IGNORE_BATTLE_TARGET, pcard))
			continue;
		++mcount;
		if(chain_attack && core.chain_attack_target && atarget != core.chain_attack_target)
			continue;
		if(select_target && (atype == 2 || atype == 4)) {
			if(atarget->is_affected_by_effect(EFFECT_CANNOT_BE_BATTLE_TARGET, pcard))
				continue;
			if(pcard->is_affected_by_effect(EFFECT_CANNOT_SELECT_BATTLE_TARGET, atarget))
				continue;
		}
		v->push_back(atarget);
	}
	if(atype <= 3)
		return atype;
	if((mcount == 0 || pcard->is_affected_by_effect(EFFECT_DIRECT_ATTACK) || core.attack_player)
		&& !pcard->is_affected_by_effect(EFFECT_CANNOT_DIRECT_ATTACK)
		&& !(!chain_attack && extra_count_m && pcard->announce_count > extra_count)
		&& !(chain_attack && core.chain_attack_target))
		pcard->direct_attackable = 1;
	return atype;
}
bool field::confirm_attack_target() {
	card_vector cv;
	get_attack_target(core.attacker, &cv, core.chain_attack, false);
	return core.attack_target && std::find(cv.begin(), cv.end(), core.attack_target) != cv.end()
		|| !core.attack_target && core.attacker->direct_attackable;
}
// update the validity for EFFECT_ATTACK_ALL (approximate solution)
void field::attack_all_target_check() {
	if(!core.attacker)
		return;
	if(!core.attack_target) {
		core.attacker->attack_all_target = FALSE;
		return;
	}
	effect* peffect = core.attacker->is_affected_by_effect(EFFECT_ATTACK_ALL);
	if(!peffect)
		return;
	if(!peffect->get_value(core.attack_target))
		core.attacker->attack_all_target = FALSE;
}
int32_t field::get_must_material_list(uint8_t playerid, uint32_t limit, card_set* must_list) {
	effect_set eset;
	filter_player_effect(playerid, limit, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i)
		must_list->insert(eset[i]->handler);
	return (int32_t)must_list->size();
}
int32_t field::check_must_material(group* mg, uint8_t playerid, uint32_t limit) {
	card_set must_list;
	int32_t m = get_must_material_list(playerid, limit, &must_list);
	if(m <= 0)
		return TRUE;
	if(!mg)
		return FALSE;
	for(auto& pcard : must_list)
		if(!mg->has_card(pcard))
			return FALSE;
	return TRUE;
}
void field::get_synchro_material(uint8_t playerid, card_set* material, effect* tuner_limit) {
	if(tuner_limit && tuner_limit->value) {
		int32_t location = tuner_limit->value;
		if(location & LOCATION_MZONE) {
			for(auto& pcard : player[playerid].list_mzone) {
				if(pcard && !pcard->is_treated_as_not_on_field())
					material->insert(pcard);
			}
		}
		if(location & LOCATION_HAND) {
			for(auto& pcard : player[playerid].list_hand) {
				if(pcard)
					material->insert(pcard);
			}
		}
	} else {
		for(auto& pcard : player[playerid].list_mzone) {
			if(pcard && !pcard->is_treated_as_not_on_field())
				material->insert(pcard);
		}
		for(auto& pcard : player[1 - playerid].list_mzone) {
			if(pcard && pcard->is_affected_by_effect(EFFECT_EXTRA_SYNCHRO_MATERIAL) && !pcard->is_treated_as_not_on_field())
				material->insert(pcard);
		}
		for(auto& pcard : player[playerid].list_hand) {
			if(pcard && pcard->is_affected_by_effect(EFFECT_EXTRA_SYNCHRO_MATERIAL))
				material->insert(pcard);
		}
	}
}
int32_t field::check_synchro_material(lua_State* L, card* pcard, int32_t findex1, int32_t findex2, int32_t min, int32_t max, card* smat, group* mg) {
	if(mg) {
		for(auto& tuner : mg->container) {
			if(check_tuner_material(L, pcard, tuner, findex1, findex2, min, max, nullptr, mg))
				return TRUE;
		}
	} else {
		card_set material;
		get_synchro_material(pcard->current.controler, &material);
		for(auto& tuner : material) {
			if(check_tuner_material(L, pcard, tuner, findex1, findex2, min, max, smat, nullptr))
				return TRUE;
		}
	}
	return FALSE;
}
int32_t field::check_tuner_material(lua_State* L, card* pcard, card* tuner, int32_t findex1, int32_t findex2, int32_t min, int32_t max, card* smat, group* mg) {
	if(!tuner || (tuner->current.location == LOCATION_MZONE && !tuner->is_position(POS_FACEUP)) || !tuner->is_tuner(pcard) || !tuner->is_can_be_synchro_material(pcard))
		return FALSE;
	effect* pcheck = tuner->is_affected_by_effect(EFFECT_SYNCHRO_CHECK);
	if(pcheck)
		pcheck->get_value(tuner);
	if((mg && !mg->has_card(tuner)) || !pduel->lua->check_filter(L, tuner, findex1, 0)) {
		pduel->restore_assumes();
		return FALSE;
	}
	effect* pcustom = tuner->is_affected_by_effect(EFFECT_SYNCHRO_MATERIAL_CUSTOM, pcard);
	if(pcustom) {
		if(!pcustom->target) {
			pduel->restore_assumes();
			return FALSE;
		}
		pduel->lua->add_param(pcustom, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(interpreter::get_function_handle(L, findex2), PARAM_TYPE_FUNCTION);
		pduel->lua->add_param(min, PARAM_TYPE_INT);
		pduel->lua->add_param(max, PARAM_TYPE_INT);
		if(pduel->lua->check_condition(pcustom->target, 5)) {
			pduel->restore_assumes();
			return TRUE;
		}
		pduel->restore_assumes();
		return FALSE;
	}
	int32_t playerid = pcard->current.controler;
	card_set must_list;
	get_must_material_list(playerid, EFFECT_MUST_BE_SMATERIAL, &must_list);
	must_list.erase(tuner);
	int32_t location_count = get_spsummonable_count(pcard, playerid);
	card_set handover_zone_cards;
	if(location_count <= 0) {
		uint32_t must_use_zone_flag = 0;
		filter_must_use_mzone(playerid, playerid, LOCATION_REASON_TOFIELD, pcard, &must_use_zone_flag);
		uint32_t handover_zone = get_rule_zone_fromex(playerid, pcard) & ~must_use_zone_flag;
		get_cards_in_zone(&handover_zone_cards, handover_zone, playerid, LOCATION_MZONE);
		if(handover_zone_cards.find(tuner) != handover_zone_cards.end())
			++location_count;
	}
	int32_t location = LOCATION_MZONE;
	effect* tuner_limit = tuner->is_affected_by_effect(EFFECT_TUNER_MATERIAL_LIMIT);
	if(tuner_limit) {
		if(tuner_limit->get_integer_value())
			location = tuner_limit->get_integer_value();
		if(tuner_limit->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
			if(tuner_limit->s_range && tuner_limit->s_range > min)
				min = tuner_limit->s_range;
			if(tuner_limit->o_range && tuner_limit->o_range < max)
				max = tuner_limit->o_range;
		}
		if(min > max) {
			pduel->restore_assumes();
			return FALSE;
		}
	}
	int32_t mzone_limit = get_mzone_limit(playerid, playerid, LOCATION_REASON_TOFIELD);
	if(mzone_limit < 0) {
		if(location == LOCATION_HAND) {
			pduel->restore_assumes();
			return FALSE;
		}
		int32_t ft = -mzone_limit;
		if(ft > min)
			min = ft;
		if(min > max) {
			pduel->restore_assumes();
			return FALSE;
		}
	}
	int32_t lv = pcard->get_level();
	card_vector nsyn;
	int32_t mcount = 1;
	nsyn.push_back(tuner);
	tuner->sum_param = tuner->get_synchro_level(pcard);
	if(smat) {
		if(pcheck)
			pcheck->get_value(smat);
		if((smat->current.location == LOCATION_MZONE && !smat->is_position(POS_FACEUP)) || !smat->is_can_be_synchro_material(pcard, tuner)) {
			pduel->restore_assumes();
			return FALSE;
		}
		interpreter::card2value(L, pcard);
		auto res = pduel->lua->check_filter(L, smat, findex2, 1);
		lua_pop(L, 1);
		if (!res) {
			pduel->restore_assumes();
			return FALSE;
		}
		if(tuner_limit) {
			if(tuner_limit->target) {
				pduel->lua->add_param(tuner_limit, PARAM_TYPE_EFFECT);
				pduel->lua->add_param(smat, PARAM_TYPE_CARD);
				if(!pduel->lua->get_function_value(tuner_limit->target, 2)) {
					pduel->restore_assumes();
					return FALSE;
				}
			}
			if(tuner_limit->get_integer_value() && !((uint32_t)smat->current.location & tuner_limit->get_integer_value())) {
				pduel->restore_assumes();
				return FALSE;
			}
		}
		--min;
		--max;
		nsyn.push_back(smat);
		smat->sum_param = smat->get_synchro_level(pcard);
		++mcount;
		must_list.erase(smat);
		if(location_count <= 0) {
			if(handover_zone_cards.find(smat) != handover_zone_cards.end())
				++location_count;
		}
		if(min == 0) {
			if(location_count > 0 && check_with_sum_limit_m(nsyn, lv, 0, 0, 0, 0xffff, 2)) {
				pduel->restore_assumes();
				return TRUE;
			}
			if(max == 0) {
				pduel->restore_assumes();
				return FALSE;
			}
		}
	}
	if(must_list.size()) {
		for(auto& mcard : must_list) {
			if(pcheck)
				pcheck->get_value(mcard);
			if((mcard->current.location == LOCATION_MZONE && !mcard->is_position(POS_FACEUP)) || !mcard->is_can_be_synchro_material(pcard, tuner)) {
				pduel->restore_assumes();
				return FALSE;
			}
			interpreter::card2value(L, pcard);
			auto res = pduel->lua->check_filter(L, mcard, findex2, 1);
			lua_pop(L, 1);
			if (!res) {
				pduel->restore_assumes();
				return FALSE;
			}
			if(tuner_limit) {
				if(tuner_limit->target) {
					pduel->lua->add_param(tuner_limit, PARAM_TYPE_EFFECT);
					pduel->lua->add_param(mcard, PARAM_TYPE_CARD);
					if(!pduel->lua->get_function_value(tuner_limit->target, 2)) {
						pduel->restore_assumes();
						return FALSE;
					}
				}
				if(tuner_limit->value && !(mcard->current.location & location)) {
					pduel->restore_assumes();
					return FALSE;
				}
			}
			--min;
			--max;
			nsyn.push_back(mcard);
			mcard->sum_param = mcard->get_synchro_level(pcard);
			++mcount;
		}
	}
	if(mg) {
		for(auto& pm : mg->container) {
			if(pm == tuner || pm == smat || must_list.find(pm) != must_list.end() || !pm->is_can_be_synchro_material(pcard, tuner))
				continue;
			if(tuner_limit) {
				if(tuner_limit->target) {
					pduel->lua->add_param(tuner_limit, PARAM_TYPE_EFFECT);
					pduel->lua->add_param(pm, PARAM_TYPE_CARD);
					if(!pduel->lua->get_function_value(tuner_limit->target, 2))
						continue;
				}
				if(tuner_limit->get_integer_value() && !((uint32_t)pm->current.location & tuner_limit->get_integer_value()))
					continue;
			}
			if(pcheck)
				pcheck->get_value(pm);
			if(pm->current.location == LOCATION_MZONE && !pm->is_position(POS_FACEUP))
				continue;
			interpreter::card2value(L, pcard);
			auto res = pduel->lua->check_filter(L, pm, findex2, 1);
			lua_pop(L, 1);
			if (!res)
				continue;
			nsyn.push_back(pm);
			pm->sum_param = pm->get_synchro_level(pcard);
		}
	} else {
		card_set material_set;
		get_synchro_material(playerid, &material_set, tuner_limit);
		for(auto& pm : material_set) {
			if(!pm || pm == tuner || pm == smat || must_list.find(pm) != must_list.end() || !pm->is_can_be_synchro_material(pcard, tuner))
				continue;
			if(tuner_limit && tuner_limit->target) {
				pduel->lua->add_param(tuner_limit, PARAM_TYPE_EFFECT);
				pduel->lua->add_param(pm, PARAM_TYPE_CARD);
				if(!pduel->lua->get_function_value(tuner_limit->target, 2))
					continue;
			}
			if(pcheck)
				pcheck->get_value(pm);
			if(pm->current.location == LOCATION_MZONE && !pm->is_position(POS_FACEUP))
				continue;
			interpreter::card2value(L, pcard);
			auto res = pduel->lua->check_filter(L, pm, findex2, 1);
			lua_pop(L, 1);
			if (!res)
				continue;
			nsyn.push_back(pm);
			pm->sum_param = pm->get_synchro_level(pcard);
		}
	}
	if(location_count > 0) {
		int32_t ret = check_other_synchro_material(nsyn, lv, min, max, mcount);
		pduel->restore_assumes();
		return ret;
	}
	auto start = nsyn.begin() + mcount;
	for(auto cit = start; cit != nsyn.end(); ++cit) {
		card* pm = *cit;
		if(handover_zone_cards.find(pm) == handover_zone_cards.end())
			continue;
		if(start != cit)
			std::iter_swap(start, cit);
		if(check_other_synchro_material(nsyn, lv, min - 1, max - 1, mcount + 1)) {
			pduel->restore_assumes();
			return TRUE;
		}
		if(start != cit)
			std::iter_swap(start, cit);
	}
	pduel->restore_assumes();
	return FALSE;
}
int32_t field::check_other_synchro_material(const card_vector& nsyn, int32_t lv, int32_t min, int32_t max, int32_t mcount) {
	if(!(core.global_flag & GLOBALFLAG_SCRAP_CHIMERA)) {
		if(check_with_sum_limit_m(nsyn, lv, 0, min, max, 0xffff, mcount)) {
			return TRUE;
		}
		return FALSE;
	}
	effect* pscrap = nullptr;
	for(auto& pm : nsyn) {
		pscrap = pm->is_affected_by_effect(EFFECT_SCRAP_CHIMERA);
		if(pscrap)
			break;
	}
	if(!pscrap) {
		if(check_with_sum_limit_m(nsyn, lv, 0, min, max, 0xffff, mcount)) {
			return TRUE;
		}
		return FALSE;
	}
	card_vector nsyn_filtered;
	for(auto& pm : nsyn) {
		if(!pscrap->get_value(pm))
			nsyn_filtered.push_back(pm);
	}
	if(nsyn_filtered.size() == nsyn.size()) {
		if(check_with_sum_limit_m(nsyn, lv, 0, min, max, 0xffff, mcount)) {
			return TRUE;
		}
	} else {
		bool mfiltered = true;
		for(int32_t i = 0; i < mcount; ++i) {
			if(pscrap->get_value(nsyn[i]))
				mfiltered = false;
		}
		if(mfiltered && check_with_sum_limit_m(nsyn_filtered, lv, 0, min, max, 0xffff, mcount)) {
			return TRUE;
		}
		for(int32_t i = 0; i < mcount; ++i) {
			if(nsyn[i]->is_affected_by_effect(EFFECT_SCRAP_CHIMERA)) {
				return FALSE;
			}
		}
		card_vector nsyn_removed;
		for(auto& pm : nsyn) {
			if(!pm->is_affected_by_effect(EFFECT_SCRAP_CHIMERA))
				nsyn_removed.push_back(pm);
		}
		if(check_with_sum_limit_m(nsyn_removed, lv, 0, min, max, 0xffff, mcount)) {
			return TRUE;
		}
	}
	return FALSE;
}
int32_t field::check_tribute(card* pcard, int32_t min, int32_t max, group* mg, uint8_t toplayer, uint32_t zone, uint32_t releasable, uint32_t pos) {
	int32_t ex = FALSE;
	uint32_t sumplayer = pcard->current.controler;
	if(toplayer == 1 - sumplayer)
		ex = TRUE;
	card_set release_list, ex_list;
	int32_t m = get_summon_release_list(pcard, &release_list, &ex_list, 0, mg, ex, releasable, pos);
	if(max > m)
		max = m;
	if(min > max)
		return FALSE;
	zone &= 0x1f;
	int32_t s = 0;
	int32_t ct = get_tofield_count(pcard, toplayer, LOCATION_MZONE, sumplayer, LOCATION_REASON_TOFIELD, zone);
	if(ct <= 0 && max <= 0)
		return FALSE;
	for(auto& release_card : release_list) {
		if(release_card->current.location == LOCATION_MZONE && release_card->current.controler == toplayer) {
			++s;
			if((zone >> release_card->current.sequence) & 1)
				++ct;
		}
	}
	if(ct <= 0)
		return FALSE;
	max -= (int32_t)ex_list.size();
	int32_t fcount = get_mzone_limit(toplayer, sumplayer, LOCATION_REASON_TOFIELD);
	if(s < -fcount + 1)
		return FALSE;
	if(max < 0)
		max = 0;
	if(max < -fcount + 1)
		return FALSE;
	return TRUE;
}
void field::get_sum_params(uint32_t sum_param, int32_t& op1, int32_t& op2) {
	op1 = sum_param & 0xffff;
	op2 = (sum_param >> 16) & 0xffff;
	if(op2 & 0x8000) {
		op1 = sum_param & 0x7fffffff;
		op2 = 0;
	}
}
int32_t field::check_with_sum_limit(const card_vector& mats, int32_t acc, int32_t index, int32_t count, int32_t min, int32_t max, int32_t opmin) {
	if(count > max)
		return FALSE;
	while(index < (int32_t)mats.size()) {
		int32_t op1, op2;
		get_sum_params(mats[index]->sum_param, op1, op2);
		if(((op1 == acc && acc + opmin > op1) || (op2 && op2 == acc && acc + opmin > op2)) && count >= min)
			return TRUE;
		++index;
		if(acc > op1 && check_with_sum_limit(mats, acc - op1, index, count + 1, min, max, std::min(opmin, op1)))
			return TRUE;
		if(op2 && acc > op2 && check_with_sum_limit(mats, acc - op2, index, count + 1, min, max, std::min(opmin, op2)))
			return TRUE;
	}
	return FALSE;
}
int32_t field::check_with_sum_limit_m(const card_vector& mats, int32_t acc, int32_t index, int32_t min, int32_t max, int32_t opmin, int32_t must_count) {
	if(acc == 0)
		return index == must_count && 0 >= min && 0 <= max;
	if(index == must_count)
		return check_with_sum_limit(mats, acc, index, 1, min, max, opmin);
	if(index >= (int32_t)mats.size())
		return FALSE;
	int32_t op1;
	int32_t op2;
	get_sum_params(mats[index]->sum_param, op1, op2);
	if(acc >= op1 && check_with_sum_limit_m(mats, acc - op1, index + 1, min, max, std::min(opmin, op1), must_count))
		return TRUE;
	if(op2 && acc >= op2 && check_with_sum_limit_m(mats, acc - op2, index + 1, min, max, std::min(opmin, op2), must_count))
		return TRUE;
	return FALSE;
}
int32_t field::check_with_sum_greater_limit(const card_vector& mats, int32_t acc, int32_t index, int32_t opmin) {
	while(index < (int32_t)mats.size()) {
		int32_t op1;
		int32_t op2;
		get_sum_params(mats[index]->sum_param, op1, op2);
		if((acc <= op1 && acc + opmin > op1) || (op2 && acc <= op2 && acc + opmin > op2))
			return TRUE;
		++index;
		if(check_with_sum_greater_limit(mats, acc - op1, index, std::min(opmin, op1)))
			return TRUE;
		if(op2 && check_with_sum_greater_limit(mats, acc - op2, index, std::min(opmin, op2)))
			return TRUE;
	}
	return FALSE;
}
int32_t field::check_with_sum_greater_limit_m(const card_vector& mats, int32_t acc, int32_t index, int32_t opmin, int32_t must_count) {
	if(acc <= 0)
		return index == must_count && acc + opmin > 0;
	if(index == must_count)
		return check_with_sum_greater_limit(mats, acc, index, opmin);
	if(index >= (int32_t)mats.size())
		return FALSE;
	int32_t op1;
	int32_t op2;
	get_sum_params(mats[index]->sum_param, op1, op2);
	if(check_with_sum_greater_limit_m(mats, acc - op1, index + 1, std::min(opmin, op1), must_count))
		return TRUE;
	if(op2 && check_with_sum_greater_limit_m(mats, acc - op2, index + 1, std::min(opmin, op2), must_count))
		return TRUE;
	return FALSE;
}
int32_t field::check_xyz_material(lua_State* L, card* scard, int32_t findex, int32_t lv, int32_t min, int32_t max, group* mg) {
	get_xyz_material(L, scard, findex, lv, max, mg);
	int32_t playerid = scard->current.controler;
	int32_t ct = get_spsummonable_count(scard, playerid);
	card_set handover_zone_cards;
	if(ct <= 0) {
		int32_t ft = ct;
		uint32_t must_use_zone_flag = 0;
		filter_must_use_mzone(playerid, playerid, LOCATION_REASON_TOFIELD, scard, &must_use_zone_flag);
		uint32_t handover_zone = get_rule_zone_fromex(playerid, scard) & ~must_use_zone_flag;
		get_cards_in_zone(&handover_zone_cards, handover_zone, playerid, LOCATION_MZONE);
		for(auto cit = core.xmaterial_lst.begin(); cit != core.xmaterial_lst.end(); ++cit) {
			card* pcard = cit->second;
			if(handover_zone_cards.find(pcard) != handover_zone_cards.end())
				++ft;
		}
		if(ft <= 0)
			return FALSE;
	}
	int32_t mzone_limit = get_mzone_limit(playerid, playerid, LOCATION_REASON_TOFIELD);
	if(mzone_limit <= 0) {
		int32_t ft = -mzone_limit + 1;
		if(ft > min)
			min = ft;
		if(min > max)
			return FALSE;
	}
	card_set mcset;
	int32_t mct = get_must_material_list(playerid, EFFECT_MUST_BE_XMATERIAL, &mcset);
	if(mct > 0) {
		if(ct == 0 && std::none_of(mcset.begin(), mcset.end(),
			[=](card* pcard) { return handover_zone_cards.find(pcard) != handover_zone_cards.end(); }))
			++mct;
		if(mct > max)
			return FALSE;
	}
	if(!(core.global_flag & GLOBALFLAG_TUNE_MAGICIAN)) {
		if(std::any_of(mcset.begin(), mcset.end(),
			[=](card* pcard) { return std::find_if(core.xmaterial_lst.begin(), core.xmaterial_lst.end(),
				[=](const std::pair<int32_t, card*>& v) { return v.second == pcard; }) == core.xmaterial_lst.end(); }))
			return FALSE;
		return (int32_t)core.xmaterial_lst.size() >= min;
	}
	for(auto& pcard : mcset) {
		effect* peffect = pcard->is_affected_by_effect(EFFECT_TUNE_MAGICIAN_X);
		if(peffect) {
			for(auto cit = core.xmaterial_lst.begin(); cit != core.xmaterial_lst.end();) {
				if(pcard != cit->second && peffect->get_value(cit->second))
					cit = core.xmaterial_lst.erase(cit);
				else
					++cit;
			}
		}
	}
	for(auto& pcard : mcset) {
		auto it = std::find_if(core.xmaterial_lst.begin(), core.xmaterial_lst.end(),
			[=](const std::pair<int32_t, card*>& v) { return v.second == pcard; });
		if(it == core.xmaterial_lst.end())
			return FALSE;
		if(min < it->first)
			min = it->first;
		core.xmaterial_lst.erase(it);
	}
	for(auto cit = core.xmaterial_lst.begin(); cit != core.xmaterial_lst.end(); ++cit)
		cit->second->sum_param = 0;
	int32_t digit = 1;
	for(auto cit = core.xmaterial_lst.begin(); cit != core.xmaterial_lst.end();) {
		card* pcard = cit->second;
		effect* peffect = pcard->is_affected_by_effect(EFFECT_TUNE_MAGICIAN_X);
		if(peffect) {
			if(std::any_of(mcset.begin(), mcset.end(),
				[=](card* mcard) { return !!peffect->get_value(mcard); })) {
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
	ct += (int32_t)std::count_if(mcset.begin(), mcset.end(),
		[=](card* pcard) { return handover_zone_cards.find(pcard) != handover_zone_cards.end(); });
	std::multimap<int32_t, card*, std::greater<int32_t>> mat;
	for(int32_t icheck = 1; icheck <= digit; icheck <<= 1) {
		mat.clear();
		for(auto cit = core.xmaterial_lst.begin(); cit != core.xmaterial_lst.end(); ++cit) {
			if(cit->second->sum_param & icheck)
				mat.insert(*cit);
		}
		if(core.global_flag & GLOBALFLAG_XMAT_COUNT_LIMIT) {
			int32_t maxc = std::min(max, (int32_t)mat.size() + mct);
			auto iter = mat.lower_bound(maxc);
			mat.erase(mat.begin(), iter);
		}
		if(ct <= 0) {
			int32_t ft = ct;
			for(auto cit = mat.begin(); cit != mat.end(); ++cit) {
				card* pcard = cit->second;
				if(handover_zone_cards.find(pcard) != handover_zone_cards.end())
					++ft;
			}
			if(ft <= 0)
				continue;
		}
		if((int32_t)mat.size() + mct >= min)
			return TRUE;
	}
	return FALSE;
}
int32_t field::is_player_can_draw(uint8_t playerid) {
	return !is_player_affected_by_effect(playerid, EFFECT_CANNOT_DRAW);
}
int32_t field::is_player_can_discard_deck(uint8_t playerid, int32_t count) {
	if(count < 0 || (int32_t)player[playerid].list_main.size() < count)
		return FALSE;
	return !is_player_affected_by_effect(playerid, EFFECT_CANNOT_DISCARD_DECK);
}
int32_t field::is_player_can_discard_deck_as_cost(uint8_t playerid, int32_t count) {
	if(count < 0 || (int32_t)player[playerid].list_main.size() < count)
		return FALSE;
	if(is_player_affected_by_effect(playerid, EFFECT_CANNOT_DISCARD_DECK))
		return FALSE;
	card* topcard = player[playerid].list_main.back();
	if((count == 1) && topcard->is_position(POS_FACEUP))
		return topcard->is_capable_cost_to_grave(playerid);
	bool cant_remove_s = !is_player_can_action(playerid, EFFECT_CANNOT_REMOVE);
	bool cant_remove_o = !is_player_can_action(1 - playerid, EFFECT_CANNOT_REMOVE);
	effect_set eset;
	filter_field_effect(EFFECT_TO_GRAVE_REDIRECT, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		uint32_t redirect = eset[i]->get_value();
		uint8_t p = eset[i]->get_handler_player();
		if(redirect & LOCATION_REMOVED) {
			if(topcard->is_affected_by_effect(EFFECT_CANNOT_REMOVE))
				continue;
			if(cant_remove_s && (p == playerid) || cant_remove_o && (p != playerid))
				continue;
		}
		if((p == playerid && eset[i]->s_range & LOCATION_DECK) || (p != playerid && eset[i]->o_range & LOCATION_DECK))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_discard_hand(uint8_t playerid, card * pcard, effect * reason_effect, uint32_t reason) {
	if(pcard->current.location != LOCATION_HAND)
		return FALSE;
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_DISCARD_HAND, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(reason, PARAM_TYPE_INT);
		if (pduel->lua->check_condition(eset[i]->target, 4))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_action(uint8_t playerid, uint32_t actionlimit) {
	effect_set eset;
	filter_player_effect(playerid, actionlimit, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_summon(uint32_t sumtype, uint8_t playerid, card * pcard, uint8_t toplayer) {
	effect_set eset;
	sumtype |= SUMMON_TYPE_NORMAL;
	filter_player_effect(playerid, EFFECT_CANNOT_SUMMON, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
		pduel->lua->add_param(POS_FACEUP, PARAM_TYPE_INT);
		pduel->lua->add_param(toplayer, PARAM_TYPE_INT);
		if(pduel->lua->check_condition(eset[i]->target, 6))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_mset(uint32_t sumtype, uint8_t playerid, card * pcard, uint8_t toplayer) {
	effect_set eset;
	sumtype |= SUMMON_TYPE_NORMAL;
	filter_player_effect(playerid, EFFECT_CANNOT_MSET, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
		pduel->lua->add_param(POS_FACEDOWN, PARAM_TYPE_INT);
		pduel->lua->add_param(toplayer, PARAM_TYPE_INT);
		if (pduel->lua->check_condition(eset[i]->target, 6))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_sset(uint8_t playerid, card * pcard) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_SSET, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if (pduel->lua->check_condition(eset[i]->target, 3))
			return FALSE;
	}
	return TRUE;
}
// check player-effect EFFECT_CANNOT_SPECIAL_SUMMON without target
int32_t field::is_player_can_spsummon(uint8_t playerid) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_SPECIAL_SUMMON, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
	}
	return is_player_can_spsummon_count(playerid, 1);
}
int32_t field::is_player_can_spsummon(effect* reason_effect, uint32_t sumtype, uint8_t sumpos, uint8_t playerid, uint8_t toplayer, card* pcard) {
	if(pcard->is_affected_by_effect(EFFECT_CANNOT_SPECIAL_SUMMON))
		return FALSE;
	if(pcard->is_status(STATUS_FORBIDDEN))
		return FALSE;
	if((pcard->data.type & TYPE_TOKEN) && (pcard->current.location & LOCATION_ONFIELD))
		return FALSE;
	if(pcard->data.type & TYPE_LINK)
		sumpos &= POS_FACEUP_ATTACK;
	uint8_t position = pcard->get_spsummonable_position(reason_effect, sumtype, sumpos, playerid, toplayer);
	if(position == 0)
		return FALSE;
	sumtype |= SUMMON_TYPE_SPECIAL;
	save_lp_cost();
	if(!pcard->check_cost_condition(EFFECT_SPSUMMON_COST, playerid, sumtype)) {
		restore_lp_cost();
		return FALSE;
	}
	restore_lp_cost();
	if(position & POS_FACEDOWN && is_player_affected_by_effect(playerid, EFFECT_DIVINE_LIGHT))
		position = (position & POS_FACEUP) | ((position & POS_FACEDOWN) >> 1);
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_SPECIAL_SUMMON, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
		pduel->lua->add_param(position, PARAM_TYPE_INT);
		pduel->lua->add_param(toplayer, PARAM_TYPE_INT);
		pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
		if (pduel->lua->check_condition(eset[i]->target, 7))
			return FALSE;
	}
	if(!check_spsummon_once(pcard, playerid))
		return FALSE;
	if(!check_spsummon_counter(playerid))
		return FALSE;
	return TRUE;
}
int32_t field::is_player_can_flipsummon(uint8_t playerid, card * pcard) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_FLIP_SUMMON, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if (pduel->lua->check_condition(eset[i]->target, 3))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_spsummon_monster(uint8_t playerid, uint8_t toplayer, uint8_t sumpos, uint32_t sumtype, card_data* pdata) {
	temp_card->data = *pdata;
	int32_t result = is_player_can_spsummon(core.reason_effect, sumtype, sumpos, playerid, toplayer, temp_card);
	temp_card->data.clear();
	return result;
}
int32_t field::is_player_can_release(uint8_t playerid, card* pcard, uint32_t reason) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_RELEASE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(reason, PARAM_TYPE_INT);
		if (pduel->lua->check_condition(eset[i]->target, 4))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_spsummon_count(uint8_t playerid, uint32_t count) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_LEFT_SPSUMMON_COUNT, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(core.reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		int32_t v = eset[i]->get_value(2);
		if(v < (int32_t)count)
			return FALSE;
	}
	return check_spsummon_counter(playerid, count);
}
int32_t field::is_player_can_place_counter(uint8_t playerid, card * pcard, uint16_t countertype, uint16_t count) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_PLACE_COUNTER, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if (pduel->lua->check_condition(eset[i]->target, 3))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_remove_counter(uint8_t playerid, card * pcard, uint8_t s, uint8_t o, uint16_t countertype, uint16_t count, uint32_t reason) {
	if((pcard && pcard->get_counter(countertype) >= count) || (!pcard && get_field_counter(playerid, s, o, countertype) >= count))
		return TRUE;
	auto pr = effects.continuous_effect.equal_range(EFFECT_RCOUNTER_REPLACE + countertype);
	tevent e;
	e.event_cards = 0;
	e.event_player = playerid;
	e.event_value = count;
	e.reason = reason;
	e.reason_effect = core.reason_effect;
	e.reason_player = playerid;
	for(auto eit = pr.first; eit != pr.second;) {
		effect* peffect = eit->second;
		++eit;
		if(peffect->is_activateable(peffect->get_handler_player(), e))
			return TRUE;
	}
	return FALSE;
}
int32_t field::is_player_can_remove_overlay_card(uint8_t playerid, card * pcard, uint8_t s, uint8_t o, uint16_t min, uint32_t reason) {
	if((pcard && (int32_t)pcard->xyz_materials.size() >= min) || (!pcard && get_overlay_count(playerid, s, o) >= min))
		return TRUE;
	auto pr = effects.continuous_effect.equal_range(EFFECT_OVERLAY_REMOVE_REPLACE);
	tevent e;
	e.event_cards = 0;
	e.event_player = playerid;
	e.event_value = min;
	e.reason = reason;
	e.reason_effect = core.reason_effect;
	e.reason_player = playerid;
	for(auto eit = pr.first; eit != pr.second;) {
		effect* peffect = eit->second;
		++eit;
		if(peffect->is_activateable(peffect->get_handler_player(), e))
			return TRUE;
	}
	return FALSE;
}
int32_t field::is_player_can_send_to_grave(uint8_t playerid, card * pcard) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_TO_GRAVE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if (pduel->lua->check_condition(eset[i]->target, 3))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_send_to_hand(uint8_t playerid, card * pcard) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_TO_HAND, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(core.reason_effect, PARAM_TYPE_EFFECT);
		if (pduel->lua->check_condition(eset[i]->target, 4))
			return FALSE;
	}
	if(pcard->is_extra_deck_monster() && !is_player_can_send_to_deck(playerid, pcard))
		return FALSE;
	return TRUE;
}
int32_t field::is_player_can_send_to_deck(uint8_t playerid, card * pcard) {
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_TO_DECK, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if (pduel->lua->check_condition(eset[i]->target, 3))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_player_can_remove(uint8_t playerid, card* pcard, uint32_t reason, effect* reason_effect) {
	if(!reason_effect)
		reason_effect = core.reason_effect;
	effect_set eset;
	filter_player_effect(playerid, EFFECT_CANNOT_REMOVE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			return FALSE;
		pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(reason, PARAM_TYPE_INT);
		pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
		if(pduel->lua->check_condition(eset[i]->target, 5))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_chain_negatable(uint8_t chaincount) {
	effect_set eset;
	if(chaincount < 0 || chaincount > core.current_chain.size())
		return FALSE;
	effect* peffect;
	if(chaincount == 0)
		peffect = core.current_chain.back().triggering_effect;
	else
		peffect = core.current_chain[chaincount - 1].triggering_effect;
	if(peffect->is_flag(EFFECT_FLAG_CANNOT_INACTIVATE))
		return FALSE;
	filter_field_effect(EFFECT_CANNOT_INACTIVATE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(chaincount, PARAM_TYPE_INT);
		if(eset[i]->check_value_condition(1))
			return FALSE;
	}
	return TRUE;
}
int32_t field::is_chain_disablable(uint8_t chaincount) {
	effect_set eset;
	if(chaincount < 0 || chaincount > core.current_chain.size())
		return FALSE;
	effect* peffect;
	if(chaincount == 0)
		peffect = core.current_chain.back().triggering_effect;
	else
		peffect = core.current_chain[chaincount - 1].triggering_effect;
	if(!peffect->get_handler()->get_status(STATUS_FORBIDDEN)) {
		if(peffect->is_flag(EFFECT_FLAG_CANNOT_DISABLE))
			return FALSE;
		filter_field_effect(EFFECT_CANNOT_DISEFFECT, &eset);
		for(effect_set::size_type i = 0; i < eset.size(); ++i) {
			pduel->lua->add_param(chaincount, PARAM_TYPE_INT);
			if(eset[i]->check_value_condition(1))
				return FALSE;
		}
	}
	return TRUE;
}
int32_t field::is_chain_disabled(uint8_t chaincount) {
	if(chaincount < 0 || chaincount > core.current_chain.size())
		return FALSE;
	chain* pchain;
	if(chaincount == 0)
		pchain = &core.current_chain.back();
	else
		pchain = &core.current_chain[chaincount - 1];
	if(pchain->flag & CHAIN_DISABLE_EFFECT)
		return TRUE;
	card* pcard = pchain->triggering_effect->get_handler();
	auto rg = pcard->single_effect.equal_range(EFFECT_DISABLE_CHAIN);
	for(; rg.first != rg.second; ++rg.first) {
		effect* peffect = rg.first->second;
		if(peffect->get_value() == pchain->chain_id) {
			peffect->reset_flag |= RESET_CHAIN;
			return TRUE;
		}
	}
	return FALSE;
}
int32_t field::check_chain_target(uint8_t chaincount, card * pcard) {
	if(chaincount < 0 || chaincount > core.current_chain.size())
		return FALSE;
	chain* pchain;
	if(chaincount == 0)
		pchain = &core.current_chain.back();
	else
		pchain = &core.current_chain[chaincount - 1];
	effect* peffect = pchain->triggering_effect;
	uint8_t tp = pchain->triggering_player;
	if(!peffect->is_flag(EFFECT_FLAG_CARD_TARGET) || !peffect->target)
		return FALSE;
	pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
	pduel->lua->add_param(tp, PARAM_TYPE_INT);
	pduel->lua->add_param(pchain->evt.event_cards , PARAM_TYPE_GROUP);
	pduel->lua->add_param(pchain->evt.event_player, PARAM_TYPE_INT);
	pduel->lua->add_param(pchain->evt.event_value, PARAM_TYPE_INT);
	pduel->lua->add_param(pchain->evt.reason_effect , PARAM_TYPE_EFFECT);
	pduel->lua->add_param(pchain->evt.reason, PARAM_TYPE_INT);
	pduel->lua->add_param(pchain->evt.reason_player, PARAM_TYPE_INT);
	pduel->lua->add_param(0, PARAM_TYPE_INT);
	pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
	return pduel->lua->check_condition(peffect->target, 10);
}
chain* field::get_chain(uint32_t chaincount) {
	if(chaincount == 0 && core.continuous_chain.size() && (core.reason_effect->type & EFFECT_TYPE_CONTINUOUS))
		return &core.continuous_chain.back();
	if(chaincount == 0 || chaincount > core.current_chain.size()) {
		chaincount = (uint32_t)core.current_chain.size();
		if(chaincount == 0)
			return 0;
	}
	return &core.current_chain[chaincount - 1];
}
int32_t field::get_cteffect(effect* peffect, int32_t playerid, int32_t store) {
	card* phandler = peffect->get_handler();
	if(phandler->data.type != (TYPE_TRAP | TYPE_CONTINUOUS))
		return FALSE;
	if(!(peffect->type & EFFECT_TYPE_ACTIVATE))
		return FALSE;
	if(peffect->code != EVENT_FREE_CHAIN)
		return FALSE;
	if(peffect->cost || peffect->target || peffect->operation)
		return FALSE;
	if(store) {
		core.select_chains.clear();
		core.select_options.clear();
	}
	for(auto& efit : phandler->field_effect) {
		effect* feffect = efit.second;
		if(!(feffect->type & (EFFECT_TYPE_TRIGGER_F | EFFECT_TYPE_TRIGGER_O | EFFECT_TYPE_QUICK_O)))
			continue;
		if(!feffect->in_range(phandler))
			continue;
		uint32_t code = efit.first;
		if(code == EVENT_FREE_CHAIN || code == (EVENT_PHASE | infos.phase)) {
			tevent test_event;
			test_event.event_code = code;
			if(get_cteffect_evt(feffect, playerid, test_event, store) && !store)
				return TRUE;
		} else {
			for(const auto& ev : core.point_event) {
				if(code != ev.event_code)
					continue;
				if(get_cteffect_evt(feffect, playerid, ev, store) && !store)
					return TRUE;
			}
			for(const auto& ev : core.instant_event) {
				if(code != ev.event_code)
					continue;
				if(get_cteffect_evt(feffect, playerid, ev, store) && !store)
					return TRUE;
			}
		}
	}
	return (store && !core.select_chains.empty()) ? TRUE : FALSE;
}
int32_t field::get_cteffect_evt(effect* feffect, int32_t playerid, const tevent& e, int32_t store) {
	if(!feffect->is_activateable(playerid, e, FALSE, FALSE, FALSE, FALSE, TRUE))
		return FALSE;
	if(store) {
		chain newchain;
		newchain.evt = e;
		newchain.triggering_effect = feffect;
		core.select_chains.push_back(newchain);
		core.select_options.push_back(feffect->description);
	}
	return TRUE;
}
int32_t field::check_cteffect_hint(effect* peffect, uint8_t playerid) {
	card* phandler = peffect->get_handler();
	if(phandler->data.type != (TYPE_TRAP | TYPE_CONTINUOUS))
		return FALSE;
	if(!(peffect->type & EFFECT_TYPE_ACTIVATE))
		return FALSE;
	if(peffect->code != EVENT_FREE_CHAIN)
		return FALSE;
	if(peffect->cost || peffect->target || peffect->operation)
		return FALSE;
	for(auto& efit : phandler->field_effect) {
		effect* feffect = efit.second;
		if(!(feffect->type & (EFFECT_TYPE_TRIGGER_F | EFFECT_TYPE_TRIGGER_O | EFFECT_TYPE_QUICK_O)))
			continue;
		if(!feffect->in_range(phandler))
			continue;
		uint32_t code = efit.first;
		if(code == EVENT_FREE_CHAIN || code == EVENT_PHASE + infos.phase) {
			tevent test_event;
			test_event.event_code = code;
			if(get_cteffect_evt(feffect, playerid, test_event, FALSE)
				&& (code != EVENT_FREE_CHAIN || check_hint_timing(feffect)))
				return TRUE;
		} else {
			for(const auto& ev : core.point_event) {
				if(code != ev.event_code)
					continue;
				if(get_cteffect_evt(feffect, playerid, ev, FALSE))
					return TRUE;
			}
			for(const auto& ev : core.instant_event) {
				if(code != ev.event_code)
					continue;
				if(get_cteffect_evt(feffect, playerid, ev, FALSE))
					return TRUE;
			}
		}
	}
	return FALSE;
}
int32_t field::check_nonpublic_trigger(chain& ch) {
	effect* peffect = ch.triggering_effect;
	card* phandler = peffect->get_handler();
	if(!peffect->is_flag(EFFECT_FLAG_FIELD_ONLY)
		&& ((peffect->type & EFFECT_TYPE_SINGLE) && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)
			&& phandler->is_has_relation(ch) && (ch.triggering_location & (LOCATION_HAND | LOCATION_DECK))
			|| (peffect->range & (LOCATION_HAND | LOCATION_DECK)))) {
		ch.flag |= CHAIN_HAND_TRIGGER;
		core.new_ochain_h.push_back(ch);
		if(ch.triggering_location == LOCATION_HAND && phandler->is_position(POS_FACEDOWN)
			|| ch.triggering_location == LOCATION_DECK
			|| peffect->range && !peffect->in_range(ch))
			return FALSE;
	}
	return TRUE;
}
int32_t field::check_trigger_effect(const chain& ch) const {
	effect* peffect = ch.triggering_effect;
	card* phandler = peffect->get_handler();
	if(core.duel_rule >= MASTER_RULE_2020)
		return phandler->is_has_relation(ch);
	else {
		if(peffect->type & EFFECT_TYPE_FIELD)
			return phandler->is_has_relation(ch);
		else {
			if((ch.triggering_location & (LOCATION_DECK | LOCATION_HAND | LOCATION_EXTRA))
				&& (ch.triggering_position & POS_FACEDOWN))
				return TRUE;
			if(!(phandler->current.location & (LOCATION_DECK | LOCATION_HAND | LOCATION_EXTRA))
				|| phandler->is_position(POS_FACEUP))
				return TRUE;
			return FALSE;
		}
	}
}
int32_t field::check_spself_from_hand_trigger(const chain& ch) const {
	effect* peffect = ch.triggering_effect;
	uint8_t tp = ch.triggering_player;
	if((peffect->status & EFFECT_STATUS_SPSELF) && (ch.flag & CHAIN_HAND_TRIGGER)) {
		return std::none_of(core.current_chain.begin(), core.current_chain.end(), [tp](chain ch) {
			return ch.triggering_player == tp
				&& (ch.triggering_effect->status & EFFECT_STATUS_SPSELF) && (ch.flag & CHAIN_HAND_TRIGGER);
		});
	}
	return TRUE;
}
int32_t field::is_able_to_enter_bp() {
	return ((core.duel_options & DUEL_ATTACK_FIRST_TURN) || infos.turn_id != 1)
	        && infos.phase < PHASE_BATTLE_START
	        && !is_player_affected_by_effect(infos.turn_player, EFFECT_CANNOT_BP);
}
