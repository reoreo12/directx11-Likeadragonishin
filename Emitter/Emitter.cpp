#include "Emitter.h"
#include "GameInstance.h"
#include "SocketEffect_Sprite.h"
#include "SocketParticle_Sprite.h"
#include "SocketEffect_Mesh.h"
#include "Effect_Decal.h"
#include "Body.h"
#include "Managers.h"

CEmitter::CEmitter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent { pDevice, pContext }
{
	lstrcpy(m_tComponentDesc.szComponentTag, CLASS_NAME(CEmitter));
}

CEmitter::CEmitter(const CEmitter& Prototype, CGameObject* pOwner)
	: CComponent(Prototype, pOwner)
{
    m_EffectGroups.clear();
}

HRESULT CEmitter::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CEmitter::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

    m_EffectGroups.clear();

	return S_OK;
}

void CEmitter::Priority_Update(const _float& fTimeDelta, CGameObject* pOwner)
{
}

void CEmitter::Update(const _float& fTimeDelta, CGameObject* pOwner)
{
    // Interval에 따라 풀에서 꺼내 활성화
    for (auto& [tag, group] : m_EffectGroups)
    {  
        if (!group.bGroupEnabled)
            continue;

        for (auto& slot : group.persistent)
        {
            if (slot.bOneShot)
                continue;
        
            slot.fElapsed += fTimeDelta;
            if (slot.fElapsed >= slot.fInterval)
            {
                slot.fElapsed -= slot.fInterval;
        
                CGameObject* pObj = slot.pEffect;
                if (pObj && !pObj->Is_Active())
                {
                    pObj->Set_Active(true);

                    Apply_Effect_Transform(pObj, nullptr, nullptr);
                    Play_Effect(pObj);
                }
            }
        }
    }

    // 죽은 이펙트 NULL
    for (auto& [_, group] : m_EffectGroups)
    {
        for (auto& slot = group.persistent)
        {
            if (slot.pEffect && slot.pEffect->Is_Dead())
                slot.pEffect = nullptr;
        }
    }
}

void CEmitter::Late_Update(const _float& fTimeDelta, CGameObject* pOwner)
{
}

HRESULT CEmitter::LoadEffects_FromFile(const _wstring& wsTag, const _wstring& wsFilePath)
{
    // 이미 로드된 그룹이면 스킵
    if (m_EffectGroups.count(wsTag))
        return S_OK;
    
    // JSON 로드
    json jData;
    ifstream ifs(wsFilePath);

    if (!ifs.is_open())
        return E_FAIL;
    
    ifs >> jData;
    ifs.close();
    
    LEVEL iCurLevel = CManagers::GetInstance()->Get_CurrentLevel();

    // 리소스 로드
    if (jData.contains("Resources"))
    {
        if (FAILED(CGameInstance::GetInstance()->Load_ResourceData(MAIN_PATH, LEVEL_STATIC, jData["Resources"])))
            return E_FAIL;
    }
    
    auto& group = m_EffectGroups[wsTag];
    group.bGroupEnabled = false;
    
    CGameObject* pOwner = Get_Owner();
    auto* pCharacter = dynamic_cast<CCharacter*>(pOwner);

    // SocketEffect
    if (jData.contains("ChildEffect") && jData["ChildEffect"].is_array())
    {
        for (auto& j : jData["ChildEffect"])
        {
            if (!j.contains("iPrototypeLevel") || !j.contains("szPrototypeTag"))
                continue;

            _uint iPrototypeLevel = j["iPrototypeLevel"].get<_uint>();
            _wstring wsPrototypeTag = StringToWstring(j["szPrototypeTag"].get<_string>());

            // 미리 생성해 두고 재활용
            CGameObject* pChild = pOwner->Create_Child(iPrototypeLevel, wsPrototypeTag, &j);
            if (!pChild)
                continue;

            // 소켓 매트릭스 셋업
            if (j.contains("SocketTag"))
            {
                _wstring wsSocketTag = StringToWstring(j["SocketTag"].get<_string>());
                if (pCharacter)
                {
                    _wstring wsBodyTag = pCharacter->Get_CurBodyTag();
                    if (auto* pBody = dynamic_cast<CBody*>(pOwner->Find_Child(wsBodyTag)))
                    {
                        if (auto* pSS = dynamic_cast<CSocketEffect_Sprite*>(pChild))
                            pSS->Set_SocketMatrix(pBody->Get_SocketMatrix(wsSocketTag));
                        else if (auto* pSM = dynamic_cast<CSocketEffect_Mesh*>(pChild))
                            pSM->Set_SocketMatrix(pBody->Get_SocketMatrix(wsSocketTag));
                        else if (auto* pDecal = dynamic_cast<CEffect_Decal*>(pChild))
                            pDecal->Set_SocketMatrix(pBody->Get_SocketMatrix(wsSocketTag));
                    }
                }
            }

            group.persistent.push_back(Make_PoolDesc(pChild, j));
        }
    }

    // ChildParticle
    if (jData.contains("ChildParticle") && jData["ChildParticle"].is_array())
    {
        for (auto& j : jData["ChildParticle"])
        {
            if (!j.contains("iPrototypeLevel") || !j.contains("szPrototypeTag"))
                continue;

            _uint iPrototypeLevel = j["iPrototypeLevel"].get<_uint>();
            _wstring wsPrototypeTag = StringToWstring(j["szPrototypeTag"].get<_string>());

            CGameObject* pChild = pOwner->Create_Child(iPrototypeLevel, wsPrototypeTag, &j);
            if (!pChild)
                continue;

            if(auto* pSocketParticle = dynamic_cast<CSocketParticle_Sprite*>(pChild))
            {
                if (j.contains("SocketTag"))
                {
                    _wstring wsSocketTag = StringToWstring(j["SocketTag"].get<_string>());
                    if (pCharacter)
                    {
                        _wstring wsBodyTag = pCharacter->Get_CurBodyTag();
                        if (auto* pBody = dynamic_cast<CBody*>(pOwner->Find_Child(wsBodyTag)))
                            pSocketParticle->Set_SocketMatrix(pBody->Get_SocketMatrix(wsSocketTag));
                    }
                }
            }

            group.persistent.push_back(Make_PoolDesc(pChild, j));
        }
    }

    // 일반 Effect/Particle
    for (auto& j : jData["GameObjects"])
    {
        if (!j.contains("iPrototypeLevel") || !j.contains("szPrototypeTag") || !j.contains("szLayerTag"))
            continue;

        _uint iPrototypeLevel = j["iPrototypeLevel"].get<_uint>();
        _wstring wsPrototypeTag = StringToWstring(j["szPrototypeTag"].get<_string>());
        _wstring wsLayerTag = StringToWstring(j["szLayerTag"].get<_string>());

        CGameObject* pObj = nullptr;
        if (FAILED(CGameInstance::GetInstance()->Add_GameObject(iPrototypeLevel, wsPrototypeTag, iCurLevel, wsLayerTag, &pObj, &j)))
            return E_FAIL;

        if (pObj)
            group.persistent.push_back(Make_PoolDesc(pObj, j));
    }

	return S_OK;
}

// 이펙트 그룹 활성화 함수
void CEmitter::Trigger(const _wstring& wsTag, const _vector* pPosition, const _vector* pDirection)
{
    auto iter = m_EffectGroups.find(wsTag);
    if (iter == m_EffectGroups.end())
        return;
    
    auto& group = iter->second;

    // 그룹 활성화
    group.bGroupEnabled = true;

    for (auto& slot : group.persistent)
    {
        CGameObject* pObj = slot.pEffect;
        if (!pObj || pObj->Is_Active())
            continue;

        pObj->Set_Active(true);

        // 활성화 시, 위치 조정이나 각도 조정이 필요하다면
        Apply_Effect_Transform(pObj, pPosition, pDirection);
        Play_Effect(pObj);

        // Trigger 호출로 재생 직후 리셋
        slot.fElapsed = 0.f;
    }
}

// 비활성화 함수
void CEmitter::Stop_Trigger(const _wstring& wsTag)
{
    auto iter = m_EffectGroups.find(wsTag);
    if (iter == m_EffectGroups.end())
        return;

    auto& group = iter->second;
    group.bGroupEnabled = false;

    for (auto& slot : group.persistent)
    {
        if (slot.pEffect)
        {
            slot.pEffect->Set_Active(false);
            slot.fElapsed = 0.f;
        }
    }
}

// 풀링 이펙트 활성화 함수
CGameObject* CEmitter::ActivateFromPool(SPAWN_GROUP& group, const _wstring& wsObjTag, const _vector* pPosition, const _vector* pDirection)
{
    for (auto& slot : group.persistent)
    {
        if (slot.wsObjectTag != wsObjTag)
            continue;

        auto* pObj = slot.pEffect;

        // 이미 활성화된 풀이라면 다음 풀 탐색
        if (!pObj || pObj->Is_Active())
            continue;

        pObj->Set_Active(true);

        Apply_Effect_Transform(pObj, pPosition, pDirection);
        Play_Effect(pObj);

        return pObj;
    }

    // 활성화 가능한 풀이 없는 경우
    return nullptr;
}

void CEmitter::Decal(const _wstring& wsGroupTag, const _vector* pPos, _float fRadius, const _vector* pDir)
{
    auto iter = m_EffectGroups.find(wsGroupTag);
    if (iter == m_EffectGroups.end())
        return;

    // 원 범위 랜덤 위치 스폰
    _vector vSpawnPos = XMVectorZero();
    const _vector* pPosArg = nullptr;
    if (pPos)
    {
        _float2 vUnit = Random_OnCircle();
        _float  fDist = Random(0.f, fRadius);
        _vector vOffset = XMVectorSet(vUnit.x * fDist, 0.f, vUnit.y * fDist, 0.f);
        vSpawnPos = *pPos + vOffset;
        pPosArg = &vSpawnPos;
    }

    auto& pool = iter->second.persistent;
    for (auto& slot : pool)
    {
        auto* pObj = slot.pEffect;
        if (!pObj || pObj->Is_Active())
            continue;

        pObj->Set_Active(true);

        if (auto* pDecal = dynamic_cast<CEffect_Decal*>(pObj))
        {
            // 소켓 데칼의 위치는 부모 소켓에게 종속되므로 방향만
            if (pDecal->Is_UseSocket())
                Apply_Effect_Transform(pObj, nullptr, pDir);
            else
                Apply_Effect_Transform(pObj, pPosArg, pDir);
        }

        else
        {
            Apply_Effect_Transform(pObj, pPosArg, pDir);
        }

        Play_Effect(pObj);

        slot.fElapsed = 0.f;

        break;
    }
}

// 그룹 활성화 확인 함수
_bool CEmitter::Is_GroupEnabled(const _wstring& wsTag) const
{
    auto iter = m_EffectGroups.find(wsTag);
    return iter != m_EffectGroups.end() && iter->second.bEnabled;
}

// 그룹 재생 주기 조정 함수
void CEmitter::Set_SpawnGroupInterval(const _wstring& wsGroupTag, _float fNewInterval)
{
    auto iter = m_EffectGroups.find(wsGroupTag);
    if (iter == m_EffectGroups.end())
        return;

    auto& group = iter->second;

    for (auto& slot : group.persistent)
    {
        slot.fInterval = fNewInterval;
        slot.fElapsed = min(slot.fElapsed, slot.fInterval);
    }
}

// 이펙트의 새 위치와 회전 값을 적용, 갱신하는 함수
void CEmitter::Apply_Effect_Transform(CGameObject* pObj, const _vector* pPosition, const _vector* pDirection)
{
    if (!pObj)
        return;
    
    auto* pTransform = pObj->Get_Transform(); 
    if (!pTransform)
        return;

    _bool bIsDirty = false;

    if (pPosition)
    {
        pTransform->Set_State(CTransform::STATE_POSITION, *pPosition);
        bIsDirty = true;
    }

    if (pDirection)
    {
        const _float fEpsilon = 1e-6f;

        _vector vDir = *pDirection;
        
        if (XMVectorGetX(XMVector3LengthSq(vDir)) > fEpsilon * fEpsilon)
        {
            vDir = XMVector3Normalize(vDir);

            if (auto* pDecal = dynamic_cast<CEffect_Decal*>(pObj))
            {
                _vector vSrc = XMVectorSet(0.f, 0.f, 1.f, 0.f);

                _float fDot = XMVectorGetX(XMVector3Dot(vSrc, vDir));
                fDot = clamp(fDot, -1.f, 1.f);

                _vector vAxis = XMVector3Cross(vSrc, vDir);
                _float fAxisLenSq = XMVectorGetX(XMVector3LengthSq(vAxis));
                _float fAngle = 0.f;

                if (fAxisLenSq < fEpsilon * fEpsilon)
                {
                    if (fDot > 0.999999f)
                    {
                        // 0도일 경우 회전 X
                        fAngle = 0.f;
                        vAxis = XMVectorSet(1.f, 0.f, 0.f, 0.f); // 임의의 축
                    }

                    else
                    {
                        // 180도 vSrc와 수직인 임의의 축
                        vAxis = XMVector3Orthogonal(vSrc);
                        vAxis = XMVector3Normalize(vAxis);
                        fAngle = XM_PI;
                    }
                }

                else
                {
                    vAxis = XMVector3Normalize(vAxis);
                    fAngle = acosf(fDot);
                }

                if (pDecal->Is_UseSocket())
                {
                    // 소켓 데칼은 오프셋 회전만 적용
                    _float3 vAxis3;
                    XMStoreFloat3(&vAxis3, vAxis);
                    pDecal->Set_OffsetRotationAxisAngle(vAxis3, fAngle);
                    bIsDirty = true;
                }

                else
                {
                    pTransform->Rotation(vAxis, fAngle);
                    bIsDirty = true;
                }
            }

            else
            {
                // 데칼이 아닌 일반 이펙트
                _vector vSrc = XMVectorSet(0.f, 1.f, 0.f, 0.f);

                _float fDot = XMVectorGetX(XMVector3Dot(vSrc, vDir));
                fDot = clamp(fDot, -1.f, 1.f);

                _vector vAxis = XMVector3Cross(vSrc, vDir);
                _float fAxisLenSq = XMVectorGetX(XMVector3LengthSq(vAxis));
                _float fAngle = 0.f;

                if (vAxisLenSq < fEpsilon * fEpsilon)
                {
                    if (fDot > 0.999999f)
                    {
                        fAngle = 0.f;
                        vAxis = XMVectorSet(1.f, 0.f, 0.f, 0.f);
                    }

                    else
                    {
                        vAxis = XMVector3Orthogonal(vSrc);
                        vAxis = XMVector3Normalize(vAxis);
                        fAngle = XM_PI;
                    }
                }

                else
                {
                    vAxis = XMVector3Normalize(vAxis);
                    fAngle = acosf(fDot);
                }

                pTransform->Rotation(vAxis, fAngle);
                bIsDirty = true;
            }
        }
    }

    if (bIsDirty)
        pTransform->Update(0.f, pObj);
}

// 이펙트/파티클 분기 재생 헬퍼 함수
void CEmitter::Play_Effect(CGameObject* pObj)
{
    if (!pObj)
        return;

    if (auto* pEffect = dynamic_cast<CEffectObject*>(pObj))
    {
        pEffect->Play_Effect(true);
    }

    else if (auto* pParticle = dynamic_cast<CParticleObject*>(pObj))
    {
        if (auto* pBuffer = pParticle->Find_Component<CVIBuffer_Point_Instancing>(CLASS_NAME(CVIBuffer)))
            pBuffer->Update_IPBuffer();
        
        pParticle->Play_Particle(true);
    }
}

POOL_DESC CEmitter::Make_PoolDesc(CGameObject* pObj, const json& j)
{
    POOL_DESC pd;
    pd.pEffect = pObj;
    pd.wsObjectTag = Get_ObjectTag_FromJson(j);
    pd.bOneShot = j.value("bOneShot", false);
    pd.fInterval = j.value("fInterval", j.value("fLifeTime", 1.f));
    pd.fElapsed = pd.fInterval; // 필요 시 Trigger 호출 직후 0으로 리셋

    return pd;
}

_wstring CEmitter::Get_ObjectTag_FromJson(const json& j)
{
    if (j.contains("szGameObjectTag"))
        return StringToWstring(j["szGameObjectTag"].get<_string>());
    if (j.contains("szPrototypeTag"))
        return StringToWstring(j["szPrototypeTag"].get<_string>());

    return L"";
}

CEmitter* CEmitter::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CEmitter* pInstance = new CEmitter(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed To Created : CEmitter");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CEmitter::Clone(void* pArg, CGameObject* pOwner)
{
	CComponent* pInstance = new CEmitter(*this, pOwner);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed To Cloned : CEmitter");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CEmitter::Free()
{
	__super::Free();
}