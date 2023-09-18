// @insp https://www.shadertoy.com/view/lt3GWj
//	36 FPS, но отличается от оригинала немного цветом и отстутстивием шума -- миниволн
#include "stdafx.h"
using namespace prj_3d::HelloWinHlsl;
using namespace prj_3d::HelloWinHlsl::ClientTy;
//using CurDxVer = DxVer::v10;
using CurDxVer = DxVer::v12;

template<class TSPE, class T = TSPE> class CaliCoastReplayTDM; // primary template

template<> class CaliCoastReplayTDM<DxVer::v10> : public CurClientApp<DxVer::v10> { using T = DxVer::v10;
	CPtr< ID3D10InputLayout> m_pcLayout;
	CPtr< ID3D10Buffer > m_pcVertexBuf;
	CPtr< ID3D10VertexShader > m_pcVs;
	CPtr< ID3D10PixelShader > m_pcPs;
	LikeDxut::Timer m_oTimer;

	struct PS_CONST_BUFF { 
		D3DXVECTOR4 ve2Resolution;
		float fTime;
	};
	PS_CONST_BUFF PsConstData = { };
	Dx::Tool::ConstantBuf::viaStruct::Accessor< T, PS_CONST_BUFF >::uptr_t m_puoConstBufAccessor;

	bool init(DxCtx<T>::cref_ptr_t crpustDxCtx, ToolCtx<T>::cref_ptr_t puoTools, Adjust<T>* poAdjustDxAux) {
		Sys::Hr hr;

		// Load shaders
		std::vector<BYTE> veShaderByte;
		m_pcVs = puoTools ->shader( ) ->loader( ) ->byteCode( ) ->fromFile( ) ->Vs( L"Cali_vs_Dx10.cso", &veShaderByte );
		m_pcPs = puoTools ->shader( ) ->loader( ) ->byteCode( ) ->fromFile( ) ->Ps( L"Cali_ps_Dx10.cso" );
		if ( !m_pcVs || !m_pcPs )
			return false;
		crpustDxCtx ->m_pcD3dDevice ->VSSetShader( m_pcVs.Get( ) );
		crpustDxCtx ->m_pcD3dDevice ->PSSetShader( m_pcPs.Get( ) );

		// Set vertex input layout
		m_pcLayout = puoTools ->quad( ) ->createLayout( veShaderByte );
		if ( !m_pcLayout )
			return false;
		crpustDxCtx ->m_pcD3dDevice ->IASetInputLayout( m_pcLayout.Get( ) );

		// Create and set vertex buffer
		auto stBuf = puoTools ->quad( ) ->createVertexBuf();
		if ( !stBuf )
			return false;
		m_pcVertexBuf = stBuf ->m_stVertexBuf.m_pcBuffer;
		crpustDxCtx ->m_pcD3dDevice ->IASetVertexBuffers( 
			0, stBuf ->m_stVertexBuf.c_uNumBuffers, &stBuf->m_stVertexBuf.m_veBuffers[ 0 ]
			, &stBuf->m_stVertexBuf.m_veStride[ 0 ], &stBuf->m_stVertexBuf.m_veOffset[ 0 ] );

		// Input assembly
		crpustDxCtx ->m_pcD3dDevice ->IASetPrimitiveTopology( stBuf ->m_enuTopology );

		// Supply the pixel shader constant data
		PS_CONST_BUFF PsConstData = { };
		DXGI_SWAP_CHAIN_DESC stPp = Dx::Descr<T>::swapChain( crpustDxCtx );
		PsConstData.ve2Resolution.x = (float)stPp.BufferDesc.Width;
		PsConstData.ve2Resolution.y = (float)stPp.BufferDesc.Height;
		// Create a constant buffer
		m_puoConstBufAccessor = puoTools ->constBufAccessorFactory( ) ->createAccessor( PsConstData );
		auto stCBuf = m_puoConstBufAccessor ->getConstantBuf( );
		if ( !stCBuf )
			return false;
		// Bind the constant buffer to the device
		crpustDxCtx ->m_pcD3dDevice ->PSSetConstantBuffers( 0, stCBuf ->c_uNumBuffers, stCBuf ->m_veConstantBuffers.data( ) );

		return true;
	}

	void render_frame(DxCtx<T>::cref_ptr_t crpustDxCtx, Dynamic<T>::cref_ptr_t) {
		// Update timer value
		m_puoConstBufAccessor ->passToShader( [this](PS_CONST_BUFF *p) { 
				p ->fTime = m_oTimer.get( );
			});
        crpustDxCtx ->m_pcD3dDevice ->Draw( 4, 0 );
	}
};
template<> class CaliCoastReplayTDM <DxVer::v12> : public CurClientApp<DxVer::v12> { using T = DxVer::v12;
	LikeDxut::Timer m_oTimer;

	CPtr< ID3DBlob > m_pcVs, m_pcPs;

	std::vector< D3D12_INPUT_ELEMENT_DESC > m_vecLayout;

	CPtr< ID3D12Resource > m_pcVertexBuf;
	D3D12_VERTEX_BUFFER_VIEW m_stVertexBufferView;
	D3D_PRIMITIVE_TOPOLOGY m_enuPrimTopology;

	struct PS_CONST_BUFF { 
		DirectX::XMFLOAT2 iResolution;
		float iTime;
	};
	CPtr< ID3D12RootSignature > m_pcRootSignature;
	Dx::Tool::ConstantBuf::viaStruct::Accessor< T, PS_CONST_BUFF >::uptr_t m_puoConstBufAccessor;

	bool init(DxCtx<T>::cref_ptr_t crpustDxCtx, ToolCtx<T>::cref_ptr_t puoTools, Adjust<T>* poAdjustDxAux) {
		Sys::Hr hr;
		// Load shaders
		m_pcVs = puoTools ->shader( ) ->loader( ) ->byteCode( ) ->fromFile( ) ->Vs( L"Cali_vs_Dx12.cso" );
		m_pcPs = puoTools ->shader( ) ->loader( ) ->byteCode( ) ->fromFile( ) ->Ps( L"Cali_ps_Dx12.cso" );
		if ( !m_pcVs || !m_pcPs )
			return false;
		// Get input layout
		m_vecLayout = puoTools ->quad( ) ->createLayout( );
		if ( m_vecLayout.empty( ) )
			return false;
		// New root signature for constants
		m_pcRootSignature = puoTools ->constBufRootSignature( ) ->fromStruct< PS_CONST_BUFF >( );
		// Set shaders and input layout and root signature 
		poAdjustDxAux ->onSetPso( 
				[this](D3D12_GRAPHICS_PIPELINE_STATE_DESC *ppsoDesc) {
					ppsoDesc ->VS = CD3DX12_SHADER_BYTECODE( m_pcVs.Get( ) );
					ppsoDesc ->PS = CD3DX12_SHADER_BYTECODE( m_pcPs.Get( ) );
					ppsoDesc ->InputLayout = { 
							m_vecLayout.data( )
							, (UINT)m_vecLayout.size( )
						};
					ppsoDesc ->pRootSignature = m_pcRootSignature.Get( );
				}
			);

		// Create and set vertex buffer
		auto stVBuf = puoTools ->quad( ) ->createVertexBuf( );
		if ( !stVBuf )
			return false;
		m_pcVertexBuf = stVBuf ->m_stVertexBuf.m_pcBuffer;
		m_stVertexBufferView = stVBuf ->m_stVertexBuf.m_vertexBufferView;
		m_enuPrimTopology = stVBuf ->m_enuTopology;

		// Supply the pixel shader constant data
		PS_CONST_BUFF PsConstData = { };
		DXGI_SWAP_CHAIN_DESC stPp = Dx::Descr<T>::swapChain( crpustDxCtx );
		PsConstData.iResolution = { (float)stPp.BufferDesc.Width, (float)stPp.BufferDesc.Height };
		// Create a constants accessor
		m_puoConstBufAccessor = puoTools ->constBufAccessorFactory( ) ->createAccessor( PsConstData );

		return true;
	}

    void render_frame(DxCtx<T>::cref_ptr_t, Dynamic<T>::cref_ptr_t crpsoDynamic) {
		// Record commands DirectX 12.
		crpsoDynamic ->m_pcCommandList ->IASetVertexBuffers( 0, 1, &m_stVertexBufferView );
        crpsoDynamic ->m_pcCommandList ->IASetPrimitiveTopology( m_enuPrimTopology );

		m_puoConstBufAccessor ->passToShader( 
				crpsoDynamic ->m_pcCommandList
				, [this](PS_CONST_BUFF *p) { 
					p ->iTime = m_oTimer.get( );
				}
			);
		crpsoDynamic ->m_pcCommandList ->DrawInstanced( 4, 1, 0, 0 );
    }

};

uptr< Sys::HolderClientApp > ClientTy::entryPoint() { 
	return std::make_unique< Sys::HolderClientApp >(new CaliCoastReplayTDM<CurDxVer>()); }
Config::uptrc_t ClientTy::configure() { 
	return *Config::getBuilder( )
		->setGameLoop(Config::EnumGameLoop::InSeparateThread)
//		->setGameLoop(ClientApp::Configurator::TStructConfig::TEnuGameLoop::Ordinary)
		//->setLimitFps( 25 )
		->setShowFps( true )
//		->setShowFps( false )
		;
}
