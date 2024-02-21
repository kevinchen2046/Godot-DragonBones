#include "dragonbones.h"

#include "dragonBones/animation/WorldClock.h"

#include "godot_cpp/classes/engine.hpp"

#include "wrappers/DragonBonesArmature.h"

using namespace godot;

#define SNAME(sn) ([] {static const StringName ret{sn};return ret; }())

/////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
//// Plugin module

void DragonBones::_cleanup() {
	b_inited = false;

	if (p_armature) {
		if (p_armature->get_parent() == this) {
			remove_child(p_armature);
			p_armature->dispose(true);
		}
		p_armature->queue_free();
		p_armature = nullptr;
	}

	if (p_instance) {
		memdelete(p_instance);
		p_instance = nullptr;
	}

	m_res.unref();
}

void DragonBones::dispatch_sound_event(const String &_str_type, const dragonBones::EventObject *_p_value) {
	using namespace dragonBones;
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	if (_str_type == EventObject::SOUND_EVENT)
		emit_signal("dragon_anim_snd_event", String(_p_value->animationState->name.c_str()), String(_p_value->name.c_str()));
}

void DragonBones::dispatch_event(const String &_str_type, const dragonBones::EventObject *_p_value) {
	using namespace dragonBones;
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (_str_type == EventObject::START)
		emit_signal("dragon_anim_start", String(_p_value->animationState->name.c_str()));
	else if (_str_type == EventObject::LOOP_COMPLETE)
		emit_signal("dragon_anim_loop_complete", String(_p_value->animationState->name.c_str()));
	else if (_str_type == EventObject::COMPLETE)
		emit_signal("dragon_anim_complete", String(_p_value->animationState->name.c_str()));
	else if (_str_type == EventObject::FRAME_EVENT) {
		int int_val = 0;
		int float_val = 0;
		const char *string_val = "";
		UserData *data = _p_value->getData();
		Armature *armature = _p_value->getArmature();

		if (data != NULL) {
			int_val = _p_value->getData()->getInt(0);
			float_val = _p_value->getData()->getFloat(0);

			if (!data->getStrings().empty()) {
				string_val = _p_value->getData()->getString(0).c_str();
			}
		}

		Dictionary dict = Dictionary();

		dict[SNAME("armature")] = String(armature->getName().c_str());
		dict[SNAME("animation")] = String(_p_value->animationState->name.c_str());
		dict[SNAME("event_name")] = String(_p_value->name.c_str());
		dict[SNAME("int")] = int_val;
		dict[SNAME("float")] = float_val;
		dict[SNAME("string")] = string_val;

		emit_signal("dragon_anim_event", dict);
	} else if (_str_type == EventObject::FADE_IN)
		emit_signal("dragon_fade_in", String(_p_value->animationState->name.c_str()));
	else if (_str_type == EventObject::FADE_IN_COMPLETE)
		emit_signal("dragon_fade_in_complete", String(_p_value->animationState->name.c_str()));
	else if (_str_type == EventObject::FADE_OUT)
		emit_signal("dragon_fade_out", String(_p_value->animationState->name.c_str()));
	else if (_str_type == EventObject::FADE_OUT_COMPLETE)
		emit_signal("dragon_fade_out_complete", String(_p_value->animationState->name.c_str()));
}

void DragonBones::_on_resource_changed() {
	// 重设资源本身
	auto to_set = m_res;
	set_factory({});
	set_factory(to_set);
}

void DragonBones::set_factory(const Ref<DragonBonesFactory> &_p_data) {
	using namespace dragonBones;
	if (m_res == _p_data)
		return;

	Ref<Texture2D> old_texture;
	if (m_res.is_valid())
		old_texture = m_res->get_default_texture();
	else if (_p_data.is_valid())
		old_texture = _p_data->get_default_texture();

	if (p_armature) {
		p_armature->stop_all_animations(false, true);
	}

	static const StringName sn{ "changed" };
	auto cb = callable_mp(this, &DragonBones::_on_resource_changed);
	if (m_res.is_valid() && m_res->is_connected(sn, cb)) {
		m_res->disconnect(sn, cb);
	}

	_cleanup();
	m_res = _p_data;

	if (m_res.is_null()) {
		// m_texture_atlas.unref();
		notify_property_list_changed();
		return;
	} else if (!m_res->is_connected(sn, cb)) {
		m_res->connect(sn, cb);
	}

	if (!m_res->can_create_dragon_bones_instance()) {
		WARN_PRINT(vformat("DragonBonesFactory \"%s\" is invalid, please setup its properties.", m_res));
		return;
	}

	// build Armature display
	p_instance = m_res->create_dragon_bones(this, p_armature);

	// add children armature
	p_armature->p_owner = this;

	// To support non-texture atlas; I'd want to look around here
	if (m_texture_atlas.is_null() || old_texture != m_res->get_default_texture())
		m_texture_atlas = m_res->get_default_texture();

	// update flip
	set_flip_x(b_flip_x);
	set_flip_y(b_flip_y);

	p_armature->setup_recursively(b_debug, m_texture_atlas);
	// add main armature
	add_child(p_armature);

	b_inited = true;

	// update color and opacity and blending
	p_armature->update_childs(true, true);

	// update material inheritance
	p_armature->update_material_inheritance(b_inherit_child_material);

	p_armature->getArmature()->advanceTime(0);

	notify_property_list_changed();
	queue_redraw();
}

Ref<DragonBonesFactory> DragonBones::get_factory() const {
	return m_res;
}

void DragonBones::set_inherit_material(bool _b_enable) {
	b_inherit_child_material = _b_enable;
	if (p_armature) {
		p_armature->update_material_inheritance(b_inherit_child_material);
	}
}

bool DragonBones::is_material_inherited() const {
	return b_inherit_child_material;
}

void DragonBones::set_active(bool _b_active) {
	b_active = _b_active;
	if (p_armature) {
		p_armature->set_active(_b_active, true);
	}
}

bool DragonBones::is_active() const {
	return b_active;
}

void DragonBones::set_debug(bool _b_debug) {
	b_debug = _b_debug;
	if (b_inited) {
		p_armature->set_debug(b_debug, true);
	}
}

bool DragonBones::is_debug() const {
	return b_debug;
}

void DragonBones::set_speed_scale(float _f_speed) {
	f_speed = _f_speed;
	if (b_inited) {
		p_instance->getClock()->timeScale = _f_speed;
	}
}

float DragonBones::get_speed_scale() const {
	return f_speed;
}

void DragonBones::set_callback_mode_process(DragonBonesArmature::AnimationCallbackModeProcess _mode) {
	callback_mode_process = _mode;
	if (p_armature) {
		p_armature->set_callback_mode_process(_mode, true);
	}
}

DragonBonesArmature::AnimationCallbackModeProcess DragonBones::get_callback_mode_process() const {
	return callback_mode_process;
}

int DragonBones::get_animation_loop() const {
	return c_loop;
}

void DragonBones::set_animation_loop(int p_animation_loop) {
	c_loop = p_animation_loop;
	if (b_inited && b_playing) {
		_reset();
		p_armature->play(str_curr_anim, c_loop);
	}
}

void DragonBones::_reset() {
	ERR_FAIL_NULL(p_armature);
	p_armature->reset(true);
}

DragonBonesArmature *DragonBones::get_armature() {
	return p_armature;
}

void DragonBones::set_flip_x(bool _b_flip) {
	b_flip_x = _b_flip;
	if (p_armature) {
		p_armature->set_flip_x(_b_flip, true);
	}
}

bool DragonBones::is_fliped_x() const {
	return b_flip_x;
}

void DragonBones::set_flip_y(bool _b_flip) {
	b_flip_y = _b_flip;
	if (p_armature) {
		p_armature->set_flip_y(_b_flip, true);
	}
}

bool DragonBones::is_fliped_y() const {
	return b_flip_y;
}

#ifdef COMPATIBILITY_ENABLED
void DragonBones::fade_in(const String &_name_anim, float _time, int _loop, int _layer, const String &_group, DragonBonesArmature::AnimFadeOutMode _fade_out_mode) {
	WARN_DEPRECATED;
	// setup speed
	set_speed_scale(f_speed);
	ERR_FAIL_NULL(p_armature);
	p_armature->fade_in(_name_anim, _time, _loop, _layer, _group, _fade_out_mode);
	if (p_armature->is_playing()) {
		b_playing = true;
	}
}

void DragonBones::fade_out(const String &_name_anim) {
	WARN_DEPRECATED;
	ERR_FAIL_NULL(p_armature);
	if (!p_armature->is_playing() || !p_armature->has_animation(_name_anim))
		return;
	p_armature->stop(_name_anim);
	b_playing = false;
	_reset();
}

void DragonBones::set_slot_display_index(const String &_slot_name, int _index) {
	WARN_DEPRECATED;
	ERR_FAIL_NULL(p_armature);
	p_armature->set_slot_display_index(_slot_name, _index);
}

void DragonBones::set_slot_by_item_name(const String &_slot_name, const String &_item_name) {
	WARN_DEPRECATED;
	ERR_FAIL_NULL(p_armature);
	p_armature->set_slot_by_item_name(_slot_name, _item_name);
}

void DragonBones::set_all_slots_by_item_name(const String &_item_name) {
	WARN_DEPRECATED;
	ERR_FAIL_NULL(p_armature);
	p_armature->set_all_slots_by_item_name(_item_name);
}

int DragonBones::get_slot_display_index(const String &_slot_name) {
	WARN_DEPRECATED;
	ERR_FAIL_NULL_V(p_armature, -1);
	return p_armature->get_slot_display_index(_slot_name);
}

int DragonBones::get_total_items_in_slot(const String &_slot_name) {
	WARN_DEPRECATED;
	ERR_FAIL_NULL_V(p_armature, -1);
	return p_armature->get_total_items_in_slot(_slot_name);
}

bool DragonBones::has_slot(const String &_slot_name) const {
	WARN_DEPRECATED;
	ERR_FAIL_NULL_V(p_armature, false);
	return p_armature->has_slot(_slot_name);
}

void DragonBones::cycle_next_item_in_slot(const String &_slot_name) {
	WARN_DEPRECATED;
	ERR_FAIL_NULL(p_armature);
	p_armature->cycle_next_item_in_slot(_slot_name);
}

void DragonBones::cycle_previous_item_in_slot(const String &_slot_name) {
	WARN_DEPRECATED;
	ERR_FAIL_NULL(p_armature);
	p_armature->cycle_next_item_in_slot(_slot_name);
}

Color DragonBones::get_slot_display_color_multiplier(const String &_slot_name) {
	WARN_DEPRECATED;
	ERR_FAIL_NULL_V((p_armature), (Color(-1.0, -1.0, -1.0, -1.0)));
	return p_armature->get_slot_display_color_multiplier(_slot_name);
}

void DragonBones::set_slot_display_color_multiplier(const String &_slot_name, const Color &_color) {
	WARN_DEPRECATED;
	ERR_FAIL_NULL(p_armature);
	p_armature->set_slot_display_color_multiplier(_slot_name, _color);
}

void DragonBones::play(bool _b_play) {
	WARN_DEPRECATED;
	b_playing = _b_play;
	if (!_b_play) {
		stop();
		return;
	}

	// setup speed
	set_speed_scale(f_speed);
	if (p_armature && p_armature->has_animation(str_curr_anim)) {
		p_armature->play(str_curr_anim, c_loop);
		b_try_playing = false;
	} else {
		// not finded animation stop playing
		b_try_playing = true;
		str_curr_anim = "[none]";
		stop();
	}
}

void DragonBones::play_from_time(float _f_time) {
	WARN_DEPRECATED;
	play();
	if (b_playing && p_armature) {
		p_armature->getAnimation()->gotoAndPlayByTime(str_curr_anim.ascii().get_data(), _f_time, c_loop);
	}
}

void DragonBones::play_from_progress(float _f_progress) {
	WARN_DEPRECATED;
	play();
	if (b_playing && p_armature) {
		p_armature->getAnimation()->gotoAndPlayByProgress(str_curr_anim.ascii().get_data(), CLAMP(_f_progress, 0, 1.f), c_loop);
	}
}

void DragonBones::play_new_animation_from_progress(const String &_str_anim, int _num_times, float _f_progress) {
	WARN_DEPRECATED;
	stop_all();
	str_curr_anim = _str_anim;
	c_loop = _num_times;

	play(true);
	play_from_progress(_f_progress);
}

void DragonBones::play_new_animation_from_time(const String &_str_anim, int _num_times, float _f_time) {
	WARN_DEPRECATED;
	stop_all();

	str_curr_anim = _str_anim;
	c_loop = _num_times;

	play(true);
	play_from_time(_f_time);
}

void DragonBones::play_new_animation(const String &_str_anim, int _num_times) {
	WARN_DEPRECATED;
	stop_all();

	str_curr_anim = _str_anim;
	c_loop = _num_times;

	play(true);
}

bool DragonBones::has_anim(const String &_str_anim) {
	WARN_DEPRECATED;
	ERR_FAIL_NULL_V(p_armature, false);
	return p_armature->has_animation(_str_anim);
}

void DragonBones::stop(bool _b_all) {
	if (!b_inited)
		return;

	b_playing = false;

	if (p_armature && p_armature->is_playing()) {
		p_armature->stop(_b_all ? String("") : str_curr_anim, true, _b_all ? true : false);
	}
}

float DragonBones::tell() {
	WARN_DEPRECATED;
	ERR_FAIL_NULL_V(p_armature, 0.0f);
	return p_armature->tell_animation(str_curr_anim);
}

void DragonBones::seek(float _f_p) {
	WARN_DEPRECATED;
	ERR_FAIL_NULL(p_armature);
	b_playing = false;
	f_progress = _f_p;
	p_armature->seek_animation(str_curr_anim, _f_p);
}

float DragonBones::get_progress() const {
	WARN_DEPRECATED;
	if (p_armature) {
		return p_armature->get_animation_progress();
	}
	return f_progress;
}

bool DragonBones::is_playing() const {
	WARN_DEPRECATED;
	if (p_armature) {
		return p_armature->is_playing();
	}
	return false;
}

String DragonBones::get_current_animation() const {
	WARN_DEPRECATED;
	if (p_armature) {
		return p_armature->get_current_animation();
	}
	return {};
}

String DragonBones::get_current_animation_on_layer(int _layer) const {
	WARN_DEPRECATED;
	if (p_armature) {
		return p_armature->get_current_animation_on_layer(_layer);
	}
	return {};
}
#endif

void DragonBones::set_texture(const Ref<Texture2D> &_p_texture) {
	if (_p_texture.is_valid() && m_texture_atlas.is_valid() && (_p_texture == m_texture_atlas || m_texture_atlas->get_height() != _p_texture->get_height() || m_texture_atlas->get_width() != _p_texture->get_width()))
		return;

	m_texture_atlas = _p_texture;

	if (p_armature) {
		p_armature->update_texture_atlas(m_texture_atlas);
		queue_redraw();
	}
}

Ref<Texture2D> DragonBones::get_texture() const {
	return m_texture_atlas;
}

void DragonBones::set_armature_settings(const Dictionary &p_settings) const {
	ERR_FAIL_NULL(p_armature);
	p_armature->set_settings(p_settings);
}

Dictionary DragonBones::get_armature_settings() const {
	if (!p_armature) {
		return {};
	}
#ifdef TOOLS_ENABLED
	return p_armature->get_settings();
#else //TOOLS_ENABLED
	ERR_FAIL_V_MSG({}, "DragonBones::get_armature_settings() can be call in editor build only.");
#endif // TOOLS_ENABLED
}

bool DragonBones::_set(const StringName &_str_name, const Variant &_c_r_value) {
	if (_str_name == SNAME("armature_settings")) {
		set_armature_settings(_c_r_value);
		return true;
	}
#ifdef TOOLS_ENABLED
	else if (_str_name == SNAME("main_armature")) {
		return true; // 禁止设置
	}
#endif //  TOOLS_ENABLED

	return false;
}

bool DragonBones::_get(const StringName &_str_name, Variant &_r_ret) const {
	if (_str_name == SNAME("armature_settings")) {
		_r_ret = get_armature_settings();
		return true;
	}
#ifdef TOOLS_ENABLED
	else if (_str_name == SNAME("main_armature")) {
		_r_ret = main_armature_ref;
		return true;
	}
#endif // TOOLS_ENABLED
	return false;
}

void DragonBones::_get_property_list(List<PropertyInfo> *_p_list) const {
	_p_list->push_back(PropertyInfo(Variant::DICTIONARY, "armature_settings", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE));
#ifdef TOOLS_ENABLED
	if (p_armature && Engine::get_singleton()->is_editor_hint()) {
		if (main_armature_ref.is_null()) {
			main_armature_ref.instantiate();
		}
		main_armature_ref->armature_node = p_armature;
		_p_list->push_back(PropertyInfo(
				Variant::OBJECT, "main_armature", PROPERTY_HINT_RESOURCE_TYPE, DragonBonesArmatureProxy::get_class_static(), PROPERTY_USAGE_EDITOR, DragonBonesArmatureProxy::get_class_static()));
	}
#endif // TOOLS_ENABLED
}

void DragonBones::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_texture", "texture"), &DragonBones::set_texture);
	ClassDB::bind_method(D_METHOD("get_texture"), &DragonBones::get_texture);

	ClassDB::bind_method(D_METHOD("set_factory", "dragonbones"), &DragonBones::set_factory);
	ClassDB::bind_method(D_METHOD("get_factory"), &DragonBones::get_factory);

	ClassDB::bind_method(D_METHOD("set_inherit_material"), &DragonBones::set_inherit_material);
	ClassDB::bind_method(D_METHOD("is_material_inherited"), &DragonBones::is_material_inherited);

	ClassDB::bind_method(D_METHOD("set_flip_x", "enable_flip"), &DragonBones::set_flip_x);
	ClassDB::bind_method(D_METHOD("is_fliped_x"), &DragonBones::is_fliped_x);
	ClassDB::bind_method(D_METHOD("set_flip_y", "enable_flip"), &DragonBones::set_flip_y);
	ClassDB::bind_method(D_METHOD("is_fliped_y"), &DragonBones::is_fliped_y);

#ifdef COMPATIBILITY_ENABLED
	/*
		All these functions act upon the base armature / display; a structure is being formed to make them available for all displays and armatures
	*/
	ClassDB::bind_method(D_METHOD("fade_in", "anim_name", "time", "loop", "layer", "group", "fade_out_mode"), &DragonBones::fade_in);
	ClassDB::bind_method(D_METHOD("fade_out", "anim_name"), &DragonBones::fade_out);

	ClassDB::bind_method(D_METHOD("stop"), &DragonBones::stop);
	ClassDB::bind_method(D_METHOD("stop_all"), &DragonBones::stop_all);
	ClassDB::bind_method(D_METHOD("reset"), &DragonBones::_reset);
	ClassDB::bind_method(D_METHOD("has_slot"), &DragonBones::has_slot);
	ClassDB::bind_method(D_METHOD("set_slot_by_item_name"), &DragonBones::set_slot_by_item_name);
	ClassDB::bind_method(D_METHOD("set_all_slots_by_item_name"), &DragonBones::set_all_slots_by_item_name);
	ClassDB::bind_method(D_METHOD("set_slot_display_index"), &DragonBones::set_slot_display_index);
	ClassDB::bind_method(D_METHOD("get_slot_display_index"), &DragonBones::get_slot_display_index);
	ClassDB::bind_method(D_METHOD("get_total_items_in_slot"), &DragonBones::get_total_items_in_slot);
	ClassDB::bind_method(D_METHOD("set_slot_display_color_multiplier"), &DragonBones::set_slot_display_color_multiplier);
	ClassDB::bind_method(D_METHOD("get_slot_display_color_multiplier"), &DragonBones::get_slot_display_color_multiplier);
	ClassDB::bind_method(D_METHOD("cycle_next_item_in_slot"), &DragonBones::cycle_next_item_in_slot);
	ClassDB::bind_method(D_METHOD("cycle_previous_item_in_slot"), &DragonBones::cycle_previous_item_in_slot);

	ClassDB::bind_method(D_METHOD("play"), &DragonBones::play);
	ClassDB::bind_method(D_METHOD("play_from_time"), &DragonBones::play_from_time);
	ClassDB::bind_method(D_METHOD("play_from_progress"), &DragonBones::play_from_progress);
	ClassDB::bind_method(D_METHOD("play_new_animation"), &DragonBones::play_new_animation);
	ClassDB::bind_method(D_METHOD("play_new_animation_from_progress"), &DragonBones::play_new_animation_from_progress);
	ClassDB::bind_method(D_METHOD("play_new_animation_from_time"), &DragonBones::play_new_animation_from_time);

	ClassDB::bind_method(D_METHOD("seek", "pos"), &DragonBones::seek);
	ClassDB::bind_method(D_METHOD("tell"), &DragonBones::tell);
	ClassDB::bind_method(D_METHOD("get_progress"), &DragonBones::get_progress);

	ClassDB::bind_method(D_METHOD("has", "name"), &DragonBones::has_anim);
	ClassDB::bind_method(D_METHOD("is_playing"), &DragonBones::is_playing);

	ClassDB::bind_method(D_METHOD("set_animation_loop", "loop_count"), &DragonBones::set_animation_loop);
	ClassDB::bind_method(D_METHOD("get_animation_loop"), &DragonBones::get_animation_loop);

	ClassDB::bind_method(D_METHOD("get_current_animation"), &DragonBones::get_current_animation);
	ClassDB::bind_method(D_METHOD("get_current_animation_on_layer"), &DragonBones::get_current_animation_on_layer);

#endif
	ClassDB::bind_method(D_METHOD("set_speed_scale", "speed"), &DragonBones::set_speed_scale);
	ClassDB::bind_method(D_METHOD("get_speed_scale"), &DragonBones::get_speed_scale);

	ClassDB::bind_method(D_METHOD("get_armature"), &DragonBones::get_armature);
	/*
		END OF BASE ARMATURE FUNCTIONS
	*/

	/*
		Let's get into the business of playing with child armatures.
	*/

	ClassDB::bind_method(D_METHOD("set_active", "active"), &DragonBones::set_active);
	ClassDB::bind_method(D_METHOD("is_active"), &DragonBones::is_active);

	ClassDB::bind_method(D_METHOD("set_debug", "debug"), &DragonBones::set_debug);
	ClassDB::bind_method(D_METHOD("is_debug"), &DragonBones::is_debug);

	ClassDB::bind_method(D_METHOD("set_callback_mode_process", "mode"), &DragonBones::set_callback_mode_process);
	ClassDB::bind_method(D_METHOD("get_callback_mode_process"), &DragonBones::get_callback_mode_process);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "factory", PROPERTY_HINT_RESOURCE_TYPE, DragonBonesFactory::get_class_static()), "set_factory", "get_factory");

	// This is how we set top level properties
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture"), "set_texture", "get_texture");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "is_active");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "debug"), "set_debug", "is_debug");

	ADD_GROUP("Flip", "flip_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_x"), "set_flip_x", "is_fliped_x");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_y"), "set_flip_y", "is_fliped_y");

	ADD_GROUP("Animation Settings", "animation_");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "animation_loop", PROPERTY_HINT_RANGE, "-1,100,1"), "set_animation_loop", "get_animation_loop");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "animation_speed_scale", PROPERTY_HINT_RANGE, "-10,10,0.01"), "set_speed_scale", "get_speed_scale");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "animation_callback_mode_process", PROPERTY_HINT_ENUM, "Physics,Idle,Manual"), "set_callback_mode_process", "get_callback_mode_process");

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "childs_use_this_material"), "set_inherit_material", "is_material_inherited");

	ADD_SIGNAL(MethodInfo("dragon_anim_start", PropertyInfo(Variant::STRING, "anim")));
	ADD_SIGNAL(MethodInfo("dragon_anim_complete", PropertyInfo(Variant::STRING, "anim")));
	ADD_SIGNAL(MethodInfo("dragon_anim_event", PropertyInfo(Variant::DICTIONARY, "event")));
	ADD_SIGNAL(MethodInfo("dragon_anim_loop_complete", PropertyInfo(Variant::STRING, "anim")));
	ADD_SIGNAL(MethodInfo("dragon_anim_snd_event", PropertyInfo(Variant::STRING, "anim"), PropertyInfo(Variant::STRING, "ev")));
	ADD_SIGNAL(MethodInfo("dragon_fade_in", PropertyInfo(Variant::STRING, "anim")));
	ADD_SIGNAL(MethodInfo("dragon_fade_in_complete", PropertyInfo(Variant::STRING, "anim")));
	ADD_SIGNAL(MethodInfo("dragon_fade_out", PropertyInfo(Variant::STRING, "anim")));
	ADD_SIGNAL(MethodInfo("dragon_fade_out_complete", PropertyInfo(Variant::STRING, "anim")));
}
