#define MAX_UI_ELEMENTS 100
#define OVERLAY_FONT_SIZE 30.0f
#define BUTTON_FONT_SIZE 30.0f

enum UI_FAB {
	UI_FAB_Button,
	UI_FAB_Text,
	UI_FAB_Overlay,
};

struct UIData {
	Vec2 min;
	Vec2 max;
	Vec4 colors[4];
};

enum UI_FLAG {
	UI_FLAG_Hover   = 1<<1,
	UI_FLAG_Press   = 1<<2,
	UI_FLAG_Movable = 1<<3,
	UI_FLAG_Text    = 1<<4,
};

struct UIDimension {
	Vec2 min;
	Vec2 max;
};

struct UIElement {
	u64 flags;
	u32 id;

	UIElement* next;
	UIElement* child;

	UIDimension dimension;

	Vec4 primary_color[2];
	Vec4 secondary_color[2];
	Vec4 tertiary_color[2];

	bool pressed;

	char** text;
	u8 font_size;
	u8 text_count;
};

struct UICachedData {
	UICachedData* next;

	u32 element_id;
	UIDimension dimension;
	bool pressed;
};

struct UIRenderer {
	UIData* data;  // Generated every frame
	UIElement* elements;
	u32 element_counter;

	UICachedData* cached_data;

	StructuredBuffer* sb;
	VertexShader* vs;
	PixelShader* ps;

	Vec2 screen_resolution;

	MemoryArena* cache_arena;
	TemporaryMemory cache_arena_temp;

	MemoryArena* frame_arena;
	TextUI* text_ui;
};

static UIElement*
AllocateUIElements(UI_FAB fab, UIRenderer* ui_renderer) {
	UIElement* result = PushStructClear(ui_renderer->frame_arena, UIElement);
	result->id = ui_renderer->element_counter++;

	switch(fab) {
		case UI_FAB_Text: {
			result->flags |= UI_FLAG_Text;
			result->next = 0;
			result->child = 0;
		} break;

		case UI_FAB_Button: {
			result->flags |= UI_FLAG_Hover;
			result->flags |= UI_FLAG_Press;

			result->next = 0;
			result->child = AllocateUIElements(UI_FAB_Text, ui_renderer);
		} break;

		case UI_FAB_Overlay: {
			result->flags |= UI_FLAG_Movable;

			result->next = 0;
			result->child = AllocateUIElements(UI_FAB_Text, ui_renderer);
		} break;
	}

	return result;
}

static void
PushUIElement(UIElement* element, UIRenderer* ui_renderer) {
	UIElement* node = ui_renderer->elements;
	if(node) {
		while(node->next) node = node->next;
		node->next = element;
	}
	else ui_renderer->elements = element;
}

static void
CacheUIData(UIElement* element, UIRenderer* ui_renderer) {
	UICachedData* cached_data = PushStruct(ui_renderer->cache_arena, UICachedData);
	cached_data->element_id = element->id;
	cached_data->dimension = element->dimension;
	cached_data->pressed = element->pressed;
	cached_data->next = 0;
	
	if(!ui_renderer->cached_data) ui_renderer->cached_data = cached_data;
	else {
		UICachedData* node = ui_renderer->cached_data;
		while(node->next) node = node->next;
		node->next = cached_data;
	}
}

static UICachedData*
GetCachedElementData(u32 id, UIRenderer* ui_renderer) {
	if(!ui_renderer->cached_data) return 0;
	UICachedData* node = ui_renderer->cached_data;
	while(node) {
		if(node->element_id == id) return node;
		node = node->next;
	}
	return 0;
}

static void
PushUIOverlay(char** text, u8 text_count, Vec2 ssp, UIRenderer* ui_renderer) {
	UIElement* overlay = AllocateUIElements(UI_FAB_Overlay, ui_renderer);
	UICachedData* cached_data = GetCachedElementData(overlay->id, ui_renderer);
	if(cached_data) ssp = cached_data->dimension.min;

	UIDimension text_dim = {};
	Vec2 overlay_size = V2(0.1f, 0.1f);
	float font_size = OVERLAY_FONT_SIZE; 
	Vec2 text_loc_min;

	text_loc_min.x = ssp.x + overlay_size.x*0.01f;
	text_loc_min.y = ssp.y + overlay_size.y*0.01f;

	text_dim.min = V2(0.01f, 0.01f);
	text_dim.max = V2(0.9f, 0.9f);

	overlay->dimension.min = ssp;
	float max_pixel_width = 0;
	float ss_font_height = font_size/ui_renderer->screen_resolution.y;
	for(u8 i=0; i<text_count; i++) {
		float pixel_width = GetPixelWidthForText(text[i], font_size, ui_renderer->text_ui);
		max_pixel_width = Max(max_pixel_width, pixel_width);
	}
	float ss_height = ss_font_height * text_count;
	float ss_width = max_pixel_width/ui_renderer->screen_resolution.x;

	Vec2 text_loc_max;
	text_loc_max.x = text_loc_min.x + ss_width;
	text_loc_max.y = text_loc_min.y + ss_height;

	overlay->dimension.max.x = text_loc_max.x + overlay_size.x*0.1f;
	overlay->dimension.max.y = text_loc_max.y + overlay_size.y*0.1f;

	overlay->primary_color[0] = V4FromV3(JET, 0.8f);
	overlay->primary_color[1] = V4FromV3(JET, 0.6f);
	overlay->secondary_color[0] = V4FromV3(JET, 0.8f);
	overlay->secondary_color[1] = V4FromV3(JET, 0.6f);
	overlay->tertiary_color[0] = V4FromV3(JET, 0.8f);
	overlay->tertiary_color[1] = V4FromV3(JET, 0.6f);

	overlay->child->dimension = text_dim;
	overlay->child->text = text;
	overlay->child->text_count = text_count;
	overlay->child->font_size= OVERLAY_FONT_SIZE;
	overlay->child->primary_color[0] = V4Z();
	overlay->child->primary_color[1] = V4Z();

	PushUIElement(overlay, ui_renderer);
}

static bool
PushUIButton(char** text, Vec2 ssp, UIRenderer* ui_renderer) {
	Assert(InRangeMinMaxInc(ssp.x, 0, 1));
	Assert(InRangeMinMaxInc(ssp.y, 0, 1));
	bool pressed = false;

	UIElement* button = AllocateUIElements(UI_FAB_Button, ui_renderer);
	UICachedData* cached_data = GetCachedElementData(button->id, ui_renderer);
	if(cached_data) pressed = cached_data->pressed;

	UIDimension text_dim = {};
	Vec2 button_size = V2(0.1f, 0.1f);
	float font_size = BUTTON_FONT_SIZE; 
	Vec2 text_loc_min;

	text_loc_min.x = ssp.x + button_size.x*0.25f;
	text_loc_min.y = ssp.y + button_size.y*0.25f;

	text_dim.min = V2(0.25f, 0.25f);
	text_dim.max = V2(0.75f, 0.75f);

	button->dimension.min = ssp;
	float pixel_width = GetPixelWidthForText(text[0], font_size, ui_renderer->text_ui);
	float ss_font_height = font_size/ui_renderer->screen_resolution.y;
	float ss_width = pixel_width/ui_renderer->screen_resolution.x;

	Vec2 text_loc_max;
	text_loc_max.x = text_loc_min.x + ss_width;
	text_loc_max.y = text_loc_min.y + ss_font_height;

	button->dimension.max.x = text_loc_max.x + button_size.x*0.25f;
	button->dimension.max.y = text_loc_max.y + button_size.y*0.25f;

	button->primary_color[0] = V4FromV3(JET, 1.0f);
	button->primary_color[1] = V4FromV3(JET, 1.0f);

	button->secondary_color[0] = V4FromV3(BLACK, 1.0f);
	button->secondary_color[1] = V4FromV3(JET, 1.0f);

	button->tertiary_color[0] = V4FromV3(JET, 1.0f);
	button->tertiary_color[1] = V4FromV3(BLACK, 1.0f);

	button->child->dimension = text_dim;
	button->child->text = text;
	button->child->text_count = 1;
	button->child->font_size = BUTTON_FONT_SIZE;
	button->child->primary_color[0] = V4Z();
	button->child->primary_color[1] = V4Z();

	PushUIElement(button, ui_renderer);

	return pressed;
}

static void
UIOperateElement(Vec2 mouse_pos, Vec2 mouse_del, bool held, UIElement* element, UIElement* parent, UIData* data, UIRenderer* ui_renderer) {
	if(parent) {
		Vec2 size = V2Sub(parent->dimension.max, parent->dimension.min);
		data->min = V2Add(parent->dimension.min, V2Mul(element->dimension.min, size));
		data->max = V2Sub(parent->dimension.max, V2Mul(element->dimension.min, size));
	}
	else {
		data->min = element->dimension.min;
		data->max = element->dimension.max;
	}
	data->colors[0] = element->primary_color[0];
	data->colors[1] = element->primary_color[1];
	data->colors[2] = element->primary_color[0];
	data->colors[3] = element->primary_color[1];

	bool inside_quad = IsPointInQuad(data->min, data->max, mouse_pos);

	if(element->flags & UI_FLAG_Hover) {
		if(inside_quad) {
			data->colors[0] = element->secondary_color[0];
			data->colors[1] = element->secondary_color[1];
			data->colors[2] = element->secondary_color[0];
			data->colors[3] = element->secondary_color[1];
		}
	}

	if(element->flags & UI_FLAG_Press) {
		if(inside_quad && held) {
			data->colors[0] = element->tertiary_color[0];
			data->colors[1] = element->tertiary_color[1];
			data->colors[2] = element->tertiary_color[0];
			data->colors[3] = element->tertiary_color[1];

			element->pressed = true;
		}
		CacheUIData(element, ui_renderer);
	}

	if(element->flags & UI_FLAG_Movable) {
		if(inside_quad && held) {
			data->min = V2Add(mouse_del, data->min);
			data->max = V2Add(mouse_del, data->max);
			element->dimension.min = data->min;
			element->dimension.max = data->max;
		}
		CacheUIData(element, ui_renderer);
	}

	if(element->flags & UI_FLAG_Text) {
		float font_size_ss = element->font_size / ui_renderer->screen_resolution.y;
		for(u8 i=0; i<element->text_count; i++) {
			Vec2 loc = V2(data->min.x, data->min.y + font_size_ss*i);
			PushTextScreenSpace(element->text[i], element->font_size, loc, ui_renderer->text_ui);
		}
	}

}

static void
CompileUIShaders(UIRenderer* ui_renderer, Renderer* renderer) {
	ui_renderer->vs = UploadVertexShader(UIShader, sizeof(UIShader), "vsf", 0, 0, renderer);
	ui_renderer->ps = UploadPixelShader(UIShader, sizeof(UIShader), "psf", renderer);
}

static UIRenderer*
InitUIRenderer(Renderer* renderer, WindowDimensions wd, TextUI* text_ui, MemoryArena* arena, MemoryArena* frame_arena) {
	UIRenderer* result = PushStructClear(arena, UIRenderer);
	result->frame_arena = frame_arena;
	result->cache_arena = (MemoryArena*)BootstrapPushSize_(sizeof(MemoryArena), 0, 0);
	result->text_ui = text_ui;
	result->screen_resolution = V2((float)wd.width, (float)wd.height);

	CompileUIShaders(result, renderer);

	result->sb = UploadStructuredBuffer(sizeof(UIData), MAX_UI_ELEMENTS, renderer);

	result->cache_arena_temp = BeginTemporaryMemory(result->cache_arena);
	return result;
}

static void
UIRenderElements(UIRenderer* ui_renderer, Renderer* renderer) {

	SetRenderTarget* set_render_target = PushRenderCommand(renderer, SetRenderTarget);
	set_render_target->render_target = &renderer->backbuffer;

	SetBlendState* set_blend_state = PushRenderCommand(renderer, SetBlendState);
	set_blend_state->type = BLEND_STATE_Regular;

	SetRasterizerState* srs = PushRenderCommand(renderer, SetRasterizerState);

	SetPrimitiveTopology* set_topology = PushRenderCommand(renderer, SetPrimitiveTopology);
	set_topology->type = PRIMITIVE_TOPOLOGY_TriangleStrip;

	SetVertexShader* set_vertex_shader = PushRenderCommand(renderer, SetVertexShader);
	set_vertex_shader->vertex = ui_renderer->vs;

	SetPixelShader* set_pixel_shader = PushRenderCommand(renderer, SetPixelShader);
	set_pixel_shader->pixel = ui_renderer->ps;

	SetStructuredBuffer* set_ui_buffer = PushRenderCommand(renderer, SetStructuredBuffer);
	set_ui_buffer->vertex_shader = true;
	set_ui_buffer->structured = ui_renderer->sb;
	set_ui_buffer->slot = 0;

	PushRenderBufferData* push_ui_buffer = PushRenderCommand(renderer,PushRenderBufferData);
	push_ui_buffer->buffer = ui_renderer->sb->buffer;
	push_ui_buffer->size = ui_renderer->element_counter * sizeof(UIData);
	push_ui_buffer->data = ui_renderer->data;

	DrawInstanced* draw_instanced = PushRenderCommand(renderer, DrawInstanced);
	draw_instanced->vertices_count = 4;
	draw_instanced->instance_count = ui_renderer->element_counter;

}

static void
UIGenerateData(Input* input, UIRenderer* ui_renderer) {
	EndTemporaryMemory(&ui_renderer->cache_arena_temp);
	ui_renderer->cache_arena_temp = BeginTemporaryMemory(ui_renderer->cache_arena);
	ui_renderer->cached_data = 0;

	Vec2 mouse_pos = *(Vec2*)&input->axes[WIN32_AXIS_MOUSE];
	Vec2 mouse_del = *(Vec2*)&input->axes[WIN32_AXIS_MOUSE_DEL];
	bool held = input->buttons[WIN32_BUTTON_LEFT_MOUSE].held;

	ui_renderer->data = PushArray(ui_renderer->frame_arena, UIData, ui_renderer->element_counter);

	u32 node_count = 0;

	UIElement* node = ui_renderer->elements;
	while(node) {
		UIOperateElement(mouse_pos, mouse_del, held, node, 0, ui_renderer->data + node_count++, ui_renderer);

		UIElement* child = node->child;
		while(child) {
			UIOperateElement(mouse_pos, mouse_del, held, child, node, ui_renderer->data + node_count++, ui_renderer);
			child = child->next;
		}

		node = node->next;
	}
}

static void
UIRendererFrame(Input* input, UIRenderer* ui_renderer, Renderer* renderer) {

	UIGenerateData(input, ui_renderer);
	if(ui_renderer->element_counter)
		UIRenderElements(ui_renderer, renderer);
	ui_renderer->element_counter = 0;
	ui_renderer->data = 0;
	ui_renderer->elements = 0;
}


