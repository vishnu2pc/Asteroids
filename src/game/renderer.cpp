#include "renderer.h"

static void
PushRenderData(ID3D11Buffer* buffer, void* data, u32 size, ID3D11DeviceContext* context) {
	D3D11_MAPPED_SUBRESOURCE msr = {};

	context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	CopyMem(msr.pData, data, size);
	context->Unmap(buffer, 0);

	data = nullptr;
}

static void* 
PushCommandBuffer(Renderer* renderer, u32 size) {
	void* result = 0;

	u8* command_buffer_end = renderer->command_buffer_base + renderer->command_buffer_size;

	if((renderer->command_buffer_cursor + size) <= command_buffer_end) {
		result = renderer->command_buffer_cursor;
		renderer->command_buffer_cursor += size;
	}
	else Assert(false);

	return result;
}

#define PushRenderCommand(renderer, type) (type *)PushRenderCommand_(renderer, sizeof(type), RENDER_COMMAND_##type)

static void*
PushRenderCommand_(Renderer* renderer, u32 size, RENDER_COMMAND type ) {
	void* result = 0;

	u32 actual_size = size + sizeof(RenderCommandHeader);
	void* ptr = PushCommandBuffer(renderer, actual_size);

	RenderCommandHeader* header = (RenderCommandHeader*)ptr;
	header->type = (u8)type;
	result = (u8*)ptr + sizeof(RenderCommandHeader);

	return result;
}

static void
ExecuteRenderCommands(Renderer* renderer) {
	u8* cursor = renderer->command_buffer_base;
	while(cursor < renderer->command_buffer_cursor) {
		RenderCommandHeader* header = (RenderCommandHeader*)cursor;
		cursor += sizeof(RenderCommandHeader);
		void* data = (u8*)header + sizeof(RenderCommandHeader);

		switch(header->type) {

			case RENDER_COMMAND_ClearRenderTarget: {
				cursor += sizeof(ClearRenderTarget);
				ClearRenderTarget* command = (ClearRenderTarget*)data;

				if(command->render_target)
					renderer->context->ClearRenderTargetView(command->render_target->view, command->color);
				else
					renderer->context->ClearRenderTargetView(renderer->readable_render_target.render_target, command->color);

			} break;

			case RENDER_COMMAND_ClearDepth: {
				cursor += sizeof(ClearDepth);
				ClearDepth* command = (ClearDepth*)data;

					renderer->context->ClearDepthStencilView(renderer->depth_stencil.view, D3D11_CLEAR_DEPTH,
							command->value, 0);
			} break;

			case RENDER_COMMAND_ClearStencil: {
				cursor += sizeof(ClearStencil);
				ClearStencil* command = (ClearStencil*)data;

				// TODO: handle default value
				renderer->context->ClearDepthStencilView(renderer->depth_stencil.view, D3D11_CLEAR_STENCIL, 0, command->value);
			} break;

			case RENDER_COMMAND_SetRenderTarget: {
				cursor += sizeof(SetRenderTarget);
				SetRenderTarget* command = (SetRenderTarget*)data;

				if(command->render_target) 
					renderer->context->OMSetRenderTargets(1, &command->render_target->view, renderer->depth_stencil.view);
				else
					renderer->context->OMSetRenderTargets(1, &renderer->readable_render_target.render_target, renderer->depth_stencil.view);
			} break;

			case RENDER_COMMAND_SetDepthStencilState: {
				cursor += sizeof(SetDepthStencilState);
				SetDepthStencilState* command = (SetDepthStencilState*)data;

				renderer->context->OMSetDepthStencilState(renderer->default_depth_stencil_state, 0);
			} break;


			case RENDER_COMMAND_SetBlendState: {
				cursor += sizeof(SetBlendState);
				SetBlendState* command = (SetBlendState*)data;

				renderer->context->OMSetBlendState(renderer->blend_states[command->type], 0, 0xffffffff);
			} break;

			case RENDER_COMMAND_SetRasterizerState: {
				cursor += sizeof(SetRasterizerState);
				SetRasterizerState* command = (SetRasterizerState*)data;

				renderer->context->RSSetState(renderer->rasterizer_states[command->type]);
			} break;

			case RENDER_COMMAND_SetSamplerState: {
				cursor += sizeof(SetSamplerState);
				SetSamplerState* command = (SetSamplerState*)data;

				renderer->context->PSSetSamplers(command->slot, 1, &renderer->samplers[command->type]);
			} break;

			case RENDER_COMMAND_SetViewport: {
				cursor += sizeof(SetViewport);
				SetViewport* command = (SetViewport*)data;

				D3D11_VIEWPORT vp = {};
				vp.TopLeftX       = command->topleft.x;
				vp.TopLeftY       = command->topleft.y;
				vp.Width          = command->dim.x;
				vp.Height         = command->dim.y;

				renderer->context->RSSetViewports(1, &vp);

				D3D11_RECT rect = {};
				rect.right = vp.Width;
				rect.bottom = vp.Height;

				renderer->context->RSSetScissorRects(1, &rect);

			} break;

			case RENDER_COMMAND_SetPrimitiveTopology: {
				cursor += sizeof(SetPrimitiveTopology);
				SetPrimitiveTopology* command = (SetPrimitiveTopology*)data;

				D3D_PRIMITIVE_TOPOLOGY topology;
				if(command->type == PRIMITIVE_TOPOLOGY_TriangleList)
					topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				if(command->type == PRIMITIVE_TOPOLOGY_TriangleStrip)
					topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

				renderer->context->IASetPrimitiveTopology(topology);
			} break;

			case RENDER_COMMAND_SetVertexShader: {
				cursor += sizeof(SetVertexShader);
				SetVertexShader* command = (SetVertexShader*)data;

				renderer->context->IASetInputLayout(command->vertex->il);
				renderer->context->VSSetShader(command->vertex->shader, 0, 0);
			} break;

			case RENDER_COMMAND_SetPixelShader: {
				cursor += sizeof(SetPixelShader);
				SetPixelShader* command = (SetPixelShader*)data;

				renderer->context->PSSetShader(command->pixel->shader, 0, 0);
			} break;

			case RENDER_COMMAND_SetVertexBuffer: {
				cursor += sizeof(SetVertexBuffer);
				SetVertexBuffer* command = (SetVertexBuffer*)data;

				renderer->context->IASetVertexBuffers(command->slot, 1, &command->vertex->buffer, &command->stride, &command->offset);
			} break;

			case RENDER_COMMAND_SetIndexBuffer: {
				cursor += sizeof(SetIndexBuffer);
				SetIndexBuffer* command = (SetIndexBuffer*)data;

				renderer->context->IASetIndexBuffer(command->index->buffer, DXGI_FORMAT_R32_UINT, command->offset);
			} break;

			case RENDER_COMMAND_SetStructuredBuffer: {
				cursor += sizeof(SetStructuredBuffer);
				SetStructuredBuffer* command = (SetStructuredBuffer*)data;

				if(command->vertex_shader)
					renderer->context->VSSetShaderResources(command->slot, 1, &command->structured->view);
				else
					renderer->context->PSSetShaderResources(command->slot, 1, &command->structured->view);
			} break;

			case RENDER_COMMAND_SetTextureBuffer: {
				cursor += sizeof(SetTextureBuffer);
				SetTextureBuffer* command = (SetTextureBuffer*)data;

				if(command->texture)
					renderer->context->PSSetShaderResources(command->slot, 1, &command->texture->view);
				else {
					ID3D11ShaderResourceView* srv[1] = { 0 };
					renderer->context->PSSetShaderResources(command->slot, 1, srv);
				}
			} break;

			case RENDER_COMMAND_SetConstantsBuffer: {
				cursor += sizeof(SetConstantsBuffer);
				SetConstantsBuffer* command = (SetConstantsBuffer*)data;

				if(command->vertex_shader)
					renderer->context->VSSetConstantBuffers(command->slot, 1, &command->constants->buffer);
				else
					renderer->context->PSSetConstantBuffers(command->slot, 1, &command->constants->buffer);
			} break;

			case RENDER_COMMAND_PushRenderBufferData: {
				cursor += sizeof(PushRenderBufferData);
				PushRenderBufferData* command = (PushRenderBufferData*)data;

				PushRenderData((ID3D11Buffer*)command->buffer, command->data, command->size, renderer->context);
			} break;

			case RENDER_COMMAND_FreeRenderResource: {
				cursor += sizeof(FreeRenderResource);
				FreeRenderResource* command = (FreeRenderResource*)data;

				ID3D11Resource* resource = (ID3D11Resource*)command->buffer;
				resource->Release();
			} break;

			case RENDER_COMMAND_DrawVertices: {
				cursor += sizeof(DrawVertices);
				DrawVertices* command = (DrawVertices*)data;

				renderer->context->Draw(command->vertices_count, command->offset);
			} break;

			case RENDER_COMMAND_DrawIndexed: {
				cursor += sizeof(DrawIndexed);
				DrawIndexed* command = (DrawIndexed*)data;

				renderer->context->DrawIndexed(command->indices_count, command->offset, 0);
			} break;

			case RENDER_COMMAND_DrawInstanced: {
				cursor += sizeof(DrawInstanced);
				DrawInstanced* command = (DrawInstanced*)data;

				renderer->context->DrawInstanced(command->vertices_count, command->instance_count, command->offset, 0);
			} break;
		}
	}
}

static ReadableRenderTarget
CreateReadableRenderTarget(Renderer* renderer) {
	HRESULT hr = {};
	ReadableRenderTarget result = {};
	ID3D11Texture2D* texture = NULL;
	ID3D11RenderTargetView*	render_target = NULL;
	ID3D11ShaderResourceView* shader_resource = NULL;

	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

	D3D11_TEXTURE2D_DESC buffer_desc = {};
	buffer_desc.ArraySize = 1;
	buffer_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.Format = format;
	buffer_desc.Width = renderer->window_dim.width;
	buffer_desc.Height = renderer->window_dim.height;
	buffer_desc.MipLevels = 1;
	buffer_desc.MiscFlags = 0;
	buffer_desc.SampleDesc.Count = renderer->msaa_sample_count;
	buffer_desc.SampleDesc.Quality = renderer->msaa_quality_level;
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	hr = renderer->device->CreateTexture2D(&buffer_desc, 0, &texture);

	D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
	render_target_view_desc.Format = format;
	render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	render_target_view_desc.Texture2D.MipSlice = 0;
	hr = renderer->device->CreateRenderTargetView(texture, 0, &render_target);

	AssertHR(hr);

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
	shader_resource_view_desc.Format = format;
	shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resource_view_desc.Texture2D.MostDetailedMip = 0;
	shader_resource_view_desc.Texture2D.MipLevels = 1;
	hr = renderer->device->CreateShaderResourceView(texture, &shader_resource_view_desc, &shader_resource);

	AssertHR(hr);

	result.texture = texture;
	result.render_target = render_target;
	result.shader_resource = shader_resource;

	return result;
}

static void
RendererBeginFrame(Renderer* renderer, WindowDimensions wd, MemoryArena* frame_arena) {
	renderer->frame_arena = frame_arena;
	renderer->command_buffer_cursor = 
		renderer->command_buffer_base = (u8*)PushSize(renderer->frame_arena, renderer->command_buffer_size);

	if(renderer->window_dim.width != wd.width ||
	   renderer->window_dim.height != wd.height) {

		renderer->window_dim = wd;

		renderer->backbuffer.texture->Release();
		renderer->backbuffer.view->Release();
		renderer->depth_stencil.texture->Release();
		renderer->depth_stencil.view->Release();

		renderer->swapchain->ResizeBuffers(0, wd.width, wd.height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

		ID3D11Texture2D* backbuffer;
		ID3D11RenderTargetView* rtv;
		renderer->swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
		renderer->device->CreateRenderTargetView((ID3D11Resource*)backbuffer, NULL, &rtv);

		D3D11_TEXTURE2D_DESC depth_desc = {};
		depth_desc.Width = wd.width;
		depth_desc.Height = wd.height;
		depth_desc.MipLevels = 1;
		depth_desc.ArraySize = 1;
		depth_desc.Format = DXGI_FORMAT_D32_FLOAT; // or use DXGI_FORMAT_D32_FLOAT_S8X24_UINT if you need stencil
		depth_desc.SampleDesc = { 1, 0 };
		depth_desc.Usage = D3D11_USAGE_DEFAULT;
		depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		// create new depth stencil texture & DepthStencil view
		ID3D11Texture2D* depth;
		ID3D11DepthStencilView* dsv;
		renderer->device->CreateTexture2D(&depth_desc, NULL, &depth);
		renderer->device->CreateDepthStencilView((ID3D11Resource*)depth, NULL, &dsv);

		renderer->backbuffer.texture = backbuffer;
		renderer->backbuffer.view = rtv;
		renderer->depth_stencil.texture = depth;
		renderer->depth_stencil.view = dsv;

		renderer->readable_render_target.texture->Release();
		renderer->readable_render_target.render_target->Release();
		renderer->readable_render_target.shader_resource->Release();

		renderer->readable_render_target = CreateReadableRenderTarget(renderer);
	}

	SetTextureBuffer* stb = PushRenderCommand(renderer, SetTextureBuffer);
	stb->texture = 0;
	stb->slot = 0;

	SetRenderTarget* set_render_target = PushRenderCommand(renderer, SetRenderTarget);
	set_render_target->render_target = 0;

	ClearRenderTarget* clear_render_target = PushRenderCommand(renderer, ClearRenderTarget);
	clear_render_target->render_target = &renderer->backbuffer;
	*(Vec4*)clear_render_target->color = { 0.1f, 0.1f, 0.1f, 1.0f };

	ClearRenderTarget* clear_render_target1 = PushRenderCommand(renderer, ClearRenderTarget);
	clear_render_target1->render_target = 0;
	*(Vec4*)clear_render_target1->color = { 0.1f, 0.1f, 0.1f, 1.0f };

	ClearDepth* clear_depth = PushRenderCommand(renderer, ClearDepth);
	clear_depth->value = 1.0f;

	SetDepthStencilState* set_depth_stencil_state = PushRenderCommand(renderer, SetDepthStencilState);
	set_depth_stencil_state->type = 0;

	SetBlendState* set_blend_state = PushRenderCommand(renderer, SetBlendState);
	set_blend_state->type = BLEND_STATE_Regular;

	SetRasterizerState* set_rasterizer_state = PushRenderCommand(renderer, SetRasterizerState);
	set_rasterizer_state->type = RASTERIZER_STATE_Default;

	SetSamplerState* set_sampler_state = PushRenderCommand(renderer, SetSamplerState);
	set_sampler_state->type = SAMPLER_STATE_Default;
	set_sampler_state->slot = 0;

	SetViewport* set_viewport = PushRenderCommand(renderer, SetViewport);
	set_viewport->topleft = V2Z();
	set_viewport->dim = V2((float)wd.width, (float)wd.height);

	SetPrimitiveTopology* set_topology = PushRenderCommand(renderer, SetPrimitiveTopology);
	set_topology->type = PRIMITIVE_TOPOLOGY_TriangleList;

}

static void
RendererEndFrame(Renderer* renderer) {
	ExecuteRenderCommands(renderer);
	renderer->swapchain->Present(0, 0);
}

static void 
MakeD3DInputElementDesc(VERTEX_BUFFER* vb_type, D3D11_INPUT_ELEMENT_DESC* d3d_il_desc, u8 count) {
	for (u32 i = 0; i < count; i++) {
		DXGI_FORMAT format;
		if (vb_type[i] == VERTEX_BUFFER_POSITION) format = DXGI_FORMAT_R32G32B32_FLOAT;
		else if (vb_type[i] == VERTEX_BUFFER_NORMAL) format = DXGI_FORMAT_R32G32B32_FLOAT;
		else if (vb_type[i] == VERTEX_BUFFER_COLOR) format = DXGI_FORMAT_R32G32B32_FLOAT;
		//else if (vb_type[i] == VERTEX_BUFFER_TANGENT) format = DXGI_FORMAT_R32G32B32_FLOAT;
		else if (vb_type[i] == VERTEX_BUFFER_TEXCOORD) format = DXGI_FORMAT_R32G32_FLOAT;
		else Assert(false);

		d3d_il_desc[i] = { vertex_buffer_names[vb_type[i]], 0, format, i, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	}	
}

static ConstantsBuffer* 
UploadConstantsBuffer(u32 size, Renderer* renderer) {
	HRESULT hr = {};
	ConstantsBuffer* cb = PushStruct(renderer->permanent_arena, ConstantsBuffer);
	ID3D11Buffer* buffer = 0;

	size += (16 - (size % 16));
	D3D11_BUFFER_DESC cb_desc = {};
	cb_desc.ByteWidth 		 = size;
	cb_desc.Usage          = D3D11_USAGE_DYNAMIC;
	cb_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = renderer->device->CreateBuffer(&cb_desc, nullptr, &buffer);

	AssertHR(hr);

	cb->buffer = buffer;
	return cb;
}

static StructuredBuffer* 
UploadStructuredBuffer(u32 struct_size, u32 count, Renderer* renderer) {
	HRESULT hr = {};
	StructuredBuffer* sb = PushStruct(renderer->permanent_arena, StructuredBuffer);

	ID3D11Buffer* buffer = 0;
	ID3D11ShaderResourceView* view = 0;

	D3D11_BUFFER_DESC buffer_desc = {};
	buffer_desc.ByteWidth = struct_size * count;
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	buffer_desc.StructureByteStride = struct_size;

	hr = renderer->device->CreateBuffer(&buffer_desc, NULL, &buffer);
	AssertHR(hr);

	D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {};
	view_desc.Format = DXGI_FORMAT_UNKNOWN;
	view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	view_desc.Buffer.ElementOffset = 0;
	view_desc.Buffer.ElementWidth = count;

	hr = renderer->device->CreateShaderResourceView(buffer, &view_desc, &view);
	AssertHR(hr);

	sb->buffer = buffer;
	sb->view = view; 

	return sb;
};

static bool
CompileShader(char* shader_code, u32 shader_length, char* entry, void** shader, ID3DBlob** blob,
		bool vertex_shader, Renderer* renderer) {
	HRESULT hr;
	ID3DBlob* error;

	if(vertex_shader)
		hr = D3DCompile(shader_code, shader_length, nullptr, nullptr, nullptr, entry, "vs_5_0",
				0, 0, blob, &error);
	else
		hr = D3DCompile(shader_code, shader_length, nullptr, nullptr, nullptr, entry, "ps_5_0",
				0, 0, blob, &error);

	if (hr) {
		char* msg = (char*)error->GetBufferPointer();
		OutputDebugStringA(msg);
		return false;
	}

	if(vertex_shader) 
		hr = renderer->device->CreateVertexShader(blob[0]->GetBufferPointer(), blob[0]->GetBufferSize(), 
				nullptr, (ID3D11VertexShader**)shader);
	else 
		hr = renderer->device->CreatePixelShader(blob[0]->GetBufferPointer(), blob[0]->GetBufferSize(), 
				nullptr, (ID3D11PixelShader**)shader);

	if(hr) return false;
	
	return true;
};

static PixelShader* 
UploadPixelShader(char* code, u32 length, char* entry, Renderer* renderer) {
	PixelShader* ps = PushStruct(renderer->permanent_arena, PixelShader);
	ID3D11PixelShader* shader;
	ID3DBlob* blob;

	Assert(CompileShader(code, length, entry, (void**)&shader, &blob, false, renderer));

	ps->shader = shader;
	return ps;
}

static VertexShader* 
UploadVertexShader(char* code, u32 length, char* entry, VERTEX_BUFFER* vertex_buffers, u8 count, Renderer* renderer) {
	HRESULT hr = {};

	VertexShader* vs = PushStruct(renderer->permanent_arena, VertexShader);
	ID3D11VertexShader* shader = 0;
	ID3D11InputLayout* il = 0;
	ID3DBlob* blob = 0;

	Assert(CompileShader(code, length, entry, (void**)&shader, &blob, true, renderer));

	if(vertex_buffers) {
		D3D11_INPUT_ELEMENT_DESC* ie_desc = PushArray(renderer->frame_arena, D3D11_INPUT_ELEMENT_DESC, count);
		MakeD3DInputElementDesc(vertex_buffers, ie_desc, count);

		hr = renderer->device->CreateInputLayout(ie_desc, count, blob->GetBufferPointer(), blob->GetBufferSize(), &il);
		AssertHR(hr);
	}

	vs->shader = shader;
	vs->il = il;

	return vs;
}

static TextureBuffer* 
UploadTexture(void* data, u32 width, u32 height, u8 num_components, bool dynamic, bool temp, Renderer* renderer) {
	TextureBuffer* texture_buffer = 0;
	if(!temp) texture_buffer = PushStruct(renderer->permanent_arena, TextureBuffer);
	else texture_buffer = PushStruct(renderer->frame_arena, TextureBuffer);

	HRESULT hr = {};
	ID3D11ShaderResourceView* view;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	DXGI_FORMAT format;

	if(num_components == 4) format = DXGI_FORMAT_R8G8B8A8_UNORM;
	else if(num_components == 1) format = DXGI_FORMAT_R8_UNORM;
	else Assert(false);

	D3D11_USAGE usage_flag = D3D11_USAGE_IMMUTABLE; 
	u32 cpu_access_flags = 0;
	if(dynamic) {
		usage_flag = D3D11_USAGE_DYNAMIC; 
		cpu_access_flags |= D3D11_CPU_ACCESS_WRITE;
	}

	desc.Format = format;
	desc.SampleDesc.Count = renderer->msaa_sample_count;
	desc.SampleDesc.Quality = renderer->msaa_quality_level;
	desc.Usage = usage_flag;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = cpu_access_flags;

	D3D11_SUBRESOURCE_DATA sr = {};
	sr.pSysMem = data;
	sr.SysMemPitch = width * num_components;

	ID3D11Texture2D* buffer;
	if(data) hr = renderer->device->CreateTexture2D(&desc, &sr, &buffer);
	else hr = renderer->device->CreateTexture2D(&desc, 0, &buffer);

	// RESEARCH: view examples
	D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {};
	view_desc.Format = desc.Format;
	view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MostDetailedMip = 0;
	view_desc.Texture2D.MipLevels = (u32)-1;
	
	hr = renderer->device->CreateShaderResourceView(buffer, &view_desc, &view);
	AssertHR(hr);

	texture_buffer->buffer = buffer;
	texture_buffer->view = view;
	return texture_buffer;
}

static IndexBuffer*
UploadIndexBuffer(void* data, u32 count, Renderer* renderer) {
	IndexBuffer* index_buffer = PushStruct(renderer->permanent_arena, IndexBuffer);

	ID3D11Buffer* buffer;
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = sizeof(u32) * count;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA sr = {};
	sr.pSysMem = data;
	renderer->device->CreateBuffer(&desc, &sr, &buffer);

	index_buffer->buffer = buffer;
	return index_buffer;
}

static VertexBuffer* 
UploadVertexBuffer(void* initial_data, u32 num_vertices, u8 num_components, bool dynamic, Renderer* renderer) {
	HRESULT hr;
	VertexBuffer* vb = PushStruct(renderer->permanent_arena, VertexBuffer);

	ID3D11Buffer* buffer;
	D3D11_BUFFER_DESC desc = {};

	u32 component_width = sizeof(float);

	D3D11_USAGE usage_flag = D3D11_USAGE_IMMUTABLE; 
	u32 cpu_access_flags = 0;
	if(dynamic) {
		usage_flag = D3D11_USAGE_DYNAMIC; 
		cpu_access_flags |= D3D11_CPU_ACCESS_WRITE;
	}
	
	desc.ByteWidth = num_components * component_width * num_vertices;
	desc.Usage = usage_flag;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = cpu_access_flags;

	if(initial_data) {
		D3D11_SUBRESOURCE_DATA sr = {};
		sr.pSysMem = initial_data;
		hr = renderer->device->CreateBuffer(&desc, &sr, &buffer);
	}
	else hr = renderer->device->CreateBuffer(&desc, 0, &buffer);
	AssertHR(hr);

	vb->buffer = buffer;

	return vb;
}

static Renderer*
InitRenderer(Win32Window* window, MemoryArena* parent_arena, MemoryArena* frame_arena) {
	Renderer* renderer;
	renderer = PushStruct(parent_arena, Renderer);
	renderer->permanent_arena = parent_arena;
	renderer->frame_arena = frame_arena;
	renderer->command_buffer_size = Megabytes(2);

	renderer->window_dim = window->dim;

	HRESULT hr = {};
	u32 msaa_quality_level = 0;
	u32 msaa_sample_count = 1;
	{
		u32 flags = 0;
		IDXGIFactory4* dxgi_factory;
		hr = CreateDXGIFactory2(flags, IID_IDXGIFactory4, (void**)&dxgi_factory);
		AssertHR(hr);

		flags = 0;
		flags = DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER;
		dxgi_factory->MakeWindowAssociation(window->handle, flags);

		IDXGIAdapter1* adapters[4] = { NULL };
		u32 adapter_count = 0;

		while ((adapter_count < ArrayCount(adapters) -1) &&
				(dxgi_factory->EnumAdapters1(adapter_count, &adapters[adapter_count])
				 != DXGI_ERROR_NOT_FOUND)) adapter_count++;

		u32 adapter_to_use = 0;
		u64 video_memory_available = 0;
		for (u32 i = 0; i < adapter_count; i++) {
			DXGI_ADAPTER_DESC1 desc = { NULL };
			adapters[i]->GetDesc1(&desc);
			if (desc.DedicatedVideoMemory > video_memory_available) {
				video_memory_available = desc.DedicatedVideoMemory;
				adapter_to_use = i;
			}
		}

		D3D_FEATURE_LEVEL target_feature_levels[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_1
		};
		D3D_FEATURE_LEVEL out_feature_levels = D3D_FEATURE_LEVEL_9_1;

		flags = 0;
		flags |= D3D11_CREATE_DEVICE_DEBUG;
		hr = D3D11CreateDevice((IDXGIAdapter*)adapters[adapter_to_use], D3D_DRIVER_TYPE_UNKNOWN, NULL,
				flags, target_feature_levels, ArrayCount(target_feature_levels), D3D11_SDK_VERSION,
				&renderer->device, &out_feature_levels, &renderer->context);

		AssertHR(hr);
		hr = renderer->device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 
				msaa_sample_count, &msaa_quality_level);
		msaa_quality_level--;

		renderer->msaa_sample_count = msaa_sample_count;
		renderer->msaa_quality_level = msaa_quality_level;

		DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
		swapchain_desc.Width = 0;
		swapchain_desc.Height = 0;
		swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchain_desc.Stereo = 0;
		//TODO:RENDERER Multisampling
		swapchain_desc.SampleDesc.Count = msaa_sample_count;
		swapchain_desc.SampleDesc.Quality = msaa_quality_level;
		swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		//TODO:RENDERER Swapchain buffer counts
		swapchain_desc.BufferCount = 2;
		swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
		swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		hr = dxgi_factory->CreateSwapChainForHwnd((IUnknown*)renderer->device, window->handle, &swapchain_desc, NULL, NULL, &renderer->swapchain);

		AssertHR(hr);
		//dxgi_factory->Release();

		hr = renderer->swapchain->QueryInterface(IID_PPV_ARGS(&renderer->swapchain));
		AssertHR(hr);
	}
	{
		ID3D11Texture2D* rtv_tex;
		ID3D11RenderTargetView* rtv;
		hr = renderer->swapchain->GetBuffer(0, IID_PPV_ARGS(&rtv_tex));
		D3D11_TEXTURE2D_DESC rtv_tex_desc = {}; 
		rtv_tex->GetDesc(&rtv_tex_desc);

		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		hr = renderer->device->CreateRenderTargetView((ID3D11Resource*)rtv_tex, &rtv_desc, &rtv);
		AssertHR(hr);

		renderer->readable_render_target = CreateReadableRenderTarget(renderer);

		ID3D11Texture2D* dsv_tex;
		ID3D11DepthStencilView* dsv;
		D3D11_TEXTURE2D_DESC dsv_tex_desc;
		rtv_tex->GetDesc(&dsv_tex_desc);
		dsv_tex_desc.MipLevels = 1;
		dsv_tex_desc.ArraySize = 1;
		dsv_tex_desc.Format = DXGI_FORMAT_D32_FLOAT;
		dsv_tex_desc.Usage = D3D11_USAGE_DEFAULT;
		dsv_tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		dsv_tex_desc.SampleDesc.Count = msaa_sample_count;
		dsv_tex_desc.SampleDesc.Quality = msaa_quality_level;

		hr = renderer->device->CreateTexture2D(&dsv_tex_desc, NULL, &dsv_tex);
		AssertHR(hr);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
		dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		hr = renderer->device->CreateDepthStencilView((ID3D11Resource*)dsv_tex, &dsv_desc, &dsv);
		AssertHR(hr);

		renderer->backbuffer.texture = rtv_tex;
		renderer->backbuffer.view = rtv;
		renderer->depth_stencil.texture = dsv_tex;
		renderer->depth_stencil.view = dsv;
	}
	{ //(FC) = (SP) (X) (SBF) (+) (DP) (X) (DPF)
		//(FA) = (SA)(SBF) (+) (DA)(DBF)
		// No blend
		// TODO:RENDERER Multiple render targets bind in OM Stage
		D3D11_BLEND_DESC bs_desc = {};
		bs_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		bs_desc.RenderTarget[0].BlendEnable = false;

		hr = renderer->device->CreateBlendState(&bs_desc, &renderer->blend_states[BLEND_STATE_NoBlend]);
		AssertHR(hr);
	}
	{ // Regular
		D3D11_BLEND_DESC bs_desc = {};
		bs_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		bs_desc.RenderTarget[0].BlendEnable = true;
		bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bs_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bs_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		bs_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

		hr = renderer->device->CreateBlendState(&bs_desc, &renderer->blend_states[BLEND_STATE_Regular]);
		AssertHR(hr);
	}
	{ // Premultiplied Alpha
		D3D11_BLEND_DESC bs_desc = {};
		bs_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		bs_desc.RenderTarget[0].BlendEnable = true;
		bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		bs_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		bs_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		bs_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

		hr = renderer->device->CreateBlendState(&bs_desc, &renderer->blend_states[BLEND_STATE_PreMulAlpha]);
		AssertHR(hr);
	}
	{
		ID3D11DepthStencilState* dss;
		D3D11_DEPTH_STENCIL_DESC dss_desc = {};
		dss_desc.DepthEnable = true;
		dss_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dss_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		hr = renderer->device->CreateDepthStencilState(&dss_desc, &dss);
		renderer->default_depth_stencil_state = dss;
		AssertHR(hr);
	}
	{ 
		// Default Rasterizer
		ID3D11RasterizerState* rs;
		D3D11_RASTERIZER_DESC rs_desc;
		rs_desc.FillMode = D3D11_FILL_SOLID;
		rs_desc.CullMode = D3D11_CULL_BACK;
		rs_desc.FrontCounterClockwise = true;
		rs_desc.MultisampleEnable = true;
		hr = renderer->device->CreateRasterizerState(&rs_desc, &rs);
		renderer->rasterizer_states[RASTERIZER_STATE_Default] = rs;
		AssertHR(hr);

		// Wireframe
		rs_desc.FillMode = D3D11_FILL_WIREFRAME;
		hr = renderer->device->CreateRasterizerState(&rs_desc, &rs);
		renderer->rasterizer_states[RASTERIZER_STATE_Wireframe] = rs;
		AssertHR(hr);

		rs_desc = {};
		rs_desc.FillMode = D3D11_FILL_SOLID;
		rs_desc.CullMode = D3D11_CULL_NONE;
		rs_desc.FrontCounterClockwise = true;
		hr = renderer->device->CreateRasterizerState(&rs_desc, &rs);
		renderer->rasterizer_states[RASTERIZER_STATE_DoubleSided] = rs;
		AssertHR(hr);
	}
	{ // default
		ID3D11SamplerState* ss;
		D3D11_SAMPLER_DESC desc = {};
		desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		desc.MaxLOD = D3D11_FLOAT32_MAX;
		hr = renderer->device->CreateSamplerState(&desc, &ss);
		AssertHR(hr);
		renderer->samplers[SAMPLER_STATE_Default] = ss;
	}


	return renderer;
}



