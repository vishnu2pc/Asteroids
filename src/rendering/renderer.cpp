#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <d3d11sdklayers.h>
#include <dxgidebug.h>

#include "renderer_structs.cpp"
#include "shader_structs.cpp"

//-------------------------------------------------------------------------
static void MakeD3DInputElementDesc(VERTEX_BUFFER_TYPE* vb_type, D3D11_INPUT_ELEMENT_DESC* d3d_il_desc, u8 count) {
	char* SemanticName[] = {
	  "NOT SET",
		"POSITION",
		"NORMAL",
		"TANGENT",
		"COLOR",
		"TEXCOORD"
	};

	for (u32 i = 0; i < count; i++) {

		u8 id = 0;
		DXGI_FORMAT format;
		if (vb_type[i] == VERTEX_BUFFER_TYPE_POSITION) { id = 1; format = DXGI_FORMAT_R32G32B32_FLOAT; }
		else if (vb_type[i] == VERTEX_BUFFER_TYPE_NORMAL) { id = 2; format = DXGI_FORMAT_R32G32B32_FLOAT; }
		//else if (vb_type[i] == VERTEX_BUFFER_TYPE_TANGENT) { id = 3; format = DXGI_FORMAT_R32G32B32_FLOAT; }
		else if (vb_type[i] == VERTEX_BUFFER_TYPE_TEXCOORD) { id = 5; format = DXGI_FORMAT_R32G32_FLOAT; }
		else assert(false);

		d3d_il_desc[i] = { SemanticName[id], 0, format, i, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	}	
}

//------------------------------------------------------------------------
static ConstantsBuffer UploadConstantsBuffer(ConstantsBufferDesc desc, ID3D11Device* device) {
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

	assertHR(hr);

	cb.buffer = buffer;
	cb.slot = desc.slot;
	cb.size = desc.size;
	return cb;
}

//------------------------------------------------------------------------
static StructuredBuffer UploadStructuredBuffer(StructuredBufferDesc desc, ID3D11Device* device) {
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
	assertHR(hr);

	D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {};
	view_desc.Format = DXGI_FORMAT_UNKNOWN;
	view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	view_desc.Buffer.ElementOffset = 0;
	view_desc.Buffer.ElementWidth = desc.count;

	hr = device->CreateShaderResourceView(buffer, &view_desc, &view);
	assertHR(hr);

	sb.slot = desc.slot;
	sb.buffer = buffer;
	sb.view = view; 
	sb.struct_size = desc.struct_size_in_bytes;
	sb.count = desc.count;

	return sb;
};

//------------------------------------------------------------------------
static PixelShader UploadPixelShader(PixelShaderDesc desc, Renderer* renderer) {
	HRESULT hr = {};

	PixelShader ps = {};
	ID3DBlob* blob;
	ID3DBlob* error;
	ID3D11PixelShader* shader;
	TEXTURE_SLOT* texture_slot = nullptr;

	u8 rb_count = desc.cb_count;
	RenderBuffer* rb = PushRenderBuffer(rb_count, renderer);

	hr = D3DCompileFromFile(desc.shader.path, nullptr, nullptr, desc.shader.entry, "ps_5_0",
																			0, 0, &blob, &error);
	if (hr) {
		char* msg = (char*)error->GetBufferPointer();
		if (hr) SDL_Log(msg);
	}
	hr = renderer->device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);
	assertHR(hr);

	for(u8 i=0; i<desc.cb_count; i++) {
		rb[i].type = RENDER_BUFFER_TYPE_CONSTANTS;
		rb[i].constants = UploadConstantsBuffer(desc.cb_desc[i], renderer->device);
	}

	if(desc.texture_count) {
		texture_slot = PushMaster(TEXTURE_SLOT, desc.texture_count);
		memcpy(texture_slot, desc.texture_slot, sizeof(TEXTURE_SLOT)*desc.texture_count);
	}

	ps.shader = shader;
	ps.rb = rb;
	ps.texture_slot = texture_slot;
	ps.rb_count = rb_count;
	ps.texture_count = desc.texture_count;

	return ps;
}

//------------------------------------------------------------------------
static VertexShader UploadVertexShader(VertexShaderDesc desc, Renderer* renderer) {
	HRESULT hr = {};
	VertexShader vs = {};

	ID3DBlob* blob;
	ID3DBlob* error;
	ID3D11VertexShader* shader = 0;
	ID3D11InputLayout* il = 0;
	VERTEX_BUFFER_TYPE* vb_type = 0;

	u8 rb_count = desc.cb_count + desc.sb_count;
	RenderBuffer* rb = PushRenderBuffer(rb_count, renderer);

	hr = D3DCompileFromFile(desc.shader.path, nullptr, nullptr, desc.shader.entry, "vs_5_0",
																			0, 0, &blob, &error);
	if (hr) {
		char* msg = (char*)error->GetBufferPointer();
		SDL_Log(msg);
	}
	hr = renderer->device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);
	assertHR(hr);

	if(desc.vb_count) {
		D3D11_INPUT_ELEMENT_DESC* ie_desc = PushScratch(D3D11_INPUT_ELEMENT_DESC, desc.vb_count);
		MakeD3DInputElementDesc(desc.vb_type, ie_desc, desc.vb_count);

		hr = renderer->device->CreateInputLayout(ie_desc, desc.vb_count, blob->GetBufferPointer(), blob->GetBufferSize(), &il);

		vb_type = PushMaster(VERTEX_BUFFER_TYPE, desc.vb_count);
		memcpy(vb_type, desc.vb_type, sizeof(VERTEX_BUFFER_TYPE)*desc.vb_count);
		assertHR(hr);
		PopScratch(D3D11_INPUT_ELEMENT_DESC, desc.vb_count);
	}

	u8 i=0;
	for(i=0; i<desc.cb_count; i++) {
		rb[i].type = RENDER_BUFFER_TYPE_CONSTANTS;
		rb[i].constants = UploadConstantsBuffer(desc.cb_desc[i], renderer->device);
	}

	for(u8 j=0; j<desc.sb_count; j++) {
		rb[i+j].type = RENDER_BUFFER_TYPE_STRUCTURED;
		rb[i+j].structured = UploadStructuredBuffer(desc.sb_desc[i], renderer->device);
	}

	//TODO: Handle structured buffer handling
	vs.shader = shader;
	vs.il = il;
	vs.vb_type = vb_type;
	vs.rb = rb;
	vs.rb_count = rb_count;
	vs.vb_count = desc.vb_count;

	return vs;
}

//------------------------------------------------------------------------
static TextureBuffer UploadTexture(TextureData texture_data, ID3D11Device* device) {
	TextureBuffer texture = {};

	HRESULT hr = {};
	ID3D11ShaderResourceView* view;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = texture_data.width;
	desc.Height = texture_data.height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	DXGI_FORMAT format;

	if(texture_data.type == TEXTURE_SLOT_ALBEDO) format = DXGI_FORMAT_R8G8B8A8_UNORM;

	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sr = {};
	sr.pSysMem = texture_data.data;
	sr.SysMemPitch = texture_data.width * texture_data.num_components;

	ID3D11Texture2D* buffer;
	hr = device->CreateTexture2D(&desc, &sr, &buffer);

	// RESEARCH: view examples
	D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {};
	view_desc.Format = desc.Format;
	view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	view_desc.Texture2D.MostDetailedMip = 0;
	view_desc.Texture2D.MipLevels = -1;
	
	hr = device->CreateShaderResourceView(buffer, &view_desc, &view);
	assertHR(hr);

	texture.slot = texture_data.type;
	texture.buffer = buffer;
	texture.view = view;
	return texture;
}

//------------------------------------------------------------------------
static VertexBuffer UploadVertexBuffer(VertexBufferData vb_data, u32 num_vertices, ID3D11Device* device) {
	HRESULT hr;
	VertexBuffer vb = {};

	ID3D11Buffer* buffer;
	D3D11_BUFFER_DESC desc = {};
	u32 num_components = 0;
	if(vb_data.type == VERTEX_BUFFER_TYPE_POSITION) num_components = 3;
	else if(vb_data.type == VERTEX_BUFFER_TYPE_NORMAL) num_components = 3;
	else if(vb_data.type == VERTEX_BUFFER_TYPE_TEXCOORD) num_components = 2;
	else if(vb_data.type == VERTEX_BUFFER_TYPE_TANGENT) num_components = 4;
	else assert(false);

	u32 component_width = sizeof(float);
	
	desc.ByteWidth = num_components * component_width * num_vertices;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA sr = {};
	sr.pSysMem = vb_data.data;
	hr = device->CreateBuffer(&desc, &sr, &buffer);
	assertHR(hr);

	vb.buffer = buffer;
	vb.stride = num_components * component_width;
	vb.type = vb_data.type;

	return vb;
}

//------------------------------------------------------------------------
static RenderBufferGroup UploadMesh(MeshData mesh_data, Renderer* renderer) {
	RenderBufferGroup rbg = {};

	RenderBuffer* rb = PushRenderBuffer(mesh_data.vb_data_count+1, renderer);

	u8 i=0;
	for(i=0; i<mesh_data.vb_data_count; i++) {
		rb[i].type = RENDER_BUFFER_TYPE_VERTEX;
		rb[i].vertex = UploadVertexBuffer(mesh_data.vb_data[i], mesh_data.vertices_count, renderer->device);
	}

	ID3D11Buffer* index_buffer;
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = sizeof(u32) * mesh_data.indices_count;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA sr = {};
	sr.pSysMem = mesh_data.indices;
	renderer->device->CreateBuffer(&desc, &sr, &index_buffer);

	rb[i].type = RENDER_BUFFER_TYPE_INDEX;
	rb[i].index.buffer = index_buffer; 
	rb[i].index.num_indices = mesh_data.indices_count;

	rbg.rb = rb;
	rbg.count = mesh_data.vb_data_count + 1;
	return rbg;
}

//------------------------------------------------------------------------
static void InitRendering(Renderer* renderer, HWND handle, WindowDimensions wd) {
	HRESULT hr;
	//-------------------------Init-----------------------------------------------
	{
		u32 flags = 0;
#ifdef DEBUG_RENDERER
		flags |= DXGI_CREATE_FACTORY_DEBUG | D3D11_CREATE_DEVICE_DEBUG;
#endif
		IDXGIFactory4* dxgi_factory;
		hr = CreateDXGIFactory2(flags, IID_IDXGIFactory4, (void**)&dxgi_factory);
		assertHR(hr);

		flags = 0;
		flags = DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER;
		dxgi_factory->MakeWindowAssociation(handle, flags);

		IDXGIAdapter1* adapters[4] = { NULL };
		u32 adapter_count = 0;

		while ((adapter_count < ARRAYSIZE(adapters) -1) &&
			(dxgi_factory->EnumAdapters1(adapter_count, &adapters[adapter_count])
				!= DXGI_ERROR_NOT_FOUND)) adapter_count++;

		u32 adapter_to_use = 0;
		u32 video_memory_available = 0;
		for (int i = 0; i < adapter_count; i++) {
			DXGI_ADAPTER_DESC1 desc = { NULL };
			adapters[i]->GetDesc1(&desc);
			SDL_Log("Adapter %i: %S", i, desc.Description);
			SDL_Log("Video Memory %i", desc.DedicatedVideoMemory);
			if (desc.DedicatedVideoMemory > video_memory_available) {
				video_memory_available = desc.DedicatedVideoMemory;
				adapter_to_use = i;
			}
		}

		D3D_FEATURE_LEVEL target_feature_levels[] = {
		                                            D3D_FEATURE_LEVEL_11_1,
																								D3D_FEATURE_LEVEL_11_0
		};
		D3D_FEATURE_LEVEL out_feature_levels = D3D_FEATURE_LEVEL_9_1;

		hr = D3D11CreateDevice((IDXGIAdapter*)adapters[adapter_to_use], D3D_DRIVER_TYPE_UNKNOWN, NULL,
			flags, target_feature_levels, ARRAYSIZE(target_feature_levels), D3D11_SDK_VERSION,
			&renderer->device, &out_feature_levels, &renderer->context);

		assertHR(hr);
		if (!hr) SDL_Log("Using adapter %i", adapter_to_use);

#ifdef DEBUG_RENDERER
		hr = renderer->device->QueryInterface(__uuidof(ID3D11Debug), (void**)&renderer->d3d_debug);
		assertHR(hr);
#endif

		DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
		swapchain_desc.Width = 0;
		swapchain_desc.Height = 0;
		swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchain_desc.Stereo = 0;
		swapchain_desc.SampleDesc.Count = 1;
		swapchain_desc.SampleDesc.Quality = 0;
		swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchain_desc.BufferCount = 2;
		swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
		swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapchain_desc.Flags = 0;

		hr = dxgi_factory->CreateSwapChainForHwnd((IUnknown*)renderer->device, handle, &swapchain_desc, NULL, NULL, &renderer->swapchain);

		assertHR(hr);
		//dxgi_factory->Release();

		hr = renderer->swapchain->QueryInterface(IID_PPV_ARGS(&renderer->swapchain));
		assertHR(hr);
	}
	//--------------------------Render Target View----------------------------------------------
	{
		ID3D11Texture2D* rtv_tex;
		hr = renderer->swapchain->GetBuffer(0, IID_PPV_ARGS(&rtv_tex));
		D3D11_TEXTURE2D_DESC rtv_tex_desc = {}; 
		rtv_tex->GetDesc(&rtv_tex_desc);

		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		hr = renderer->device->CreateRenderTargetView((ID3D11Resource*)rtv_tex, &rtv_desc, &renderer->rtv);
		assertHR(hr);

		ID3D11Texture2D* dsv_tex;
		D3D11_TEXTURE2D_DESC dsv_tex_desc;
		rtv_tex->GetDesc(&dsv_tex_desc);
		dsv_tex_desc.MipLevels = 1;
		dsv_tex_desc.ArraySize = 1;
		dsv_tex_desc.Format = DXGI_FORMAT_D32_FLOAT;
		dsv_tex_desc.Usage = D3D11_USAGE_DEFAULT;
		dsv_tex_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		dsv_tex_desc.SampleDesc.Count = 1;

		hr = renderer->device->CreateTexture2D(&dsv_tex_desc, NULL, &dsv_tex);
		assertHR(hr);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
		dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		hr = renderer->device->CreateDepthStencilView((ID3D11Resource*)dsv_tex, &dsv_desc, &renderer->dsv);
		assertHR(hr);
	}
	//---------------------------Blend state---------------------------------------------
	{ // final.xyz = (src.xyz * src_blend) (BlendOp) (dest.rgb * dest_blend)
		{	// No blend
			D3D11_BLEND_DESC bs_desc = {};
			bs_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			bs_desc.RenderTarget[0].BlendEnable = false;

			hr = renderer->device->CreateBlendState(&bs_desc, &renderer->bs[BLEND_STATE_NO_BLEND]);
			assertHR(hr);
		}
		{ // Enabled
			D3D11_BLEND_DESC bs_desc = {};
			bs_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			bs_desc.RenderTarget[0].BlendEnable = true;
			bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR;
			bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
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
		assertHR(hr);
	}
	//----------------------------Rasterizer--------------------------------------------
	{ 
		// Default Rasterizer
		ID3D11RasterizerState* rs;
		D3D11_RASTERIZER_DESC rs_desc;
		rs_desc.FillMode = D3D11_FILL_SOLID;
		rs_desc.CullMode = D3D11_CULL_BACK;
		rs_desc.FrontCounterClockwise = true;
		hr = renderer->device->CreateRasterizerState(&rs_desc, &rs);
		renderer->rs[RASTERIZER_STATE_DEFAULT] = rs;
		assertHR(hr);

		// Wireframe
		rs_desc.FillMode = D3D11_FILL_WIREFRAME;
		hr = renderer->device->CreateRasterizerState(&rs_desc, &rs);
		renderer->rs[RASTERIZER_STATE_WIREFRAME] = rs;
		assertHR(hr);

		rs_desc = {};
		rs_desc.FillMode = D3D11_FILL_SOLID;
		rs_desc.CullMode = D3D11_CULL_NONE;
		rs_desc.FrontCounterClockwise = true;
		hr = renderer->device->CreateRasterizerState(&rs_desc, &rs);
		renderer->rs[RASTERIZER_STATE_DOUBLE_SIDED] = rs;
		assertHR(hr);
	}
	//-----------------------------Viewport-------------------------------------------
	{	
		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = wd.width;
		vp.Height = wd.height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		renderer->vp[VIEWPORT_DEFAULT] = vp;
	}
	//------------------------Samplers------------------------------------------------
	{
		{
			// default
			ID3D11SamplerState* ss;
			D3D11_SAMPLER_DESC desc = {};
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			desc.MaxLOD = D3D11_FLOAT32_MAX;
			hr = renderer->device->CreateSamplerState(&desc, &ss);
			assertHR(hr);
			renderer->ss[SAMPLER_STATE_DEFAULT] = ss;
		}
		{
			// Tiling
			ID3D11SamplerState* ss;
			D3D11_SAMPLER_DESC desc = {};
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			desc.MaxLOD = D3D11_FLOAT32_MAX;
			hr = renderer->device->CreateSamplerState(&desc, &ss);
			assertHR(hr);
			renderer->ss[SAMPLER_STATE_TILE] = ss;
		}
	}
	//---------------------------Vertex Shaders---------------------------------------------
	{	// 
		VertexShaderDesc vs_desc = {};
		vs_desc.shader = { L"../assets/shaders/diffuse.hlsl", "vsf" };

		ConstantsBufferDesc cb_desc[] = { { CONSTANTS_BINDING_SLOT_CAMERA, sizeof(Mat4) },
														 { CONSTANTS_BINDING_SLOT_INSTANCE, sizeof(Mat4) } };
		VERTEX_BUFFER_TYPE vb_type[] = { VERTEX_BUFFER_TYPE_POSITION, VERTEX_BUFFER_TYPE_NORMAL };

		vs_desc.vb_type = vb_type;
		vs_desc.cb_desc = cb_desc;
		vs_desc.vb_count = ARRAY_LENGTH(vb_type);
		vs_desc.cb_count = ARRAY_LENGTH(cb_desc);

		renderer->vs[VERTEX_SHADER_POS_NOR] = UploadVertexShader(vs_desc, renderer);
	}
	{	 // 
		VertexShaderDesc vs_desc = {};
		vs_desc.shader = { L"../assets/shaders/diffuse.hlsl", "vsf_tex" };
		ConstantsBufferDesc cb_desc[] = { { CONSTANTS_BINDING_SLOT_CAMERA, sizeof(Mat4) },
																			{ CONSTANTS_BINDING_SLOT_INSTANCE, sizeof(Mat4) } };

		VERTEX_BUFFER_TYPE vb_type[] = { VERTEX_BUFFER_TYPE_POSITION, VERTEX_BUFFER_TYPE_NORMAL, 
																		 VERTEX_BUFFER_TYPE_TEXCOORD };

		vs_desc.vb_type = vb_type;
		vs_desc.cb_desc = cb_desc;
		vs_desc.vb_count = ARRAY_LENGTH(vb_type);
		vs_desc.cb_count = ARRAY_LENGTH(cb_desc);

		renderer->vs[VERTEX_SHADER_POS_NOR_TEX] = UploadVertexShader(vs_desc, renderer);
	}
	{
		VertexShaderDesc vs_desc = {};
		vs_desc.shader = { L"../assets/shaders/text.hlsl", "vsf" };

		StructuredBufferDesc sb_desc[] = { { STRUCTURED_BINDING_SLOT_FRAME, sizeof(GlyphQuad), MAX_DEBUG_TEXT_GLYPHS } };
		vs_desc.sb_desc = sb_desc;
		vs_desc.sb_count = 1;

		renderer->vs[VERTEX_SHADER_TEXT] = UploadVertexShader(vs_desc, renderer);
	}
	//-------------------------Pixel Shaders-----------------------------------------------
	{	
		{	// Unlit shader
			PixelShaderDesc ps_desc = {};

			ps_desc.shader = { L"../assets/shaders/unlit.hlsl", "ps_main" };
			ConstantsBufferDesc cb_desc[] = { { CONSTANTS_BINDING_SLOT_OBJECT, sizeof(Vec3) } };

			ps_desc.cb_desc = cb_desc;
			ps_desc.cb_count = ARRAY_LENGTH(cb_desc);

			renderer->ps[PIXEL_SHADER_UNLIT] = UploadPixelShader(ps_desc, renderer);
		}
		/*
		{	// Unlit textured
			PixelShaderDesc ps_desc = {};
			ps_desc.shader = { L"../assets/shaders/unlit.hlsl", "ps_textured" };
			TEXTURE_SLOT tl[] = { { TEXTURE_SLOT_ALBEDO } };
			renderer->ps[PIXEL_SHADER_UNLIT_TEXTURED] = UploadPixelShader(ps_desc, renderer->device);
		}
		*/
		{	// Diffuse shader
			PixelShaderDesc ps_desc = {};

			ps_desc.shader = { L"../assets/shaders/diffuse.hlsl", "psf" }; 
			ConstantsBufferDesc cb_desc[] = { { CONSTANTS_BINDING_SLOT_CAMERA, sizeof(DiffusePC) },
																				{ CONSTANTS_BINDING_SLOT_OBJECT, sizeof(DiffusePM) } };
			ps_desc.cb_desc = cb_desc;
			ps_desc.cb_count = ARRAY_LENGTH(cb_desc);

			renderer->ps[PIXEL_SHADER_DIFFUSE] = UploadPixelShader(ps_desc, renderer);
		}
		{	// Diffuse textured shader
			PixelShaderDesc ps_desc = {};
			ps_desc.shader = { L"../assets/shaders/diffuse.hlsl", "psf_tex" };

			ConstantsBufferDesc cb_desc[] = { { CONSTANTS_BINDING_SLOT_CAMERA, sizeof(DiffusePC) },
																				{ CONSTANTS_BINDING_SLOT_OBJECT, sizeof(DiffusePM) } };

			TEXTURE_SLOT texture_slot[] = { TEXTURE_SLOT_ALBEDO };

			ps_desc.texture_slot = texture_slot;
			ps_desc.cb_desc = cb_desc;
			ps_desc.texture_count = ARRAY_LENGTH(texture_slot);
			ps_desc.cb_count = ARRAY_LENGTH(cb_desc);

			renderer->ps[PIXEL_SHADER_DIFFUSE_TEXTURED] = UploadPixelShader(ps_desc, renderer);
		}
		{	// Text shader
			PixelShaderDesc ps_desc = {};
			ps_desc.shader = { L"../assets/shaders/text.hlsl", "psf" };

			TEXTURE_SLOT texture_slot[] = { TEXTURE_SLOT_ALBEDO };
			ps_desc.texture_slot = texture_slot;
			ps_desc.texture_count = ARRAY_LENGTH(texture_slot);;

			renderer->ps[PIXEL_SHADER_TEXT] = UploadPixelShader(ps_desc, renderer);
		}
	}
	//----------------------------Meshes--------------------------------------------
	{ 
		ModelData model_data = {};
		MeshData mesh_data = {};
		// Cube mesh
		{
			model_data = LoadModelDataGLTF("../assets/models/shapes/cube.gltf", "cube");
			mesh_data = model_data.mesh_data[0];
			renderer->rbg[RENDER_BUFFER_GROUP_CUBE] = UploadMesh(mesh_data, renderer);
		}
		// Sphere mesh
		{
			model_data = LoadModelDataGLTF("../assets/models/shapes/sphere.gltf", "sphere");
			mesh_data = model_data.mesh_data[0];
			renderer->rbg[RENDER_BUFFER_GROUP_SPHERE] = UploadMesh(mesh_data, renderer);
		}
		// Cone mesh
		{
			model_data = LoadModelDataGLTF("../assets/models/shapes/cone.gltf", "cone");
			mesh_data = model_data.mesh_data[0];
			renderer->rbg[RENDER_BUFFER_GROUP_CONE] = UploadMesh(mesh_data, renderer);
		}
		// Plane mesh
		{
			model_data = LoadModelDataGLTF("../assets/models/shapes/plane.gltf", "plane");
			mesh_data = model_data.mesh_data[0];
			renderer->rbg[RENDER_BUFFER_GROUP_PLANE] = UploadMesh(mesh_data, renderer);
		}
		// Torus mesh
		{
			model_data = LoadModelDataGLTF("../assets/models/shapes/torus.gltf", "torus");
			mesh_data = model_data.mesh_data[0];
			renderer->rbg[RENDER_BUFFER_GROUP_TORUS] = UploadMesh(mesh_data, renderer);
		}
	}
}

//------------------------------------------------------------------------
// Move init stuff into separate files
static void PushConstantsData(void* data, ConstantsBuffer cb, ID3D11DeviceContext* context) {
	assert(data);
	assert(cb.buffer);

	D3D11_MAPPED_SUBRESOURCE msr = {};

	context->Map(cb.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy(msr.pData, data, cb.size);
	context->Unmap(cb.buffer, 0);

	data = nullptr;
}

static void PushStructuredData(StructuredBufferData sb_data, StructuredBuffer sb, ID3D11DeviceContext* context) {
	assert(sb_data.data);
	assert(sb.buffer);

	D3D11_MAPPED_SUBRESOURCE msr = {};

	context->Map(sb.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy(msr.pData, sb_data.data, sb.struct_size*sb_data.count);
	context->Unmap(sb.buffer, 0);

	sb_data.data = nullptr;
}

//------------------------------------------------------------------------
static void BeginRendering(Renderer* renderer) {
	renderer->rq_id = 0;

	float color[4] = { 0.392f, 0.584f, 0.929f, 1.0f };
	renderer->context->ClearRenderTargetView(renderer->rtv, color);
	renderer->context->ClearDepthStencilView(renderer->dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
	renderer->context->OMSetRenderTargets(1, &renderer->rtv, renderer->dsv);
}

//------------------------------------------------------------------------
static void ExecuteRenderPipeline(RenderPipeline rp, Renderer* renderer) {
	DEPTH_STENCIL_STATE dss_t = renderer->state_overrides.dss ? renderer->state_overrides.dss : rp.rs.dss;
	BLEND_STATE bs_t = renderer->state_overrides.bs ? renderer->state_overrides.bs : rp.rs.bs;
	RASTERIZER_STATE rs_t = renderer->state_overrides.rs ? renderer->state_overrides.rs : rp.rs.rs;
	VIEWPORT vp_t = renderer->state_overrides.vp ? renderer->state_overrides.vp : rp.rs.vp;
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
		renderer->context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	//------------------------------------------------------------------------
	if(rp.ps) {
		PixelShader ps = renderer->ps[rp.ps];
		renderer->context->PSSetShader(ps.shader, nullptr, 0);
		for(u8 i=0; i<ps.rb_count; i++) 
			if(ps.rb[i].type == RENDER_BUFFER_TYPE_CONSTANTS) 
				renderer->context->PSSetConstantBuffers(ps.rb[i].constants.slot, 1, &ps.rb[i].constants.buffer);
		
		for(u8 i=0; i<rp.prbd_count; i++) 
			if(rp.prbd[i].type == RENDER_BUFFER_TYPE_CONSTANTS) 
				for(u8 j=0; j<ps.rb_count; j++) 
					if(ps.rb[j].type == RENDER_BUFFER_TYPE_CONSTANTS)
						if(rp.prbd[i].constants.slot == ps.rb[j].constants.slot)
							PushConstantsData(rp.prbd[i].constants.data, ps.rb[j].constants, renderer->context);

		RenderBufferGroup rbg = renderer->rbg[rp.prbg];
		if(rbg.count) {
			for(u8 i=0; i<ps.texture_count; i++) {
				bool found = false;
				for(u8 j=0; j<rbg.count; j++) {
					if(ps.texture_slot[i] == rbg.rb[i].texture.slot) {
						found = true;
						renderer->context->PSSetShaderResources(rbg.rb[i].texture.slot, 1, &rbg.rb[i].texture.view);
					}
				}
				assert(found);
			}
		}
	}
	//------------------------------------------------------------------------
	if(rp.vs) {
		VertexShader vs = renderer->vs[rp.vs];
		renderer->context->VSSetShader(vs.shader, nullptr, 0);
		if(vs.il) renderer->context->IASetInputLayout(vs.il);

		for(u8 i=0; i<vs.rb_count; i++) {
			if(vs.rb[i].type == RENDER_BUFFER_TYPE_CONSTANTS) 
				renderer->context->VSSetConstantBuffers(vs.rb[i].constants.slot, 1, &vs.rb[i].constants.buffer);
			if(vs.rb[i].type == RENDER_BUFFER_TYPE_STRUCTURED) 
				renderer->context->VSSetShaderResources(vs.rb[i].structured.slot, 1, &vs.rb[i].structured.view);
		}

		for(u8 i=0; i<rp.vrbd_count; i++) {
			if(rp.vrbd[i].type == RENDER_BUFFER_TYPE_CONSTANTS) {
				for(u8 j=0; j<vs.rb_count; j++) 
					if(vs.rb[j].type == RENDER_BUFFER_TYPE_CONSTANTS) 
						if(rp.vrbd[i].constants.slot == vs.rb[j].constants.slot)
							PushConstantsData(rp.vrbd[i].constants.data, vs.rb[j].constants, renderer->context);
			}
			if(rp.vrbd[i].type == RENDER_BUFFER_TYPE_STRUCTURED) {
				for(u8 j=0; j<vs.rb_count; j++) {
					if(vs.rb[j].type = RENDER_BUFFER_TYPE_STRUCTURED) 
						if(rp.vrbd[i].structured.slot == vs.rb[j].structured.slot)
							PushStructuredData(rp.vrbd[i].structured, vs.rb[j].structured, renderer->context);
				}
			}
		}

		if(rp.vrbg) {
			RenderBufferGroup rbg = renderer->rbg[rp.vrbg];
			for(u8 i=0; i<vs.vb_count; i++) {
				bool found = false;
				for(u8 j=0; j<rbg.count; j++) {
					if(rbg.rb[j].type == RENDER_BUFFER_TYPE_VERTEX && rbg.rb[j].vertex.type == vs.vb_type[i]) {
						found = true;	u32 offset = 0;
						renderer->context->IASetVertexBuffers(i, 1, &rbg.rb[j].vertex.buffer, &rbg.rb[j].vertex.stride, &offset);
					}
				}
				assert(found);
			}

			for(u8 i=0; i<rbg.count; i++) 
				if(rbg.rb[i].type == RENDER_BUFFER_TYPE_INDEX) {
					renderer->context->IASetIndexBuffer(rbg.rb[i].index.buffer, DXGI_FORMAT_R32_UINT, 0);
					if(rp.dc.type == DRAW_CALL_DEFAULT) renderer->context->DrawIndexed(rbg.rb[i].index.num_indices, 0, 0);
				}
		}
	}
	//------------------------------------------------------------------------
	{
		if(rp.dc.type == DRAW_CALL_INDEXED)
			renderer->context->DrawIndexed(rp.dc.indices_count, 0, 0);
		if(rp.dc.type == DRAW_CALL_VERTICES)
			renderer->context->Draw(rp.dc.vertices_count, 0);
	}
}

//------------------------------------------------------------------------
static void Render(Renderer* renderer) {
	for(u32 i=0; i<renderer->rq_id; i++) ExecuteRenderPipeline(renderer->rq[i], renderer);
}

//------------------------------------------------------------------------

static void EndRendering(Renderer* renderer) {
	renderer->swapchain->Present(0, 0);
}
