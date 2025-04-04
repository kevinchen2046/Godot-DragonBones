#pragma once

#include "dragonBones/core/DragonBones.h"
#include "dragonbones_armature.h"
#include "dragonbones_factory.h"
#include "wrappers/GDDisplay.h"

namespace godot {
/// TODO: 修改dragonBones库的new delete,供给Godot追踪内存
class DragonBones : public GDOwnerNode, public dragonBones::IEventDispatcher {
	GDCLASS(DragonBones, GDOwnerNode)

public:
	// sound IEventDispatcher
	virtual void addDBEventListener(const std::string &type, const std::function<void(dragonBones::EventObject *)> &listener) override {}
	virtual void removeDBEventListener(const std::string &type, const std::function<void(dragonBones::EventObject *)> &listener) override {}

	virtual bool hasDBEventListener(const std::string &type) const override { return true; }
	virtual void dispatchDBEvent(const std::string &type, dragonBones::EventObject *value) override {
		dispatch_sound_event(to_gd_str(type), value);
	}

	enum AnimationCallbackModeProcess {
		ANIMATION_CALLBACK_MODE_PROCESS_PHYSICS = 0,
		ANIMATION_CALLBACK_MODE_PROCESS_IDLE = 1,
		ANIMATION_CALLBACK_MODE_PROCESS_MANUAL = 2,
	};

private:
	dragonBones::DragonBones *p_instance{ nullptr };

	Ref<DragonBonesFactory> m_res;
	DragonBonesArmature *p_armature{ nullptr };
	AnimationCallbackModeProcess callback_mode_process{ ANIMATION_CALLBACK_MODE_PROCESS_IDLE };
	String instantiate_dragon_bones_data_name{ "" };
	String instantiate_armature_name{ "" };
	String instantiate_skin_name{ "" };
	float f_time_scale{ 1.0f };
	float f_progress{ 0.0f };
	int c_loop{ 0 };
	bool b_active{ true };
	bool processing{ false };
	bool b_playing{ false };
	bool b_debug{ false };
	bool b_inited{ false };
	bool b_try_playing{ false };

	bool b_flip_x{ false };
	bool b_flip_y{ false };
#ifdef COMPATIBILITY_ENABLED
#endif // COMPATIBILITY_ENABLED

	bool armatures_inherit_material{ true };

protected:
	static void _bind_methods();
	_DEFINE_TO_STRING()

	bool _set(const StringName &_str_name, const Variant &_c_r_value);
	bool _get(const StringName &_str_name, Variant &_r_ret) const;
	void _get_property_list(List<PropertyInfo> *_p_list) const;

#ifdef TOOLS_ENABLED
	void _validate_property(PropertyInfo &p_property) const;
#endif // TOOLS_ENABLED

	void _notification(int p_what);

public:
	DragonBones();
	~DragonBones() { _cleanup(true); }

	void _cleanup(bool p_for_destructor = false);

	// to initial pose current animation
	void _reset();

	virtual void dispatch_event(const String &_str_type, const dragonBones::EventObject *_p_value) override;
	virtual void dispatch_sound_event(const String &_str_type, const dragonBones::EventObject *_p_value) override;
	virtual Ref<CanvasItemMaterial> get_material_to_set_blend_mode(bool p_required) override;

	// setters/getters
	void set_factory(const Ref<DragonBonesFactory> &_p_data);
	Ref<DragonBonesFactory> get_factory() const;

	void set_inherit_material(bool _b_enable);
	bool is_material_inherited() const;

	void set_active(bool _b_active);
	bool is_active() const;

	void set_time_scale(float p_time_scale);
	float get_time_scale() const;

	void set_instantiate_dragon_bones_data_name(String p_name);
	String get_instantiate_dragon_bones_data_name() const;

	void set_instantiate_armature_name(String p_name);
	String get_instantiate_armature_name() const;

	void set_instantiate_skin_name(String p_name);
	String get_instantiate_skin_name() const;

	void set_callback_mode_process(AnimationCallbackModeProcess _mode);
	AnimationCallbackModeProcess get_callback_mode_process() const;

	int get_animation_loop() const;
	void set_animation_loop(int p_animation_loop);

	void advance(float p_delta) {
		if (p_instance) {
			p_instance->advanceTime(p_delta);
		}
	}

	void set_debug(bool _b_debug);
	bool is_debug() const;

	/* deprecated */ void set_flip_x(bool _b_flip);
	/* deprecated */ bool is_fliped_x() const;
	/* deprecated */ void set_flip_y(bool _b_flip);
	/* deprecated */ bool is_fliped_y() const;

#ifdef COMPATIBILITY_ENABLED
	/**
		THESE DEPRECATED FUNCTIONS WILL BE REMOVED IN VERSION 3.2.53
	*/
	/* deprecated */ void fade_in(const String &_name_anim, float _time, int _loop, int _layer, const String &_group, DragonBonesArmature::AnimFadeOutMode _fade_out_mode);
	/* deprecated */ void fade_out(const String &_name_anim);
	/* deprecated */ String get_current_animation() const;
	/* deprecated */ String get_current_animation_on_layer(int _layer) const;
	/* deprecated */ float tell();
	/* deprecated */ void seek(float _f_p);
	/* deprecated */ float get_progress() const;
	/* deprecated */ bool has_anim(const String &_str_anim);
	/* deprecated */ bool has_slot(const String &_slot_name) const;
	/* deprecated */ Color get_slot_display_color_multiplier(const String &_slot_name);
	/* deprecated */ void set_slot_display_color_multiplier(const String &_slot_name, const Color &_color);
	/* deprecated */ void set_slot_display_index(const String &_slot_name, int _index = 0);
	/* deprecated */ void set_slot_by_item_name(const String &_slot_name, const String &_item_name);
	/* deprecated */ void set_all_slots_by_item_name(const String &_item_name);
	/* deprecated */ int get_slot_display_index(const String &_slot_name);
	/* deprecated */ int get_total_items_in_slot(const String &_slot_name);
	/* deprecated */ void cycle_next_item_in_slot(const String &_slot_name);
	/* deprecated */ void cycle_previous_item_in_slot(const String &_slot_name);
	/* deprecated */ bool is_playing() const;
	/* deprecated */ void play_from_time(float _f_time);
	/* deprecated */ void play_from_progress(float _f_progress);
	/* deprecated */ void play_new_animation(const String &_str_anim, int _num_times);
	/* deprecated */ void play_new_animation_from_progress(const String &_str_anim, int _num_times, float _f_progress);
	/* deprecated */ void play_new_animation_from_time(const String &_str_anim, int _num_times, float _f_time);
	/* deprecated */ void play();
	/* deprecated */ void stop(bool _b_all = false);
	/* deprecated */ inline void stop_all() { stop(true); }
#endif

	DragonBonesArmature *get_armature();
	void set_armature(DragonBonesArmature *) const; // readonly

	void for_each_armature_(const Callable &p_action);

	template <class FUNC, std::enable_if_t<std::is_invocable_v<FUNC, DragonBonesArmature *, int>> *_dummy = nullptr>
	void for_each_armature(FUNC &&p_action) {
		if (!p_armature) {
			return;
		}

		if constexpr (std::is_invocable_r_v<bool, FUNC, DragonBonesArmature *, int>) {
			if (p_action(p_armature, 0)) {
				return;
			}
		} else {
			p_action(p_armature, 0);
		}
		p_armature->for_each_armature_recursively(p_action, 1);
	}

private:
	void _set_process(bool p_process, bool p_force = false);
	void _on_resource_changed();

	void set_armature_settings(const Dictionary &p_settings) const;
	Dictionary get_armature_settings() const;

#ifdef TOOLS_ENABLED
	mutable Ref<DragonBonesArmatureProxy> main_armature_ref;
#endif // TOOLS_ENABLED
};

class DragonBonesUserData : public RefCounted {
	GDCLASS(DragonBonesUserData, RefCounted)

	using v_size_t = int64_t;

private:
	dragonBones::UserData *user_data{ nullptr };

protected:
	static void _bind_methods();
	_DEFINE_TO_STRING()

public:
	DragonBonesUserData() = default;
	DragonBonesUserData(dragonBones::UserData *p_user_data) :
			user_data(p_user_data) {};

	bool has_data() const { return user_data; }

	PackedInt32Array get_ints() const;
	void set_ints(const PackedInt32Array &); // readonly

	PackedFloat32Array get_floats() const;
	void set_floats(const PackedFloat32Array &); // readonly

	PackedStringArray get_strings() const;
	void set_strings(const PackedStringArray &); // readonly

	int get_int(v_size_t p_index = 0) const;
	float get_float(v_size_t p_index = 0) const;
	String get_string(v_size_t p_index = 0) const;

	v_size_t get_ints_size() const;
	v_size_t get_floats_size() const;
	v_size_t get_strings_size() const;
};

} //namespace godot

VARIANT_ENUM_CAST(godot::DragonBones::AnimationCallbackModeProcess);
