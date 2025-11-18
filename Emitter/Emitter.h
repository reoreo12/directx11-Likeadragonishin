#pragma once
#include "Component.h"

BEGIN(Engine)
class CAlphaObject;
END

BEGIN(Client)

class CEmitter : public CComponent
{
public:
	struct SPAWN_ENTRY
	{
		CGameObject*		pEffect = { nullptr }; // Pool
		_bool				bAutoSpawn = { false };
		_wstring			wsObjectTag;
	};

	struct PROTO_DESC
	{
		_uint				iPrototypeLevel = {};
		_wstring			wsPrototypeTag = {};
		json				jSpawnParams;
		_wstring			wsLayerTag = {};

		_float				fInterval = {};
		_float				fElapsed = {};

		_bool				bOneShot = { false };
		_bool				bHasSpawned = { false };
	};

	struct SPAWN_GROUP
	{
		vector<SPAWN_ENTRY> persistent; // 재활용 가능한 이펙트 -> 미리 Create & 재활용
		vector<PROTO_DESC>  prototypes; // 동적 생성해 줄 이펙트의 원형 정보 저장 
		vector<SPAWN_ENTRY> dynamic;    // 동적 생성된 인스턴스
	};

private:
	CEmitter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CEmitter(const CEmitter& Prototype, CGameObject* pOwner);
	virtual ~CEmitter() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(const _float& fTimeDelta, CGameObject* pOwner) override;
	virtual void			Update(const _float& fTimeDelta, CGameObject* pOwner) override;
	virtual void			Late_Update(const _float& fTimeDelta, CGameObject* pOwner) override;

public:
	HRESULT					LoadEffects_FromFile(const _wstring& wsTag, const _wstring& wsFilePath);
	virtual void			Trigger(const _wstring& wsTag, const _vector* pPosition = nullptr, const _vector* pDirection = nullptr) override;
	virtual void			Stop_Trigger(const _wstring& wsTag) override;

	void					Decal(const _wstring& wsGroupTag, const _vector* pPos, _float fRadius, const _vector* pDir = nullptr);
	virtual void			Play_Pool(const _wstring& wsGroupTag, const _vector* pPosition = nullptr, const _vector* pDirection = nullptr) override;

	_bool                   Is_GroupEnabled(const _wstring& wsTag) const;
	void					Set_SpawnGroupInterval(const _wstring& wsGroupTag, _float fNewInterval);

	map<_wstring, SPAWN_GROUP>		m_SpawnGroups;
	unordered_map<_wstring, _bool>	m_GroupEnabled;

private:
	CGameObject*			Spawn_Prototype(const wstring& wsGroupTag, const PROTO_DESC& pd, const _vector* pPosition = nullptr, const _vector* pDirection = nullptr);

public:
	static CEmitter* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent* Clone(void* pArg, class CGameObject* pOwner) override;
	virtual void Free() override;
};

END