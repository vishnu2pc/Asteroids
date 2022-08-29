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
		if (bl[i].vertex_buffer_type == VBT_POSITION) id = 1;
		else if (bl[i].vertex_buffer_type == VBT_NORMAL) id = 2;
		else if (bl[i].vertex_buffer_type == VBT_TANGENT) id = 3;
		else if (bl[i].vertex_buffer_type == VBT_TEXCOORD) id = 4;
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

	ps.tl = PushMaster(TextureLayout, desc.tl_count);
	ps.tl_count = desc.tl_count;
	memcpy(ps.tl, desc.tl, sizeof(TextureLayout) * desc.tl_count);

	hr = D3DCompileFromFile(desc.shader.path, nullptr, nullptr, desc.shader.entry, "ps_5_0",
																			0, 0, &blob, &error);
	if (hr) {
		char* msg = (char*)error->GetBufferPointer();
		if (hr) SDL_Log(msg);
	}
	hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &ps.ps);
	assertHR(hr);

	// TODO: Remove constantsbuffer desc
	for (u8 i = 0; i < desc.cb_desc_count; i++) {
		ConstantsBufferDesc current_desc = desc.cb_desc[i];
		ps.cb[current_desc.slot].buffer = UploadConstantsBuffer(current_desc.size_in_bytes, device);
		ps.cb[current_desc.slot].size_in_bytes = current_desc.size_in_bytes;
	}
	ps.ss_count = 1;

	assertHR(hr);
	return ps;
}

//------------------------------------------------------------------------
static VertexShader UploadVertexShader(VertexShaderDesc desc, ID3D11Device* device) {
	HRESULT hr = {};
	VertexShader vs = {};

	ID3DBlob* blob;
	ID3DBlob* error;

	hr = D3DCompileFromFile(desc.shader.path, nullptr, nullptr, desc.shader.entry, "vs_5_0",
																			0, 0, &blob, &error);
	if (hr) {
		char* msg = (char*)error->GetBufferPointer();
		SDL_Log(msg);
	}
	hr = device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &vs.vs);
	assertHR(hr);

	D3D11_INPUT_ELEMENT_DESC* ie_desc = PushScratch(D3D11_INPUT_ELEMENT_DESC, desc.bl_count);
	MakeD3DInputElementDesc(desc.bl, ie_desc, desc.bl_count);

	vs.bl = PushMaster(BufferLayout, desc.bl_count);
	vs.bl_count = desc.bl_count;
	memcpy(vs.bl, desc.bl, sizeof(BufferLayout) * desc.bl_count);

	hr = device->CreateInputLayout(ie_desc, desc.bl_count, blob->GetBufferPointer(), blob->GetBufferSize(), &vs.il);
	assertHR(hr);

	for (u8 i = 0; i < desc.cb_desc_count; i++) {
		ConstantsBufferDesc current_desc = desc.cb_desc[i];
		vs.cb[current_desc.slot].buffer = UploadConstantsBuffer(current_desc.size_in_bytes, device);
		vs.cb[current_desc.slot].size_in_bytes = current_desc.size_in_bytes;
	}
	PopScratch(D3D11_INPUT_ELEMENT_DESC, desc.bl_count);
	return vs;
}

//------------------------------------------------------------------------
static ID3D11ShaderResourceView* UploadTexture(Texture texture, ID3D11Device* device) {
	HRESULT hr = {};
	ID3D11ShaderResourceView* srv;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = texture.width;
	desc.Height = texture.height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA sr = {};
	sr.pSysMem = texture.data;
	sr.SysMemPitch = texture.width * texture.num_components;

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
	return srv;
}

//------------------------------------------------------------------------
static ID3D11Buffer* UploadVertexBuffer(VertexBuffer vb, u32 num_vertices, ID3D11Device* device) {
	HRESULT hr;
	ID3D11Buffer* d3d_buffer;
	D3D11_BUFFER_DESC desc = {};
	u32 num_components = vb.num_components;

	u32 component_width = sizeof(float);
	
	desc.ByteWidth = num_components * component_width * num_vertices;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA sr = {};
	sr.pSysMem = vb.data;
	hr = device->CreateBuffer(&desc, &sr, &d3d_buffer);
	assertHR(hr);
	return d3d_buffer;
}

//------------------------------------------------------------------------
static Mesh UploadMesh(MeshData mesh_data, VERTEX_SHADER vst, Renderer renderer) {
	Mesh mesh = {};

	mesh.vs = vst;
	VertexShader vs = renderer.vs[vst];
	mesh.vertex_buffers = PushMaster(ID3D11Buffer*, vs.bl_count);
	assert(mesh_data.vb_count >= vs.bl_count);

	mesh.strides = PushMaster(u32, vs.bl_count);
	for (u8 i = 0; i < vs.bl_count; i++) {
		VERTEX_BUFFER_TYPE vbt = vs.bl[i].vertex_buffer_type;
		COMPONENT_TYPE ct = vs.bl[i].component_type;
		BUFFER_FORMAT bf = vs.bl[i].buffer_format;

		bool found = false;
		for (u8 j = 0; j < mesh_data.vb_count; j++) {
			VertexBuffer vb = mesh_data.vertex_buffers[j];
			if (vb.type == vbt) {
				mesh.vertex_buffers[i] = UploadVertexBuffer(vb, mesh_data.vertices_count, renderer.device);
				mesh.strides[i] = sizeof(float) * vb.num_components;
				found = true;
			}
		}
		assert(found);
	}
	mesh.vertices_count = mesh_data.vertices_count;

	if(mesh_data.indices_count) {
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = sizeof(u32) * mesh_data.indices_count;
		desc.Usage = D3D11_USAGE_IMMUTABLE;
		desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		D3D11_SUBRESOURCE_DATA sr = {};
		sr.pSysMem = mesh_data.indices;
		renderer.device->CreateBuffer(&desc, &sr, &mesh.index_buffer);

		mesh.indices_count = mesh_data.indices_count;
	}
	return mesh;
}

//------------------------------------------------------------------------
static Renderer InitRendering(HWND handle, WindowDimensions wd) {
	Renderer renderer = {};
	HRESULT hr;
	//------------------------------------------------------------------------
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
			&renderer.device, &out_feature_levels, &renderer.context);

		assertHR(hr);
		if (!hr) SDL_Log("Using adapter %i", adapter_to_use);

#ifdef DEBUG_RENDERER
		hr = renderer.device->QueryInterface(__uuidof(ID3D11Debug), (void**)&renderer.d3d_debug);
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

		hr = dxgi_factory->CreateSwapChainForHwnd((IUnknown*)renderer.device, handle, &swapchain_desc, NULL, NULL, &renderer.swapchain);

		assertHR(hr);
		dxgi_factory->Release();

		hr = renderer.swapchain->QueryInterface(IID_PPV_ARGS(&renderer.swapchain));
		assertHR(hr);
	}
	//------------------------------------------------------------------------
	{
		ID3D11Texture2D* rtv_tex;
		hr = renderer.swapchain->GetBuffer(0, IID_PPV_ARGS(&rtv_tex));
		D3D11_TEXTURE2D_DESC rtv_tex_desc = {}; 
		rtv_tex->GetDesc(&rtv_tex_desc);

		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		hr = renderer.device->CreateRenderTargetView((ID3D11Resource*)rtv_tex, &rtv_desc, &renderer.rtv);
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

		hr = renderer.device->CreateTexture2D(&dsv_tex_desc, NULL, &dsv_tex);
		rtv_tex->Release();
		assertHR(hr);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
		dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		hr = renderer.device->CreateDepthStencilView((ID3D11Resource*)dsv_tex, &dsv_desc, &renderer.dsv);
		assertHR(hr);
		dsv_tex->Release();
	}
	//------------------------------------------------------------------------
	{
		D3D11_BLEND_DESC bs_desc = {};
		bs_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = renderer.device->CreateBlendState(&bs_desc, &renderer.bs[0]);
		assertHR(hr);
	}
	//------------------------------------------------------------------------
	{
		ID3D11DepthStencilState* dss;
		D3D11_DEPTH_STENCIL_DESC dss_desc = {};
		dss_desc.DepthEnable = true;
		dss_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		dss_desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		hr = renderer.device->CreateDepthStencilState(&dss_desc, &dss);
		renderer.dss[DSS_DEFAULT] = dss;
		assertHR(hr);
	}
	//------------------------------------------------------------------------
	{ 
		// Default Rasterizer
		ID3D11RasterizerState* rs;
		D3D11_RASTERIZER_DESC rs_desc;
		rs_desc.FillMode = D3D11_FILL_SOLID;
		rs_desc.CullMode = D3D11_CULL_BACK;
		rs_desc.FrontCounterClockwise = true;
		hr = renderer.device->CreateRasterizerState(&rs_desc, &rs);
		renderer.rs[RS_DEFAULT] = rs;
		assertHR(hr);

		// Wireframe
		rs_desc.FillMode = D3D11_FILL_WIREFRAME;
		hr = renderer.device->CreateRasterizerState(&rs_desc, &rs);
		renderer.rs[RS_WIREFRAME] = rs;
		assertHR(hr);
	}
	//------------------------------------------------------------------------
	{	
		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0.0f;
		vp.TopLeftY = 0.0f;
		vp.Width = wd.width;
		vp.Height = wd.height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		renderer.vp[VP_DEFAULT] = vp;
	}
	//------------------------------------------------------------------------
	{	
		VertexShaderDesc vs_desc = {};
		vs_desc.shader = { L"../assets/shaders/diffuse.hlsl", "vs_main" };
		BufferLayout bl[] = { { VBT_POSITION, BF_VEC3, CT_FLOAT },
													{ VBT_NORMAL, BF_VEC3, CT_FLOAT	} };
		ConstantsBufferDesc vcb_desc[] = { { CS_PER_CAMERA, sizeof(Mat4) },
																			 { CS_PER_OBJECT, sizeof(Mat4) } };
		vs_desc.bl = bl;
		vs_desc.bl_count = ARRAY_LENGTH(bl);
		vs_desc.cb_desc = vcb_desc;
		vs_desc.cb_desc_count = ARRAY_LENGTH(vcb_desc);

		renderer.vs[VS_DIFFUSE] = UploadVertexShader(vs_desc, renderer.device);
	}
	//------------------------------------------------------------------------
	{	
		// Unlit shader
		{
			PixelShaderDesc unlit_desc = {};
			unlit_desc.shader = { L"../assets/shaders/unlit.hlsl", "ps_main" };
			ConstantsBufferDesc unlit_cb_desc[] = { { CS_PER_MESH_MATERIAL, sizeof(Vec3) } };
			unlit_desc.cb_desc = unlit_cb_desc;
			unlit_desc.cb_desc_count = ARRAY_LENGTH(unlit_cb_desc);
			renderer.ps[PS_UNLIT] = UploadPixelShader(unlit_desc, renderer.device);
		}
		// Diffuse shader
		{
			PixelShaderDesc diffuse_desc = {};
			diffuse_desc.shader = { L"../assets/shaders/diffuse.hlsl", "ps_main" }; 
			ConstantsBufferDesc diffuse_cb_desc[] = { { CS_PER_CAMERA, sizeof(DiffusePC) },
																								{ CS_PER_MESH_MATERIAL, sizeof(DiffusePM) } };
			diffuse_desc.cb_desc = diffuse_cb_desc;
			diffuse_desc.cb_desc_count = ARRAY_LENGTH(diffuse_cb_desc);
			renderer.ps[PS_DIFFUSE] = UploadPixelShader(diffuse_desc, renderer.device);
		}
	}
	//------------------------------------------------------------------------
	{ // Default materials
		// Unlit material
		{
			Material mat = {};
			mat.ps = PS_UNLIT;
			mat.unlit_params.color = WHITE;
			renderer.mat[MAT_UNLIT] = mat;
		}
		// Diffuse material
		{
			Material mat = {};
			mat.ps = PS_DIFFUSE;
			mat.diffuse_params.diffuse_factor = 1.0f;
			mat.diffuse_params.color = ORANGE;
			renderer.mat[MAT_DIFFUSE] = mat;
		}
	}
	//------------------------------------------------------------------------
	{ 
		ModelData model_data = {};
		MeshData mesh_data = {};
		// Cube mesh
		{
			model_data = LoadModelDataGLTF("../assets/models/cube/cube.gltf", "cube");
			mesh_data = model_data.mesh_data[0];
			renderer.mesh[MESH_CUBE] = UploadMesh(mesh_data, VS_DIFFUSE, renderer);
		}
		// Sphere mesh
		{
			model_data = LoadModelDataGLTF("../assets/models/sphere/sphere.gltf", "sphere");
			mesh_data = model_data.mesh_data[0];
			renderer.mesh[MESH_SPHERE] = UploadMesh(mesh_data, VS_DIFFUSE, renderer);
		}
	}
	//------------------------------------------------------------------------
	return renderer;
}

// Move init stuff into separate files
static void PushConstantsData(void* data, ConstantsBuffer cb, ID3D11DeviceContext* context) {
	assert(data);

	D3D11_MAPPED_SUBRESOURCE msr = {};

	context->Map(cb.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy(msr.pData, data, cb.size_in_bytes);
	context->Unmap(cb.buffer, 0);

	data = nullptr;
}

//------------------------------------------------------------------------
static void BeginRendering(Renderer renderer) {
	float color[4] = { 0.2f, 0.3f, 0.3f, 1.0f };
	renderer.context->ClearRenderTargetView(renderer.rtv, color);
	renderer.context->ClearDepthStencilView(renderer.dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
	renderer.context->OMSetRenderTargets(1, &renderer.rtv, renderer.dsv);
}

//------------------------------------------------------------------------
static void ExecuteRenderPipeline(RenderPipeline rp, Renderer renderer) {
	DEPTH_STENCIL_STATE dss_t = renderer.state_overrides.dss ? renderer.state_overrides.dss : rp.rs.dss;
	BLEND_STATE bs_t = renderer.state_overrides.bs ? renderer.state_overrides.bs : rp.rs.bs;
	RASTERIZER_STATE rs_t = renderer.state_overrides.rs ? renderer.state_overrides.rs : rp.rs.rs;
	VIEWPORT vp_t = renderer.state_overrides.vp ? renderer.state_overrides.vp : rp.rs.vp;

	{
		ID3D11DepthStencilState* dss = renderer.dss[dss_t];
		renderer.context->OMSetDepthStencilState(dss, 0);
	}
	//------------------------------------------------------------------------0
	{
		ID3D11BlendState* bs = renderer.bs[bs_t];
		float val[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		renderer.context->OMSetBlendState(bs, val, 0xFFFFFFFF);
	}
	//------------------------------------------------------------------------
	{
		ID3D11RasterizerState* rs = renderer.rs[rs_t];
		renderer.context->RSSetState(rs);
	}
	//------------------------------------------------------------------------
	{
		D3D11_VIEWPORT vp = renderer.vp[vp_t];
		renderer.context->RSSetViewports(1, &vp);
		D3D11_RECT rect = {};
		rect.right = vp.Width;
		rect.bottom = vp.Height;
		renderer.context->RSSetScissorRects(1, &rect);
		renderer.context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	//------------------------------------------------------------------------
	Material mat = renderer.mat[rp.mat_id];
	{
		PixelShader ps = renderer.ps[mat.ps];
		renderer.context->PSSetShader(ps.ps, nullptr, 0);

		ConstantsBuffer cb;
		for(u8 i=0; i<CS_PER_MESH_MATERIAL; i++) {
			cb = ps.cb[i];
			if(cb.buffer) {
				renderer.context->PSSetConstantBuffers(i, 1, &cb.buffer);
				PushConstantsData(ps.cb_data[i], cb, renderer.context);
			}
		}

		cb = ps.cb[CS_PER_MESH_MATERIAL];
		if(cb.buffer) {
			renderer.context->PSSetConstantBuffers(CS_PER_MESH_MATERIAL, 1, &cb.buffer);

			switch(mat.ps) {
				case PS_DIFFUSE: {
					DiffusePM dpm = { mat.diffuse_params.color, mat.diffuse_params.diffuse_factor };
					PushConstantsData(&dpm, cb, renderer.context);
				} break;
			}
		}

		cb = ps.cb[CS_PER_OBJECT];
		if(cb.buffer) {
			renderer.context->PSSetConstantBuffers(CS_PER_OBJECT, 1, &cb.buffer);
			PushConstantsData(rp.psc_per_object_data, cb, renderer.context);
		}
	}
	//------------------------------------------------------------------------
	Mesh mesh = renderer.mesh[rp.mesh_id];
	VertexShader vs = renderer.vs[mesh.vs];
	{
		renderer.context->VSSetShader(vs.vs, nullptr, 0);
		renderer.context->IASetInputLayout(vs.il);

		ConstantsBuffer cb = {};
		for(u8 i=0; i<CS_PER_MESH_MATERIAL; i++) {
			cb = vs.cb[i];
			if(cb.buffer) {
				renderer.context->VSSetConstantBuffers(i, 1, &cb.buffer);
				PushConstantsData(vs.cb_data[i], cb, renderer.context);
			}
		}

		// Handle cs_per_mesh

		cb = vs.cb[CS_PER_OBJECT];
		if(cb.buffer) {
			renderer.context->VSSetConstantBuffers(CS_PER_OBJECT, 1, &cb.buffer);
			PushConstantsData(rp.vsc_per_object_data, cb, renderer.context);
		}
	}
	//------------------------------------------------------------------------
	{
		u32* offsets = PushScratch(u32, vs.bl_count);
		memset(offsets, 0, sizeof(u32)*vs.bl_count);
		renderer.context->IASetVertexBuffers(0, vs.bl_count, mesh.vertex_buffers, mesh.strides, offsets);
		if(mesh.index_buffer) {
			renderer.context->IASetIndexBuffer(mesh.index_buffer, DXGI_FORMAT_R32_UINT, 0);
			renderer.context->DrawIndexed(mesh.indices_count, 0, 0);
		}
		PopScratch(u32, vs.bl_count);
	}
}

//------------------------------------------------------------------------
static void EndRendering(Renderer renderer) {
	renderer.swapchain->Present(0, 0);
}
