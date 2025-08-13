/*
 * card.cpp
 *
 *  Created on: 2010-5-7
 *      Author: Argon
 */

#include "card.h"
#include "field.h"
#include "effect.h"
#include "duel.h"
#include "group.h"
#include "interpreter.h"
#include "ocgapi.h"
#include "buffer.h"
#include <algorithm>

bool card_sort::operator()(card* const& c1, card* const& c2) const {
	return c1->cardid < c2->cardid;
}
bool card_state::is_location(uint32_t loc) const {
	if((loc & LOCATION_FZONE) && location == LOCATION_SZONE && sequence == 5)
		return true;
	if((loc & LOCATION_PZONE) && location == LOCATION_SZONE && pzone)
		return true;
	if(location & loc)
		return true;
	return false;
}
void card_state::init_state() {
	code = UINT32_MAX;
	code2 = UINT32_MAX;
	type = UINT32_MAX;
	level = UINT32_MAX;
	rank = UINT32_MAX;
	link = UINT32_MAX;
	lscale = UINT32_MAX;
	rscale = UINT32_MAX;
	attribute = UINT32_MAX;
	race = UINT32_MAX;
	attack = -1;
	defense = -1;
	base_attack = -1;
	base_defense = -1;
	controler = UINT8_MAX;
	location = UINT8_MAX;
	sequence = UINT8_MAX;
	position = UINT8_MAX;
	reason = UINT32_MAX;
	reason_player = UINT8_MAX;
}
void query_cache::clear_cache() {
	info_location = UINT32_MAX;
	current_code = UINT32_MAX;
	type = UINT32_MAX;
	level = UINT32_MAX;
	rank = UINT32_MAX;
	link = UINT32_MAX;
	attribute = UINT32_MAX;
	race = UINT32_MAX;
	attack = -1;
	defense = -1;
	base_attack = -1;
	base_defense = -1;
	reason = UINT32_MAX;
	status = UINT32_MAX;
	lscale = UINT32_MAX;
	rscale = UINT32_MAX;
	link_marker = UINT32_MAX;
}
bool card::card_operation_sort(card* c1, card* c2) {
	duel* pduel = c1->pduel;
	int32_t cp1 = c1->overlay_target ? c1->overlay_target->current.controler : c1->current.controler;
	int32_t cp2 = c2->overlay_target ? c2->overlay_target->current.controler : c2->current.controler;
	if(cp1 != cp2) {
		if(cp1 == PLAYER_NONE || cp2 == PLAYER_NONE)
			return cp1 < cp2;
		if(pduel->game_field->infos.turn_player == 0)
			return cp1 < cp2;
		else
			return cp1 > cp2;
	}
	if(c1->current.location != c2->current.location)
		return c1->current.location < c2->current.location;
	if(c1->current.location == LOCATION_OVERLAY) {
		if(c1->overlay_target && c2->overlay_target && c1->overlay_target->current.sequence != c2->overlay_target->current.sequence)
			return c1->overlay_target->current.sequence < c2->overlay_target->current.sequence;
		else
			return c1->current.sequence < c2->current.sequence;
	} else if (c1->current.location == LOCATION_DECK && pduel->game_field->is_select_hide_deck_sequence(cp1)) {
		// if deck reversed and the card being at the top, it should go first
		if(pduel->game_field->core.deck_reversed) {
			if(c1 == pduel->game_field->player[cp1].list_main.back())
				return false;
			if(c2 == pduel->game_field->player[cp2].list_main.back())
				return true;
		}
		// faceup deck cards should go at the very first
		auto c1_faceup = c1->current.position & POS_FACEUP;
		auto c2_faceup = c2->current.position & POS_FACEUP;
		if(c1_faceup || c2_faceup) {
			if(c1_faceup && c2_faceup)
				return c1->current.sequence > c2->current.sequence;
			else
				return c2_faceup;
		}
		// sort deck as card property
		auto c1_type = c1->data.type & (TYPE_MONSTER | TYPE_SPELL | TYPE_TRAP);
		auto c2_type = c2->data.type & (TYPE_MONSTER | TYPE_SPELL | TYPE_TRAP);
		// monster should go before spell, and then trap
		if(c1_type != c2_type)
			return c1_type > c2_type;
		if(c1_type & TYPE_MONSTER) {
			if (c1->data.level != c2->data.level)
				return c1->data.level < c2->data.level;
			// TODO: more sorts here
		}
		if(c1->data.code != c2->data.code)
			return c1->data.code > c2->data.code;
		return c1->current.sequence > c2->current.sequence;
	} else {
		if(c1->current.location & (LOCATION_DECK | LOCATION_EXTRA | LOCATION_GRAVE | LOCATION_REMOVED))
			return c1->current.sequence > c2->current.sequence;
		else
			return c1->current.sequence < c2->current.sequence;
	}
}
bool card::check_card_setcode(uint32_t code, uint32_t value) {
	card_data dat;
	::read_card(code, &dat);
	return dat.is_setcode(value);
}
void card::attacker_map::addcard(card* pcard) {
	auto fid = pcard ? pcard->fieldid_r : 0;
	auto pr = emplace(fid, mapped_type(pcard, 0));
	++pr.first->second.second;
}
uint32_t card::attacker_map::findcard(card* pcard) {
	auto fid = pcard ? pcard->fieldid_r : 0;
	auto it = find(fid);
	if(it == end())
		return 0;
	else
		return it->second.second;
}
card::card(duel* pd)
	:pduel(pd) {
	temp.init_state();
	current.controler = PLAYER_NONE;
}
inline void update_cache(uint32_t tdata, uint32_t& cache, byte*& p, uint32_t& query_flag, uint32_t flag) {
	if (tdata != cache) {
		cache = tdata;
		buffer_write<uint32_t>(p, tdata);
	}
	else
		query_flag &= ~flag;
}
int32_t card::get_infos(byte* buf, uint32_t query_flag, int32_t use_cache) {
	byte* p = buf;
	std::pair<int32_t, int32_t> atk_def(-10, -10);
	std::pair<int32_t, int32_t> base_atk_def(-10, -10);
	if ((query_flag & QUERY_ATTACK) || (query_flag & QUERY_DEFENSE)) {
		atk_def = get_atk_def();
	}
	if ((query_flag & QUERY_BASE_ATTACK) || (query_flag & QUERY_BASE_DEFENSE)) {
		base_atk_def = get_base_atk_def();
	}
	//first 8 bytes: data length, query flag
	p += 2 * sizeof(uint32_t);
	if (query_flag & QUERY_CODE) {
		buffer_write<uint32_t>(p, data.code);
	}
	if (query_flag & QUERY_POSITION) {
		uint32_t tdata = get_info_location();
		buffer_write<uint32_t>(p, tdata);
		if (q_cache.info_location != tdata) {
			q_cache.clear_cache();
			q_cache.info_location = tdata;
			use_cache = 0;
		}
	}
	if(!use_cache) {
		if (query_flag & QUERY_ALIAS) {
			uint32_t tdata = get_code();
			buffer_write<uint32_t>(p, tdata);
			q_cache.current_code = tdata;
		}
		if (query_flag & QUERY_TYPE) {
			uint32_t tdata = get_type();
			buffer_write<uint32_t>(p, tdata);
			q_cache.type = tdata;
		}
		if (query_flag & QUERY_LEVEL) {
			uint32_t tdata = get_level();
			buffer_write<uint32_t>(p, tdata);
			q_cache.level = tdata;
		}
		if (query_flag & QUERY_RANK) {
			uint32_t tdata = get_rank();
			buffer_write<uint32_t>(p, tdata);
			q_cache.rank = tdata;
		}
		if (query_flag & QUERY_ATTRIBUTE) {
			uint32_t tdata = get_attribute();
			buffer_write<uint32_t>(p, tdata);
			q_cache.attribute = tdata;
		}
		if (query_flag & QUERY_RACE) {
			uint32_t tdata = get_race();
			buffer_write<uint32_t>(p, tdata);
			q_cache.race = tdata;
		}
		if (query_flag & QUERY_ATTACK) {
			buffer_write<int32_t>(p, atk_def.first);
			q_cache.attack = atk_def.first;
		}
		if (query_flag & QUERY_DEFENSE) {
			buffer_write<int32_t>(p, atk_def.second);
			q_cache.defense = atk_def.second;
		}
		if (query_flag & QUERY_BASE_ATTACK) {
			buffer_write<int32_t>(p, base_atk_def.first);
			q_cache.base_attack = base_atk_def.first;
		}
		if (query_flag & QUERY_BASE_DEFENSE) {
			buffer_write<int32_t>(p, base_atk_def.second);
			q_cache.base_defense = base_atk_def.second;
		}
		if (query_flag & QUERY_REASON) {
			buffer_write<uint32_t>(p, current.reason);
			q_cache.reason = current.reason;
		}
	}
	else {
		if((query_flag & QUERY_ALIAS)) {
			uint32_t tdata = get_code();
			update_cache(tdata, q_cache.current_code, p, query_flag, QUERY_ALIAS);
		} 
		if((query_flag & QUERY_TYPE)) {
			uint32_t tdata = get_type();
			update_cache(tdata, q_cache.type, p, query_flag, QUERY_TYPE);
		}
		if((query_flag & QUERY_LEVEL)) {
			uint32_t tdata = get_level();
			update_cache(tdata, q_cache.level, p, query_flag, QUERY_LEVEL);
		}
		if((query_flag & QUERY_RANK)) {
			uint32_t tdata = get_rank();
			update_cache(tdata, q_cache.rank, p, query_flag, QUERY_RANK);
		}
		if((query_flag & QUERY_ATTRIBUTE)) {
			uint32_t tdata = get_attribute();
			update_cache(tdata, q_cache.attribute, p, query_flag, QUERY_ATTRIBUTE);
		}
		if((query_flag & QUERY_RACE)) {
			uint32_t tdata = get_race();
			update_cache(tdata, q_cache.race, p, query_flag, QUERY_RACE);
		}
		if((query_flag & QUERY_ATTACK)) {
			if (atk_def.first != q_cache.attack) {
				q_cache.attack = atk_def.first;
				buffer_write<int32_t>(p, atk_def.first);
			}
			else
				query_flag &= ~QUERY_ATTACK;
		}
		if((query_flag & QUERY_DEFENSE)) {
			if (atk_def.second != q_cache.defense) {
				q_cache.defense = atk_def.second;
				buffer_write<int32_t>(p, atk_def.second);
			}
			else
				query_flag &= ~QUERY_DEFENSE;
		}
		if((query_flag & QUERY_BASE_ATTACK)) {
			if (base_atk_def.first != q_cache.base_attack) {
				q_cache.base_attack = base_atk_def.first;
				buffer_write<int32_t>(p, base_atk_def.first);
			}
			else
				query_flag &= ~QUERY_BASE_ATTACK;
		}
		if((query_flag & QUERY_BASE_DEFENSE)) {
			if (base_atk_def.second != q_cache.base_defense) {
				q_cache.base_defense = base_atk_def.second;
				buffer_write<int32_t>(p, base_atk_def.second);
			}
			else
				query_flag &= ~QUERY_BASE_DEFENSE;
		}
		if((query_flag & QUERY_REASON)) {
			uint32_t tdata = current.reason;
			update_cache(tdata, q_cache.reason, p, query_flag, QUERY_REASON);
		}
	}
	if (query_flag & QUERY_REASON_CARD) {
		uint32_t tdata = current.reason_card ? current.reason_card->get_info_location() : 0;
		buffer_write<uint32_t>(p, tdata);
	}
	if(query_flag & QUERY_EQUIP_CARD) {
		if (equiping_target) {
			uint32_t tdata = equiping_target->get_info_location();
			buffer_write<uint32_t>(p, tdata);
		}
		else
			query_flag &= ~QUERY_EQUIP_CARD;
	}
	if(query_flag & QUERY_TARGET_CARD) {
		buffer_write<int32_t>(p, (int32_t)effect_target_cards.size());
		for (auto& pcard : effect_target_cards) {
			uint32_t tdata = pcard->get_info_location();
			buffer_write<uint32_t>(p, tdata);
		}
	}
	if(query_flag & QUERY_OVERLAY_CARD) {
		buffer_write<int32_t>(p, (int32_t)xyz_materials.size());
		for (auto& xcard : xyz_materials) {
			buffer_write<uint32_t>(p, xcard->data.code);
		}
	}
	if(query_flag & QUERY_COUNTERS) {
		buffer_write<int32_t>(p, (int32_t)counters.size());
		for (const auto& cmit : counters) {
			uint32_t tdata = cmit.first | ((uint32_t)cmit.second << 16);
			buffer_write<uint32_t>(p, tdata);
		}
	}
	if (query_flag & QUERY_OWNER) {
		int32_t tdata = owner;
		buffer_write<int32_t>(p, tdata);
	}
	if(query_flag & QUERY_STATUS) {
		uint32_t tdata = status & (STATUS_DISABLED | STATUS_FORBIDDEN | STATUS_PROC_COMPLETE);
		if(!use_cache || (tdata != q_cache.status)) {
			q_cache.status = tdata;
			buffer_write<uint32_t>(p, tdata);
		}
		else
			query_flag &= ~QUERY_STATUS;
	}
	if(!use_cache) {
		if (query_flag & QUERY_LSCALE) {
			uint32_t tdata = get_lscale();
			buffer_write<uint32_t>(p, tdata);
			q_cache.lscale = tdata;
		}
		if (query_flag & QUERY_RSCALE) {
			uint32_t tdata = get_rscale();
			buffer_write<uint32_t>(p, tdata);
			q_cache.rscale = tdata;
		}
		if(query_flag & QUERY_LINK) {
			uint32_t tdata = get_link();
			buffer_write<uint32_t>(p, tdata);
			q_cache.link = tdata;
			tdata = get_link_marker();
			buffer_write<uint32_t>(p, tdata);
			q_cache.link_marker = tdata;
		}
	}
	else {
		if((query_flag & QUERY_LSCALE)) {
			uint32_t tdata = get_lscale();
			update_cache(tdata, q_cache.lscale, p, query_flag, QUERY_LSCALE);
		}
		if((query_flag & QUERY_RSCALE)) {
			uint32_t tdata = get_rscale();
			update_cache(tdata, q_cache.rscale, p, query_flag, QUERY_RSCALE);
		}
		if(query_flag & QUERY_LINK) {
			uint32_t link = get_link();
			uint32_t link_marker = get_link_marker();
			if((link != q_cache.link) || (link_marker != q_cache.link_marker)) {
				q_cache.link = link;
				buffer_write<uint32_t>(p, link);
				q_cache.link_marker = link_marker;
				buffer_write<uint32_t>(p, link_marker);
			}
			else
				query_flag &= ~QUERY_LINK;
		}
	}
	byte* finalize = buf;
	buffer_write<int32_t>(finalize, (int32_t)(p - buf));
	buffer_write<uint32_t>(finalize, query_flag);
	return (int32_t)(p - buf);
}
uint32_t card::get_info_location() const {
	if(overlay_target) {
		uint32_t c = overlay_target->current.controler;
		uint32_t l = overlay_target->current.location | LOCATION_OVERLAY;
		uint32_t s = overlay_target->current.sequence;
		uint32_t ss = current.sequence;
		return c | (l << 8) | (s << 16) | (ss << 24);
	} else {
		uint32_t c = current.controler;
		uint32_t l = current.location;
		uint32_t s = current.sequence;
		uint32_t ss = current.position;
		return c | (l << 8) | (s << 16) | (ss << 24);
	}
}
// get the printed code on card
uint32_t card::get_original_code() const {
	return data.get_original_code();
}
// get the original code in duel (can be different from printed code)
std::tuple<uint32_t, uint32_t> card::get_original_code_rule() const {
	auto it = second_code.find(data.code);
	if (it != second_code.end()) {
		return std::make_tuple(data.code, it->second);
	}
	else {
		if (data.alias)
			return std::make_tuple(data.alias, 0);
		else
			return std::make_tuple(data.code, 0);
	}
}
// return: the current card name
// for double-name cards, it returns printed name
uint32_t card::get_code() {
	if(assume_type == ASSUME_CODE)
		return assume_value;
	if(temp.code != UINT32_MAX) // prevent recursion, return the former value
		return temp.code;
	effect_set effects;
	uint32_t code = std::get<0>(get_original_code_rule());
	temp.code = code;
	filter_effect(EFFECT_CHANGE_CODE, &effects);
	if (effects.size())
		code = effects.back()->get_value(this);
	temp.code = UINT32_MAX;
	return code;
}
// return: the current second card name
// for double-name cards, it returns the name in description
uint32_t card::get_another_code() {
	uint32_t code1 = get_code();
	if (is_affected_by_effect(EFFECT_CHANGE_CODE)) {
		auto it = second_code.find(code1);
		if (it != second_code.end())
			return it->second;
		else
			return 0;
	}
	uint32_t code2 = std::get<1>(get_original_code_rule());
	effect_set eset;
	filter_effect(EFFECT_ADD_CODE, &eset);
	if(!eset.size())
		return code2;
	uint32_t otcode = eset.back()->get_value(this);
	if(code1 != otcode)
		return otcode;
	return 0;
}
int32_t card::is_set_card(uint32_t set_code) {
	uint32_t code1 = get_code();
	card_data dat1;
	if (code1 == data.code) {
		if (data.is_setcode(set_code))
			return TRUE;
	}
	else {
		if (check_card_setcode(code1, set_code))
			return TRUE;
	}
	uint32_t code2 = get_another_code();
	if (code2 && check_card_setcode(code2, set_code))
		return TRUE;
	//add set code
	effect_set eset;
	filter_effect(EFFECT_ADD_SETCODE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		uint32_t value = eset[i]->get_value(this);
		uint16_t new_setcode = value & 0xffff;
		if (check_setcode(new_setcode, set_code))
			return TRUE;
	}
	return FALSE;
}
int32_t card::is_origin_set_card(uint32_t set_code) {
	if (data.is_setcode(set_code))
		return TRUE;
	uint32_t code2 = std::get<1>(get_original_code_rule());
	if (code2 && check_card_setcode(code2, set_code))
		return TRUE;
	return FALSE;
}
int32_t card::is_pre_set_card(uint32_t set_code) {
	uint32_t code = previous.code;
	if (code == data.code) {
		if (data.is_setcode(set_code))
			return TRUE;
	}
	else {
		if (check_card_setcode(code, set_code))
			return TRUE;
	}
	uint32_t code2 = previous.code2;
	if (code2 && check_card_setcode(code2, set_code))
		return TRUE;
	//add set code
	for(auto& presetcode : previous.setcode) {
		if (check_setcode(presetcode, set_code))
			return TRUE;
	}
	return FALSE;
}
int32_t card::is_fusion_set_card(uint32_t set_code) {
	if(is_set_card(set_code))
		return TRUE;
	if(pduel->game_field->core.not_material)
		return FALSE;
	effect_set eset;
	filter_effect(EFFECT_ADD_FUSION_CODE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		uint32_t code = eset[i]->get_value(this);
		if (check_card_setcode(code, set_code))
			return TRUE;
	}
	eset.clear();
	filter_effect(EFFECT_ADD_FUSION_SETCODE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		uint32_t value = eset[i]->get_value(this);
		uint16_t new_setcode = value & 0xffff;
		if (check_setcode(new_setcode, set_code))
			return TRUE;
	}
	return FALSE;
}
int32_t card::is_link_set_card(uint32_t set_code) {
	if(is_set_card(set_code))
		return TRUE;
	effect_set eset;
	filter_effect(EFFECT_ADD_LINK_CODE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		uint32_t code = eset[i]->get_value(this);
		if (check_card_setcode(code, set_code))
			return TRUE;
	}
	return FALSE;
}
int32_t card::is_special_summon_set_card(uint32_t set_code) {
	uint32_t code = spsummon.code;
	if (check_card_setcode(code, set_code))
		return TRUE;
	uint32_t code2 = spsummon.code2;
	if (code2 && check_card_setcode(code2, set_code))
		return TRUE;
	//add set code
	for(auto& spsetcode : spsummon.setcode) {
		if (check_setcode(spsetcode, set_code))
			return TRUE;
	}
	return FALSE;
}
uint32_t card::get_type() {
	if(assume_type == ASSUME_TYPE)
		return assume_value;
	if(!(current.location & (LOCATION_ONFIELD | LOCATION_HAND | LOCATION_GRAVE)))
		return data.type;
	if(current.is_location(LOCATION_PZONE))
		return TYPE_PENDULUM + TYPE_SPELL;
	if(temp.type != UINT32_MAX) // prevent recursion, return the former value
		return temp.type;
	effect_set effects;
	int32_t type = data.type;
	temp.type = data.type;
	filter_effect(EFFECT_ADD_TYPE, &effects, FALSE);
	filter_effect(EFFECT_REMOVE_TYPE, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_TYPE, &effects);
	for (effect_set::size_type i = 0; i < effects.size(); ++i) {
		if (effects[i]->code == EFFECT_ADD_TYPE)
			type |= effects[i]->get_value(this);
		else if (effects[i]->code == EFFECT_REMOVE_TYPE)
			type &= ~(effects[i]->get_value(this));
		else
			type = effects[i]->get_value(this);
		temp.type = type;
	}
	temp.type = UINT32_MAX;
	if (data.type & TYPE_TOKEN)
		type |= TYPE_TOKEN;
	return type;
}
uint32_t card::get_fusion_type() {
	if(current.location == LOCATION_SZONE && (data.type & TYPE_MONSTER) && !pduel->game_field->core.not_material)
		return data.type;
	return get_type();
}
uint32_t card::get_synchro_type() {
	if(current.location == LOCATION_SZONE && (data.type & TYPE_MONSTER))
		return data.type;
	return get_type();
}
uint32_t card::get_xyz_type() {
	if(current.location == LOCATION_SZONE && (data.type & TYPE_MONSTER))
		return data.type;
	return get_type();
}
uint32_t card::get_link_type() {
	if(current.location == LOCATION_SZONE)
		return data.type | TYPE_MONSTER;
	return get_type();
}
std::pair<int32_t, int32_t> card::get_base_atk_def() {
	if (!(data.type & TYPE_MONSTER) && !(get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_PRE_MONSTER))
		return std::pair<int32_t, int32_t>(0, 0);
	std::pair<int32_t, int32_t> ret(data.attack, 0);
	if (!(data.type & TYPE_LINK))
		ret.second = data.defense;
	if (current.location != LOCATION_MZONE || get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
		return ret;	
	if (temp.base_attack != -1){
		ret.first = temp.base_attack;
		ret.second = temp.base_defense;
		return ret;
	}
	int32_t batk = data.attack;
	if (batk < 0)
		batk = 0;
	int32_t bdef = 0;
	if (!(data.type & TYPE_LINK)) {
		bdef = data.defense;
		if (bdef < 0)
			bdef = 0;
	}
	temp.base_attack = batk;
	temp.base_defense = bdef;
	effect_set eset;
	filter_effect(EFFECT_SET_BASE_ATTACK, &eset, FALSE);
	filter_effect(EFFECT_SET_BASE_ATTACK_FINAL, &eset, FALSE);
	if (!(data.type & TYPE_LINK)) {
		filter_effect(EFFECT_SWAP_BASE_AD, &eset, FALSE);
		filter_effect(EFFECT_SET_BASE_DEFENSE, &eset, FALSE);
		filter_effect(EFFECT_SET_BASE_DEFENSE_FINAL, &eset, FALSE);
	}
	std::sort(eset.begin(), eset.end(), effect_sort_id);
	// calculate single effects of this first
	for(effect_set::size_type i = 0; i < eset.size();) {
		if (eset[i]->type & EFFECT_TYPE_SINGLE) {
			switch(eset[i]->code) {
			case EFFECT_SET_BASE_ATTACK:
				batk = eset[i]->get_value(this);
				if(batk < 0)
					batk = 0;
				temp.base_attack = batk;
				eset.erase(eset.begin() + i);
				continue;
			case EFFECT_SET_BASE_DEFENSE:
				bdef = eset[i]->get_value(this);
				if(bdef < 0)
					bdef = 0;
				temp.base_defense = bdef;
				eset.erase(eset.begin() + i);
				continue;
			}
		}
		++i;
	}
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		switch(eset[i]->code) {
		case EFFECT_SET_BASE_ATTACK:
		case EFFECT_SET_BASE_ATTACK_FINAL:
			batk = eset[i]->get_value(this);
			if(batk < 0)
				batk = 0;
			break;
		case EFFECT_SET_BASE_DEFENSE:
		case EFFECT_SET_BASE_DEFENSE_FINAL:
			bdef = eset[i]->get_value(this);
			if(bdef < 0)
				bdef = 0;
			break;
		case EFFECT_SWAP_BASE_AD:
			std::swap(batk, bdef);
			break;
		}
		temp.base_attack = batk;
		temp.base_defense = bdef;
	}
	ret.first = batk;
	ret.second = bdef;
	temp.base_attack = -1;
	temp.base_defense = -1;
	return ret;
}
std::pair<int32_t, int32_t> card::get_atk_def() {
	if (assume_type == ASSUME_ATTACK)
		return std::pair<int32_t, int32_t>(assume_value, 0);
	if (assume_type == ASSUME_DEFENSE) {
		if (data.type & TYPE_LINK)
			return std::pair<int32_t, int32_t>(0, 0);
		else
			return std::pair<int32_t, int32_t>(0, assume_value);
	}
	if (!(data.type & TYPE_MONSTER) && !(get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_PRE_MONSTER))
		return std::pair<int32_t, int32_t>(0, 0);
	std::pair<int32_t, int32_t> ret(data.attack, 0);
	if (!(data.type & TYPE_LINK))
		ret.second = data.defense;
	if (current.location != LOCATION_MZONE || get_status(STATUS_SUMMONING | STATUS_SPSUMMON_STEP))
		return ret;
	if (temp.attack != -1) {
		ret.first = temp.attack;
		ret.second = temp.defense;
		return ret;
	}
	int32_t atk = -1, def = -1;
	int32_t up_atk = 0, upc_atk = 0;
	int32_t up_def = 0, upc_def = 0;
	bool swap_final = false;
	effect_set eset;
	effect_set effects_atk_final, effects_atk_wicked, effects_atk_option;
	effect_set effects_def_final, effects_def_wicked, effects_def_option;
	effect_set effects_repeat_update_atk, effects_repeat_update_def;
	int32_t batk = data.attack;
	if (batk < 0)
		batk = 0;
	int32_t bdef = 0;
	if (!(data.type & TYPE_LINK)) {
		bdef = data.defense;
		if (bdef < 0)
			bdef = 0;
	}
	temp.attack = batk;
	temp.defense = bdef;
	std::pair<int32_t, int32_t> base_val = get_base_atk_def();
	atk = base_val.first;
	def = base_val.second;
	temp.attack = atk;
	temp.defense = def;
	filter_effect(EFFECT_UPDATE_ATTACK, &eset, FALSE);
	filter_effect(EFFECT_SET_ATTACK, &eset, FALSE);
	filter_effect(EFFECT_SET_ATTACK_FINAL, &eset, FALSE);
	if (!(data.type & TYPE_LINK)) {
		filter_effect(EFFECT_SWAP_AD, &eset, FALSE);
		filter_effect(EFFECT_UPDATE_DEFENSE, &eset, FALSE);
		filter_effect(EFFECT_SET_DEFENSE, &eset, FALSE);
		filter_effect(EFFECT_SET_DEFENSE_FINAL, &eset, FALSE);
	}
	std::sort(eset.begin(), eset.end(), effect_sort_id);
	bool rev = false;
	if (is_affected_by_effect(EFFECT_REVERSE_UPDATE))
		rev = true;
	for (effect_set::size_type i = 0; i < eset.size();) {
		if (eset[i]->type & EFFECT_TYPE_SINGLE) {
			switch (eset[i]->code) {
			case EFFECT_SET_ATTACK:
				atk = eset[i]->get_value(this);
				if (atk < 0)
					atk = 0;
				temp.attack = atk;
				eset.erase(eset.begin() + i);
				continue;
			case EFFECT_SET_DEFENSE:
				def = eset[i]->get_value(this);
				if (def < 0)
					def = 0;
				temp.defense = def;
				eset.erase(eset.begin() + i);
				continue;
			}
		}
		++i;
	}
	for (effect_set::size_type i = 0; i < eset.size(); ++i) {
		switch (eset[i]->code) {
		case EFFECT_UPDATE_ATTACK:
			if (eset[i]->is_flag(EFFECT_FLAG2_REPEAT_UPDATE))
				effects_repeat_update_atk.push_back(eset[i]);
			else if ((eset[i]->type & EFFECT_TYPE_SINGLE) && !eset[i]->is_flag(EFFECT_FLAG_SINGLE_RANGE))
				up_atk += eset[i]->get_value(this);
			else
				upc_atk += eset[i]->get_value(this);
			break;
		case EFFECT_SET_ATTACK:
			atk = eset[i]->get_value(this);
			if (atk < 0)
				atk = 0;
			up_atk = 0;
			break;
		case EFFECT_SET_ATTACK_FINAL:
			if ((eset[i]->type & EFFECT_TYPE_SINGLE) && !eset[i]->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
				atk = eset[i]->get_value(this);
				if (atk < 0)
					atk = 0;
				up_atk = 0;
				upc_atk = 0;
			}
			else if (eset[i]->is_flag(EFFECT_FLAG2_OPTION)) {
				effects_atk_option.push_back(eset[i]);
			}
			else if (eset[i]->is_flag(EFFECT_FLAG2_WICKED)) {
				effects_atk_wicked.push_back(eset[i]);
			}
			else {
				effects_atk_final.push_back(eset[i]);
			}
			break;
		// def
		case EFFECT_UPDATE_DEFENSE:
			if (eset[i]->is_flag(EFFECT_FLAG2_REPEAT_UPDATE))
				effects_repeat_update_def.push_back(eset[i]);
			else if ((eset[i]->type & EFFECT_TYPE_SINGLE) && !eset[i]->is_flag(EFFECT_FLAG_SINGLE_RANGE))
				up_def += eset[i]->get_value(this);
			else
				upc_def += eset[i]->get_value(this);
			break;
		case EFFECT_SET_DEFENSE:
			def = eset[i]->get_value(this);
			if (def < 0)
				def = 0;
			up_def = 0;
			break;
		case EFFECT_SET_DEFENSE_FINAL:
			if ((eset[i]->type & EFFECT_TYPE_SINGLE) && !eset[i]->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
				def = eset[i]->get_value(this);
				if (def < 0)
					def = 0;
				up_def = 0;
				upc_def = 0;
			}
			else if (eset[i]->is_flag(EFFECT_FLAG2_OPTION)) {
				effects_def_option.push_back(eset[i]);
			}
			else if (eset[i]->is_flag(EFFECT_FLAG2_WICKED)) {
				effects_def_wicked.push_back(eset[i]);
			}
			else {
				effects_def_final.push_back(eset[i]);
			}
			break;
		case EFFECT_SWAP_AD:
			swap_final = !swap_final;
			break;
		}
		if (!rev) {
			temp.attack = atk + up_atk + upc_atk;
			temp.defense = def + up_def + upc_def;
		}
		else {
			temp.attack = atk - up_atk - upc_atk;
			temp.defense = def - up_def - upc_def;
		}
		if (temp.attack < 0)
			temp.attack = 0;
		if (temp.defense < 0)
			temp.defense = 0;
	}
	for (effect_set::size_type i = 0; i < effects_repeat_update_atk.size(); ++i) {
		temp.attack += effects_repeat_update_atk[i]->get_value(this);
		if (temp.attack < 0)
			temp.attack = 0;
	}
	for (effect_set::size_type i = 0; i < effects_repeat_update_def.size(); ++i) {
		temp.defense += effects_repeat_update_def[i]->get_value(this);
		if (temp.defense < 0)
			temp.defense = 0;
	}
	for (effect_set::size_type i = 0; i < effects_atk_final.size(); ++i) {
		atk = effects_atk_final[i]->get_value(this);
		if (atk < 0)
			atk = 0;
		temp.attack = atk;
	}
	for (effect_set::size_type i = 0; i < effects_def_final.size(); ++i){
		def = effects_def_final[i]->get_value(this);
		if (def < 0)
			def = 0;
		temp.defense = def;
	}
	if (swap_final)
		std::swap(temp.attack, temp.defense);
	// The Wicked
	for (effect_set::size_type i = 0; i < effects_atk_wicked.size(); ++i) {
		atk = effects_atk_wicked[i]->get_value(this);
		if (atk < 0)
			atk = 0;
		temp.attack = atk;
	}
	for (effect_set::size_type i = 0; i < effects_def_wicked.size(); ++i) {
		def = effects_def_wicked[i]->get_value(this);
		if (def < 0)
			def = 0;
		temp.defense = def;
	}
	// Gradius' Option
	for (effect_set::size_type i = 0; i < effects_atk_option.size(); ++i) {
		atk = effects_atk_option[i]->get_value(this);
		if (atk < 0)
			atk = 0;
		temp.attack = atk;
	}
	for (effect_set::size_type i = 0; i < effects_def_option.size(); ++i) {
		def = effects_def_option[i]->get_value(this);
		if (def < 0)
			def = 0;
		temp.defense = def;
	}
	ret.first = temp.attack;
	ret.second = temp.defense;
	temp.attack = -1;
	temp.defense = -1;
	return ret;
}
int32_t card::get_base_attack() {
	return get_base_atk_def().first;
}
int32_t card::get_attack() {
	return get_atk_def().first;
}
int32_t card::get_base_defense() {
	return get_base_atk_def().second;
}
int32_t card::get_defense() {
	return get_atk_def().second;
}
int32_t card::get_battle_attack() {
	effect_set eset;
	filter_effect(EFFECT_SET_BATTLE_ATTACK, &eset);
	if (eset.size()) {
		int32_t atk = eset.back()->get_value(this);
		if (atk < 0)
			atk = 0;
		return atk;
	}
	else
		return get_atk_def().first;
}
int32_t card::get_battle_defense() {
	effect_set eset;
	filter_effect(EFFECT_SET_BATTLE_DEFENSE, &eset);
	if (eset.size()) {
		int32_t def = eset.back()->get_value(this);
		if (def < 0)
			def = 0;
		return def;
	}
	else
		return get_atk_def().second;
}
uint32_t card::get_level() {
	if((data.type & (TYPE_XYZ | TYPE_LINK)) || (status & STATUS_NO_LEVEL)
	        || (!(data.type & TYPE_MONSTER) && !(get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_PRE_MONSTER)))
		return 0;
	if(assume_type == ASSUME_LEVEL)
		return assume_value;
	if(temp.level != UINT32_MAX) // prevent recursion, return the former value
		return temp.level;
	effect_set effects;
	int32_t level = data.level;
	temp.level = level;
	int32_t up = 0;
	filter_effect(EFFECT_UPDATE_LEVEL, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_LEVEL, &effects);
	for (effect_set::size_type i = 0; i < effects.size(); ++i) {
		switch (effects[i]->code) {
		case EFFECT_UPDATE_LEVEL:
			up += effects[i]->get_value(this);
			break;
		case EFFECT_CHANGE_LEVEL:
			level = effects[i]->get_value(this);
			up = 0;
			break;
		}
		temp.level = level + up;
	}
	level += up;
	if (level < 1)
		level = 1;
	temp.level = UINT32_MAX;
	return level;
}
uint32_t card::get_rank() {
	if (!(data.type & TYPE_XYZ))
		return 0;
	if(assume_type == ASSUME_RANK)
		return assume_value;
	if(!(current.location & LOCATION_MZONE))
		return data.level;
	if(temp.level != UINT32_MAX) // prevent recursion, return the former value
		return temp.level;
	effect_set effects;
	int32_t rank = data.level;
	temp.level = rank;
	int32_t up = 0;
	filter_effect(EFFECT_UPDATE_RANK, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_RANK, &effects);
	for (effect_set::size_type i = 0; i < effects.size(); ++i) {
		switch (effects[i]->code) {
		case EFFECT_UPDATE_RANK:
			up += effects[i]->get_value(this);
			break;
		case EFFECT_CHANGE_RANK:
			rank = effects[i]->get_value(this);
			up = 0;
			break;
		}
		temp.level = rank + up;
	}
	rank += up;
	if (rank < 1)
		rank = 1;
	temp.level = UINT32_MAX;
	return rank;
}
uint32_t card::get_link() {
	if (!(data.type & TYPE_LINK))
		return 0;
	return data.level;
}

uint32_t card::get_mat_level_from_effect(card* pcard, uint32_t effect_code) {
	if(!effect_code)
		return 0;
	effect_set eset;
	filter_effect(effect_code, &eset);
	for (auto& peffect : eset) {
		uint32_t lev = peffect->get_value(pcard);
		if (lev)
			return lev;
	}
	return 0;
}
uint32_t card::get_mat_level(card* pcard, uint32_t level_effect_code, uint32_t allow_effect_code) {
	if((data.type & (TYPE_XYZ | TYPE_LINK)) || (status & STATUS_NO_LEVEL))
		return get_mat_level_from_effect(pcard, allow_effect_code);
	auto lv = get_mat_level_from_effect(pcard, level_effect_code);
	if(lv)
		return lv;
	return get_level();
}
uint32_t card::get_synchro_level(card* pcard) {
	return get_mat_level(pcard, EFFECT_SYNCHRO_LEVEL, EFFECT_SYNCHRO_LEVEL_EX);
}
uint32_t card::get_ritual_level(card* pcard) {
	return get_mat_level(pcard, EFFECT_RITUAL_LEVEL, EFFECT_RITUAL_LEVEL_EX);
}
uint32_t card::check_xyz_level(card* pcard, uint32_t lv) {
	if(status & STATUS_NO_LEVEL)
		return 0;
	int32_t min_count = 0;
	effect_set mset;
	filter_effect(EFFECT_XYZ_MIN_COUNT, &mset);
	for (auto& peffect: mset) {
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		int32_t count = peffect->get_value(2);
		if (count > min_count)
			min_count = count;
	}
	if (min_count > 0xf)
		min_count = 0xf;
	effect_set eset;
	filter_effect(EFFECT_XYZ_LEVEL, &eset);
	if(!eset.size()) {
		uint32_t card_lv = get_level();
		if (card_lv == lv)
			return (card_lv & MAX_XYZ_LEVEL) | ((uint32_t)min_count << 12);
		return 0;
	}
	for (auto& peffect: eset) {
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		uint32_t lev = peffect->get_value(2);
		uint16_t lv1 = lev & MAX_XYZ_LEVEL;
		uint16_t count1 = (lev & 0xf000) >> 12;
		if (count1 < min_count)
			count1 = min_count;
		if (lv1 == lv)
			return lv1 | ((uint32_t)count1 << 12);
		lev >>= 16;
		uint16_t lv2 = lev & MAX_XYZ_LEVEL;
		uint16_t count2 = (lev & 0xf000) >> 12;
		if (count2 < min_count)
			count2 = min_count;
		if (lv2 == lv)
			return lv2 | ((uint32_t)count2 << 12);
	}
	return 0;
}
uint32_t card::get_attribute() {
	if(assume_type == ASSUME_ATTRIBUTE)
		return assume_value;
	if(!(data.type & TYPE_MONSTER) && !(get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_PRE_MONSTER))
		return 0;
	if(temp.attribute != UINT32_MAX) // prevent recursion, return the former value
		return temp.attribute;
	effect_set effects;
	auto attribute = data.attribute;
	temp.attribute = data.attribute;
	filter_effect(EFFECT_ADD_ATTRIBUTE, &effects, FALSE);
	filter_effect(EFFECT_REMOVE_ATTRIBUTE, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_ATTRIBUTE, &effects);
	for (effect_set::size_type i = 0; i < effects.size(); ++i) {
		if (effects[i]->code == EFFECT_ADD_ATTRIBUTE)
			attribute |= effects[i]->get_value(this);
		else if (effects[i]->code == EFFECT_REMOVE_ATTRIBUTE)
			attribute &= ~(effects[i]->get_value(this));
		else if (effects[i]->code == EFFECT_CHANGE_ATTRIBUTE)
			attribute = effects[i]->get_value(this);
		temp.attribute = attribute;
	}
	temp.attribute = UINT32_MAX;
	return attribute;
}
uint32_t card::get_fusion_attribute(uint8_t playerid) {
	effect_set effects;
	filter_effect(EFFECT_CHANGE_FUSION_ATTRIBUTE, &effects);
	if(!effects.size() || pduel->game_field->core.not_material)
		return get_attribute();
	uint32_t attribute = 0;
	for(effect_set::size_type i = 0; i < effects.size(); ++i) {
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		attribute = effects[i]->get_value(this, 1);
	}
	return attribute;
}
uint32_t card::get_link_attribute(uint8_t playerid) {
	effect_set effects;
	filter_effect(EFFECT_ADD_LINK_ATTRIBUTE, &effects);
	uint32_t attribute = get_attribute();
	for (effect_set::size_type i = 0; i < effects.size(); ++i) {
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		attribute |= effects[i]->get_value(this, 1);
	}
	return attribute;
}
uint32_t card::get_grave_attribute(uint8_t playerid) {
	if(!(data.type & TYPE_MONSTER))
		return 0;
	if(current.is_location(LOCATION_GRAVE))
		return get_attribute();
	uint32_t attribute = data.attribute;
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, EFFECT_CHANGE_GRAVE_ATTRIBUTE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			attribute = eset[i]->get_value(this);
		else {
			pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
			pduel->lua->add_param(this, PARAM_TYPE_CARD);
			if(pduel->lua->check_condition(eset[i]->target, 2))
				attribute = eset[i]->get_value(this);
		}
	}
	return attribute;
}
uint32_t card::get_race() {
	if(assume_type == ASSUME_RACE)
		return assume_value;
	if(!(data.type & TYPE_MONSTER) && !(get_type() & TYPE_MONSTER) && !is_affected_by_effect(EFFECT_PRE_MONSTER))
		return 0;
	if(temp.race != UINT32_MAX) // prevent recursion, return the former value
		return temp.race;
	effect_set effects;
	auto race = data.race;
	temp.race = data.race;
	filter_effect(EFFECT_ADD_RACE, &effects, FALSE);
	filter_effect(EFFECT_REMOVE_RACE, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_RACE, &effects);
	for (effect_set::size_type i = 0; i < effects.size(); ++i) {
		if (effects[i]->code == EFFECT_ADD_RACE)
			race |= effects[i]->get_value(this);
		else if (effects[i]->code == EFFECT_REMOVE_RACE)
			race &= ~(effects[i]->get_value(this));
		else if (effects[i]->code == EFFECT_CHANGE_RACE)
			race = effects[i]->get_value(this);
		temp.race = race;
	}
	temp.race = UINT32_MAX;
	return race;
}
uint32_t card::get_link_race(uint8_t playerid) {
	effect_set effects;
	filter_effect(EFFECT_ADD_LINK_RACE, &effects);
	uint32_t race = get_race();
	for (effect_set::size_type i = 0; i < effects.size(); ++i) {
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		race |= effects[i]->get_value(this, 1);
	}
	return race;
}
uint32_t card::get_grave_race(uint8_t playerid) {
	if(!(data.type & TYPE_MONSTER))
		return 0;
	if(current.is_location(LOCATION_GRAVE))
		return get_race();
	uint32_t race = data.race;
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, EFFECT_CHANGE_GRAVE_RACE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(!eset[i]->target)
			race = eset[i]->get_value(this);
		else {
			pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
			pduel->lua->add_param(this, PARAM_TYPE_CARD);
			if(pduel->lua->check_condition(eset[i]->target, 2))
				race = eset[i]->get_value(this);
		}
	}
	return race;
}
uint32_t card::get_lscale() {
	if(!current.is_location(LOCATION_PZONE))
		return data.lscale;
	if(temp.lscale != UINT32_MAX) // prevent recursion, return the former value
		return temp.lscale;
	effect_set effects;
	int32_t lscale = data.lscale;
	temp.lscale = data.lscale;
	int32_t up = 0, upc = 0;
	filter_effect(EFFECT_UPDATE_LSCALE, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_LSCALE, &effects);
	for (effect_set::size_type i = 0; i < effects.size(); ++i) {
		if (effects[i]->code == EFFECT_UPDATE_LSCALE) {
			if ((effects[i]->type & EFFECT_TYPE_SINGLE) && !effects[i]->is_flag(EFFECT_FLAG_SINGLE_RANGE))
				up += effects[i]->get_value(this);
			else
				upc += effects[i]->get_value(this);
		} else {
			lscale = effects[i]->get_value(this);
			up = 0;
		}
		temp.lscale = lscale;
	}
	lscale += up + upc;
	if(lscale < 0 && current.pzone)
		lscale = 0;
	temp.lscale = UINT32_MAX;
	return lscale;
}
uint32_t card::get_rscale() {
	if(!current.is_location(LOCATION_PZONE))
		return data.rscale;
	if(temp.rscale != UINT32_MAX) // prevent recursion, return the former value
		return temp.rscale;
	effect_set effects;
	int32_t rscale = data.rscale;
	temp.rscale = data.rscale;
	int32_t up = 0, upc = 0;
	filter_effect(EFFECT_UPDATE_RSCALE, &effects, FALSE);
	filter_effect(EFFECT_CHANGE_RSCALE, &effects);
	for (effect_set::size_type i = 0; i < effects.size(); ++i) {
		if (effects[i]->code == EFFECT_UPDATE_RSCALE) {
			if ((effects[i]->type & EFFECT_TYPE_SINGLE) && !effects[i]->is_flag(EFFECT_FLAG_SINGLE_RANGE))
				up += effects[i]->get_value(this);
			else
				upc += effects[i]->get_value(this);
		} else {
			rscale = effects[i]->get_value(this);
			up = 0;
		}
		temp.rscale = rscale;
	}
	rscale += up + upc;
	if(rscale < 0 && current.pzone)
		rscale = 0;
	temp.rscale = UINT32_MAX;
	return rscale;
}
uint32_t card::get_link_marker() {
	if(!(data.type & TYPE_LINK))
		return 0;
	return data.link_marker;
}
uint32_t card::is_link_marker(uint32_t dir) {
	return get_link_marker() & dir;
}
uint32_t card::get_linked_zone() {
	if(!(data.type & TYPE_LINK) || current.location != LOCATION_MZONE || is_treated_as_not_on_field())
		return 0;
	uint32_t zones = 0;
	int32_t s = current.sequence;
	if(s > 0 && s <= 4 && is_link_marker(LINK_MARKER_LEFT))
		zones |= 1u << (s - 1);
	if(s <= 3 && is_link_marker(LINK_MARKER_RIGHT))
		zones |= 1u << (s + 1);
	if((s == 0 && is_link_marker(LINK_MARKER_TOP_RIGHT))
		|| (s == 1 && is_link_marker(LINK_MARKER_TOP))
		|| (s == 2 && is_link_marker(LINK_MARKER_TOP_LEFT)))
		zones |= (1u << 5) | (1u << (16 + 6));
	if((s == 2 && is_link_marker(LINK_MARKER_TOP_RIGHT))
		|| (s == 3 && is_link_marker(LINK_MARKER_TOP))
		|| (s == 4 && is_link_marker(LINK_MARKER_TOP_LEFT)))
		zones |= (1u << 6) | (1u << (16 + 5));
	if(s == 5) {
		if(is_link_marker(LINK_MARKER_BOTTOM_LEFT))
			zones |= 1u << 0;
		if(is_link_marker(LINK_MARKER_BOTTOM))
			zones |= 1u << 1;
		if(is_link_marker(LINK_MARKER_BOTTOM_RIGHT))
			zones |= 1u << 2;
		if(is_link_marker(LINK_MARKER_TOP_LEFT))
			zones |= 1u << (16 + 4);
		if(is_link_marker(LINK_MARKER_TOP))
			zones |= 1u << (16 + 3);
		if(is_link_marker(LINK_MARKER_TOP_RIGHT))
			zones |= 1u << (16 + 2);
	}
	if(s == 6) {
		if(is_link_marker(LINK_MARKER_BOTTOM_LEFT))
			zones |= 1u << 2;
		if(is_link_marker(LINK_MARKER_BOTTOM))
			zones |= 1u << 3;
		if(is_link_marker(LINK_MARKER_BOTTOM_RIGHT))
			zones |= 1u << 4;
		if(is_link_marker(LINK_MARKER_TOP_LEFT))
			zones |= 1u << (16 + 2);
		if(is_link_marker(LINK_MARKER_TOP))
			zones |= 1u << (16 + 1);
		if(is_link_marker(LINK_MARKER_TOP_RIGHT))
			zones |= 1u << (16 + 0);
	}
	return zones;
}
void card::get_linked_cards(card_set* cset) {
	cset->clear();
	if(!(data.type & TYPE_LINK) || current.location != LOCATION_MZONE)
		return;
	int32_t p = current.controler;
	uint32_t linked_zone = get_linked_zone();
	pduel->game_field->get_cards_in_zone(cset, linked_zone, p, LOCATION_MZONE);
	pduel->game_field->get_cards_in_zone(cset, linked_zone >> 16, 1 - p, LOCATION_MZONE);
}
uint32_t card::get_mutual_linked_zone() {
	if(!(data.type & TYPE_LINK) || current.location != LOCATION_MZONE || is_treated_as_not_on_field())
		return 0;
	uint32_t zones = 0;
	int32_t p = current.controler;
	int32_t s = current.sequence;
	uint32_t linked_zone = get_linked_zone();
	uint32_t icheck = 0x1U;
	for(int32_t i = 0; i < 7; ++i, icheck <<= 1) {
		if(icheck & linked_zone) {
			card* pcard = pduel->game_field->player[p].list_mzone[i];
			if(pcard && (pcard->get_linked_zone() & (0x1u << s)))
				zones |= icheck;
		}
	}
	icheck = 0x10000U;
	for(uint32_t i = 0; i < 7; ++i, icheck <<= 1) {
		if(icheck & linked_zone) {
			card* pcard = pduel->game_field->player[1 - p].list_mzone[i];
			if(pcard && (pcard->get_linked_zone() & (0x1u << (s + 16))))
				zones |= icheck;
		}
	}
	return zones;
}
void card::get_mutual_linked_cards(card_set* cset) {
	cset->clear();
	if(!(data.type & TYPE_LINK) || current.location != LOCATION_MZONE)
		return;
	int32_t p = current.controler;
	uint32_t mutual_linked_zone = get_mutual_linked_zone();
	pduel->game_field->get_cards_in_zone(cset, mutual_linked_zone, p, LOCATION_MZONE);
	pduel->game_field->get_cards_in_zone(cset, mutual_linked_zone >> 16, 1 - p, LOCATION_MZONE);
}
int32_t card::is_link_state() {
	if(current.location != LOCATION_MZONE)
		return FALSE;
	card_set cset;
	get_linked_cards(&cset);
	if(cset.size())
		return TRUE;
	int32_t p = current.controler;
	uint32_t linked_zone = pduel->game_field->get_linked_zone(p);
	if((linked_zone >> current.sequence) & 0x1U)
		return TRUE;
	return FALSE;
}
int32_t card::is_extra_link_state() {
	if(current.location != LOCATION_MZONE)
		return FALSE;
	uint32_t checked = 0x1U << current.sequence;
	uint32_t linked_zone = get_mutual_linked_zone();
	const auto& list_mzone0 = pduel->game_field->player[current.controler].list_mzone;
	const auto& list_mzone1 = pduel->game_field->player[1 - current.controler].list_mzone;
	while(true) {
		if(((linked_zone >> 5) | (linked_zone >> (16 + 6))) & ((linked_zone >> 6) | (linked_zone >> (16 + 5))) & 0x1U)
			return TRUE;
		uint32_t checking = linked_zone & ~checked;
		if(!checking)
			return FALSE;
		uint32_t rightmost = checking & (~checking + 1);
		checked |= rightmost;
		if(rightmost < 0x10000U) {
			for(int32_t i = 0; i < 7; ++i) {
				if(rightmost & 0x1U) {
					card* pcard = list_mzone0[i];
					linked_zone |= pcard->get_mutual_linked_zone();
					break;
				}
				rightmost >>= 1;
			}
		} else {
			rightmost >>= 16;
			for(int32_t i = 0; i < 7; ++i) {
				if(rightmost & 0x1U) {
					card* pcard = list_mzone1[i];
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
int32_t card::is_position(uint32_t pos) const {
	return current.position & pos;
}
void card::set_status(uint32_t x, int32_t enabled) {
	if (enabled)
		status |= x;
	else
		status &= ~x;
}
// get match status
int32_t card::get_status(uint32_t x) const {
	return status & x;
}
// match all status
int32_t card::is_status(uint32_t x) const {
	if ((status & x) == x)
		return TRUE;
	return FALSE;
}
uint32_t card::get_column_zone(int32_t location) {
	int32_t zones = 0;
	uint8_t seq = current.sequence;
	if(!(location & LOCATION_ONFIELD) || !(current.location & LOCATION_ONFIELD) || current.location == LOCATION_SZONE && seq >= 5)
		return 0;
	if(seq <= 4) {
		if(location & LOCATION_MZONE) {
			if(!(current.location & LOCATION_MZONE))
				zones |= 1u << seq;
			zones |= 1u << (16 + (4 - seq));
			if(seq == 1)
				zones |= (1u << 5) | (1u << (16 + 6));
			if(seq == 3)
				zones |= (1u << 6) | (1u << (16 + 5));
		}
		if(location & LOCATION_SZONE) {
			if(!(current.location & LOCATION_SZONE))
				zones |= 1u << (seq + 8);
			zones |= 1u << (16 + 8 + (4 - seq));
		}
	}
	if(seq == 5) {
		if(location & LOCATION_MZONE)
			zones |= (1u << 1) | (1u << (16 + 3));
		if(location & LOCATION_SZONE)
			zones |= (1u << (8 + 1)) | (1u << (16 + 8 + 3));
	}
	if(seq == 6) {
		if(location & LOCATION_MZONE)
			zones |= (1u << 3) | (1u << (16 + 1));
		if(location & LOCATION_SZONE)
			zones |= (1u << (8 + 3)) | (1u << (16 + 8 + 1));
	}
	return zones;
}
void card::get_column_cards(card_set* cset) {
	cset->clear();
	if(!(current.location & LOCATION_ONFIELD))
		return;
	int32_t p = current.controler;
	uint32_t column_mzone = get_column_zone(LOCATION_MZONE);
	uint32_t column_szone = get_column_zone(LOCATION_SZONE);
	pduel->game_field->get_cards_in_zone(cset, column_mzone, p, LOCATION_MZONE);
	pduel->game_field->get_cards_in_zone(cset, column_mzone >> 16, 1 - p, LOCATION_MZONE);
	pduel->game_field->get_cards_in_zone(cset, column_szone >> 8, p, LOCATION_SZONE);
	pduel->game_field->get_cards_in_zone(cset, column_szone >> 24, 1 - p, LOCATION_SZONE);
}
int32_t card::is_all_column() {
	if(!(current.location & LOCATION_ONFIELD))
		return FALSE;
	card_set cset;
	get_column_cards(&cset);
	int32_t full = 3;
	if(pduel->game_field->core.duel_rule >= NEW_MASTER_RULE && (current.sequence == 1 || current.sequence == 3))
		++full;
	if(cset.size() == full)
		return TRUE;
	return FALSE;
}
uint8_t card::get_select_sequence(uint8_t *deck_seq_pointer) {
	if(current.location == LOCATION_DECK && pduel->game_field->is_select_hide_deck_sequence(current.controler)) {
		return (*deck_seq_pointer)++;
	} else {
		return current.sequence;
	}
}
uint32_t card::get_select_info_location(uint8_t *deck_seq_pointer) {
	if(current.location == LOCATION_DECK) {
		uint32_t c = current.controler;
		uint32_t l = current.location;
		uint32_t s = get_select_sequence(deck_seq_pointer);
		uint32_t ss = current.position;
		return c + (l << 8) + (s << 16) + (ss << 24);
	} else {
		return get_info_location();
	}
}
int32_t card::is_treated_as_not_on_field() const {
	return get_status(STATUS_SUMMONING | STATUS_SUMMON_DISABLED | STATUS_ACTIVATE_DISABLED | STATUS_SPSUMMON_STEP);
}
void card::equip(card* target, uint32_t send_msg) {
	if (equiping_target)
		return;
	target->equiping_cards.insert(this);
	equiping_target = target;
	for (auto& it : equip_effect) {
		if (it.second->is_disable_related())
			pduel->game_field->add_to_disable_check_list(equiping_target);
	}
	if(send_msg) {
		pduel->write_buffer8(MSG_EQUIP);
		pduel->write_buffer32(get_info_location());
		pduel->write_buffer32(target->get_info_location());
	}
	return;
}
void card::unequip() {
	if (!equiping_target)
		return;
	for (auto& it : equip_effect) {
		if (it.second->is_disable_related())
			pduel->game_field->add_to_disable_check_list(equiping_target);
	}
	equiping_target->equiping_cards.erase(this);
	pre_equip_target = equiping_target;
	equiping_target = 0;
	return;
}
int32_t card::get_union_count() {
	int32_t count = 0;
	for(auto& pcard : equiping_cards) {
		if((pcard->data.type & TYPE_UNION) && pcard->is_affected_by_effect(EFFECT_UNION_STATUS))
			++count;
	}
	return count;
}
int32_t card::get_old_union_count() {
	int32_t count = 0;
	for(auto& pcard : equiping_cards) {
		if((pcard->data.type & TYPE_UNION) && pcard->is_affected_by_effect(EFFECT_OLDUNION_STATUS))
			++count;
	}
	return count;
}
void card::xyz_overlay(const card_set& materials) {
	if(materials.empty())
		return;
	card_set des, leave_grave, leave_deck;
	card_vector cv(materials.begin(), materials.end());
	std::sort(cv.begin(), cv.end(), card::card_operation_sort);
	if(pduel->game_field->core.global_flag & GLOBALFLAG_DECK_REVERSE_CHECK) {
		int32_t d0 = (int32_t)pduel->game_field->player[0].list_main.size() - 1, s0 = d0;
		int32_t d1 = (int32_t)pduel->game_field->player[1].list_main.size() - 1, s1 = d1;
		for(auto& pcard : cv) {
			if(pcard->current.location != LOCATION_DECK)
				continue;
			if((pcard->current.controler == 0) && (pcard->current.sequence == s0))
				--s0;
			if((pcard->current.controler == 1) && (pcard->current.sequence == s1))
				--s1;
		}
		if((s0 != d0) && (s0 > 0)) {
			card* ptop = pduel->game_field->player[0].list_main[s0];
			if(pduel->game_field->core.deck_reversed || (ptop->current.position == POS_FACEUP_DEFENSE)) {
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
			card* ptop = pduel->game_field->player[1].list_main[s1];
			if(pduel->game_field->core.deck_reversed || (ptop->current.position == POS_FACEUP_DEFENSE)) {
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
	for(auto& pcard : cv) {
		if(pcard->overlay_target == this)
			continue;
		pcard->current.reason = REASON_XYZ + REASON_MATERIAL;
		pcard->reset(RESET_LEAVE + RESET_OVERLAY, RESET_EVENT);
		if(pcard->unique_code)
			pduel->game_field->remove_unique_card(pcard);
		if(pcard->equiping_target)
			pcard->unequip();
		des.insert(pcard->equiping_cards.begin(), pcard->equiping_cards.end());
		for(auto cit = pcard->equiping_cards.begin(); cit != pcard->equiping_cards.end();) {
			card* equipc = *cit++;
			equipc->unequip();
		}
		pcard->clear_card_target();
		pduel->write_buffer8(MSG_MOVE);
		pduel->write_buffer32(pcard->data.code);
		pduel->write_buffer32(pcard->get_info_location());
		if(pcard->overlay_target) {
			pcard->overlay_target->xyz_remove(pcard);
		} else {
			pcard->enable_field_effect(false);
			pduel->game_field->remove_card(pcard);
			pduel->game_field->add_to_disable_check_list(pcard);
		}
		xyz_add(pcard);
		if(pcard->previous.location == LOCATION_GRAVE) {
			leave_grave.insert(pcard);
			pduel->game_field->raise_single_event(pcard, 0, EVENT_LEAVE_GRAVE, pduel->game_field->core.reason_effect, pcard->current.reason, pduel->game_field->core.reason_player, 0, 0);
		} else if(pcard->previous.location == LOCATION_DECK || pcard->previous.location == LOCATION_EXTRA) {
			leave_deck.insert(pcard);
			pduel->game_field->raise_single_event(pcard, 0, EVENT_LEAVE_DECK, pduel->game_field->core.reason_effect, pcard->current.reason, pduel->game_field->core.reason_player, 0, 0);
		}
		pduel->write_buffer32(pcard->get_info_location());
		pduel->write_buffer32(pcard->current.reason);
	}
	if(leave_grave.size() || leave_deck.size()) {
		if(leave_grave.size()) {
			pduel->game_field->raise_event(leave_grave, EVENT_LEAVE_GRAVE, pduel->game_field->core.reason_effect, REASON_XYZ + REASON_MATERIAL, pduel->game_field->core.reason_player, 0, 0);
		}
		if(leave_deck.size()) {
			pduel->game_field->raise_event(leave_deck, EVENT_LEAVE_DECK, pduel->game_field->core.reason_effect, REASON_XYZ + REASON_MATERIAL, pduel->game_field->core.reason_player, 0, 0);
		}
		pduel->game_field->process_single_event();
		pduel->game_field->process_instant_event();
	}
	if(des.size())
		pduel->game_field->destroy(des, 0, REASON_LOST_TARGET + REASON_RULE, PLAYER_NONE);
	else
		pduel->game_field->adjust_instant();
}
void card::xyz_add(card* mat) {
	if(mat->current.location != 0)
		return;
	xyz_materials.push_back(mat);
	mat->overlay_target = this;
	mat->current.controler = PLAYER_NONE;
	mat->current.location = LOCATION_OVERLAY;
	mat->current.sequence = (uint8_t)xyz_materials.size() - 1;
	for(auto& eit : mat->xmaterial_effect) {
		effect* peffect = eit.second;
		if(peffect->type & EFFECT_TYPE_FIELD)
			pduel->game_field->add_effect(peffect);
	}
}
void card::xyz_remove(card* mat) {
	if(mat->overlay_target != this)
		return;
	if (std::find(xyz_materials.begin(), xyz_materials.end(), mat) == xyz_materials.end())
		return;
	xyz_materials.erase(std::remove(xyz_materials.begin(), xyz_materials.end(), mat), xyz_materials.end());
	mat->previous.controler = mat->current.controler;
	mat->previous.location = mat->current.location;
	mat->previous.sequence = mat->current.sequence;
	mat->previous.pzone = mat->current.pzone;
	mat->current.controler = PLAYER_NONE;
	mat->current.location = 0;
	mat->current.sequence = 0;
	mat->overlay_target = 0;
	for(auto clit = xyz_materials.begin(); clit != xyz_materials.end(); ++clit)
		(*clit)->current.sequence = (uint8_t)(clit - xyz_materials.begin());
	for(auto& eit : mat->xmaterial_effect) {
		effect* peffect = eit.second;
		if(peffect->type & EFFECT_TYPE_FIELD)
			pduel->game_field->remove_effect(peffect);
	}
}
void card::apply_field_effect() {
	if (current.controler == PLAYER_NONE)
		return;
	for (auto& it : field_effect) {
		if (it.second->in_range(this) || it.second->is_hand_trigger()) {
			pduel->game_field->add_effect(it.second);
		}
	}
	if(unique_code && (current.location & unique_location))
		pduel->game_field->add_unique_card(this);
	spsummon_counter[0] = spsummon_counter[1] = 0;
}
void card::cancel_field_effect() {
	if (current.controler == PLAYER_NONE)
		return;
	for (auto& it : field_effect) {
		pduel->game_field->remove_effect(it.second);
	}
	if(unique_code && (current.location & unique_location))
		pduel->game_field->remove_unique_card(this);
}
// STATUS_EFFECT_ENABLED: the card is ready to use
// false: before moving, summoning, chaining
// true: ready
void card::enable_field_effect(bool enabled) {
	if (current.location == 0)
		return;
	if ((enabled && get_status(STATUS_EFFECT_ENABLED)) || (!enabled && !get_status(STATUS_EFFECT_ENABLED)))
		return;
	refresh_disable_status();
	if (enabled) {
		set_status(STATUS_EFFECT_ENABLED, TRUE);
		for (auto& it : single_effect) {
			if (it.second->is_flag(EFFECT_FLAG_SINGLE_RANGE) && it.second->in_range(this))
				it.second->id = pduel->game_field->infos.field_id++;
		}
		for (auto& it : field_effect) {
			if (it.second->in_range(this))
				it.second->id = pduel->game_field->infos.field_id++;
		}
		if(current.location == LOCATION_SZONE) {
			for (auto& it : equip_effect)
				it.second->id = pduel->game_field->infos.field_id++;
		}
		for (auto& it : target_effect) {
			if (it.second->in_range(this))
				it.second->id = pduel->game_field->infos.field_id++;
		}
		if (get_status(STATUS_DISABLED))
			reset(RESET_DISABLE, RESET_EVENT);
	} else
		set_status(STATUS_EFFECT_ENABLED, FALSE);
	if (get_status(STATUS_DISABLED | STATUS_FORBIDDEN))
		return;
	filter_disable_related_cards();
}
int32_t card::add_effect(effect* peffect) {
	if (!peffect)
		return 0;
	if (get_status(STATUS_COPYING_EFFECT) && peffect->is_flag(EFFECT_FLAG_UNCOPYABLE)) {
		pduel->uncopy.insert(peffect);
		return 0;
	}
	if (indexer.find(peffect) != indexer.end())
		return 0;
	if (peffect->type & EFFECT_TYPE_SINGLE && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE) && peffect->owner == this 
		&& get_status(STATUS_DISABLED) && (peffect->reset_flag & RESET_DISABLE))
		return 0;
	if (peffect->type & EFFECT_TYPES_TRIGGER_LIKE && is_continuous_event(peffect->code))
		return 0;
	// the trigger effect in phase is "once per turn" by default
	if (peffect->get_code_type() == CODE_PHASE && peffect->code & (PHASE_DRAW | PHASE_STANDBY | PHASE_END)
		&& peffect->type & (EFFECT_TYPE_TRIGGER_O | EFFECT_TYPE_TRIGGER_F) && !peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
		peffect->flag[0] |= EFFECT_FLAG_COUNT_LIMIT;
		peffect->count_limit = 1;
		peffect->count_limit_max = 1;
	}
	// add EFFECT_FLAG_IGNORE_IMMUNE to EFFECT_CANNOT_TRIGGER by default
	if (peffect->code == EFFECT_CANNOT_TRIGGER) {
		peffect->flag[0] |= EFFECT_FLAG_IGNORE_IMMUNE;
	}
	card_set check_target = { this };
	effect_container::iterator eit;
	if (peffect->type & EFFECT_TYPE_SINGLE) {
		// assign atk/def
		if(peffect->code == EFFECT_SET_ATTACK && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			for(auto it = single_effect.begin(); it != single_effect.end();) {
				auto rm = it++;
				if(rm->second->code == EFFECT_SET_ATTACK && !rm->second->is_flag(EFFECT_FLAG_SINGLE_RANGE))
					remove_effect(rm->second);
			}
		}
		if(peffect->code == EFFECT_SET_ATTACK_FINAL && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			for(auto it = single_effect.begin(); it != single_effect.end();) {
				auto rm = it++;
				auto code = rm->second->code;
				if((code == EFFECT_UPDATE_ATTACK || code == EFFECT_SET_ATTACK || code == EFFECT_SET_ATTACK_FINAL)
						&& !rm->second->is_flag(EFFECT_FLAG_SINGLE_RANGE))
					remove_effect(rm->second);
			}
		}
		if(peffect->code == EFFECT_SET_DEFENSE && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			for(auto it = single_effect.begin(); it != single_effect.end();) {
				auto rm = it++;
				if(rm->second->code == EFFECT_SET_DEFENSE && !rm->second->is_flag(EFFECT_FLAG_SINGLE_RANGE))
					remove_effect(rm->second);
			}
		}
		if(peffect->code == EFFECT_SET_DEFENSE_FINAL && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			for(auto it = single_effect.begin(); it != single_effect.end();) {
				auto rm = it++;
				auto code = rm->second->code;
				if((code == EFFECT_UPDATE_DEFENSE || code == EFFECT_SET_DEFENSE || code == EFFECT_SET_DEFENSE_FINAL)
						&& !rm->second->is_flag(EFFECT_FLAG_SINGLE_RANGE))
					remove_effect(rm->second);
			}
		}
		// assign base atk/def
		if (peffect->code == EFFECT_SET_BASE_ATTACK && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			for (auto it = single_effect.begin(); it != single_effect.end();) {
				auto rm = it++;
				if (rm->second->code == EFFECT_SET_BASE_ATTACK && !rm->second->is_flag(EFFECT_FLAG_SINGLE_RANGE))
					remove_effect(rm->second);
			}
		}
		if (peffect->code == EFFECT_SET_BASE_ATTACK_FINAL && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			for (auto it = single_effect.begin(); it != single_effect.end();) {
				auto rm = it++;
				auto code = rm->second->code;
				if ((code == EFFECT_SET_BASE_ATTACK || code == EFFECT_SET_BASE_ATTACK_FINAL || code == EFFECT_SET_ATTACK || code == EFFECT_SET_ATTACK_FINAL)
					&& !rm->second->is_flag(EFFECT_FLAG_SINGLE_RANGE))
					remove_effect(rm->second);
			}
		}
		if (peffect->code == EFFECT_SET_BASE_DEFENSE && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			for (auto it = single_effect.begin(); it != single_effect.end();) {
				auto rm = it++;
				if (rm->second->code == EFFECT_SET_BASE_DEFENSE && !rm->second->is_flag(EFFECT_FLAG_SINGLE_RANGE))
					remove_effect(rm->second);
			}
		}
		if (peffect->code == EFFECT_SET_BASE_DEFENSE_FINAL && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
			for (auto it = single_effect.begin(); it != single_effect.end();) {
				auto rm = it++;
				auto code = rm->second->code;
				if ((code == EFFECT_SET_BASE_DEFENSE || code == EFFECT_SET_BASE_DEFENSE_FINAL || code == EFFECT_SET_DEFENSE || code == EFFECT_SET_DEFENSE_FINAL)
					&& !rm->second->is_flag(EFFECT_FLAG_SINGLE_RANGE))
					remove_effect(rm->second);
			}
		}
		eit = single_effect.emplace(peffect->code, peffect);
	} else if (peffect->type & EFFECT_TYPE_EQUIP) {
		eit = equip_effect.emplace(peffect->code, peffect);
		if (equiping_target)
			check_target = { equiping_target };
		else
			check_target.clear();
	} else if(peffect->type & EFFECT_TYPE_TARGET) {
		eit = target_effect.emplace(peffect->code, peffect);
		if(!effect_target_cards.empty())
			check_target = effect_target_cards;
		else
			check_target.clear();
	} else if (peffect->type & EFFECT_TYPE_XMATERIAL) {
		eit = xmaterial_effect.emplace(peffect->code, peffect);
		if (overlay_target)
			check_target = { overlay_target };
		else
			check_target.clear();
	} else if (peffect->type & EFFECT_TYPE_FIELD) {
		eit = field_effect.emplace(peffect->code, peffect);
	} else
		return 0;
	peffect->id = pduel->game_field->infos.field_id++;
	peffect->card_type = data.type;
	if (get_status(STATUS_INITIALIZING))
		peffect->flag[0] |= EFFECT_FLAG_INITIAL;
	else if (get_status(STATUS_COPYING_EFFECT))
		peffect->flag[0] |= EFFECT_FLAG_COPY;
	if (get_status(STATUS_COPYING_EFFECT)) {
		peffect->copy_id = pduel->game_field->infos.copy_id;
		peffect->reset_flag |= pduel->game_field->core.copy_reset;
		peffect->reset_count = pduel->game_field->core.copy_reset_count;
	}
	effect* reason_effect = pduel->game_field->core.reason_effect;
	indexer.emplace(peffect, eit);
	peffect->handler = this;
	if (peffect->is_flag(EFFECT_FLAG_INITIAL))
		initial_effect.insert(peffect);
	else if (peffect->is_flag(EFFECT_FLAG_COPY))
		owning_effect.insert(peffect);
	if((peffect->type & EFFECT_TYPE_FIELD)) {
		if(peffect->in_range(this) || current.controler != PLAYER_NONE && peffect->is_hand_trigger())
			pduel->game_field->add_effect(peffect);
	}
	if (current.controler != PLAYER_NONE && !check_target.empty()) {
		if (peffect->is_disable_related()) {
			for (auto& target : check_target)
				pduel->game_field->add_to_disable_check_list(target);
		}
	}
	if(peffect->is_flag(EFFECT_FLAG_OATH)) {
		pduel->game_field->effects.oath.emplace(peffect, reason_effect);
	}
	if(peffect->reset_flag & RESET_PHASE) {
		pduel->game_field->effects.pheff.insert(peffect);
		if(peffect->reset_count == 0)
			peffect->reset_count = 1;
	}
	if(peffect->reset_flag & RESET_CHAIN)
		pduel->game_field->effects.cheff.insert(peffect);
	if(peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT))
		pduel->game_field->effects.rechargeable.insert(peffect);
	if(peffect->is_flag(EFFECT_FLAG_CLIENT_HINT)) {
		pduel->write_buffer8(MSG_CARD_HINT);
		pduel->write_buffer32(get_info_location());
		pduel->write_buffer8(CHINT_DESC_ADD);
		pduel->write_buffer32(peffect->description);
	}
	if(peffect->type & EFFECT_TYPE_SINGLE && peffect->code == EFFECT_UPDATE_LEVEL && !peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE)) {
		int32_t val = peffect->get_value(this);
		if(val > 0) {
			pduel->game_field->raise_single_event(this, 0, EVENT_LEVEL_UP, peffect, 0, 0, 0, val);
			pduel->game_field->process_single_event();
		}
	}
	return peffect->id;
}
effect_indexer::iterator card::remove_effect(effect* peffect) {
	auto index = indexer.find(peffect);
	if (index == indexer.end())
		return index;
	auto& it = index->second;
	card_set check_target = { this };
	if (peffect->type & EFFECT_TYPE_SINGLE) {
		single_effect.erase(it);
	} else if (peffect->type & EFFECT_TYPE_EQUIP) {
		equip_effect.erase(it);
		if (equiping_target)
			check_target = { equiping_target };
		else
			check_target.clear();
	} else if(peffect->type & EFFECT_TYPE_TARGET) {
		target_effect.erase(it);
		if(!effect_target_cards.empty())
			check_target = effect_target_cards;
		else
			check_target.clear();
	} else if (peffect->type & EFFECT_TYPE_XMATERIAL) {
		xmaterial_effect.erase(it);
		if (overlay_target)
			check_target = { overlay_target };
		else
			check_target.clear();
	} else if (peffect->type & EFFECT_TYPE_FIELD) {
		check_target.clear();
		if (peffect->is_available() && peffect->is_disable_related()) {
			pduel->game_field->update_disable_check_list(peffect);
		}
		field_effect.erase(it);
		pduel->game_field->remove_effect(peffect);
	}
	if ((current.controler != PLAYER_NONE) && !get_status(STATUS_DISABLED | STATUS_FORBIDDEN) && !check_target.empty()) {
		if (peffect->is_disable_related()) {
			for (auto& target : check_target)
				pduel->game_field->add_to_disable_check_list(target);
		}
	}
	auto ret = indexer.erase(index);
	if (peffect->is_flag(EFFECT_FLAG_INITIAL))
		initial_effect.erase(peffect);
	else if (peffect->is_flag(EFFECT_FLAG_COPY))
		owning_effect.erase(peffect);
	if(peffect->is_flag(EFFECT_FLAG_OATH))
		pduel->game_field->effects.oath.erase(peffect);
	if(peffect->reset_flag & RESET_PHASE)
		pduel->game_field->effects.pheff.erase(peffect);
	if(peffect->reset_flag & RESET_CHAIN)
		pduel->game_field->effects.cheff.erase(peffect);
	if(peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT))
		pduel->game_field->effects.rechargeable.erase(peffect);
	if(peffect->get_code_type() == CODE_COUNTER && (peffect->code & 0xf0000) == EFFECT_COUNTER_PERMIT && (peffect->type & EFFECT_TYPE_SINGLE)) {
		auto cmit = counters.find(peffect->code & 0xffff);
		if(cmit != counters.end()) {
			pduel->write_buffer8(MSG_REMOVE_COUNTER);
			pduel->write_buffer16(cmit->first);
			pduel->write_buffer8(current.controler);
			pduel->write_buffer8(current.location);
			pduel->write_buffer8(current.sequence);
			pduel->write_buffer16(cmit->second);
			counters.erase(cmit);
		}
	}
	if(peffect->is_flag(EFFECT_FLAG_CLIENT_HINT)) {
		pduel->write_buffer8(MSG_CARD_HINT);
		pduel->write_buffer32(get_info_location());
		pduel->write_buffer8(CHINT_DESC_REMOVE);
		pduel->write_buffer32(peffect->description);
	}
	if(peffect->code == EFFECT_UNIQUE_CHECK) {
		pduel->game_field->remove_unique_card(this);
		unique_pos[0] = unique_pos[1] = 0;
		unique_code = 0;
	}
	pduel->game_field->core.reseted_effects.insert(peffect);
	return ret;
}
int32_t card::copy_effect(uint32_t code, uint32_t reset, int32_t count) {
	card_data cdata;
	::read_card(code, &cdata);
	if(cdata.type & TYPE_NORMAL)
		return -1;
	if (!reset)
		reset = RESETS_STANDARD;
	set_status(STATUS_COPYING_EFFECT, TRUE);
	auto cr = pduel->game_field->core.copy_reset;
	auto crc = pduel->game_field->core.copy_reset_count;
	pduel->game_field->core.copy_reset = reset;
	pduel->game_field->core.copy_reset_count = count;
	pduel->lua->add_param(this, PARAM_TYPE_CARD);
	pduel->lua->call_code_function(code, "initial_effect", 1, 0);
	++pduel->game_field->infos.copy_id;
	set_status(STATUS_COPYING_EFFECT, FALSE);
	pduel->game_field->core.copy_reset = cr;
	pduel->game_field->core.copy_reset_count = crc;
	for(auto& peffect : pduel->uncopy)
		pduel->delete_effect(peffect);
	pduel->uncopy.clear();
	if((data.type & TYPE_MONSTER) && !(data.type & TYPE_EFFECT)) {
		effect* peffect = pduel->new_effect();
		if(pduel->game_field->core.reason_effect)
			peffect->owner = pduel->game_field->core.reason_effect->get_handler();
		else
			peffect->owner = this;
		peffect->handler = this;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_ADD_TYPE;
		peffect->value = TYPE_EFFECT;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = reset;
		peffect->reset_count = count;
		this->add_effect(peffect);
	}
	return pduel->game_field->infos.copy_id - 1;
}
int32_t card::replace_effect(uint32_t code, uint32_t reset, int32_t count) {
	card_data cdata;
	::read_card(code, &cdata);
	if(cdata.type & TYPE_NORMAL)
		return -1;
	if (!reset)
		reset = RESETS_STANDARD;
	if(is_status(STATUS_EFFECT_REPLACED))
		set_status(STATUS_EFFECT_REPLACED, FALSE);
	for(auto it = indexer.begin(); it != indexer.end();) {
		effect* const& peffect = it->first;
		if (peffect->is_flag(EFFECT_FLAG_INITIAL))
			it = remove_effect(peffect);
		else
			++it;
	}
	auto cr = pduel->game_field->core.copy_reset;
	auto crc = pduel->game_field->core.copy_reset_count;
	pduel->game_field->core.copy_reset = reset;
	pduel->game_field->core.copy_reset_count = count;
	set_status(STATUS_INITIALIZING | STATUS_COPYING_EFFECT, TRUE);
	pduel->lua->add_param(this, PARAM_TYPE_CARD);
	pduel->lua->call_code_function(code, "initial_effect", 1, 0);
	set_status(STATUS_INITIALIZING | STATUS_COPYING_EFFECT, FALSE);
	++pduel->game_field->infos.copy_id;
	pduel->game_field->core.copy_reset = cr;
	pduel->game_field->core.copy_reset_count = crc;
	set_status(STATUS_EFFECT_REPLACED, TRUE);
	for(auto& peffect : pduel->uncopy)
		pduel->delete_effect(peffect);
	pduel->uncopy.clear();
	if((data.type & TYPE_MONSTER) && !(data.type & TYPE_EFFECT)) {
		effect* peffect = pduel->new_effect();
		if(pduel->game_field->core.reason_effect)
			peffect->owner = pduel->game_field->core.reason_effect->get_handler();
		else
			peffect->owner = this;
		peffect->handler = this;
		peffect->type = EFFECT_TYPE_SINGLE;
		peffect->code = EFFECT_ADD_TYPE;
		peffect->value = TYPE_EFFECT;
		peffect->flag[0] = EFFECT_FLAG_CANNOT_DISABLE;
		peffect->reset_flag = reset;
		peffect->reset_count = count;
		this->add_effect(peffect);
	}
	return pduel->game_field->infos.copy_id - 1;
}
void card::reset(uint32_t id, uint32_t reset_type) {
	if (reset_type != RESET_EVENT && reset_type != RESET_PHASE && reset_type != RESET_CODE && reset_type != RESET_COPY && reset_type != RESET_CARD)
		return;
	if (reset_type == RESET_EVENT) {
		for (auto rit = relations.begin(); rit != relations.end();) {
			if (rit->second & 0xffff0000 & id)
				rit = relations.erase(rit);
			else
				++rit;
		}
		if(id & (RESET_TODECK | RESET_TOHAND | RESET_TOGRAVE | RESET_REMOVE | RESET_TEMP_REMOVE
			| RESET_OVERLAY | RESET_MSCHANGE))
			clear_relate_effect();
		if(id & (RESET_TODECK | RESET_TOHAND | RESET_TOGRAVE | RESET_REMOVE | RESET_TEMP_REMOVE
			| RESET_OVERLAY | RESET_MSCHANGE | RESET_LEAVE | RESET_TOFIELD)) {
			indestructable_effects.clear();
			announced_cards.clear();
			attacked_cards.clear();
			attack_announce_count = 0;
			announce_count = 0;
			attacked_count = 0;
			attack_all_target = TRUE;
		}
		if(id & (RESET_TODECK | RESET_TOHAND | RESET_TOGRAVE | RESET_REMOVE | RESET_TEMP_REMOVE
			| RESET_OVERLAY | RESET_MSCHANGE | RESET_LEAVE | RESET_TOFIELD | RESET_TURN_SET)) {
			battled_cards.clear();
			reset_effect_count();
			auto pr = field_effect.equal_range(EFFECT_DISABLE_FIELD);
			for(; pr.first != pr.second; ++pr.first){
				if(!pr.first->second->is_flag(EFFECT_FLAG_FUNC_VALUE))
					pr.first->second->value = 0;
			}
		}
		if(id & (RESET_TODECK | RESET_TOHAND | RESET_TOGRAVE | RESET_REMOVE | RESET_TEMP_REMOVE
			| RESET_OVERLAY | RESET_MSCHANGE | RESET_TOFIELD  | RESET_TURN_SET)) {
			counters.clear();
		}
		if(id & (RESET_TODECK | RESET_TOHAND | RESET_TOGRAVE | RESET_REMOVE | RESET_TEMP_REMOVE
			| RESET_LEAVE | RESET_TOFIELD | RESET_TURN_SET | RESET_CONTROL)) {
			auto pr = field_effect.equal_range(EFFECT_USE_EXTRA_MZONE);
			for(; pr.first != pr.second; ++pr.first)
				pr.first->second->value = pr.first->second->value & 0xffff;
			pr = field_effect.equal_range(EFFECT_USE_EXTRA_SZONE);
			for(; pr.first != pr.second; ++pr.first)
				pr.first->second->value = pr.first->second->value & 0xffff;
		}
		if(id & RESET_TOFIELD) {
			pre_equip_target = 0;
		}
		if(id & RESET_DISABLE) {
			for(auto cmit = counters.begin(); cmit != counters.end();) {
				if (cmit->first & COUNTER_WITHOUT_PERMIT) {
					++cmit;
					continue;
				}
				pduel->write_buffer8(MSG_REMOVE_COUNTER);
				pduel->write_buffer16(cmit->first);
				pduel->write_buffer8(current.controler);
				pduel->write_buffer8(current.location);
				pduel->write_buffer8(current.sequence);
				pduel->write_buffer16(cmit->second);
				cmit = counters.erase(cmit);
			}
		}
	}
	else if (reset_type == RESET_COPY) {
		delete_card_target(TRUE);
		effect_target_cards.clear();
	}
	bool reload = false;
	for (auto it = indexer.begin(); it != indexer.end();) {
		effect* const& peffect = it->first;
		if (peffect->reset(id, reset_type)) {
			if (is_status(STATUS_EFFECT_REPLACED) && peffect->is_flag(EFFECT_FLAG_INITIAL) && peffect->copy_id)
				reload = true;
			it = remove_effect(peffect);
		}
		else
			++it;
	}
	if (reload) {
		set_status(STATUS_EFFECT_REPLACED, FALSE);
		if (interpreter::is_load_script(data)) {
			set_status(STATUS_INITIALIZING, TRUE);
			pduel->lua->add_param(this, PARAM_TYPE_CARD);
			pduel->lua->call_card_function(this, "initial_effect", 1, 0);
			set_status(STATUS_INITIALIZING, FALSE);
		}
	}
}
void card::reset_effect_count() {
	for (auto& i : indexer) {
		effect* const& peffect = i.first;
		if (peffect->is_flag(EFFECT_FLAG_COUNT_LIMIT))
			peffect->recharge();
	}
}
// refresh STATUS_DISABLED based on EFFECT_DISABLE and EFFECT_CANNOT_DISABLE
// refresh STATUS_FORBIDDEN based on EFFECT_FORBIDDEN
void card::refresh_disable_status() {
	filter_immune_effect();
	// forbidden
	if (is_affected_by_effect(EFFECT_FORBIDDEN))
		set_status(STATUS_FORBIDDEN, TRUE);
	else
		set_status(STATUS_FORBIDDEN, FALSE);
	// disabled
	if (!is_affected_by_effect(EFFECT_CANNOT_DISABLE) && is_affected_by_effect(EFFECT_DISABLE))
		set_status(STATUS_DISABLED, TRUE);
	else
		set_status(STATUS_DISABLED, FALSE);
}
std::tuple<uint8_t, effect*> card::refresh_control_status() {
	uint8_t final = owner;
	effect* ceffect = nullptr;
	uint32_t last_id = 0;
	if(pduel->game_field->core.remove_brainwashing && is_affected_by_effect(EFFECT_REMOVE_BRAINWASHING))
		last_id = pduel->game_field->core.last_control_changed_id;
	effect_set eset;
	filter_effect(EFFECT_SET_CONTROL, &eset);
	if(eset.size()) {
		effect* peffect = eset.back();
		if(peffect->id >= last_id) {
			final = (uint8_t)peffect->get_value(this);
			ceffect = peffect;
		}
	}
	return std::make_tuple(final, ceffect);
}
void card::count_turn(uint16_t ct) {
	turn_counter = ct;
	pduel->write_buffer8(MSG_CARD_HINT);
	pduel->write_buffer32(get_info_location());
	pduel->write_buffer8(CHINT_TURN);
	pduel->write_buffer32(ct);
}
void card::create_relation(card* target, uint32_t reset) {
	if (relations.find(target) != relations.end())
		return;
	relations[target] = reset;
}
int32_t card::is_has_relation(card* target) {
	if (relations.find(target) != relations.end())
		return TRUE;
	return FALSE;
}
void card::release_relation(card* target) {
	if (relations.find(target) == relations.end())
		return;
	relations.erase(target);
}
void card::create_relation(const chain& ch) {
	relate_effect.emplace(ch.triggering_effect, ch.chain_id);
}
int32_t card::is_has_relation(const chain& ch) {
	if (relate_effect.find(effect_relation::value_type(ch.triggering_effect, ch.chain_id)) != relate_effect.end())
		return TRUE;
	return FALSE;
}
void card::release_relation(const chain& ch) {
	relate_effect.erase(effect_relation::value_type(ch.triggering_effect, ch.chain_id));
}
void card::clear_relate_effect() {
	relate_effect.clear();
}
void card::create_relation(effect* peffect) {
	for(auto it = pduel->game_field->core.current_chain.rbegin(); it != pduel->game_field->core.current_chain.rend(); ++it) {
		if(it->triggering_effect == peffect) {
			create_relation(*it);
			return;
		}
	}
	relate_effect.emplace(peffect, (uint16_t)0);
}
int32_t card::is_has_relation(effect* peffect) {
	for(auto& it : relate_effect) {
		if(it.first == peffect)
			return TRUE;
	}
	return FALSE;
}
void card::release_relation(effect* peffect) {
	for(auto it = pduel->game_field->core.current_chain.rbegin(); it != pduel->game_field->core.current_chain.rend(); ++it) {
		if(it->triggering_effect == peffect) {
			release_relation(*it);
			return;
		}
	}
	relate_effect.erase(effect_relation::value_type(peffect, 0));
}
int32_t card::leave_field_redirect(uint32_t reason) {
	effect_set es;
	uint32_t redirect;
	uint32_t redirects = 0;
	if(data.type & TYPE_TOKEN)
		return 0;
	filter_effect(EFFECT_LEAVE_FIELD_REDIRECT, &es);
	for(effect_set::size_type i = 0; i < es.size(); ++i) {
		effect* peffect = es[i];
		redirect = peffect->get_value(this, 0);
		if((redirect & LOCATION_HAND) && !is_affected_by_effect(EFFECT_CANNOT_TO_HAND) && pduel->game_field->is_player_can_send_to_hand(es[i]->get_handler_player(), this))
			redirects |= redirect;
		else if((redirect & LOCATION_DECK) && !is_affected_by_effect(EFFECT_CANNOT_TO_DECK) && pduel->game_field->is_player_can_send_to_deck(es[i]->get_handler_player(), this))
			redirects |= redirect;
		else if((redirect & LOCATION_REMOVED) && !is_affected_by_effect(EFFECT_CANNOT_REMOVE) && pduel->game_field->is_player_can_remove(es[i]->get_handler_player(), this, REASON_EFFECT | REASON_REDIRECT, peffect))
			redirects |= redirect;
	}
	if(redirects & LOCATION_REMOVED)
		return LOCATION_REMOVED;
	// the ruling for the priority of the following redirects can't be confirmed for now
	if(redirects & LOCATION_DECK) {
		if((redirects & LOCATION_DECKBOT) == LOCATION_DECKBOT)
			return LOCATION_DECKBOT;
		if((redirects & LOCATION_DECKSHF) == LOCATION_DECKSHF)
			return LOCATION_DECKSHF;
		return LOCATION_DECK;
	}
	if(redirects & LOCATION_HAND)
		return LOCATION_HAND;
	return 0;
}
int32_t card::destination_redirect(uint8_t destination, uint32_t reason) {
	effect_set es;
	uint32_t redirect;
	if(data.type & TYPE_TOKEN)
		return 0;
	if(destination == LOCATION_HAND)
		filter_effect(EFFECT_TO_HAND_REDIRECT, &es);
	else if(destination == LOCATION_DECK)
		filter_effect(EFFECT_TO_DECK_REDIRECT, &es);
	else if(destination == LOCATION_GRAVE)
		filter_effect(EFFECT_TO_GRAVE_REDIRECT, &es);
	else if(destination == LOCATION_REMOVED)
		filter_effect(EFFECT_REMOVE_REDIRECT, &es);
	else
		return 0;
	for(effect_set::size_type i = 0; i < es.size(); ++i) {
		effect* peffect = es[i];
		redirect = peffect->get_value(this, 0);
		if((redirect & LOCATION_HAND) && !is_affected_by_effect(EFFECT_CANNOT_TO_HAND) && pduel->game_field->is_player_can_send_to_hand(es[i]->get_handler_player(), this))
			return redirect;
		if((redirect & LOCATION_DECK) && !is_affected_by_effect(EFFECT_CANNOT_TO_DECK) && pduel->game_field->is_player_can_send_to_deck(es[i]->get_handler_player(), this))
			return redirect;
		if((redirect & LOCATION_REMOVED) && !is_affected_by_effect(EFFECT_CANNOT_REMOVE) && pduel->game_field->is_player_can_remove(es[i]->get_handler_player(), this, REASON_EFFECT | REASON_REDIRECT, peffect))
			return redirect;
		if((redirect & LOCATION_GRAVE) && !is_affected_by_effect(EFFECT_CANNOT_TO_GRAVE) && pduel->game_field->is_player_can_send_to_grave(es[i]->get_handler_player(), this))
			return redirect;
	}
	return 0;
}
int32_t card::add_counter(uint8_t playerid, uint16_t countertype, uint16_t count, uint8_t singly) {
	if(!is_can_add_counter(playerid, countertype, count, singly, 0))
		return FALSE;
	uint16_t cttype = countertype;
	auto pr = counters.emplace(cttype, 0);
	auto cmit = pr.first;
	auto pcount = count;
	if(singly) {
		effect_set eset;
		int32_t limit = 0;
		filter_effect(EFFECT_COUNTER_LIMIT + cttype, &eset);
		if (eset.size())
			limit = eset.back()->get_value();
		if(limit) {
			int32_t mcount = limit - get_counter(cttype);
			if (mcount < 0)
				mcount = 0;
			if (pcount > mcount)
				pcount = (uint16_t)mcount;
		}
	}
	cmit->second += pcount;
	pduel->write_buffer8(MSG_ADD_COUNTER);
	pduel->write_buffer16(cttype);
	pduel->write_buffer8(current.controler);
	pduel->write_buffer8(current.location);
	pduel->write_buffer8(current.sequence);
	pduel->write_buffer16(pcount);
	pduel->game_field->raise_single_event(this, 0, EVENT_ADD_COUNTER + countertype, pduel->game_field->core.reason_effect, REASON_EFFECT, playerid, playerid, pcount);
	pduel->game_field->process_single_event();
	return TRUE;
}
int32_t card::remove_counter(uint16_t countertype, uint16_t count) {
	auto cmit = counters.find(countertype);
	if(cmit == counters.end())
		return FALSE;
	auto remove_count = count;
	if (cmit->second <= count) {
		remove_count = cmit->second;
		counters.erase(cmit);
	}
	else {
		cmit->second -= count;
	}
	pduel->write_buffer8(MSG_REMOVE_COUNTER);
	pduel->write_buffer16(countertype);
	pduel->write_buffer8(current.controler);
	pduel->write_buffer8(current.location);
	pduel->write_buffer8(current.sequence);
	pduel->write_buffer16(remove_count);
	return TRUE;
}
// return: the player can put a counter on this or not
int32_t card::is_can_add_counter(uint8_t playerid, uint16_t countertype, uint16_t count, uint8_t singly, uint32_t loc) {
	effect_set eset;
	if (!pduel->game_field->is_player_can_place_counter(playerid, this, countertype, count))
		return FALSE;
	if (!loc && (!(current.location & LOCATION_ONFIELD) || !is_position(POS_FACEUP)))
		return FALSE;
	uint32_t check = countertype & COUNTER_WITHOUT_PERMIT;
	if(!check) {
		filter_effect(EFFECT_COUNTER_PERMIT + countertype, &eset);
		for(effect_set::size_type i = 0; i < eset.size(); ++i) {
			uint32_t prange = eset[i]->range;
			if(loc)
				check = loc & prange;
			else
				check = TRUE;
			if(check)
				break;
		}
		eset.clear();
	}
	if(!check)
		return FALSE;
	int32_t limit = -1;
	int32_t cur = 0;
	auto cmit = counters.find(countertype);
	if (cmit != counters.end())
		cur = cmit->second;
	filter_effect(EFFECT_COUNTER_LIMIT + countertype, &eset);
	if (eset.size())
		limit = eset.back()->get_value();
	if(limit > 0 && (cur + (singly ? 1 : count) > limit))
		return FALSE;
	return TRUE;
}
// return: this have a EFFECT_COUNTER_PERMIT of countertype or not
int32_t card::is_can_have_counter(uint16_t countertype) {
	effect_set eset;
	if (countertype & COUNTER_WITHOUT_PERMIT)
		return FALSE;
	else {
		filter_self_effect(EFFECT_COUNTER_PERMIT + countertype, &eset);
		if (current.is_location(LOCATION_ONFIELD)) {
			for (effect_set::size_type i = 0; i < eset.size(); ++i) {
				if (eset[i]->is_single_ready())
					return TRUE;
			}
			return FALSE;
		}
		else if (eset.size())
			return TRUE;
		else
			return FALSE;
	}
	return FALSE;
}
int32_t card::get_counter(uint16_t countertype) {
	auto cmit = counters.find(countertype);
	if(cmit == counters.end())
		return 0;
	return cmit->second;
}
void card::set_material(card_set* materials) {
	if(!materials) {
		material_cards.clear();
	} else
		material_cards = *materials;
	for(auto& pcard : material_cards)
		pcard->current.reason_card = this;
	effect_set eset;
	filter_effect(EFFECT_MATERIAL_CHECK, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		eset[i]->get_value(this);
	}
}
void card::add_card_target(card* pcard) {
	effect_target_cards.insert(pcard);
	pcard->effect_target_owner.insert(this);
	for(auto& it : target_effect) {
		if(it.second->is_disable_related())
			pduel->game_field->add_to_disable_check_list(pcard);
	}
	pduel->write_buffer8(MSG_CARD_TARGET);
	pduel->write_buffer32(get_info_location());
	pduel->write_buffer32(pcard->get_info_location());
}
void card::cancel_card_target(card* pcard) {
	auto cit = effect_target_cards.find(pcard);
	if(cit != effect_target_cards.end()) {
		effect_target_cards.erase(cit);
		pcard->effect_target_owner.erase(this);
		for(auto& it : target_effect) {
			if(it.second->is_disable_related())
				pduel->game_field->add_to_disable_check_list(pcard);
		}
		pduel->write_buffer8(MSG_CANCEL_TARGET);
		pduel->write_buffer32(get_info_location());
		pduel->write_buffer32(pcard->get_info_location());
	}
}
void card::delete_card_target(uint32_t send_msg) {
	for (auto& pcard : effect_target_cards) {
		pcard->effect_target_owner.erase(this);
		for (auto& it : target_effect) {
			if (it.second->is_disable_related())
				pduel->game_field->add_to_disable_check_list(pcard);
		}
		for (auto it = pcard->single_effect.begin(); it != pcard->single_effect.end();) {
			auto rm = it++;
			effect* const& peffect = rm->second;
			if ((peffect->owner == this) && peffect->is_flag(EFFECT_FLAG_OWNER_RELATE))
				pcard->remove_effect(peffect);
		}
		if (send_msg) {
			pduel->write_buffer8(MSG_CANCEL_TARGET);
			pduel->write_buffer32(get_info_location());
			pduel->write_buffer32(pcard->get_info_location());
		}
	}
}
void card::clear_card_target() {
	for(auto& pcard : effect_target_owner) {
		pcard->effect_target_cards.erase(this);
		for(auto& it : pcard->target_effect) {
			if(it.second->is_disable_related())
				pduel->game_field->add_to_disable_check_list(this);
		}
	}
	delete_card_target(FALSE);
	effect_target_owner.clear();
	effect_target_cards.clear();
}
void card::set_special_summon_status(effect* peffect) {
	if((peffect->code == EFFECT_SPSUMMON_PROC || peffect->code == EFFECT_SPSUMMON_PROC_G)
		&& peffect->is_flag(EFFECT_FLAG_CANNOT_DISABLE) && peffect->is_flag(EFFECT_FLAG_UNCOPYABLE)) {
		spsummon.code = 0;
		spsummon.code2 = 0;
		spsummon.type = 0;
		spsummon.level = 0;
		spsummon.rank = 0;
		spsummon.attribute = 0;
		spsummon.race = 0;
		spsummon.attack = 0;
		spsummon.defense = 0;
		spsummon.setcode.clear();
		spsummon.reason_effect = nullptr;
		spsummon.reason_player = PLAYER_NONE;
		return;
	}
	card* pcard = peffect->get_handler();
	auto cait = pduel->game_field->core.current_chain.rbegin();
	if(!(peffect->type & EFFECT_TYPES_CHAIN_LINK) || (pcard->is_has_relation(*cait) && !(pcard->get_type() & TYPE_TRAPMONSTER))) {
		spsummon.code = pcard->get_code();
		spsummon.code2 = pcard->get_another_code();
		spsummon.type = pcard->get_type();
		spsummon.level = pcard->get_level();
		spsummon.rank = pcard->get_rank();
		spsummon.attribute = pcard->get_attribute();
		spsummon.race = pcard->get_race();
		std::pair<int32_t, int32_t> atk_def = pcard->get_atk_def();
		spsummon.attack = atk_def.first;
		spsummon.defense = atk_def.second;
		spsummon.setcode.clear();
		effect_set eset;
		pcard->filter_effect(EFFECT_ADD_SETCODE, &eset);
		for(effect_set::size_type i = 0; i < eset.size(); ++i) {
			spsummon.setcode.push_back(eset[i]->get_value(pcard) & 0xffffU);
		}
		spsummon.reason_effect = peffect;
		if(pduel->game_field->core.current_chain.size())
			spsummon.reason_player = cait->triggering_player;
		else
			spsummon.reason_player = summon_player;
	} else {
		pcard = cait->triggering_effect->get_handler();
		spsummon.code = cait->triggering_state.code;
		spsummon.code2 = cait->triggering_state.code2;
		spsummon.type = cait->triggering_effect->card_type;
		spsummon.level = cait->triggering_state.level;
		spsummon.rank = cait->triggering_state.rank;
		spsummon.attribute = cait->triggering_state.attribute;
		spsummon.race = cait->triggering_state.race;
		spsummon.attack = cait->triggering_state.attack;
		spsummon.defense = cait->triggering_state.defense;
		spsummon.setcode.clear();
		effect_set eset;
		pcard->filter_effect(EFFECT_ADD_SETCODE, &eset);
		for(effect_set::size_type i = 0; i < eset.size(); ++i) {
			spsummon.setcode.push_back(eset[i]->get_value(pcard) & 0xffffU);
		}
		spsummon.reason_effect = cait->triggering_effect;
		spsummon.reason_player = cait->triggering_player;
	}
}
auto default_single_filter = [](card* c, effect* peffect) -> bool {
	return peffect->is_available() && (!peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE) || c->is_affect_by_effect(peffect));
};
auto default_equip_filter = [](card* c, effect* peffect) -> bool {
	return peffect->is_available() && c->is_affect_by_effect(peffect);
};
auto default_target_filter = [](card* c, effect* peffect) -> bool {
	return peffect->is_available() && peffect->is_target(c) && c->is_affect_by_effect(peffect);
};
auto default_xmaterial_filter = [](card* c, effect* peffect) -> bool {
	return !(peffect->type & EFFECT_TYPE_FIELD) && peffect->is_available() && c->is_affect_by_effect(peffect);
};
auto default_aura_filter = [](card* c, effect* peffect) -> bool {
	return !peffect->is_flag(EFFECT_FLAG_PLAYER_TARGET) && peffect->is_available() && peffect->is_target(c) && c->is_affect_by_effect(peffect);
};
auto accept_filter = [](card* c, effect* peffect) -> bool {
	return true;
};
template<typename T>
void card::filter_effect_container(const effect_container& container, uint32_t code, effect_filter f, T& eset) {
	auto rg = container.equal_range(code);
	for (auto it = rg.first; it != rg.second; ++it) {
		if (f(this, it->second))
			eset.push_back(it->second);
	}
}
void card::filter_effect_container(const effect_container& container, uint32_t code, effect_filter f, effect_collection& eset) {
	auto rg = container.equal_range(code);
	for (auto it = rg.first; it != rg.second; ++it) {
		if (f(this, it->second))
			eset.insert(it->second);
	}
}
void card::filter_effect(uint32_t code, effect_set* eset, uint8_t sort) {
	filter_effect_container(single_effect, code, default_single_filter, *eset);
	for (auto& pcard : equiping_cards)
		filter_effect_container(pcard->equip_effect, code, default_equip_filter, *eset);
	for (auto& pcard : effect_target_owner)
		filter_effect_container(pcard->target_effect, code, default_target_filter, *eset);
	for (auto& pcard : xyz_materials)
		filter_effect_container(pcard->xmaterial_effect, code, default_xmaterial_filter, *eset);
	filter_effect_container(pduel->game_field->effects.aura_effect, code, default_aura_filter, *eset);
	if(sort)
		std::sort(eset->begin(), eset->end(), effect_sort_id);
}
void card::filter_single_continuous_effect(uint32_t code, effect_set* eset, uint8_t sort) {
	filter_effect_container(single_effect, code, accept_filter, *eset);
	for (auto& pcard : equiping_cards)
		filter_effect_container(pcard->equip_effect, code, accept_filter, *eset);
	auto target_filter = [](card* c, effect* peffect) -> bool {
		return peffect->is_target(c);
	};
	for (auto& pcard : effect_target_owner)
		filter_effect_container(pcard->target_effect, code, target_filter, *eset);
	auto xmaterial_filter = [](card* c, effect* peffect) -> bool {
		return !(peffect->type & EFFECT_TYPE_FIELD);
	};
	for (auto& pcard : xyz_materials)
		filter_effect_container(pcard->xmaterial_effect, code, xmaterial_filter, *eset);
	if(sort)
		std::sort(eset->begin(), eset->end(), effect_sort_id);
}
void card::filter_self_effect(uint32_t code, effect_set* eset, uint8_t sort) {
	auto single_filter = [](card* c, effect* peffect) -> bool {
		return peffect->is_flag(EFFECT_FLAG_SINGLE_RANGE);
	};
	filter_effect_container(single_effect, code, single_filter, *eset);
	auto xmaterial_filter = [](card* c, effect* peffect) -> bool {
		return !(peffect->type & EFFECT_TYPE_FIELD);
	};
	for (auto& pcard : xyz_materials)
		filter_effect_container(pcard->xmaterial_effect, code, xmaterial_filter, *eset);
	if (sort)
		std::sort(eset->begin(), eset->end(), effect_sort_id);
}
// refresh this->immune_effect
void card::filter_immune_effect() {
	immune_effect.clear();
	filter_effect_container(single_effect, EFFECT_IMMUNE_EFFECT, accept_filter, immune_effect);
	for (auto& pcard : equiping_cards)
		filter_effect_container(pcard->equip_effect, EFFECT_IMMUNE_EFFECT, accept_filter, immune_effect);
	auto target_filter = [](card* c, effect* peffect) -> bool {
		return peffect->is_target(c);
	};
	for (auto& pcard : effect_target_owner)
		filter_effect_container(pcard->target_effect, EFFECT_IMMUNE_EFFECT, target_filter, immune_effect);
	auto xmaterial_filter = [](card* c, effect* peffect) -> bool {
		return !(peffect->type & EFFECT_TYPE_FIELD);
	};
	for (auto& pcard : xyz_materials)
		filter_effect_container(pcard->xmaterial_effect, EFFECT_IMMUNE_EFFECT, xmaterial_filter, immune_effect);
	filter_effect_container(pduel->game_field->effects.aura_effect, EFFECT_IMMUNE_EFFECT, target_filter, immune_effect);
	std::sort(immune_effect.begin(), immune_effect.end(), effect_sort_id);
}
// for all disable-related peffect of this,
// 1. Insert all cards in the target of peffect into effects.disable_check_list.
// 2. Insert equiping_target of peffect into it.
// 3. Insert overlay_target of peffect into it.
// 4. Insert continuous target of this into it.
void card::filter_disable_related_cards() {
	for (auto& it : indexer) {
		effect* const& peffect = it.first;
		if (peffect->is_disable_related()) {
			if (peffect->type & EFFECT_TYPE_FIELD)
				pduel->game_field->update_disable_check_list(peffect);
			else if ((peffect->type & EFFECT_TYPE_EQUIP) && equiping_target)
				pduel->game_field->add_to_disable_check_list(equiping_target);
			else if((peffect->type & EFFECT_TYPE_TARGET) && !effect_target_cards.empty()) {
				for(auto& target : effect_target_cards)
					pduel->game_field->add_to_disable_check_list(target);
			} else if((peffect->type & EFFECT_TYPE_XMATERIAL) && overlay_target)
				pduel->game_field->add_to_disable_check_list(overlay_target);
		}
	}
}
// put all summon procedures except ordinay summon in peset (see is_can_be_summoned())
// return value:
// -2 = this has a EFFECT_LIMIT_SUMMON_PROC, 0 available
// -1 = this has a EFFECT_LIMIT_SUMMON_PROC, at least 1 available
// 0 = no EFFECT_LIMIT_SUMMON_PROC, and ordinary summon is not available
// 1 = no EFFECT_LIMIT_SUMMON_PROC, and ordinary summon is available
int32_t card::filter_summon_procedure(uint8_t playerid, effect_set* peset, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone) {
	effect_set eset;
	filter_effect(EFFECT_LIMIT_SUMMON_PROC, &eset);
	if(eset.size()) {
		for(effect_set::size_type i = 0; i < eset.size(); ++i) {
			if(check_summon_procedure(eset[i], playerid, ignore_count, min_tribute, zone))
				peset->push_back(eset[i]);
		}
		if(peset->size())
			return -1;
		return -2;
	}
	eset.clear();
	filter_effect(EFFECT_SUMMON_PROC, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(check_summon_procedure(eset[i], playerid, ignore_count, min_tribute, zone))
			peset->push_back(eset[i]);
	}
	// ordinary summon
	if(!pduel->game_field->is_player_can_summon(SUMMON_TYPE_NORMAL, playerid, this, playerid))
		return FALSE;
	if(pduel->game_field->check_unique_onfield(this, playerid, LOCATION_MZONE))
		return FALSE;
	int32_t rcount = get_summon_tribute_count();
	int32_t min = rcount & 0xffff;
	int32_t max = (rcount >> 16) & 0xffff;
	if(!pduel->game_field->is_player_can_summon(SUMMON_TYPE_ADVANCE, playerid, this, playerid))
		max = 0;
	if(min < min_tribute)
		min = min_tribute;
	if(max < min)
		return FALSE;
	if(!ignore_count && !pduel->game_field->core.extra_summon[playerid]
			&& pduel->game_field->core.summon_count[playerid] >= pduel->game_field->get_summon_count_limit(playerid)) {
		effect_set extra_count;
		filter_effect(EFFECT_EXTRA_SUMMON_COUNT, &extra_count);
		for(effect_set::size_type i = 0; i < extra_count.size(); ++i) {
			std::vector<lua_Integer> retval;
			extra_count[i]->get_value(this, 0, retval);
			int32_t new_min = retval.size() > 0 ? static_cast<int32_t>(retval[0]) : 0;
			uint32_t new_zone = retval.size() > 1 ? static_cast<uint32_t>(retval[1]) : 0x1f;
			int32_t releasable = retval.size() > 2 ? (retval[2] < 0 ? 0xff00ff + static_cast<int32_t>(retval[2]) : static_cast<int32_t>(retval[2])) : 0xff00ff;
			if(new_min < min)
				new_min = min;
			new_zone &= zone;
			if(pduel->game_field->check_tribute(this, new_min, max, 0, current.controler, new_zone, releasable))
				return TRUE;
		}
	} else
		return pduel->game_field->check_tribute(this, min, max, 0, current.controler, zone);
	return FALSE;
}
int32_t card::check_summon_procedure(effect* proc, uint8_t playerid, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone) {
	if(!proc->check_count_limit(playerid))
		return FALSE;
	uint8_t toplayer = playerid;
	if(proc->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
		if(proc->o_range)
			toplayer = 1 - playerid;
	}
	if(!pduel->game_field->is_player_can_summon(proc->get_value(this), playerid, this, toplayer))
		return FALSE;
	if(pduel->game_field->check_unique_onfield(this, toplayer, LOCATION_MZONE))
		return FALSE;
	// the script will check min_tribute, Duel.CheckTribute()
	if(!ignore_count && !pduel->game_field->core.extra_summon[playerid]
			&& pduel->game_field->core.summon_count[playerid] >= pduel->game_field->get_summon_count_limit(playerid)) {
		effect_set eset;
		filter_effect(EFFECT_EXTRA_SUMMON_COUNT, &eset);
		for(effect_set::size_type i = 0; i < eset.size(); ++i) {
			std::vector<lua_Integer> retval;
			eset[i]->get_value(this, 0, retval);
			int32_t new_min_tribute = retval.size() > 0 ? static_cast<int32_t>(retval[0]) : 0;
			uint32_t new_zone = retval.size() > 1 ? static_cast<uint32_t>(retval[1]) : 0x1f;
			int32_t releasable = retval.size() > 2 ? (retval[2] < 0 ? 0xff00ff + static_cast<int32_t>(retval[2]) : static_cast<int32_t>(retval[2])) : 0xff00ff;
			if(new_min_tribute < min_tribute)
				new_min_tribute = min_tribute;
			new_zone &= zone;
			if(is_summonable(proc, new_min_tribute, new_zone, releasable))
				return TRUE;
		}
	} else
		return is_summonable(proc, min_tribute, zone);
	return FALSE;
}
// put all set procedures except ordinay set in peset (see is_can_be_summoned())
int32_t card::filter_set_procedure(uint8_t playerid, effect_set* peset, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone) {
	effect_set eset;
	filter_effect(EFFECT_LIMIT_SET_PROC, &eset);
	if(eset.size()) {
		for(effect_set::size_type i = 0; i < eset.size(); ++i) {
			if(check_set_procedure(eset[i], playerid, ignore_count, min_tribute, zone))
				peset->push_back(eset[i]);
		}
		if(peset->size())
			return -1;
		return -2;
	}
	eset.clear();
	filter_effect(EFFECT_SET_PROC, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(check_set_procedure(eset[i], playerid, ignore_count, min_tribute, zone))
			peset->push_back(eset[i]);
	}
	if(!pduel->game_field->is_player_can_mset(SUMMON_TYPE_NORMAL, playerid, this, playerid))
		return FALSE;
	int32_t rcount = get_set_tribute_count();
	int32_t min = rcount & 0xffff;
	int32_t max = (rcount >> 16) & 0xffff;
	if(!pduel->game_field->is_player_can_mset(SUMMON_TYPE_ADVANCE, playerid, this, playerid))
		max = 0;
	if(min < min_tribute)
		min = min_tribute;
	if(max < min)
		return FALSE;
	if(!ignore_count && !pduel->game_field->core.extra_summon[playerid]
			&& pduel->game_field->core.summon_count[playerid] >= pduel->game_field->get_summon_count_limit(playerid)) {
		effect_set extra_count;
		filter_effect(EFFECT_EXTRA_SET_COUNT, &extra_count);
		for(effect_set::size_type i = 0; i < extra_count.size(); ++i) {
			std::vector<lua_Integer> retval;
			extra_count[i]->get_value(this, 0, retval);
			int32_t new_min = retval.size() > 0 ? static_cast<int32_t>(retval[0]) : 0;
			uint32_t new_zone = retval.size() > 1 ? static_cast<uint32_t>(retval[1]) : 0x1f;
			int32_t releasable = retval.size() > 2 ? (retval[2] < 0 ? 0xff00ff + static_cast<int32_t>(retval[2]) : static_cast<int32_t>(retval[2])) : 0xff00ff;
			if(new_min < min)
				new_min = min;
			new_zone &= zone;
			if(pduel->game_field->check_tribute(this, new_min, max, 0, current.controler, new_zone, releasable, POS_FACEDOWN_DEFENSE))
				return TRUE;
		}
	} else
		return pduel->game_field->check_tribute(this, min, max, 0, current.controler, zone, 0xff00ff, POS_FACEDOWN_DEFENSE);
	return FALSE;
}
int32_t card::check_set_procedure(effect* proc, uint8_t playerid, uint8_t ignore_count, uint8_t min_tribute, uint32_t zone) {
	if(!proc->check_count_limit(playerid))
		return FALSE;
	uint8_t toplayer = playerid;
	if(proc->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
		if(proc->o_range)
			toplayer = 1 - playerid;
	}
	if(!pduel->game_field->is_player_can_mset(proc->get_value(this), playerid, this, toplayer))
		return FALSE;
	if(!ignore_count && !pduel->game_field->core.extra_summon[playerid]
			&& pduel->game_field->core.summon_count[playerid] >= pduel->game_field->get_summon_count_limit(playerid)) {
		effect_set eset;
		filter_effect(EFFECT_EXTRA_SET_COUNT, &eset);
		for(effect_set::size_type i = 0; i < eset.size(); ++i) {
			std::vector<lua_Integer> retval;
			eset[i]->get_value(this, 0, retval);
			int32_t new_min_tribute = retval.size() > 0 ? static_cast<int32_t>(retval[0]) : 0;
			uint32_t new_zone = retval.size() > 1 ? static_cast<uint32_t>(retval[1]) : 0x1f;
			int32_t releasable = retval.size() > 2 ? (retval[2] < 0 ? 0xff00ff + static_cast<int32_t>(retval[2]) : static_cast<int32_t>(retval[2])) : 0xff00ff;
			if(new_min_tribute < min_tribute)
				new_min_tribute = min_tribute;
			new_zone &= zone;
			if(is_summonable(proc, new_min_tribute, new_zone, releasable))
				return TRUE;
		}
	} else
		return is_summonable(proc, min_tribute, zone);
	return FALSE;
}
void card::filter_spsummon_procedure(uint8_t playerid, effect_set* peset, uint32_t summon_type, material_info info) {
	effect_collection proc_set;
	filter_effect_container(field_effect, EFFECT_SPSUMMON_PROC, accept_filter, proc_set);
	for (auto& peffect : proc_set) {
		uint8_t toplayer{};
		uint8_t topos{};
		if(peffect->is_flag(EFFECT_FLAG_SPSUM_PARAM)) {
			topos = (uint8_t)peffect->s_range;
			if(peffect->o_range == 0)
				toplayer = playerid;
			else
				toplayer = 1 - playerid;
		} else {
			topos = POS_FACEUP;
			toplayer = playerid;
		}
		if(peffect->is_available() && peffect->check_count_limit(playerid) && is_spsummonable(peffect, info)
				&& ((topos & POS_FACEDOWN) || !pduel->game_field->check_unique_onfield(this, toplayer, LOCATION_MZONE))) {
			effect* sumeffect = pduel->game_field->core.reason_effect;
			if(!sumeffect)
				sumeffect = peffect;
			uint32_t sumtype = peffect->get_value(this);
			if((!summon_type || summon_type == sumtype)
			        && pduel->game_field->is_player_can_spsummon(sumeffect, sumtype, topos, playerid, toplayer, this))
				peset->push_back(peffect);
		}
	}
}
void card::filter_spsummon_procedure_g(uint8_t playerid, effect_set* peset) {
	effect_collection proc_set;
	filter_effect_container(field_effect, EFFECT_SPSUMMON_PROC_G, accept_filter, proc_set);
	for (auto& peffect : proc_set) {
		if(!peffect->is_available() || !peffect->check_count_limit(playerid))
			continue;
		if(current.controler != playerid && !peffect->is_flag(EFFECT_FLAG_BOTH_SIDE))
			continue;
		effect* oreason = pduel->game_field->core.reason_effect;
		uint8_t op = pduel->game_field->core.reason_player;
		pduel->game_field->core.reason_effect = peffect;
		pduel->game_field->core.reason_player = this->current.controler;
		pduel->game_field->save_lp_cost();
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		if(pduel->lua->check_condition(peffect->condition, 2))
			peset->push_back(peffect);
		pduel->game_field->restore_lp_cost();
		pduel->game_field->core.reason_effect = oreason;
		pduel->game_field->core.reason_player = op;
	}
}
effect* card::find_effect(const effect_container& container, uint32_t code, effect_filter f) {
	auto rg = container.equal_range(code);
	for (auto it = rg.first; it != rg.second; ++it) {
		if (f(this, it->second))
			return it->second;
	}
	return nullptr;
}
effect* card::find_effect_with_target(const effect_container& container, uint32_t code, effect_filter_target f, card* target) {
	auto rg = container.equal_range(code);
	for (auto it = rg.first; it != rg.second; ++it) {
		if (f(this, it->second, target))
			return it->second;
	}
	return nullptr;
}
// find an effect with code which affects this
effect* card::is_affected_by_effect(uint32_t code) {
	effect* peffect = find_effect(single_effect, code, default_single_filter);
	if (peffect)
		return peffect;
	for (auto& pcard : equiping_cards) {
		peffect = find_effect(pcard->equip_effect, code, default_equip_filter);
		if (peffect)
			return peffect;
	}
	for (auto& pcard : effect_target_owner) {
		peffect = find_effect(pcard->target_effect, code, default_target_filter);
		if (peffect)
			return peffect;
	}
	for (auto& pcard : xyz_materials) {
		peffect = find_effect(pcard->xmaterial_effect, code, default_xmaterial_filter);
		if (peffect)
			return peffect;
	}
	peffect = find_effect(pduel->game_field->effects.aura_effect, code, default_aura_filter);
	if (peffect)
		return peffect;
	return nullptr;
}
effect* card::is_affected_by_effect(int32_t code, card* target) {
	auto single_filter = [](card* c, effect* peffect, card* target) -> bool {
		return default_single_filter(c, peffect) && peffect->get_value(target);
	};
	effect* peffect = find_effect_with_target(single_effect, code, single_filter, target);
	if (peffect)
		return peffect;
	auto equip_filter = [](card* c, effect* peffect, card* target) -> bool {
		return default_equip_filter(c, peffect) && peffect->get_value(target);
	};
	for (auto& pcard : equiping_cards) {
		peffect = find_effect_with_target(pcard->equip_effect, code, equip_filter, target);
		if (peffect)
			return peffect;
	}
	auto target_filter = [](card* c, effect* peffect, card* target) -> bool {
		return default_target_filter(c, peffect) && peffect->get_value(target);
	};
	for (auto& pcard : effect_target_owner) {
		peffect = find_effect_with_target(pcard->target_effect, code, target_filter, target);
		if (peffect)
			return peffect;
	}
	auto xmaterial_filter = [](card* c, effect* peffect, card* target) -> bool {
		return default_xmaterial_filter(c, peffect) && peffect->get_value(target);
	};
	for (auto& pcard : xyz_materials) {
		peffect = find_effect_with_target(pcard->xmaterial_effect, code, xmaterial_filter, target);
		if (peffect)
			return peffect;
	}
	auto aura_filter = [](card* c, effect* peffect, card* target) -> bool {
		return default_aura_filter(c, peffect) && peffect->get_value(target);
	};
	peffect = find_effect_with_target(pduel->game_field->effects.aura_effect, code, aura_filter, target);
	if (peffect)
		return peffect;
	return nullptr;
}
int32_t card::fusion_check(group* fusion_m, card* cg, uint32_t chkf, uint8_t not_material) {
	group* matgroup = nullptr;
	if(fusion_m && !not_material) {
		matgroup = pduel->new_group(fusion_m->container);
		uint32_t summon_type = SUMMON_TYPE_FUSION;
		if((chkf & 0x200) > 0)
			summon_type = SUMMON_TYPE_SPECIAL;
		effect_set eset;
		filter_effect(EFFECT_MATERIAL_LIMIT, &eset);
		for(auto cit = matgroup->container.begin(); cit != matgroup->container.end();) {
			card* pcard = *cit;
			bool is_erase = false;
			for(effect_set::size_type i = 0; i < eset.size(); ++i) {
				pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
				pduel->lua->add_param(this, PARAM_TYPE_CARD);
				pduel->lua->add_param(summon_type, PARAM_TYPE_INT);
				if (!eset[i]->check_value_condition(3)) {
					is_erase = true;
					break;
				}
			}
			if (is_erase)
				cit = matgroup->container.erase(cit);
			else
				++cit;
		}
	} else if(fusion_m) {
		matgroup = pduel->new_group(fusion_m->container);
	}
	auto ecit = single_effect.find(EFFECT_FUSION_MATERIAL);
	if(ecit == single_effect.end())
		return FALSE;
	effect* peffect = ecit->second;
	if(!peffect->condition)
		return FALSE;
	pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
	pduel->lua->add_param(matgroup, PARAM_TYPE_GROUP);
	pduel->lua->add_param(cg, PARAM_TYPE_CARD);
	pduel->lua->add_param(chkf, PARAM_TYPE_INT);
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = peffect;
	pduel->game_field->core.reason_player = peffect->get_handler_player();
	pduel->game_field->core.not_material = not_material;
	int32_t res = pduel->lua->check_condition(peffect->condition, 4);
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	pduel->game_field->core.not_material = 0;
	return res;
}
void card::fusion_select(uint8_t playerid, group* fusion_m, card* cg, uint32_t chkf, uint8_t not_material) {
	group* matgroup = nullptr;
	if(fusion_m && !not_material) {
		matgroup = pduel->new_group(fusion_m->container);
		uint32_t summon_type = SUMMON_TYPE_FUSION;
		if((chkf & 0x200) > 0)
			summon_type = SUMMON_TYPE_SPECIAL;
		effect_set eset;
		filter_effect(EFFECT_MATERIAL_LIMIT, &eset);
		for(auto cit = matgroup->container.begin(); cit != matgroup->container.end();) {
			card* pcard = *cit;
			bool is_erase = false;
			for(effect_set::size_type i = 0; i < eset.size(); ++i) {
				pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
				pduel->lua->add_param(this, PARAM_TYPE_CARD);
				pduel->lua->add_param(summon_type, PARAM_TYPE_INT);
				if (!eset[i]->check_value_condition(3)) {
					is_erase = true;
					break;
				}
			}
			if (is_erase)
				cit = matgroup->container.erase(cit);
			else
				++cit;
		}
	} else if(fusion_m) {
		matgroup = pduel->new_group(fusion_m->container);
	}
	effect* peffect = nullptr;
	auto ecit = single_effect.find(EFFECT_FUSION_MATERIAL);
	if(ecit != single_effect.end())
		peffect = ecit->second;
	pduel->game_field->add_process(PROCESSOR_SELECT_FUSION, 0, peffect, matgroup, playerid + (chkf << 16), not_material, 0, 0, cg);
}
int32_t card::check_fusion_substitute(card* fcard) {
	effect_set eset;
	filter_effect(EFFECT_FUSION_SUBSTITUTE, &eset);
	if(eset.size() == 0)
		return FALSE;
	for(effect_set::size_type i = 0; i < eset.size(); ++i)
		if(!eset[i]->value || eset[i]->get_value(fcard))
			return TRUE;
	return FALSE;
}
int32_t card::is_not_tuner(card* scard) {
	if(!(get_synchro_type() & TYPE_TUNER))
		return TRUE;
	effect_set eset;
	filter_effect(EFFECT_NONTUNER, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i)
		if(!eset[i]->value || eset[i]->get_value(scard))
			return TRUE;
	return FALSE;
}
int32_t card::is_tuner(card* scard) {
	if (get_synchro_type() & TYPE_TUNER)
		return TRUE;
	effect_set eset;
	filter_effect(EFFECT_TUNER, &eset);
	for (effect_set::size_type i = 0; i < eset.size(); ++i)
		if (!eset[i]->value || eset[i]->get_value(scard))
			return TRUE;
	return FALSE;
}
int32_t card::check_unique_code(card* pcard) {
	if(!unique_code)
		return FALSE;
	if(unique_code == 1) {
		pduel->lua->add_param(pcard, PARAM_TYPE_CARD);
		return pduel->lua->get_function_value(unique_function, 1);
	}
	uint32_t code1 = pcard->get_code();
	uint32_t code2 = pcard->get_another_code();
	if(code1 == unique_code || (code2 && code2 == unique_code))
		return TRUE;
	return FALSE;
}
void card::get_unique_target(card_set* cset, int32_t controler, card* icard) {
	cset->clear();
	for(int32_t p = 0; p < 2; ++p) {
		if(!unique_pos[p])
			continue;
		const auto& player = pduel->game_field->player[controler ^ p];
		if(unique_location & LOCATION_MZONE) {
			for(auto& pcard : player.list_mzone) {
				if(pcard && (pcard != icard) && pcard->is_position(POS_FACEUP) && !pcard->get_status(STATUS_SPSUMMON_STEP)
					&& check_unique_code(pcard))
					cset->insert(pcard);
			}
		}
		if(unique_location & LOCATION_SZONE) {
			for(auto& pcard : player.list_szone) {
				if(pcard && (pcard != icard) && pcard->is_position(POS_FACEUP) && check_unique_code(pcard))
					cset->insert(pcard);
			}
		}
	}
}
int32_t card::check_cost_condition(int32_t ecode, int32_t playerid) {
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, ecode, &eset, FALSE);
	filter_effect(ecode, &eset);
	int32_t res = TRUE;
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		effect* peffect = eset[i];
		pduel->game_field->core.reason_effect = peffect;
		pduel->game_field->core.reason_player = playerid;
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(!pduel->lua->check_condition(peffect->cost, 3)) {
			res = FALSE;
			break;
		}
	}
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	return res;
}
int32_t card::check_cost_condition(int32_t ecode, int32_t playerid, int32_t sumtype) {
	effect_set eset;
	pduel->game_field->filter_player_effect(playerid, ecode, &eset, FALSE);
	filter_effect(ecode, &eset);
	int32_t res = TRUE;
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		effect* peffect = eset[i];
		pduel->game_field->core.reason_effect = peffect;
		pduel->game_field->core.reason_player = playerid;
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
		if(!pduel->lua->check_condition(peffect->cost, 4)) {
			res = FALSE;
			break;
		}
	}
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	return res;
}
int32_t card::is_summonable_card() const {
	if(!(data.type & TYPE_MONSTER))
		return FALSE;
	if (data.type & TYPES_EXTRA_DECK)
		return FALSE;
	return !(data.type & (TYPE_RITUAL | TYPE_SPSUMMON | TYPE_TOKEN));
}
int32_t card::is_spsummonable_card() {
	if(!(data.type & TYPE_MONSTER))
		return FALSE;
	if(is_affected_by_effect(EFFECT_REVIVE_LIMIT) && !is_status(STATUS_PROC_COMPLETE)) {
		if(current.location & (LOCATION_GRAVE | LOCATION_REMOVED | LOCATION_SZONE))
			return FALSE;
		if((data.type & TYPE_PENDULUM) && current.location == LOCATION_EXTRA && (current.position & POS_FACEUP))
			return FALSE;
	}
	effect_set eset;
	filter_effect(EFFECT_SPSUMMON_CONDITION, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(pduel->game_field->core.reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pduel->game_field->core.reason_player, PARAM_TYPE_INT);
		pduel->lua->add_param(SUMMON_TYPE_SPECIAL, PARAM_TYPE_INT);
		pduel->lua->add_param(0, PARAM_TYPE_INT);
		pduel->lua->add_param(0, PARAM_TYPE_INT);
		if(!eset[i]->check_value_condition(5))
			return FALSE;
	}
	return TRUE;
}
int32_t card::is_fusion_summonable_card(uint32_t summon_type) {
	if(!(data.type & TYPE_FUSION))
		return FALSE;
	if((data.type & TYPE_PENDULUM) && current.location == LOCATION_EXTRA && (current.position & POS_FACEUP))
		return FALSE;
	summon_type |= SUMMON_TYPE_FUSION;
	effect_set eset;
	filter_effect(EFFECT_SPSUMMON_CONDITION, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(pduel->game_field->core.reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(pduel->game_field->core.reason_player, PARAM_TYPE_INT);
		pduel->lua->add_param(summon_type, PARAM_TYPE_INT);
		pduel->lua->add_param(0, PARAM_TYPE_INT);
		pduel->lua->add_param(0, PARAM_TYPE_INT);
		if(!eset[i]->check_value_condition(5))
			return FALSE;
	}
	return TRUE;
}
// check the condition function of proc
int32_t card::is_spsummonable(effect* proc, material_info info) {
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = proc;
	pduel->game_field->core.reason_player = this->current.controler;
	pduel->game_field->save_lp_cost();
	pduel->lua->add_param(proc, PARAM_TYPE_EFFECT);
	pduel->lua->add_param(this, PARAM_TYPE_CARD);
	uint32_t result = FALSE;
	if(info.limit_tuner || info.limit_syn) {
		pduel->lua->add_param(info.limit_tuner, PARAM_TYPE_CARD);
		pduel->lua->add_param(info.limit_syn, PARAM_TYPE_GROUP);
		uint32_t param_count = 4;
		if(info.limit_syn_minc) {
			pduel->lua->add_param(info.limit_syn_minc, PARAM_TYPE_INT);
			pduel->lua->add_param(info.limit_syn_maxc, PARAM_TYPE_INT);
			param_count = 6;
		}
		if(pduel->lua->check_condition(proc->condition, param_count))
			result = TRUE;
	} else if(info.limit_xyz) {
		pduel->lua->add_param(info.limit_xyz, PARAM_TYPE_GROUP);
		uint32_t param_count = 3;
		if(info.limit_xyz_minc) {
			pduel->lua->add_param(info.limit_xyz_minc, PARAM_TYPE_INT);
			pduel->lua->add_param(info.limit_xyz_maxc, PARAM_TYPE_INT);
			param_count = 5;
		}
		if(pduel->lua->check_condition(proc->condition, param_count))
			result = TRUE;
	} else if(info.limit_link || info.limit_link_card) {
		pduel->lua->add_param(info.limit_link, PARAM_TYPE_GROUP);
		pduel->lua->add_param(info.limit_link_card, PARAM_TYPE_CARD);
		uint32_t param_count = 4;
		if(info.limit_link_minc) {
			pduel->lua->add_param(info.limit_link_minc, PARAM_TYPE_INT);
			pduel->lua->add_param(info.limit_link_maxc, PARAM_TYPE_INT);
			param_count = 6;
		}
		if(pduel->lua->check_condition(proc->condition, param_count))
			result = TRUE;
	} else {
		if(pduel->lua->check_condition(proc->condition, 2))
			result = TRUE;
	}
	pduel->game_field->restore_lp_cost();
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	return result;
}
int32_t card::is_summonable(effect* proc, uint8_t min_tribute, uint32_t zone, uint32_t releasable) {
	effect* oreason = pduel->game_field->core.reason_effect;
	uint8_t op = pduel->game_field->core.reason_player;
	pduel->game_field->core.reason_effect = proc;
	pduel->game_field->core.reason_player = this->current.controler;
	pduel->game_field->save_lp_cost();
	uint32_t result = FALSE;
	pduel->lua->add_param(proc, PARAM_TYPE_EFFECT);
	pduel->lua->add_param(this, PARAM_TYPE_CARD);
	pduel->lua->add_param(min_tribute, PARAM_TYPE_INT);
	pduel->lua->add_param(zone, PARAM_TYPE_INT);
	pduel->lua->add_param(releasable, PARAM_TYPE_INT);
	pduel->game_field->core.limit_extra_summon_zone = zone;
	pduel->game_field->core.limit_extra_summon_releasable = releasable;
	if(pduel->lua->check_condition(proc->condition, 5))
		result = TRUE;
	pduel->game_field->core.limit_extra_summon_zone = 0;
	pduel->game_field->core.limit_extra_summon_releasable = 0;
	pduel->game_field->restore_lp_cost();
	pduel->game_field->core.reason_effect = oreason;
	pduel->game_field->core.reason_player = op;
	return result;
}
int32_t card::is_can_be_summoned(uint8_t playerid, uint8_t ignore_count, effect* peffect, uint8_t min_tribute, uint32_t zone) {
	if(!is_summonable_card())
		return FALSE;
	if(!ignore_count && (pduel->game_field->core.extra_summon[playerid] || !is_affected_by_effect(EFFECT_EXTRA_SUMMON_COUNT))
	        && pduel->game_field->core.summon_count[playerid] >= pduel->game_field->get_summon_count_limit(playerid))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_SUMMON_COST, playerid)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	if(current.location == LOCATION_MZONE) {
		if(is_position(POS_FACEDOWN)
		        || !is_affected_by_effect(EFFECT_DUAL_SUMMONABLE)
		        || is_affected_by_effect(EFFECT_DUAL_STATUS)
		        || !pduel->game_field->is_player_can_summon(SUMMON_TYPE_DUAL, playerid, this, playerid)
		        || is_affected_by_effect(EFFECT_CANNOT_SUMMON)) {
			pduel->game_field->restore_lp_cost();
			return FALSE;
		}
	} else if(current.location == LOCATION_HAND) {
		if(is_affected_by_effect(EFFECT_CANNOT_SUMMON)) {
			pduel->game_field->restore_lp_cost();
			return FALSE;
		}
		effect_set proc;
		int32_t res = filter_summon_procedure(playerid, &proc, ignore_count, min_tribute, zone);
		if(peffect) {
			if(res < 0 || !check_summon_procedure(peffect, playerid, ignore_count, min_tribute, zone)) {
				pduel->game_field->restore_lp_cost();
				return FALSE;
			}
		} else {
			if(!proc.size() && (!res || res == -2)) {
				pduel->game_field->restore_lp_cost();
				return FALSE;
			}
		}
	}
	pduel->game_field->restore_lp_cost();
	return TRUE;
}
int32_t card::is_summon_negatable(uint32_t sumtype, effect* reason_effect) {
	uint32_t code = 0;
	if (sumtype & SUMMON_TYPE_NORMAL)
		code = EFFECT_CANNOT_DISABLE_SUMMON;
	else if (sumtype & SUMMON_TYPE_FLIP)
		code = EFFECT_CANNOT_DISABLE_FLIP_SUMMON;
	else if (sumtype & SUMMON_TYPE_SPECIAL)
		code = EFFECT_CANNOT_DISABLE_SPSUMMON;
	else
		return FALSE;
	if (is_affected_by_effect(code))
		return FALSE;
	if (sumtype == SUMMON_TYPE_DUAL || sumtype & SUMMON_TYPE_FLIP) {
		if (!is_status(STATUS_FLIP_SUMMONING))
			return FALSE;
		if (!is_affect_by_effect(reason_effect))
			return FALSE;
		if (sumtype == SUMMON_TYPE_DUAL && (!is_affected_by_effect(EFFECT_DUAL_SUMMONABLE) || is_affected_by_effect(EFFECT_DUAL_STATUS)))
			return FALSE;
	}
	return TRUE;
}
int32_t card::get_summon_tribute_count() {
	int32_t min = 0, max = 0;
	int32_t level = get_level();
	if(level < 5)
		return 0;
	else if(level < 7)
		min = max = 1;
	else
		min = max = 2;
	std::vector<int32_t> duplicate;
	effect_set eset;
	filter_effect(EFFECT_DECREASE_TRIBUTE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT) && eset[i]->count_limit == 0)
			continue;
		std::vector<lua_Integer> retval;
		eset[i]->get_value(this, 0, retval);
		int32_t dec = retval.size() > 0 ? static_cast<int32_t>(retval[0]) : 0;
		int32_t effect_code = retval.size() > 1 ? static_cast<int32_t>(retval[1]) : 0;
		if(effect_code > 0) {
			auto it = std::find(duplicate.begin(), duplicate.end(), effect_code);
			if(it == duplicate.end())
				duplicate.push_back(effect_code);
			else
				continue;
		}
		min -= dec & 0xffff;
		max -= dec >> 16;
	}
	if(min < 0) min = 0;
	if(max < min) max = min;
	return min + (max << 16);
}
int32_t card::get_set_tribute_count() {
	int32_t min = 0, max = 0;
	int32_t level = get_level();
	if(level < 5)
		return 0;
	else if(level < 7)
		min = max = 1;
	else
		min = max = 2;
	std::vector<int32_t> duplicate;
	effect_set eset;
	filter_effect(EFFECT_DECREASE_TRIBUTE_SET, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT) && eset[i]->count_limit == 0)
			continue;
		std::vector<lua_Integer> retval;
		eset[i]->get_value(this, 0, retval);
		int32_t dec = retval.size() > 0 ? static_cast<int32_t>(retval[0]) : 0;
		int32_t effect_code = retval.size() > 1 ? static_cast<int32_t>(retval[1]) : 0;
		if(effect_code > 0) {
			auto it = std::find(duplicate.begin(), duplicate.end(), effect_code);
			if(it == duplicate.end())
				duplicate.push_back(effect_code);
			else
				continue;
		}
		min -= dec & 0xffff;
		max -= dec >> 16;
	}
	if(min < 0) min = 0;
	if(max < min) max = min;
	return min + (max << 16);
}
int32_t card::is_can_be_flip_summoned(uint8_t playerid) {
	if(is_status(STATUS_CANNOT_CHANGE_FORM))
		return FALSE;
	if(current.location != LOCATION_MZONE)
		return FALSE;
	if(!(current.position & POS_FACEDOWN))
		return FALSE;
	if(pduel->game_field->check_unique_onfield(this, playerid, LOCATION_MZONE))
		return FALSE;
	if(!pduel->game_field->is_player_can_flipsummon(playerid, this))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_FLIP_SUMMON))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_CHANGE_POSITION))
		return FALSE;
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_FLIPSUMMON_COST, playerid)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	pduel->game_field->restore_lp_cost();
	return TRUE;
}
// check if this can be sp_summoned by EFFECT_SPSUMMON_PROC
// call filter_spsummon_procedure()
int32_t card::is_special_summonable(uint8_t playerid, uint32_t summon_type, material_info info) {
	if(!(data.type & TYPE_MONSTER))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_SPECIAL_SUMMON))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_SPSUMMON_COST, playerid, summon_type)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	effect_set eset;
	filter_spsummon_procedure(playerid, &eset, summon_type, info);
	pduel->game_field->restore_lp_cost();
	return !!eset.size();
}
int32_t card::is_can_be_special_summoned(effect* reason_effect, uint32_t sumtype, uint8_t sumpos, uint8_t sumplayer, uint8_t toplayer, uint8_t nocheck, uint8_t nolimit, uint32_t zone) {
	if(reason_effect->get_handler() == this)
		reason_effect->status |= EFFECT_STATUS_SPSELF;
	if(current.location == LOCATION_MZONE)
		return FALSE;
	if(current.location == LOCATION_REMOVED && (current.position & POS_FACEDOWN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_REVIVE_LIMIT) && !is_status(STATUS_PROC_COMPLETE)) {
		if((!nolimit && (current.location & (LOCATION_GRAVE | LOCATION_REMOVED | LOCATION_SZONE)))
			|| (!nocheck && !nolimit && (current.location & (LOCATION_DECK | LOCATION_HAND))))
			return FALSE;
		if(!nolimit && (data.type & TYPE_PENDULUM) && current.location == LOCATION_EXTRA && (current.position & POS_FACEUP))
			return FALSE;
	}
	if((data.type & TYPE_PENDULUM) && current.location == LOCATION_EXTRA && (current.position & POS_FACEUP)
		&& (sumtype == SUMMON_TYPE_FUSION || sumtype == SUMMON_TYPE_SYNCHRO || sumtype == SUMMON_TYPE_XYZ))
		return FALSE;
	if((sumpos & POS_FACEDOWN) && pduel->game_field->is_player_affected_by_effect(sumplayer, EFFECT_DIVINE_LIGHT))
		sumpos = (sumpos & POS_FACEUP) | ((sumpos & POS_FACEDOWN) >> 1);
	if(!(sumpos & POS_FACEDOWN) && pduel->game_field->check_unique_onfield(this, toplayer, LOCATION_MZONE))
		return FALSE;
	sumtype |= SUMMON_TYPE_SPECIAL;
	if((sumplayer == 0 || sumplayer == 1) && !pduel->game_field->is_player_can_spsummon(reason_effect, sumtype, sumpos, sumplayer, toplayer, this))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_SPECIAL_SUMMON))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(zone != 0xff) {
		if(pduel->game_field->get_useable_count(this, toplayer, LOCATION_MZONE, sumplayer, LOCATION_REASON_TOFIELD, zone, 0) <= 0)
			return FALSE;
	}
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_SPSUMMON_COST, sumplayer, sumtype)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	if(!nocheck) {
		effect_set eset;
		if(!(data.type & TYPE_MONSTER)) {
			pduel->game_field->restore_lp_cost();
			return FALSE;
		}
		filter_effect(EFFECT_SPSUMMON_CONDITION, &eset);
		for(effect_set::size_type i = 0; i < eset.size(); ++i) {
			pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(sumplayer, PARAM_TYPE_INT);
			pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
			pduel->lua->add_param(sumpos, PARAM_TYPE_INT);
			pduel->lua->add_param(toplayer, PARAM_TYPE_INT);
			if(!eset[i]->check_value_condition(5)) {
				pduel->game_field->restore_lp_cost();
				return FALSE;
			}
		}
	}
	pduel->game_field->restore_lp_cost();
	return TRUE;
}
uint8_t card::get_spsummonable_position(effect* reason_effect, uint32_t sumtype, uint8_t sumpos, uint8_t sumplayer, uint8_t toplayer) {
	uint8_t position = 0;
	uint8_t positions[4] = { POS_FACEUP_ATTACK, POS_FACEDOWN_ATTACK, POS_FACEUP_DEFENSE, POS_FACEDOWN_DEFENSE };
	effect_set eset;
	for(int32_t p = 0; p < 4; ++p) {
		bool poscheck = true;
		if(!(positions[p] & sumpos))
			continue;
		if((data.type & (TYPE_TOKEN | TYPE_LINK)) && (positions[p] & POS_FACEDOWN))
			continue;
		pduel->game_field->filter_player_effect(sumplayer, EFFECT_LIMIT_SPECIAL_SUMMON_POSITION, &eset);
		for(effect_set::size_type i = 0; i < eset.size(); ++i) {
			if(!eset[i]->target)
				continue;
			pduel->lua->add_param(eset[i], PARAM_TYPE_EFFECT);
			pduel->lua->add_param(this, PARAM_TYPE_CARD);
			pduel->lua->add_param(sumplayer, PARAM_TYPE_INT);
			pduel->lua->add_param(sumtype, PARAM_TYPE_INT);
			pduel->lua->add_param(positions[p], PARAM_TYPE_INT);
			pduel->lua->add_param(toplayer, PARAM_TYPE_INT);
			pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
			if(pduel->lua->check_condition(eset[i]->target, 7))
				poscheck = false;
		}
		eset.clear();
		if(poscheck)
			position |= positions[p];
	}
	return position;
}
int32_t card::is_setable_mzone(uint8_t playerid, uint8_t ignore_count, effect* peffect, uint8_t min_tribute, uint32_t zone) {
	if(!is_summonable_card())
		return FALSE;
	if(current.location != LOCATION_HAND)
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_MSET))
		return FALSE;
	if(!ignore_count && (pduel->game_field->core.extra_summon[playerid] || !is_affected_by_effect(EFFECT_EXTRA_SET_COUNT))
	        && pduel->game_field->core.summon_count[playerid] >= pduel->game_field->get_summon_count_limit(playerid))
		return FALSE;
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_MSET_COST, playerid)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	effect_set eset;
	int32_t res = filter_set_procedure(playerid, &eset, ignore_count, min_tribute, zone);
	if(peffect) {
		if(res < 0 || !check_set_procedure(peffect, playerid, ignore_count, min_tribute, zone)) {
			pduel->game_field->restore_lp_cost();
			return FALSE;
		}
	} else {
		if(!eset.size() && (!res || res == -2)) {
			pduel->game_field->restore_lp_cost();
			return FALSE;
		}
	}
	pduel->game_field->restore_lp_cost();
	return TRUE;
}
int32_t card::is_setable_szone(uint8_t playerid, uint8_t ignore_fd) {
	if(!(data.type & TYPE_FIELD) && !ignore_fd && pduel->game_field->get_useable_count(this, current.controler, LOCATION_SZONE, current.controler, LOCATION_REASON_TOFIELD) <= 0)
		return FALSE;
	if(data.type & TYPE_MONSTER && !is_affected_by_effect(EFFECT_MONSTER_SSET))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_SSET))
		return FALSE;
	if(!pduel->game_field->is_player_can_sset(playerid, this))
		return FALSE;
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_SSET_COST, playerid)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	pduel->game_field->restore_lp_cost();
	return TRUE;
}
int32_t card::is_affect_by_effect(effect* reason_effect) {
	if (is_status(STATUS_SUMMONING))
		return reason_effect && affect_summoning_effect.find(reason_effect->code) != affect_summoning_effect.end();
	if(!reason_effect || reason_effect->is_flag(EFFECT_FLAG_IGNORE_IMMUNE))
		return TRUE;
	if(reason_effect->is_immuned(this))
		return FALSE;
	return TRUE;
}
int32_t card::is_can_be_disabled_by_effect(effect* reason_effect, bool is_monster_effect) {
	if (is_monster_effect && is_status(STATUS_DISABLED))
		return FALSE;
	if(!is_monster_effect && !(get_type() & TYPE_TRAPMONSTER) && is_status(STATUS_DISABLED))
		return FALSE;
	if (is_affected_by_effect(EFFECT_CANNOT_DISABLE))
		return FALSE;
	if (!is_affect_by_effect(reason_effect))
		return FALSE;
	return TRUE;
}
int32_t card::is_destructable() {
	if(overlay_target)
		return FALSE;
	if(current.location & (LOCATION_GRAVE | LOCATION_REMOVED))
		return FALSE;
	return TRUE;
}
int32_t card::is_destructable_by_battle(card * pcard) {
	if(is_affected_by_effect(EFFECT_INDESTRUCTABLE_BATTLE, pcard))
		return FALSE;
	return TRUE;
}
effect* card::check_indestructable_by_effect(effect* reason_effect, uint8_t playerid) {
	if(!reason_effect)
		return nullptr;
	effect_set eset;
	filter_effect(EFFECT_INDESTRUCTABLE_EFFECT, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		if(eset[i]->check_value_condition(3))
			return eset[i];
	}
	return nullptr;
}
int32_t card::is_destructable_by_effect(effect* reason_effect, uint8_t playerid) {
	if(!is_affect_by_effect(reason_effect))
		return FALSE;
	if(check_indestructable_by_effect(reason_effect, playerid))
		return FALSE;
	effect_set eset;
	eset.clear();
	filter_effect(EFFECT_INDESTRUCTABLE, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(REASON_EFFECT, PARAM_TYPE_INT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(eset[i]->check_value_condition(3)) {
			return FALSE;
			break;
		}
	}
	eset.clear();
	filter_effect(EFFECT_INDESTRUCTABLE_COUNT, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		if(eset[i]->is_flag(EFFECT_FLAG_COUNT_LIMIT)) {
			if(eset[i]->count_limit == 0)
				continue;
			pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(REASON_EFFECT, PARAM_TYPE_INT);
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			if(eset[i]->check_value_condition(3)) {
				return FALSE;
				break;
			}
		} else {
			pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
			pduel->lua->add_param(REASON_EFFECT, PARAM_TYPE_INT);
			pduel->lua->add_param(playerid, PARAM_TYPE_INT);
			int32_t ct = eset[i]->get_value(3);
			if(ct) {
				auto it = indestructable_effects.emplace(eset[i]->id, 0);
				if(it.first->second + 1 <= ct) {
					return FALSE;
					break;
				}
			}
		}
	}
	return TRUE;
}
int32_t card::is_removeable(uint8_t playerid, uint8_t pos, uint32_t reason) {
	if(!pduel->game_field->is_player_can_remove(playerid, this, reason))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_REMOVE))
		return FALSE;
	if((data.type & TYPE_TOKEN) && (pos & POS_FACEDOWN))
		return FALSE;
	return TRUE;
}
int32_t card::is_removeable_as_cost(uint8_t playerid, uint8_t pos) {
	uint32_t redirect = 0;
	uint32_t dest = LOCATION_REMOVED;
	if(current.location == LOCATION_REMOVED)
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
		return FALSE;
	if(!is_removeable(playerid, pos, REASON_COST))
		return FALSE;
	int32_t redirchk = FALSE;
	auto op_param = sendto_param;
	sendto_param.location = dest;
	if(current.location & LOCATION_ONFIELD)
		redirect = leave_field_redirect(REASON_COST) & 0xffff;
	if(redirect) {
		redirchk = TRUE;
		dest = redirect;
	}
	redirect = destination_redirect(dest, REASON_COST) & 0xffff;
	if(redirect) {
		redirchk = TRUE;
		dest = redirect;
	}
	sendto_param = op_param;
	if(dest != LOCATION_REMOVED || (redirchk && (pos & POS_FACEDOWN)))
		return FALSE;
	return TRUE;
}
int32_t card::is_releasable_by_summon(uint8_t playerid, card *pcard) {
	if(is_status(STATUS_SUMMONING))
		return FALSE;
	if(overlay_target)
		return FALSE;
	if(current.location & (LOCATION_GRAVE | LOCATION_REMOVED))
		return FALSE;
	if(!pduel->game_field->is_player_can_release(playerid, this, REASON_SUMMON))
		return FALSE;
	if(is_affected_by_effect(EFFECT_UNRELEASABLE_SUM, pcard))
		return FALSE;
	if(pcard->is_affected_by_effect(EFFECT_TRIBUTE_LIMIT, this))
		return FALSE;
	return TRUE;
}
int32_t card::is_releasable_by_nonsummon(uint8_t playerid, uint32_t reason) {
	if(is_status(STATUS_SUMMONING))
		return FALSE;
	if(overlay_target)
		return FALSE;
	if(current.location & (LOCATION_GRAVE | LOCATION_REMOVED))
		return FALSE;
	if((current.location == LOCATION_HAND) && (data.type & (TYPE_SPELL | TYPE_TRAP)))
		return FALSE;
	if(!pduel->game_field->is_player_can_release(playerid, this, reason))
		return FALSE;
	if(is_affected_by_effect(EFFECT_UNRELEASABLE_NONSUM))
		return FALSE;
	return TRUE;
}
int32_t card::is_releasable_by_effect(uint8_t playerid, effect* reason_effect) {
	if(!reason_effect)
		return TRUE;
	effect_set eset;
	filter_effect(EFFECT_UNRELEASABLE_EFFECT, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(reason_effect, PARAM_TYPE_EFFECT);
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		if(eset[i]->check_value_condition(3))
			return FALSE;
	}
	return TRUE;
}
int32_t card::is_capable_send_to_grave(uint8_t playerid) {
	if(is_affected_by_effect(EFFECT_CANNOT_TO_GRAVE))
		return FALSE;
	if(!pduel->game_field->is_player_can_send_to_grave(playerid, this))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_send_to_hand(uint8_t playerid) {
	if(is_status(STATUS_LEAVE_CONFIRMED))
		return FALSE;
	if((current.location == LOCATION_EXTRA) && is_extra_deck_monster())
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_TO_HAND))
		return FALSE;
	if(is_extra_deck_monster() && !is_capable_send_to_deck(playerid))
		return FALSE;
	if(!pduel->game_field->is_player_can_send_to_hand(playerid, this))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_send_to_deck(uint8_t playerid, uint8_t send_activating) {
	if(!send_activating && is_status(STATUS_LEAVE_CONFIRMED))
		return FALSE;
	if((current.location == LOCATION_EXTRA) && is_extra_deck_monster())
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_TO_DECK))
		return FALSE;
	if(!pduel->game_field->is_player_can_send_to_deck(playerid, this))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_send_to_extra(uint8_t playerid) {
	if(!is_extra_deck_monster() && !(data.type & TYPE_PENDULUM))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_TO_DECK))
		return FALSE;
	if(!pduel->game_field->is_player_can_send_to_deck(playerid, this))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_cost_to_grave(uint8_t playerid) {
	uint32_t redirect = 0;
	uint32_t dest = LOCATION_GRAVE;
	if(data.type & TYPE_TOKEN)
		return FALSE;
	if((data.type & TYPE_PENDULUM) && (current.location & LOCATION_ONFIELD) && is_capable_send_to_extra(playerid))
		return FALSE;
	if(current.location == LOCATION_GRAVE)
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_TO_GRAVE_AS_COST))
		return FALSE;
	if(!is_capable_send_to_grave(playerid))
		return FALSE;
	auto op_param = sendto_param;
	sendto_param.location = dest;
	if(current.location & LOCATION_ONFIELD)
		redirect = leave_field_redirect(REASON_COST) & 0xffff;
	if(redirect)
		dest = redirect;
	redirect = destination_redirect(dest, REASON_COST) & 0xffff;
	if(redirect)
		dest = redirect;
	sendto_param = op_param;
	if(dest != LOCATION_GRAVE)
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_cost_to_hand(uint8_t playerid) {
	uint32_t redirect = 0;
	uint32_t dest = LOCATION_HAND;
	if(data.type & (TYPE_TOKEN) || is_extra_deck_monster())
		return FALSE;
	if(current.location == LOCATION_HAND)
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
		return FALSE;
	if(!is_capable_send_to_hand(playerid))
		return FALSE;
	auto op_param = sendto_param;
	sendto_param.location = dest;
	if(current.location & LOCATION_ONFIELD)
		redirect = leave_field_redirect(REASON_COST) & 0xffff;
	if(redirect)
		dest = redirect;
	redirect = destination_redirect(dest, REASON_COST) & 0xffff;
	if(redirect)
		dest = redirect;
	sendto_param = op_param;
	if(dest != LOCATION_HAND)
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_cost_to_deck(uint8_t playerid) {
	uint32_t redirect = 0;
	uint32_t dest = LOCATION_DECK;
	if(data.type & (TYPE_TOKEN) || is_extra_deck_monster())
		return FALSE;
	if(current.location == LOCATION_DECK)
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
		return FALSE;
	if(!is_capable_send_to_deck(playerid))
		return FALSE;
	auto op_param = sendto_param;
	sendto_param.location = dest;
	if(current.location & LOCATION_ONFIELD)
		redirect = leave_field_redirect(REASON_COST) & 0xffff;
	if(redirect)
		dest = redirect;
	redirect = destination_redirect(dest, REASON_COST) & 0xffff;
	if(redirect)
		dest = redirect;
	sendto_param = op_param;
	if(dest != LOCATION_DECK)
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_cost_to_extra(uint8_t playerid) {
	uint32_t redirect = 0;
	uint32_t dest = LOCATION_DECK;
	if(!is_extra_deck_monster())
		return FALSE;
	if(current.location == LOCATION_EXTRA)
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_USE_AS_COST))
		return FALSE;
	if(!is_capable_send_to_deck(playerid))
		return FALSE;
	auto op_param = sendto_param;
	sendto_param.location = dest;
	if(current.location & LOCATION_ONFIELD)
		redirect = leave_field_redirect(REASON_COST) & 0xffff;
	if(redirect)
		dest = redirect;
	redirect = destination_redirect(dest, REASON_COST) & 0xffff;
	if(redirect)
		dest = redirect;
	sendto_param = op_param;
	if(dest != LOCATION_DECK)
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_attack() {
	if(!is_position(POS_FACEUP_ATTACK) && !(is_position(POS_FACEUP_DEFENSE) && is_affected_by_effect(EFFECT_DEFENSE_ATTACK)))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_ATTACK))
		return FALSE;
	if(is_affected_by_effect(EFFECT_ATTACK_DISABLED))
		return FALSE;
	if(pduel->game_field->is_player_affected_by_effect(pduel->game_field->infos.turn_player, EFFECT_SKIP_BP))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_attack_announce(uint8_t playerid) {
	if(!is_capable_attack())
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_ATTACK_ANNOUNCE))
		return FALSE;
	pduel->game_field->save_lp_cost();
	if(!check_cost_condition(EFFECT_ATTACK_COST, playerid)) {
		pduel->game_field->restore_lp_cost();
		return FALSE;
	}
	pduel->game_field->restore_lp_cost();
	return TRUE;
}
int32_t card::is_capable_change_position(uint8_t playerid) {
	if(is_status(STATUS_CANNOT_CHANGE_FORM))
		return FALSE;
	if(data.type & TYPE_LINK)
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_CHANGE_POSITION))
		return FALSE;
	if(pduel->game_field->is_player_affected_by_effect(playerid, EFFECT_CANNOT_CHANGE_POSITION))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_change_position_by_effect(uint8_t playerid) {
	if(data.type & TYPE_LINK)
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_turn_set(uint8_t playerid) {
	if(data.type & (TYPE_LINK | TYPE_TOKEN))
		return FALSE;
	if(is_position(POS_FACEDOWN))
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_TURN_SET))
		return FALSE;
	if(pduel->game_field->is_player_affected_by_effect(playerid, EFFECT_CANNOT_TURN_SET))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_change_control() {
	if(is_affected_by_effect(EFFECT_CANNOT_CHANGE_CONTROL))
		return FALSE;
	return TRUE;
}
int32_t card::is_control_can_be_changed(int32_t ignore_mzone, uint32_t zone) {
	if(current.controler == PLAYER_NONE)
		return FALSE;
	if(current.location != LOCATION_MZONE)
		return FALSE;
	if(!ignore_mzone && pduel->game_field->get_useable_count(this, 1 - current.controler, LOCATION_MZONE, current.controler, LOCATION_REASON_CONTROL, zone) <= 0)
		return FALSE;
	if(pduel->game_field->core.duel_rule <= 4 && (get_type() & TYPE_TRAPMONSTER)
		&& pduel->game_field->get_useable_count(this, 1 - current.controler, LOCATION_SZONE, current.controler, LOCATION_REASON_CONTROL) <= 0)
		return FALSE;
	if(is_affected_by_effect(EFFECT_CANNOT_CHANGE_CONTROL))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_be_battle_target(card* pcard) {
	if(is_affected_by_effect(EFFECT_CANNOT_BE_BATTLE_TARGET, pcard))
		return FALSE;
	if(pcard->is_affected_by_effect(EFFECT_CANNOT_SELECT_BATTLE_TARGET, this))
		return FALSE;
	if(is_affected_by_effect(EFFECT_IGNORE_BATTLE_TARGET, pcard))
		return FALSE;
	return TRUE;
}
int32_t card::is_capable_be_effect_target(effect* reason_effect, uint8_t playerid) {
	if(is_status(STATUS_SUMMONING) || is_status(STATUS_BATTLE_DESTROYED))
		return FALSE;
	if(current.location & (LOCATION_DECK | LOCATION_EXTRA | LOCATION_HAND))
		return FALSE;
	effect_set eset;
	filter_effect(EFFECT_CANNOT_BE_EFFECT_TARGET, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(playerid, PARAM_TYPE_INT);
		if(eset[i]->get_value(reason_effect, 1))
			return FALSE;
	}
	eset.clear();
	reason_effect->get_handler()->filter_effect(EFFECT_CANNOT_SELECT_EFFECT_TARGET, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(this, PARAM_TYPE_CARD);
		if(eset[i]->get_value(reason_effect, 1))
			return FALSE;
	}
	return TRUE;
}
int32_t card::is_capable_overlay(uint8_t playerid) {
	if(data.type & TYPE_TOKEN)
		return FALSE;
	if(!(current.location & LOCATION_ONFIELD) && is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(current.controler != playerid && !is_capable_change_control())
		return FALSE;
	return TRUE;
}
int32_t card::is_can_be_fusion_material(card* fcard, uint32_t summon_type) {
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	if(pduel->game_field->core.not_material)
		return TRUE;
	effect_set eset;
	filter_effect(EFFECT_CANNOT_BE_FUSION_MATERIAL, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i) {
		pduel->lua->add_param(summon_type, PARAM_TYPE_INT);
		if(eset[i]->get_value(fcard, 1))
			return FALSE;
	}
	eset.clear();
	filter_effect(EFFECT_EXTRA_FUSION_MATERIAL, &eset);
	if(eset.size()) {
		for(effect_set::size_type i = 0; i < eset.size(); ++i)
			if(eset[i]->get_value(fcard))
				return TRUE;
		return FALSE;
	}
	return TRUE;
}
int32_t card::is_can_be_synchro_material(card* scard, card* tuner) {
	if((data.type & (TYPE_XYZ | TYPE_LINK)) && !get_synchro_level(scard))
		return FALSE;
	if(!(get_synchro_type() & TYPE_MONSTER))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	//special fix for scrap chimera, not perfect yet
	if(tuner && (pduel->game_field->core.global_flag & GLOBALFLAG_SCRAP_CHIMERA)) {
		if(is_affected_by_effect(EFFECT_SCRAP_CHIMERA, tuner))
			return false;
	}
	effect_set eset;
	filter_effect(EFFECT_CANNOT_BE_SYNCHRO_MATERIAL, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i)
		if(eset[i]->get_value(scard))
			return FALSE;
	return TRUE;
}
int32_t card::is_can_be_ritual_material(card* scard) {
	if(!(get_type() & TYPE_MONSTER))
		return FALSE;
	if(current.location == LOCATION_GRAVE) {
		effect_set eset;
		filter_effect(EFFECT_EXTRA_RITUAL_MATERIAL, &eset);
		for(effect_set::size_type i = 0; i < eset.size(); ++i)
			if(eset[i]->get_value(scard))
				return TRUE;
		return FALSE;
	}
	return TRUE;
}
int32_t card::is_can_be_xyz_material(card* scard) {
	if(data.type & TYPE_TOKEN)
		return FALSE;
	if(!(get_xyz_type() & TYPE_MONSTER))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	effect_set eset;
	filter_effect(EFFECT_CANNOT_BE_XYZ_MATERIAL, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i)
		if(eset[i]->get_value(scard))
			return FALSE;
	return TRUE;
}
int32_t card::is_can_be_link_material(card* scard) {
	if(!(get_link_type() & TYPE_MONSTER))
		return FALSE;
	if(is_status(STATUS_FORBIDDEN))
		return FALSE;
	effect_set eset;
	filter_effect(EFFECT_CANNOT_BE_LINK_MATERIAL, &eset);
	for(effect_set::size_type i = 0; i < eset.size(); ++i)
		if(eset[i]->get_value(scard))
			return FALSE;
	return TRUE;
}
/**
* @param filter Lua function filter(e)
*/
int32_t card::is_original_effect_property(int32_t filter) {
	for (auto& peffect : initial_effect) {
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		if (pduel->lua->check_condition(filter, 1))
			return TRUE;
	}
	return FALSE;
}
/**
* @param filter Lua function filter(e)
*/
int32_t card::is_effect_property(int32_t filter) {
	for (auto& peffect : initial_effect) {
		if (current.is_location(LOCATION_MZONE) && !peffect->is_monster_effect())
			continue;
		if (current.is_location(LOCATION_SZONE) && !peffect->in_range(this))
			continue;
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		if(pduel->lua->check_condition(filter, 1))
			return TRUE;
	}
	for (auto& peffect : owning_effect) {
		if (current.is_location(LOCATION_MZONE) && !peffect->is_monster_effect())
			continue;
		if (current.is_location(LOCATION_SZONE) && !peffect->in_range(this))
			continue;
		pduel->lua->add_param(peffect, PARAM_TYPE_EFFECT);
		if (pduel->lua->check_condition(filter, 1))
			return TRUE;
	}
	return FALSE;
}
