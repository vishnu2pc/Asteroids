#include "renderer.h"

//------------------------------------------------------------------------
static void RendererBeginFrame(Renderer* renderer, WindowDimensions wd) {
	renderer->transient.used = 0;
	renderer->vp[VIEWPORT_DEFAULT].Width = wd.width;
	renderer->vp[VIEWPORT_DEFAULT].Height = wd.height;

	float color[4] = { 0.25f, 0.25f, 0.25f, 1.0f };
	renderer->context->ClearRenderTargetView(renderer->rtv, color);
	renderer->context->ClearDepthStencilView(renderer->dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
	renderer->context->OMSetRenderTargets(1, &renderer->rtv, renderer->dsv);
	renderer->context->PSSetSamplers(0, 1, &renderer->ss[SAMPLER_STATE_DEFAULT]);
}

//------------------------------------------------------------------------
static void 
PushConstantsData(void* data, ConstantsBuffer* cb, ID3D11DeviceContext* context) {
	Assert(data);
	Assert(cb->buffer);

	D3D11_MAPPED_SUBRESOURCE msr = {};

	context->Map(cb->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy(msr.pData, data, cb->size);
	context->Unmap(cb->buffer, 0);

	data = nullptr;
}

static void 
PushStructuredData(PushStructuredBufferData* sb_data, StructuredBuffer* sb, ID3D11DeviceContext* context) {
	Assert(sb_data->data);
	Assert(sb->buffer);

	D3D11_MAPPED_SUBRESOURCE msr = {};

	context->Map(sb->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	CopyMem(msr.pData, sb_data->data, sb->struct_size*sb_data->count);
	context->Unmap(sb->buffer, 0);

	sb_data->data = nullptr;
}

static void 
PushIndexData(PushIndexBufferData* data, IndexBuffer* ib, ID3D11DeviceContext* context) {
	Assert(data);
	Assert(ib->buffer);

	D3D11_MAPPED_SUBRESOURCE msr = {};

	context->Map(ib->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	CopyMem(msr.pData, data->data, sizeof(u32)*data->indices_count);
	context->Unmap(ib->buffer, 0);

	data = nullptr;
}

static void 
PushVertexData(PushVertexBufferData* data, VertexBuffer* vb, ID3D11DeviceContext* context) {
	Assert(data);
	Assert(vb->buffer);

	D3D11_MAPPED_SUBRESOURCE msr = {};

	context->Map(vb->buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	CopyMem(msr.pData, data->data, vb->stride*data->vertices_count);
	context->Unmap(vb->buffer, 0);

	data = nullptr;
}

//------------------------------------------------------------------------
static void ExecuteRenderPipeline(RenderPipeline rp, Renderer* renderer) {
	if(rp.rs.command == RENDER_COMMAND_CLEAR_DEPTH_STENCIL)
		renderer->context->ClearDepthStencilView(renderer->dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

	DEPTH_STENCIL_STATE dss_t = renderer->state_overrides.dss ? renderer->state_overrides.dss : rp.rs.dss;
	BLEND_STATE bs_t          = renderer->state_overrides.bs ? renderer->state_overrides.bs : rp.rs.bs;
	RASTERIZER_STATE rs_t     = renderer->state_overrides.rs ? renderer->state_overrides.rs : rp.rs.rs;
	VIEWPORT vp_t             = renderer->state_overrides.vp ? renderer->state_overrides.vp : rp.rs.vp;
	//------------------------------------------------------------------------
	if(rp.rs.dss) {
		ID3D11DepthStencilState* dss = renderer->dss[dss_t];
		renderer->context->OMSetDepthStencilState(dss, 0);
	}
	//------------------------------------------------------------------------
	if(rp.rs.bs) {
		ID3D11BlendState* bs = renderer->bs[bs_t];
		float val[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		renderer->context->OMSetBlendState(bs, val, 0xFFFFFFFF);
	}
	//------------------------------------------------------------------------=
	if(rp.rs.rs) {
		ID3D11RasterizerState* rs = renderer->rs[rs_t];
		renderer->context->RSSetState(rs);
	}
	//------------------------------------------------------------------------
	if(rp.rs.vp) {
		D3D11_VIEWPORT vp = renderer->vp[vp_t];
		renderer->context->RSSetViewports(1, &vp);
		D3D11_RECT rect = {};
		rect.right = vp.Width;
		rect.bottom = vp.Height;
		renderer->context->RSSetScissorRects(1, &rect);
	}
	renderer->context->IASetPrimitiveTopology(rp.rs.topology);
	//------------------------------------------------------------------------
	if(rp.ps) {
		PixelShader* ps = rp.ps;
		renderer->context->PSSetShader(ps->shader, nullptr, 0);
		for(u8 i=0; i<ps->rb_count; i++) 
			if(ps->rb[i].type == RENDER_BUFFER_TYPE_CONSTANTS) 
				renderer->context->PSSetConstantBuffers(ps->rb[i].constants.slot, 1, &ps->rb[i].constants.buffer);
		
		for(u8 i=0; i<rp.prbd_count; i++) 
			if(rp.prbd[i].type == RENDER_BUFFER_TYPE_CONSTANTS) 
				for(u8 j=0; j<ps->rb_count; j++) 
					if(ps->rb[j].type == RENDER_BUFFER_TYPE_CONSTANTS)
						if(rp.prbd[i].constants.slot == ps->rb[j].constants.slot)
							PushConstantsData(rp.prbd[i].constants.data, &ps->rb[j].constants, renderer->context);

		RenderBufferGroup* rbg = rp.prbg;
		if(rbg) {
			for(u8 i=0; i<ps->texture_count; i++) {
				bool found = false;
				for(u8 j=0; j<rbg->count; j++) {
					if(ps->texture_slot[i] == rbg->rb[i].texture.slot) {
						found = true;
						renderer->context->PSSetShaderResources(rbg->rb[i].texture.slot, 1, &rbg->rb[i].texture.view);
					}
				}
				Assert(found);
			}
		}
	}
	//------------------------------------------------------------------------
	if(rp.vs) {
		VertexShader* vs = rp.vs;
		RenderBuffer* vrb = vs->rb;
		u8 vrb_count = vs->rb_count;

		renderer->context->VSSetShader(vs->shader, nullptr, 0);
		renderer->context->IASetInputLayout(vs->il);

		for(u8 i=0; i<vrb_count; i++) {
			if(vrb[i].type == RENDER_BUFFER_TYPE_CONSTANTS) 
				renderer->context->VSSetConstantBuffers(vrb[i].constants.slot, 1, &vrb[i].constants.buffer);
			if(vrb[i].type == RENDER_BUFFER_TYPE_STRUCTURED) 
				renderer->context->VSSetShaderResources(vrb[i].structured.slot, 1, &vrb[i].structured.view);
		}

		PushRenderBufferData* data = rp.vrbd;
		u8 data_count = rp.vrbd_count;

		RenderBufferGroup* rbg = rp.vrbg;

		for(u8 i=0; i<data_count; i++) {
			switch(data[i].type) {
				case RENDER_BUFFER_TYPE_CONSTANTS: {
					for(u8 j=0; j<vrb_count; j++) {

						if(vrb[j].type == RENDER_BUFFER_TYPE_CONSTANTS && 
							 data[i].constants.slot == vrb[j].constants.slot)

								PushConstantsData(data[i].constants.data, &vrb[j].constants, renderer->context);
					}
				} break;

				case RENDER_BUFFER_TYPE_STRUCTURED: {
					for(u8 j=0; j<vrb_count; j++) {

						if(vrb[j].type == RENDER_BUFFER_TYPE_STRUCTURED &&
							 data[i].structured.slot == vrb[j].structured.slot) 

								PushStructuredData(&data[i].structured, &vrb[j].structured, renderer->context);
					}
				} break;

				case RENDER_BUFFER_TYPE_VERTEX: {
					for(u8 j=0; j<rbg->count; j++) {

						if(rbg->rb[j].type == RENDER_BUFFER_TYPE_VERTEX &&
							 data[i].vertex.type == rbg->rb[j].vertex.type)

							PushVertexData(&data[i].vertex, &rbg->rb[j].vertex, renderer->context);
					}
				} break;

				case RENDER_BUFFER_TYPE_INDEX: {
					for(u8 j=0; j<rbg->count; j++) {

						if(rbg->rb[j].type == RENDER_BUFFER_TYPE_INDEX)
							PushIndexData(&data[i].index, &rbg->rb[j].index, renderer->context);
					}

				}
			}
		}

		if(rp.vrbg) {
			for(u8 i=0; i<vs->vb_count; i++) {
				bool found = false;
				for(u8 j=0; j<rbg->count; j++) {
					if(rbg->rb[j].type == RENDER_BUFFER_TYPE_VERTEX && rbg->rb[j].vertex.type == vs->vb_type[i]) {
						found = true;	u32 offset = 0;
						renderer->context->IASetVertexBuffers(i, 1, &rbg->rb[j].vertex.buffer, &rbg->rb[j].vertex.stride, &offset);
					}
				}
				Assert(found);
			}

			if(rp.dc.type == DRAW_CALL_DEFAULT) {
				bool has_index_buffer = false;
				for(u8 i=0; i<rbg->count; i++) {
					if(rbg->rb[i].type == RENDER_BUFFER_TYPE_INDEX) {
						renderer->context->IASetIndexBuffer(rbg->rb[i].index.buffer, DXGI_FORMAT_R32_UINT, 0);
						has_index_buffer = true;
						renderer->context->DrawIndexed(rbg->rb[i].index.num_indices, 0, 0);
					}
				}

				if(!has_index_buffer) {
					for(u8 i=0; i<rbg->count; i++) 
						if(rbg->rb[i].type == RENDER_BUFFER_TYPE_VERTEX) {
							renderer->context->Draw(rbg->rb[i].vertex.vertices_count, 0);
							break;
						}
				}

			}
		}
	}
	//------------------------------------------------------------------------
	{
		if(rp.dc.type == DRAW_CALL_INDEXED)
			renderer->context->DrawIndexed(rp.dc.indices_count, rp.dc.offset, 0);
		if(rp.dc.type == DRAW_CALL_VERTICES)
			renderer->context->Draw(rp.dc.vertices_count, rp.dc.offset);
		if(rp.dc.type == DRAW_CALL_INSTANCED) 
			renderer->context->DrawInstanced(rp.dc.vertices_count, rp.dc.instance_count, 0, 0);
	}
}

//------------------------------------------------------------------------
static void RendererFrame(Renderer* renderer) {
	for(u32 i=0; i<renderer->rq_id; i++) ExecuteRenderPipeline(renderer->rq[i], renderer);
}

//------------------------------------------------------------------------
static void RendererEndFrame(Renderer* renderer) {
	renderer->swapchain->Present(0, 0);
	renderer->rq_id = 0;
}
//-------------------------------------------------------------------------
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

//------------------------------------------------------------------------
static ConstantsBuffer 
UploadConstantsBuffer(ConstantsBufferDesc desc, ID3D11Device* device) {
	HRESULT hr = {};
	ConstantsBuffer cb = {};
	ID3D11Buffer* buffer = 0;

	desc.size += (16 - (desc.size % 16));
	D3D11_BUFFER_DESC cb_desc = {};
	cb_desc.ByteWidth 		 = desc.size;
	cb_desc.Usage          = D3D11_USAGE_DYNAMIC;
	cb_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = device->CreateBuffer(&cb_desc, nullptr, &buffer);

	AssertHR(hr);

	cb.buffer = buffer;
	cb.slot = desc.slot;
	cb.size = desc.size;
	return cb;
}

//------------------------------------------------------------------------
static StructuredBuffer 
UploadStructuredBuffer(StructuredBufferDesc desc, ID3D11Device* device) {
	HRESULT hr = {};
	StructuredBuffer sb = {};

	ID3D11Buffer* buffer = 0;
	ID3D11ShaderResourceView* view = 0;

	D3D11_BUFFER_DESC buffer_desc = {};
	buffer_desc.ByteWidth = desc.struct_size_in_bytes * desc.count;
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	buffer_desc.StructureByteStride = desc.struct_size_in_bytes;

	hr = device->CreateBuffer(&buffer_desc, NULL, &buffer);
	AssertHR(hr);

	D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {};
	view_desc.Format = DXGI_FORMAT_UNKNOWN;
	view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	view_desc.Buffer.ElementOffset = 0;
	view_desc.Buffer.ElementWidth = desc.count;

	hr = device->CreateShaderResourceView(buffer, &view_desc, &view);
	AssertHR(hr);

	sb.slot = desc.slot;
	sb.buffer = buffer;
	sb.view = view; 
	sb.struct_size = desc.struct_size_in_bytes;
	sb.count = desc.count;

	return sb;
};

//------------------------------------------------------------------------
static PixelShader* 
UploadPixelShader(PixelShaderDesc desc, Renderer* renderer) {
	HRESULT hr = {};

	PixelShader* ps = PushStruct(&renderer->permanent, PixelShader);
	ID3DBlob* blob;
	ID3DBlob* error;
	ID3D11PixelShader* shader;
	TEXTURE_SLOT* texture_slot = nullptr;

	u8 rb_count = desc.cb_count;
	RenderBuffer* rb = PushArray(&renderer->permanent, RenderBuffer, rb_count);

	hr = D3DCompile(desc.shader.code, desc.shader.size, nullptr, nullptr, nullptr, desc.shader.entry, "ps_5_0",
																			0, 0, &blob, &error);
	if (hr) {
		char* msg = (char*)error->GetBufferPointer();
		OutputDebugStringA(msg);
	}
	hr = renderer->device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);
	AssertHR(hr);

	for(u8 i=0; i<desc.cb_count; i++) {
		rb[i].type = RENDER_BUFFER_TYPE_CONSTANTS;
		rb[i].constants = UploadConstantsBuffer(desc.cb_desc[i], renderer->device);
	}

	if(desc.texture_count) {
		texture_slot = PushArray(&renderer->permanent, TEXTURE_SLOT, desc.texture_count);
		memcpy(texture_slot, desc.texture_slot, sizeof(TEXTURE_SLOT)*desc.texture_count);
	}

	ps->shader = shader;
	ps->rb = rb;
	ps->texture_slot = texture_slot;
	ps->rb_count = rb_count;
	ps->texture_count = desc.texture_count;

	return ps;
}

//------------------------------------------------------------------------
static VertexShader* 
UploadVertexShader(VertexShaderDesc desc, Renderer* renderer) {
	HRESULT hr = {};

	VertexShader* vs = PushStruct(&renderer->permanent, VertexShader);
	ID3DBlob* blob;
	ID3DBlob* error;
	ID3D11VertexShader* shader = 0;
	ID3D11InputLayout* il = 0;
	VERTEX_BUFFER* vb_type = 0;

	u8 rb_count = desc.cb_count + desc.sb_count;
	RenderBuffer* rb = PushArray(&renderer->permanent, RenderBuffer, rb_count);

	hr = D3DCompile(desc.shader.code, desc.shader.size, nullptr, nullptr, nullptr, desc.shader.entry, "vs_5_0",
																			0, 0, &blob, &error);
	if (hr) {
		char* msg = (char*)error->GetBufferPointer();
		OutputDebugStringA(msg);
	}
	hr = renderer->device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);
	AssertHR(hr);

	if(desc.vb_count) {
		D3D11_INPUT_ELEMENT_DESC* ie_desc = PushArray(&renderer->transient, D3D11_INPUT_ELEMENT_DESC, desc.vb_count);
		MakeD3DInputElementDesc(desc.vb_type, ie_desc, desc.vb_count);

		hr = renderer->device->CreateInputLayout(ie_desc, desc.vb_count, blob->GetBufferPointer(), blob->GetBufferSize(), &il);

		vb_type = PushArray(&renderer->permanent, VERTEX_BUFFER, desc.vb_count);
		memcpy(vb_type, desc.vb_type, sizeof(VERTEX_BUFFER)*desc.vb_count);
		AssertHR(hr);
		PopArray(&renderer->transient, D3D11_INPUT_ELEMENT_DESC, desc.vb_count);
	}

	u8 i=0;
	for(i=0; i<desc.cb_count; i++) {
		rb[i].type = RENDER_BUFFER_TYPE_CONSTANTS;
		rb[i].constants = UploadConstantsBuffer(desc.cb_desc[i], renderer->device);
	}

	for(u8 j=0; j<desc.sb_count; j++) {
		rb[i+j].type = RENDER_BUFFER_TYPE_STRUCTURED;
		rb[i+j].structured = UploadStructuredBuffer(desc.sb_desc[j], renderer->device);
	}

	vs->shader = shader;
	vs->il = il;
	vs->vb_type = vb_type;
	vs->rb = rb;
	vs->rb_count = rb_count;
	vs->vb_count = desc.vb_count;

	return vs;
}

//------------------------------------------------------------------------
static RenderBuffer* 
UploadTexture(TextureData* texture_data, Renderer* renderer) {
	RenderBuffer* texture_buffer = PushStruct(&renderer->permanent, RenderBuffer);

	HRESULT hr = {};
	ID3D11ShaderResourceView* view;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = texture_data->width;
	desc.Height = texture_data->height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	DXGI_FORMAT format;

	if(texture_data->num_components == 4) format = DXGI_FORMAT_R8G8B8A8_UNORM;
	else Assert(false);

	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sr = {};
	sr.pSysMem = texture_data->pixels;
	sr.SysMemPitch = texture_data->width * texture_data->num_components;

	ID3D11Texture2D* buffer;
	hr = renderer->device->CreateTexture2D(&desc, &sr, &buffer);

	// RESEARCH: view examples
	D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {};
	view_desc.Format = desc.Format;
	view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MostDetailedMip = 0;
	view_desc.Texture2D.MipLevels = (u32)-1;
	
	hr = renderer->device->CreateShaderResourceView(buffer, &view_desc, &view);
	AssertHR(hr);

	texture_buffer->type = RENDER_BUFFER_TYPE_TEXTURE;
	texture_buffer->texture.slot = texture_data->type;
	texture_buffer->texture.buffer = buffer;
	texture_buffer->texture.view = view;
	return texture_buffer;
}

//------------------------------------------------------------------------
static RenderBufferGroup* 
UploadTextureFromFile(char* filepath, TEXTURE_SLOT slot, Renderer* renderer) {
	RenderBufferGroup* result;
	int x, y, n;
	void* png = stbi_load(filepath, &x, &y, &n, 4);
	Assert(png);

	TextureData texture_data = { TEXTURE_SLOT_DIFFUSE, png, (u32)x, (u32)y, 4 };

	RenderBuffer* rb = UploadTexture(&texture_data, renderer);
	STBI_FREE(png);

	result = PushStruct(&renderer->permanent, RenderBufferGroup);
	result->rb = rb;
	result->count = 1;
	return result;
}

//------------------------------------------------------------------------
static VertexBuffer 
UploadVertexBuffer(VertexBufferData vb_data, u32 num_vertices, bool dynamic, ID3D11Device* device) {
	HRESULT hr;
	VertexBuffer vb = {};

	ID3D11Buffer* buffer;
	D3D11_BUFFER_DESC desc = {};
	u32 num_components = 0;
	if(vb_data.type == VERTEX_BUFFER_POSITION) num_components = 3;
	else if(vb_data.type == VERTEX_BUFFER_COLOR) num_components = 3;
	else if(vb_data.type == VERTEX_BUFFER_NORMAL) num_components = 3;
	else if(vb_data.type == VERTEX_BUFFER_TEXCOORD) num_components = 2;
	else if(vb_data.type == VERTEX_BUFFER_TANGENT) num_components = 4;
	else Assert(false);

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

	D3D11_SUBRESOURCE_DATA sr = {};
	sr.pSysMem = vb_data.data;
	hr = device->CreateBuffer(&desc, &sr, &buffer);
	AssertHR(hr);

	vb.buffer = buffer;
	vb.stride = num_components * component_width;
	vb.type = vb_data.type;
	vb.vertices_count = num_vertices;

	return vb;
}

//------------------------------------------------------------------------
static RenderBufferGroup* 
UploadMesh(MeshData* mesh_data, MeshDesc* mesh_desc, Renderer* renderer) {
	Assert(mesh_data);
	RenderBufferGroup* rbg = PushStruct(&renderer->permanent, RenderBufferGroup);

	bool has_indices = false;
	if(mesh_data->indices) has_indices = true;
	RenderBuffer* rb = PushArray(&renderer->permanent, RenderBuffer, mesh_data->vb_data_count+(u32)has_indices);

	u8 i=0;
	for(i=0; i<mesh_data->vb_data_count; i++) {
		bool dynamic = false;

		if(mesh_desc) {
			for(u8 j=0; j<mesh_desc->vb_count; j++) 
				if(mesh_data->vb_data[i].type == mesh_desc->vb_desc[j].type)
					dynamic = mesh_desc->vb_desc[j].dynamic;
		}

		rb[i].type = RENDER_BUFFER_TYPE_VERTEX;
		rb[i].vertex = UploadVertexBuffer(mesh_data->vb_data[i], mesh_data->vertices_count, dynamic, renderer->device);
	}

	if(has_indices) {
		ID3D11Buffer* index_buffer;
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(u32) * mesh_data->indices_count;

		D3D11_USAGE usage_flag = D3D11_USAGE_IMMUTABLE; 
		u32 cpu_access_flags = 0;
		if(mesh_desc && mesh_desc->dynamic_index_buffer) {
			usage_flag = D3D11_USAGE_DYNAMIC; 
			cpu_access_flags |= D3D11_CPU_ACCESS_WRITE;
		}

		//TODO:Renderer Clean this up?
		desc.Usage = usage_flag;
		desc.BindFlags = cpu_access_flags;
		D3D11_SUBRESOURCE_DATA sr = {};
		sr.pSysMem = mesh_data->indices;
		renderer->device->CreateBuffer(&desc, &sr, &index_buffer);

		rb[i].type = RENDER_BUFFER_TYPE_INDEX;
		rb[i].index.buffer = index_buffer; 
		rb[i].index.num_indices = mesh_data->indices_count;
	}

	rbg->rb = rb;
	rbg->count = mesh_data->vb_data_count + (u32)has_indices;
	return rbg;
}


//---------------------------------------------------------------------------
static Renderer* InitRenderer(Win32Window* window, GameAssets* assets, MemoryArena* arena) {
	Renderer* renderer;
	u8* memblock = GetMemory(arena, Megabytes(5));
	renderer = (Renderer*)memblock;

	renderer->permanent.memory.bp = memblock;
	renderer->permanent.memory.size = Megabytes(3);
	renderer->permanent.used = sizeof(Renderer);

	renderer->transient.memory.bp = memblock + Megabytes(3);
	renderer->transient.memory.size = Megabytes(2);

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

		hr = D3D11CreateDevice((IDXGIAdapter*)adapters[adapter_to_use], D3D_DRIVER_TYPE_UNKNOWN, NULL,
				flags, target_feature_levels, ArrayCount(target_feature_levels), D3D11_SDK_VERSION,
				&renderer->device, &out_feature_levels, &renderer->context);

		AssertHR(hr);
		hr = renderer->device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 
				msaa_sample_count, &msaa_quality_level);
		msaa_quality_level--;

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
		swapchain_desc.Flags = 0;

		hr = dxgi_factory->CreateSwapChainForHwnd((IUnknown*)renderer->device, window->handle, &swapchain_desc, NULL, NULL, &renderer->swapchain);

		AssertHR(hr);
		//dxgi_factory->Release();

		hr = renderer->swapchain->QueryInterface(IID_PPV_ARGS(&renderer->swapchain));
		AssertHR(hr);
	}
	//--------------------------Render Target and Depth Stencil View----------------------------------------------
	{
		ID3D11Texture2D* rtv_tex;
		hr = renderer->swapchain->GetBuffer(0, IID_PPV_ARGS(&rtv_tex));
		D3D11_TEXTURE2D_DESC rtv_tex_desc = {}; 
		rtv_tex->GetDesc(&rtv_tex_desc);

		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		hr = renderer->device->CreateRenderTargetView((ID3D11Resource*)rtv_tex, &rtv_desc, &renderer->rtv);
		AssertHR(hr);

		ID3D11Texture2D* dsv_tex;
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
		hr = renderer->device->CreateDepthStencilView((ID3D11Resource*)dsv_tex, &dsv_desc, &renderer->dsv);
		AssertHR(hr);
	}
	//---------------------------Blend state---------------------------------------------
	{ // final.xyz = (src.xyz * src_blend) (BlendOp) (dest.rgb * dest_blend)
		{	// No blend
			// TODO:RENDERER Multiple render targets bind in OM Stage
			D3D11_BLEND_DESC bs_desc = {};
			bs_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			bs_desc.RenderTarget[0].BlendEnable = false;

			hr = renderer->device->CreateBlendState(&bs_desc, &renderer->bs[BLEND_STATE_NO_BLEND]);
			AssertHR(hr);
		}
		{ // Enabled
			D3D11_BLEND_DESC bs_desc = {};
			bs_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			bs_desc.RenderTarget[0].BlendEnable = true;
			bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			bs_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			bs_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
			bs_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;


			hr = renderer->device->CreateBlendState(&bs_desc, &renderer->bs[BLEND_STATE_ENABLED]);
			AssertHR(hr);
		}
	}
	//---------------------------Depth Stencil State---------------------------------------------
	{
		ID3D11DepthStencilState* dss;
		D3D11_DEPTH_STENCIL_DESC dss_desc = {};
		dss_desc.DepthEnable = true;
		dss_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dss_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		hr = renderer->device->CreateDepthStencilState(&dss_desc, &dss);
		renderer->dss[DEPTH_STENCIL_STATE_DEFAULT] = dss;
		AssertHR(hr);
	}
	//----------------------------Rasterizer--------------------------------------------
	{ 
		// Default Rasterizer
		ID3D11RasterizerState* rs;
		D3D11_RASTERIZER_DESC rs_desc;
		rs_desc.FillMode = D3D11_FILL_SOLID;
		rs_desc.CullMode = D3D11_CULL_BACK;
		rs_desc.FrontCounterClockwise = true;
		rs_desc.MultisampleEnable = true;
		hr = renderer->device->CreateRasterizerState(&rs_desc, &rs);
		renderer->rs[RASTERIZER_STATE_DEFAULT] = rs;
		AssertHR(hr);

		// Wireframe
		rs_desc.FillMode = D3D11_FILL_WIREFRAME;
		hr = renderer->device->CreateRasterizerState(&rs_desc, &rs);
		renderer->rs[RASTERIZER_STATE_WIREFRAME] = rs;
		AssertHR(hr);

		rs_desc = {};
		rs_desc.FillMode = D3D11_FILL_SOLID;
		rs_desc.CullMode = D3D11_CULL_NONE;
		rs_desc.FrontCounterClockwise = true;
		hr = renderer->device->CreateRasterizerState(&rs_desc, &rs);
		renderer->rs[RASTERIZER_STATE_DOUBLE_SIDED] = rs;
		AssertHR(hr);
	}
	//-----------------------------Viewport-------------------------------------------
	{	
		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = window->dim.width;
		vp.Height = window->dim.height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		renderer->vp[VIEWPORT_DEFAULT] = vp;
	}
	//------------------------Samplers------------------------------------------------
	{
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
			renderer->ss[SAMPLER_STATE_DEFAULT] = ss;
		}
		{ // Tiling
			ID3D11SamplerState* ss;
			D3D11_SAMPLER_DESC desc = {};
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			desc.MaxLOD = D3D11_FLOAT32_MAX;
			hr = renderer->device->CreateSamplerState(&desc, &ss);
			AssertHR(hr);
			renderer->ss[SAMPLER_STATE_TILE] = ss;
		}
	}
	return renderer;
}

