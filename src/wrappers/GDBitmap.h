#pragma once
#include "GDDisplay.h"
#include "dragonBones/event/EventObject.h"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include <godot_cpp/classes/engine.hpp>

namespace godot {

class GDBitmap : public GDDisplay {
	GDCLASS(GDBitmap, Node2D)
private:
	GDBitmap(const GDBitmap &);

protected:
	static void _bind_methods() {}
	_DEFINE_TO_STRING()

public:
    dragonBones::TextureData* textureData;

public:
	GDBitmap() = default;
	virtual ~GDBitmap() override = default;

	virtual Ref<CanvasItemMaterial> get_material_to_set_blend_mode(bool p_required) override {
		if (get_use_parent_material()) {
			auto parent = dynamic_cast<GDOwnerNode *>(get_parent());
			if (parent) {
				return parent->get_material_to_set_blend_mode(p_required);
			}
		}

		Ref<CanvasItemMaterial> ret = get_material();
		if (ret.is_null() && p_required) {
			ret.instantiate();
			set_material(ret);
		}
		return ret;
	}

	virtual void dispatch_event(const String &_str_type, const dragonBones::EventObject *_p_value) override {
		if (p_owner) {
			p_owner->dispatch_event(_str_type, _p_value);
		}
	}

	virtual void dispatch_sound_event(const String &_str_type, const dragonBones::EventObject *_p_value) override {
		if (p_owner) {
			p_owner->dispatch_sound_event(_str_type, _p_value);
		}
	}

	virtual void _draw() override {
        if (texture.is_valid() && textureData!= nullptr) {
            draw_texture_rect_region(
                texture,
                Rect2(0, 0, textureData->region.width, textureData->region.height),
                Rect2(textureData->region.x, textureData->region.y, textureData->region.width, textureData->region.height)
            );
        }
		// auto owner = static_cast<GDDisplay *>(p_owner);
		// const Ref<Texture2D> texture_to_draw = owner && owner->texture.is_valid() ? owner->texture : this->texture;

		// if (texture_to_draw.is_valid()) {
        //     texture=texture_to_draw;
		// }
	}

	virtual void update_modulate(const Color &p_modulate) override {
		GDDisplay::update_modulate(p_modulate);
	}
};

} //namespace godot