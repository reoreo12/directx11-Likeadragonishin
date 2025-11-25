#pragma once
#include "Component.h"

BEGIN(Engine)
class CAlphaObject;
END

BEGIN(Client)

class CEmitter : public CComponent
{
public:
	struct POOL_DESC
	{
		CGameObject*		pEffect = { nullptr }; // Pool
		_wstring			wsObjectTag;

		_bool				bOneShot = { false }; // 주기적으로 꺼낼 필요가 없는 이펙트인지

		_float				fInterval = {};
		_float				fElapsed = {};
	};

	struct EFFECT_GROUP
	{
		vector<POOL_DESC> persistent; // 그룹에 포함되는 이펙트 풀과 재생 주기 파라미터를 포함
		_bool bGroupEnabled = false; // 그룹 활성화 플래그
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

	CGameObject*			ActivateFromPool(EFFECT_GROUP& group, const _wstring& wsObjTag, const _vector* pPosition = nullptr, const _vector* pDirection = nullptr);
	void					Decal(const _wstring& wsGroupTag, const _vector* pPos, _float fRadius, const _vector* pDir = nullptr);

	_bool                   Is_GroupEnabled(const _wstring& wsTag) const;
	void					Set_SpawnGroupInterval(const _wstring& wsGroupTag, _float fNewInterval);

private:
	void					Apply_Effect_Transform(CGameObject* pObj, const _vector* pPosition, const _vector* pDirection);
	void					Play_Effect(CGameObject* pObj);

	static POOL_DESC		Make_PoolDesc(CGameObject* pObj, const json& j);
	static _wstring			Get_ObjectTag_FromJson(const json& j);

private:
	map<_wstring, EFFECT_GROUP>	m_EffectGroups;

public:
	static CEmitter* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent* Clone(void* pArg, class CGameObject* pOwner) override;
	virtual void Free() override;
};

END