#include "SimpleDx.h"

HWND SimpleDx::hwnd = {};
WNDCLASSEX SimpleDx::wnd = {};

ID3D12Device* SimpleDx::dev = nullptr;
IDXGIFactory6* SimpleDx::dxgiFactory = nullptr;
IDXGISwapChain4* SimpleDx::swapChain = nullptr;
ID3D12CommandAllocator* SimpleDx::cmdAllocator = nullptr;
ID3D12GraphicsCommandList* SimpleDx::cmdList = nullptr;
ID3D12CommandQueue* SimpleDx::cmdQueue = nullptr;
std::vector<ID3D12Resource*> SimpleDx::backBuffers;
ID3D12DescriptorHeap* SimpleDx::rtvHeaps = nullptr;
ID3D12Fence* SimpleDx::fence = nullptr;
UINT64 SimpleDx::fenceVal = 0;



LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

int SimpleDx::Initialize(long width, long height, wchar_t* title)
{
	HRESULT result = S_OK;
	wnd = {};

	wnd.cbSize = sizeof(WNDCLASSEX);
	wnd.lpfnWndProc = (WNDPROC)WindowProcedure;//�R�[���o�b�N�֐��̎w��
	wnd.lpszClassName = title;//�A�v���P�[�V�����N���X��(�K���ł����ł�)
	wnd.hInstance = GetModuleHandle(0);//�n���h���̎擾
	RegisterClassEx(&wnd);//�A�v���P�[�V�����N���X(���������̍�邩���낵������OS�ɗ\������)

	RECT wrc = { 0, 0, width, height };

	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);//�E�B���h�E�̃T�C�Y�͂�����Ɩʓ|�Ȃ̂Ŋ֐����g���ĕ␳����
	//�E�B���h�E�I�u�W�F�N�g�̐���
	hwnd = CreateWindow(wnd.lpszClassName,//�N���X���w��
		title,//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,//�^�C�g���o�[�Ƌ��E��������E�B���h�E�ł�
		CW_USEDEFAULT,//�\��X���W��OS�ɂ��C�����܂�
		CW_USEDEFAULT,//�\��Y���W��OS�ɂ��C�����܂�
		wrc.right - wrc.left,//�E�B���h�E��
		wrc.bottom - wrc.top,//�E�B���h�E��
		nullptr,//�e�E�B���h�E�n���h��
		nullptr,//���j���[�n���h��
		wnd.hInstance,//�Ăяo���A�v���P�[�V�����n���h��
		nullptr);//�ǉ��p�����[�^

	if (CreateDevice() == -1)
	{
		return -1;
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //�s�N�Z���t�H�[�}�b�g
	swapChainDesc.Stereo = false; //�X�e���I�\���t���O
	swapChainDesc.SampleDesc.Count = 1; //�}���`�T���v���̎w��
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapChainDesc.BufferCount = 2; //�_�u���o�b�t�@�[�Ȃ̂�2

	//�o�b�N�o�b�t�@�[�͐L�яk�݉\
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	//�t���b�v��͑��₩�ɔj��
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//���Ɏw��Ȃ�
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	//�E�B���h�E�ƃX�N���[���̐ؑ։\
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	result = dxgiFactory->CreateSwapChainForHwnd(cmdQueue, hwnd, &swapChainDesc, nullptr, nullptr, (IDXGISwapChain1**)&swapChain);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; //�����_�[�^�[�Q�b�g�r���[�Ȃ̂�RTV
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2; //�\����2��
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; //���Ɏw��Ȃ�

	result = dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	DXGI_SWAP_CHAIN_DESC swDesc = {};
	result = swapChain->GetDesc(&swDesc);

	backBuffers = std::vector <ID3D12Resource*>(swDesc.BufferCount);
	D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < swDesc.BufferCount; i++)
	{
		result = swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));
		dev->CreateRenderTargetView(backBuffers[i], nullptr, handle);
		handle.ptr += dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}


	result = dev->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));

	ShowWindow(hwnd, SW_SHOW);

	return 0;
}


void SimpleDx::Refresh()
{
	//DirectX����
	//�o�b�N�o�b�t�@�̃C���f�b�N�X���擾
	UINT bbIdx = swapChain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = backBuffers[bbIdx];
	BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	cmdList->ResourceBarrier(1, &BarrierDesc);

	//�����_�[�^�[�Q�b�g���w��
	auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += static_cast<ULONG_PTR>(bbIdx * dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	cmdList->OMSetRenderTargets(1, &rtvH, false, nullptr);

	//��ʃN���A
	float clearColor[] = { 1.0f,1.0f,0.0f,1.0f };//���F
	cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	cmdList->ResourceBarrier(1, &BarrierDesc);

	//���߂̃N���[�Y
	cmdList->Close();


	//�R�}���h���X�g�̎��s
	ID3D12CommandList* cmdlists[] = { cmdList };
	cmdQueue->ExecuteCommandLists(1, cmdlists);
	////�҂�
	cmdQueue->Signal(fence, ++fenceVal);

	if (fence->GetCompletedValue() != fenceVal)
	{
		auto event = CreateEvent(nullptr, false, false, nullptr);
		fence->SetEventOnCompletion(fenceVal, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}
	cmdAllocator->Reset();//�L���[���N���A
	cmdList->Reset(cmdAllocator, nullptr);//�ĂуR�}���h���X�g�����߂鏀��


	//�t���b�v
	swapChain->Present(1, 0);
}

int SimpleDx::CreateDevice()
{
	//DX�̃t�B�[�`���[���x�����`
	D3D_FEATURE_LEVEL levels[] =
	{
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
	};

	//�Ή����Ă���h���C�o�[���Ȃ���ΏI��
	HRESULT result = S_OK;
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory))))
	{
		if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory))))
		{
			return -1;
		}
	}

	//���p�\��Adapter����������
	//Adapter: 1�܂��͕����� GPU�ADAC�A����уr�f�I�������[
	std::vector <IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	for (IDXGIAdapter* adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	//Direct3D�f�o�C�X�̏�����
	//�f�o�C�X�̑Ή����Ă���t�B�[�`���[���x��������
	D3D_FEATURE_LEVEL featureLevel;
	for (D3D_FEATURE_LEVEL l : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, l, IID_PPV_ARGS(&dev)) == S_OK)
		{
			featureLevel = l;
			break;
		}
	}

	result = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAllocator));
	result = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAllocator, nullptr, IID_PPV_ARGS(&cmdList));

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};

	//�^�C���A�E�g�Ȃ�
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	//�A�_�v�^�[��1�����g��Ȃ��Ƃ���0�ŗǂ�
	cmdQueueDesc.NodeMask = 0;
	//�v���C�I���e�B�͓��Ɏw��Ȃ�
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	//�R�}���h���X�g�ƍ��킹��
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	result = dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&cmdQueue));

	return 0;
}

void SimpleDx::Dispose()
{
	UnregisterClass(wnd.lpszClassName, wnd.hInstance);
}

