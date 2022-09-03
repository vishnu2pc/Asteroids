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
static void MakeD3DInputElementDesc(BufferLayout* bl, D3D11_INPUT_ELEMENT_DESC* d3d_il_desc, u8 count) {
	for (u32 i = 0; i < count; i++) {
		SemanticNameIndex id = 0;
		// TODO: rethink this
		if (bl[i].vb_type == VBT_POSITION) id = 1;
		else if (bl[i].vb_type == VBT_NORMAL) id = 2;
		else if (bl[i].vb_type == VBT_TANGENT) id = 3;
		else if (bl[i].vb_type == VBT_TEXCOORD) id = 4;
		else assert(false);

		assert(bl[i].component_type == CT_FLOAT);

		DXGI_FORMAT format;
		// TODO: rethink this
		if (bl[i].buffer_format == BF_VEC2) format = DXGI_FORMAT_R32G32_FLOAT;
		else if (bl[i].buffer_format == BF_VEC3) format = DXGI_FORMAT_R32G32B32_FLOAT;
		else if (bl[i].buffer_format == BF_VEC4) format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		else assert(false);

		d3d_il_desc[i] = { SemanticName[id], 0, format, i, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	}	
}

//------------------------------------------------------------------------
static ID3D11Buffer* UploadConstantsBuffer(u32 byte_width, ID3D11Device* device) {
	HRESULT hr = {};
	ID3D11Buffer* cb;

	byte_width += (16 - (byte_width % 16));
	D3D11_BUFFER_DESC cb_desc = {};
	cb_desc.ByteWidth 		 = byte_width;
	cb_desc.Usage          = D3D11_USAGE_DYNAMIC;
	cb_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	hr = device->CreateBuffer(&cb_desc, nullptr, &cb);

	assertHR(hr);
	return cb;
}

//------------------------------------------------------------------------
static PixelShader UploadPixelShader(PixelShaderDesc desc, ID3D11Device* device) {
	HRESULT hr = {};

	PixelShader ps = {};
	ID3DBlob* blob;
	ID3DBlob* error;
	ID3D11PixelShader* shader;

	hr = D3DCompileFromFile(desc.shader.path, nullptr, nullptr, desc.shader.entry, "ps_5_0",
																			0, 0, &blob, &error);
	if (hr) {
		char* msg = (char*)error->GetBufferPointer();
		if (hr) SDL_Log(msg);
	}
	hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);
	assertHR(hr);

	ConstantsBuffer* cb = PushMaster(ConstantsBuffer, desc.cb_desc_count);
	for(u8 i=0; i<desc.cb_desc_count; i++) {
		cb[i].buffer = UploadConstantsBuffer(desc.cb_desc[i].size, device);
		cb[i].slot = desc.cb_desc[i].slot;
		cb[i].size = desc.cb_desc[i].size;
	}

	TEXTURE_TYPE* texture_type = nullptr;
	if(desc.texture_count) {
		texture_type = PushMaster(TEXTURE_TYPE, desc.texture_count);
		for(u8 i=0; i<desc.texture_count; i++) texture_type[i] = desc.texture_type[i];
	}

	ps.shader = shader;
	ps.texture_type = texture_type;
	ps.cb = cb;
	ps.texture_type = texture_type;
	ps.texture_count = desc.texture_count;
	ps.ss_count = desc.ss_count;
	ps.cb_count = desc.cb_desc_count;

	return ps;
}

//------------------------------------------------------------------------
static VertexShader UploadVertexShader(VertexShaderDesc desc, ID3D11Device* device) {
	HRESULT hr = {};
	VertexShader vs = {};

	ID3DBlob* blob;
	ID3DBlob* error;
	ID3D11VertexShader* shader;
	ID3D11InputLayout* il;

	hr = D3DCompileFromFile(desc.shader.path, nullptr, nullptr, desc.shader.entry, "vs_5_0",
																			0, 0, &blob, &error);
	if (hr) {
		char* msg = (char*)error->GetBufferPointer();
		SDL_Log(msg);
	}
	hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);
	assertHR(hr);

	// TODO: JANKY, redo this
	D3D11_INPUT_ELEMENT_DESC* ie_desc = PushScratch(D3D11_INPUT_ELEMENT_DESC, desc.bl_count);
	MakeD3DInputElementDesc(desc.bl, ie_desc, desc.bl_count);

	VERTEX_BUFFER_TYPE* vb_type = PushMaster(VERTEX_BUFFER_TYPE, desc.bl_count);
	for(u8 i=0; i<desc.bl_count; i++) vb_type[i] = desc.bl[i].vb_type;

	hr = device->CreateInputLayout(ie_desc, desc.bl_count, blob->GetBufferPointer(), blob->GetBufferSize(), &il);
	assertHR(hr);

	ConstantsBuffer* cb = PushMaster(ConstantsBuffer, desc.cb_desc_count);
	for(u8 i=0; i<desc.cb_desc_count; i++) {
		cb[i].buffer = UploadConstantsBuffer(desc.cb_desc[i].size, device);
		cb[i].slot = desc.cb_desc[i].slot;
		cb[i].size = desc.cb_desc[i].size;
	}
	PopScratch(D3D11_INPUT_ELEMENT_DESC, desc.bl_count);

	vs.shader = shader;
	vs.il = il;
	vs.cb = cb;
	vs.vb_type = vb_type;
	vs.cb_count = desc.cb_desc_count;
	vs.vb_count = desc.bl_count;

	return vs;
}

//------------------------------------------------------------------------
static Texture UploadTexture(TextureData texture_data, ID3D11Device* device) {
	Texture texture = {};

	HRESULT hr = {};
	ID3D11ShaderResourceView* srv;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = texture_data.width;
	desc.Height = texture_data.height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sr = {};
	sr.pSysMem = texture_data.data;
	sr.SysMemPitch = texture_data.width * texture_data.num_components;

	// RESEARCH: Would we need to carry arount tex2D
	ID3D11Texture2D* tex;
	hr = device->CreateTexture2D(&desc, &sr, &tex);

	// RESEARCH: srv examples
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	
	hr = device->CreateShaderResourceView(tex, &srvDesc, &srv);
	tex->Release();
	assertHR(hr);

	texture.type = texture_data.type;
	texture.srv = srv;
	return texture;
}

//------------------------------------------------------------------------
static VertexBuffer UploadVertexBuffer(VertexBufferData vb_data, u32 num_vertices, ID3D11Device* device) {
	HRESULT hr;
	VertexBuffer vb = {};

	VERTEX_BUFFER_TYPE vb_type;
	if(vb_data.type == VBT_POSITION) vb_type = VBT_POSITION;
	if(vb_data.type == VBT_TEXCOORD) vb_type = VBT_TEXCOORD;
	if(vb_data.type == VBT_NORMAL) vb_type = VBT_NORMAL;

	ID3D11Buffer* buffer;
	D3D11_BUFFER_DESC desc = {};
	u32 num_components = vb_data.num_components;
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
	vb.type = vb_type;

	return vb;
}

//------------------------------------------------------------------------
static Mesh UploadMesh(MeshData mesh_data, ID3D11Device* device) {
	Mesh mesh = {};

	VertexBuffer* vb = PushMaster(VertexBuffer, mesh_data.vb_data_count);

	for(u8 i=0; i<mesh_data.vb_data_count; i++)
		vb[i] = UploadVertexBuffer(mesh_data.vb_data[i], mesh_data.vertices_count, device);

	ID3D11Buffer* index_buffer;
	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = sizeof(u32) * mesh_data.indices_count;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	D3D11_SUBRESOURCE_DATA sr = {};
	sr.pSysMem = mesh_data.indices;
	device->CreateBuffer(&desc, &sr, &index_buffer);

	mesh.vb = vb;
	mesh.vb_count = mesh_data.vb_data_count;
	mesh.ib = index_buffer;
	mesh.indices_count = mesh_data.indices_count;

	return mesh;
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
		dxgi_factory->Release();

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
		rtv_tex->Release();
		assertHR(hr);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
		dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		hr = renderer->device->CreateDepthStencilView((ID3D11Resource*)dsv_tex, &dsv_desc, &renderer->dsv);
		assertHR(hr);
		dsv_tex->Release();
	}
	//---------------------------Blend state---------------------------------------------
	{
		D3D11_BLEND_DESC bs_desc = {};
		bs_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = renderer->device->CreateBlendState(&bs_desc, &renderer->bs[0]);
		assertHR(hr);
	}
	//---------------------------Depth Stencil State---------------------------------------------
	{
		ID3D11DepthStencilState* dss;
		D3D11_DEPTH_STENCIL_DESC dss_desc = {};
		dss_desc.DepthEnable = true;
		dss_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dss_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		hr = renderer->device->CreateDepthStencilState(&dss_desc, &dss);
		renderer->dss[DSS_DEFAULT] = dss;
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
		renderer->rs[RS_DEFAULT] = rs;
		assertHR(hr);

		// Wireframe
		rs_desc.FillMode = D3D11_FILL_WIREFRAME;
		hr = renderer->device->CreateRasterizerState(&rs_desc, &rs);
		renderer->rs[RS_WIREFRAME] = rs;
		assertHR(hr);

		rs_desc = {};
		rs_desc.FillMode = D3D11_FILL_SOLID;
		rs_desc.CullMode = D3D11_CULL_NONE;
		rs_desc.FrontCounterClockwise = true;
		hr = renderer->device->CreateRasterizerState(&rs_desc, &rs);
		renderer->rs[RS_DOUBLE_SIDED] = rs;
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
		renderer->vp[VP_DEFAULT] = vp;
	}
	//---------------------------Vertex Shaders---------------------------------------------
	{	// 
		VertexShaderDesc vs_desc = {};
		vs_desc.shader = { L"../assets/shaders/diffuse.hlsl", "vs_main" };
		BufferLayout bl[] = { { VBT_POSITION, BF_VEC3, CT_FLOAT },
													{ VBT_NORMAL, BF_VEC3, CT_FLOAT	} };
		ConstantsBufferDesc cb_desc[] = { { CS_PER_CAMERA, sizeof(Mat4) },
														 { CS_PER_OBJECT, sizeof(Mat4) } };
		vs_desc.bl = bl;
		vs_desc.bl_count = ARRAY_LENGTH(bl);
		vs_desc.cb_desc = cb_desc;
		vs_desc.cb_desc_count = ARRAY_LENGTH(cb_desc);

		renderer->vs[VS_POS_NOR] = UploadVertexShader(vs_desc, renderer->device);
	}
	/*
	{	 // 
		VertexShaderDesc vs_desc = {};
		vs_desc.shader = { L"../assets/shaders/unlit.hlsl", "vs_textured" };
		BufferLayout bl[] = { { VBT_POSITION, BF_VEC3, CT_FLOAT },
		{ VBT_TEXCOORD, BF_VEC2, CT_FLOAT	} };
		ConstantsBufferDesc cb_desc[] = { { CS_PER_CAMERA, sizeof(Mat4) },
		{ CS_PER_OBJECT, sizeof(Mat4) } };
		vs_desc.bl = bl;
		vs_desc.bl_count = ARRAY_LENGTH(bl);
		vs_desc.cb_desc = cb_desc;
		vs_desc.cb_desc_count = ARRAY_LENGTH(cb_desc);

		renderer->vs[VS_POS_TEX] = UploadVertexShader(vs_desc, renderer->device);
	}
	*/
	//-------------------------Pixel Shaders-----------------------------------------------
	{	
		{	// Unlit shader
			PixelShaderDesc ps_desc = {};
			ps_desc.shader = { L"../assets/shaders/unlit.hlsl", "ps_main" };
			ConstantsBufferDesc cb_desc[] = { { CS_PER_MESH_MATERIAL, sizeof(Vec3) } };
			ps_desc.cb_desc = cb_desc;
			ps_desc.cb_desc_count = ARRAY_LENGTH(cb_desc);
			renderer->ps[PS_UNLIT] = UploadPixelShader(ps_desc, renderer->device);
		}
		/*
		{	// Unlit textured
			PixelShaderDesc ps_desc = {};
			ps_desc.shader = { L"../assets/shaders/unlit.hlsl", "ps_textured" };
			TEXTURE_TYPE tl[] = { { TEXTURE_ALBEDO } };
			renderer->ps[PS_UNLIT_TEXTURED] = UploadPixelShader(ps_desc, renderer->device);
		}
		*/
		{	// Diffuse shader
			PixelShaderDesc ps_desc = {};
			ps_desc.shader = { L"../assets/shaders/diffuse.hlsl", "ps_main" }; 
			ConstantsBufferDesc cb_desc[] = { { CS_PER_CAMERA, sizeof(DiffusePC) },
																								{ CS_PER_MESH_MATERIAL, sizeof(DiffusePM) } };
			ps_desc.cb_desc = cb_desc;
			ps_desc.cb_desc_count = ARRAY_LENGTH(cb_desc);
			renderer->ps[PS_DIFFUSE] = UploadPixelShader(ps_desc, renderer->device);
		}
	}
	//------------------------Samplers------------------------------------------------
	{
		/*
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
		assert(hr);
		renderer->ss[SS_TILE] = ss;
		*/
	}
	//------------------------Materials------------------------------------------------
	{ 
		{	// Unlit material
			Material mat = {};
			mat.type = MT_UNLIT;
			mat.constants_data.params_unlit.color = WHITE;
			renderer->mat[MAT_UNLIT] = mat;
		}
		{	// Diffuse material
			Material mat = {};
			mat.type = MT_DIFFUSE;
			mat.constants_data.params_diffuse.diffuse_factor = 1.0f;
			mat.constants_data.params_diffuse.color = ORANGE;
			renderer->mat[MAT_DIFFUSE] = mat;
		}
		{	// Grid material
			/*
			Material mat = {};

			int x, y, n;
			void* data = stbi_load("../assets/textures/grid.png", &x, &y, &n, 4);
			assert(data);
			TextureData texture_data = { TEXTURE_ALBEDO, data, (u32)x, (u32)y, 4 };
			Texture texture = UploadTexture(texture_data, renderer->device);

			mat.texture = PushMaster(Texture, 1);
			mat.texture_count = 1;
			mat.texture[0] = texture;

			renderer->mat[MAT_GRID] = mat;
			*/
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
			renderer->mesh[MESH_CUBE] = UploadMesh(mesh_data, renderer->device);
		}
		// Sphere mesh
		{
			model_data = LoadModelDataGLTF("../assets/models/shapes/sphere.gltf", "sphere");
			mesh_data = model_data.mesh_data[0];
			renderer->mesh[MESH_SPHERE] = UploadMesh(mesh_data, renderer->device);
		}
		// Cone mesh
		{
			model_data = LoadModelDataGLTF("../assets/models/shapes/cone.gltf", "cone");
			mesh_data = model_data.mesh_data[0];
			renderer->mesh[MESH_CONE] = UploadMesh(mesh_data, renderer->device);
		}
		// Plane mesh
		{
			model_data = LoadModelDataGLTF("../assets/models/shapes/plane.gltf", "plane");
			mesh_data = model_data.mesh_data[0];
			renderer->mesh[MESH_PLANE] = UploadMesh(mesh_data, renderer->device);
		}
		// Torus mesh
		{
			model_data = LoadModelDataGLTF("../assets/models/shapes/torus.gltf", "torus");
			mesh_data = model_data.mesh_data[0];
			renderer->mesh[MESH_TORUS] = UploadMesh(mesh_data, renderer->device);
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

//------------------------------------------------------------------------
static void BeginRendering(Renderer* renderer) {
	float color[4] = { 0.2f, 0.3f, 0.3f, 1.0f };
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
		for(u8 i=0; i<ps.cb_count; i++) 
			renderer->context->PSSetConstantBuffers(ps.cb[i].slot, 1, &ps.cb[i].buffer);
		
		if(rp.cd_pixel.data) {
			for(u8 i=0; i<ps.cb_count; i++) 
				if(ps.cb[i].slot == rp.cd_pixel.slot) 
					PushConstantsData(rp.cd_pixel.data, ps.cb[i], renderer->context);
		}

		if(rp.mat_id) {
			Material mat = renderer->mat[rp.mat_id];
			for(u8 i=0; i<ps.texture_count; i++) {
				bool found = false;
				for(u8 j=0; j<mat.texture_count; j++) {
					if(ps.texture_type[i] == mat.texture[j].type) {
						found = true;
						renderer->context->PSSetShaderResources(i, 1, &mat.texture[j].srv);
					}
				}
				assert(found);
			}

			ConstantsBuffer per_mat_cb = {};
			for(u8 i=0; i<ps.cb_count; i++)
				if(ps.cb[i].slot == CS_PER_MESH_MATERIAL) per_mat_cb = ps.cb[i];

			if(per_mat_cb.buffer) PushConstantsData(&mat.constants_data, per_mat_cb, renderer->context);
		}

	}
	//------------------------------------------------------------------------
	if(rp.vs) {
		VertexShader vs = renderer->vs[rp.vs];
		renderer->context->VSSetShader(vs.shader, nullptr, 0);
		renderer->context->IASetInputLayout(vs.il);
		for(u8 i=0; i<vs.cb_count; i++) 
			renderer->context->VSSetConstantBuffers(vs.cb[i].slot, 1, &vs.cb[i].buffer); 

		if(rp.cd_vertex.data) {
			for(u8 i=0; i<vs.cb_count; i++) 
				if(vs.cb[i].slot == rp.cd_vertex.slot) 
					PushConstantsData(rp.cd_vertex.data, vs.cb[i], renderer->context);
		}

		if(rp.mesh_id) {
			Mesh mesh = renderer->mesh[rp.mesh_id];
			for(u8 i=0; i<vs.vb_count; i++) {
				bool found = false;
				for(u8 j=0; j<mesh.vb_count; j++) {
					if(vs.vb_type[i] == mesh.vb[j].type) {
						found = true;
						u32 offset = 0;
						renderer->context->IASetVertexBuffers(i, 1, &mesh.vb[j].buffer, &mesh.vb[j].stride, &offset);
					}
				}
				assert(found);
			}
			renderer->context->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R32_UINT, 0);
			renderer->context->DrawIndexed(mesh.indices_count, 0, 0);
		}
	}

}

//------------------------------------------------------------------------
static void EndRendering(Renderer* renderer) {
	renderer->swapchain->Present(0, 0);
}
