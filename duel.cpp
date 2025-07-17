/*
 * duel.cpp
 *
 *  Created on: 2010-5-2
 *      Author: Argon
 */

#include <cstring>
#include "duel.h"
#include "interpreter.h"
#include "field.h"
#include "card.h"
#include "effect.h"
#include "group.h"
#include "ocgapi.h"
#include "buffer.h"

duel::duel() {
	lua = new interpreter(this, false);
	game_field = new field(this);
	game_field->temp_card = new_card(TEMP_CARD_ID);
	message_buffer.reserve(SIZE_MESSAGE_BUFFER);
#ifdef _WIN32
	_set_error_mode(_OUT_TO_MSGBOX);
#endif // _WIN32
}
duel::~duel() {
	for(auto& pcard : cards)
		delete pcard;
	for(auto& pgroup : groups)
		delete pgroup;
	for(auto& peffect : effects)
		delete peffect;
	delete lua;
	delete game_field;
}
void duel::clear() {
	for(auto& pcard : cards)
		delete pcard;
	for(auto& pgroup : groups)
		delete pgroup;
	for(auto& peffect : effects)
		delete peffect;
	delete game_field;
	cards.clear();
	groups.clear();
	effects.clear();
	assumes.clear();
	sgroups.clear();
	uncopy.clear();
	game_field = new field(this);
	game_field->temp_card = new_card(TEMP_CARD_ID);
}
card* duel::new_card(uint32_t code) {
	card* pcard = new card(this);
	cards.insert(pcard);
	if (code != TEMP_CARD_ID)
		::read_card(code, &(pcard->data));
	pcard->data.code = code;
	lua->register_card(pcard);
	return pcard;
}
group* duel::register_group(group* pgroup) {
	groups.insert(pgroup);
	if(lua->call_depth)
		sgroups.insert(pgroup);
	lua->register_group(pgroup);
	return pgroup;
}
group* duel::new_group() {
	group* pgroup = new group(this);
	return register_group(pgroup);
}
group* duel::new_group(card* pcard) {
	group* pgroup = new group(this, pcard);
	return register_group(pgroup);
}
group* duel::new_group(const card_set& cset) {
	group* pgroup = new group(this, cset);
	return register_group(pgroup);
}
effect* duel::new_effect() {
	effect* peffect = new effect(this);
	effects.insert(peffect);
	lua->register_effect(peffect);
	return peffect;
}
void duel::delete_card(card* pcard) {
	cards.erase(pcard);
	delete pcard;
}
void duel::delete_group(group* pgroup) {
	lua->unregister_group(pgroup);
	groups.erase(pgroup);
	sgroups.erase(pgroup);
	delete pgroup;
}
void duel::delete_effect(effect* peffect) {
	lua->unregister_effect(peffect);
	effects.erase(peffect);
	delete peffect;
}
int32_t duel::read_buffer(byte* buf) {
	auto size = buffer_size();
	if (size)
		std::memcpy(buf, message_buffer.data(), size);
	return (int32_t)size;
}
void duel::release_script_group() {
	for(auto& pgroup : sgroups) {
		if(pgroup->is_readonly == GTYPE_DEFAULT) {
			lua->unregister_group(pgroup);
			groups.erase(pgroup);
			delete pgroup;
		}
	}
	sgroups.clear();
}
void duel::restore_assumes() {
	for(auto& pcard : assumes)
		pcard->assume_type = 0;
	assumes.clear();
}
void duel::write_buffer(const void* data, int size) {
	vector_write_block(message_buffer, data, size);
}
void duel::write_buffer32(uint32_t value) {
	vector_write<uint32_t>(message_buffer, value);
}
void duel::write_buffer16(uint16_t value) {
	vector_write<uint16_t>(message_buffer, value);
}
void duel::write_buffer8(uint8_t value) {
	vector_write<unsigned char>(message_buffer, value);
}
void duel::clear_buffer() {
	message_buffer.clear();
}
void duel::set_responsei(uint32_t resp) {
	game_field->returns.ivalue[0] = resp;
}
void duel::set_responseb(byte* resp) {
	std::memcpy(game_field->returns.bvalue, resp, SIZE_RETURN_VALUE);
}
int32_t duel::get_next_integer(int32_t l, int32_t h) {
	if (rng_version == 1)
		return random.get_random_integer_v1(l, h);
	return random.get_random_integer_v2(l, h);
}
