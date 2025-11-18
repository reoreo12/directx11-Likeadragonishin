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
    m_SpawnGroups.clear();
    m_GroupEnabled.clear();
}

HRESULT CEmitter::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CEmitter::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

    m_GroupEnabled.clear();
    m_SpawnGroups.clear();

	return S_OK;
}

void CEmitter::Priority_Update(const _float& fTimeDelta, CGameObject* pOwner)
{
    int a = 0;
}

void CEmitter::Update(const _float& fTimeDelta, CGameObject* pOwner)
{
    // 활성화된 그룹의 prototype 생성
    for (auto& [tag, group] : m_SpawnGroups)
    {
        if (!m_GroupEnabled[tag])
            continue;

        for (auto& prototype : group.prototypes)
        {
            if (prototype.bOneShot && prototype.bHasSpawned)
                continue;
        
            prototype.fElapsed += fTimeDelta;
            if (prototype.fElapsed >= prototype.fInterval)
            {
                prototype.fElapsed -= prototype.fInterval;
        
                // pPosition가 nullptr이면 원래 소켓 위치대로 생성
                Spawn_Prototype(tag, prototype, nullptr, nullptr);
                prototype.bHasSpawned = true; // OneShot이면 더 이상 스폰하지 않도록
            }
        }
    }

     // 죽은 동적 인스턴스 정리
     for (auto& [_, group] : m_SpawnGroups)
     {
         auto& vec = group.dynamic;

         for (auto iter = vec.begin(); iter != vec.end();)
         {
             if (iter->pEffect && iter->pEffect->Is_Dead())
                 iter = vec.erase(iter);
             else
                 iter++;
         }
     }
#pragma endregion
}

void CEmitter::Late_Update(const _float& fTimeDelta, CGameObject* pOwner)
{
    int a = 0;
}

HRESULT CEmitter::LoadEffects_FromFile(const _wstring& wsTag, const _wstring& wsFilePath)
{
    // 이미 로드된 그룹이면 스킵
    if (m_SpawnGroups.count(wsTag))
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
    
    auto& group = m_SpawnGroups[wsTag];
    m_GroupEnabled[wsTag] = false;
    
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
    
            // !DestroyOnEnd -> persistent로 분류
            _bool bDestroyOnEnd = j.value("bDestroyOnEnd", false);
    
            if (!bDestroyOnEnd)
            {
                // 미리 생성해 두고 재활용
                CGameObject* pChild = pOwner->Create_Child(iPrototypeLevel, wsPrototypeTag, &j);
                if (!pChild)
                    continue;

                _string sObjTag = j.value("szGameObjectTag", j["szPrototypeTag"].get<_string>());
                _wstring wsObjTag = StringToWstring(sObjTag);

                if (auto* pSocketEffect = dynamic_cast<CSocketEffect_Sprite*>(pChild))
                {
                    // 소켓 매트릭스 셋업
                    if (j.contains("SocketTag"))
                    {
                        _wstring wsSocketTag = StringToWstring(j["SocketTag"].get<_string>());
                        if (pCharacter)
                        {
                            _wstring wsBodyTag = pCharacter->Get_CurBodyTag();
                            if (auto* pBody = dynamic_cast<CBody*>(pOwner->Find_Child(wsBodyTag)))
                            {
                                pSocketEffect->Set_SocketMatrix(pBody->Get_SocketMatrix(wsSocketTag));
                            }
                        }
                    }

                    group.persistent.push_back({ pSocketEffect, false, wsObjTag });
                }

                else if (auto* pSocketEffect = dynamic_cast<CSocketEffect_Mesh*>(pChild))
                {
                    // 소켓 매트릭스 셋업
                    if (j.contains("SocketTag"))
                    {
                        _wstring wsSocketTag = StringToWstring(j["SocketTag"].get<_string>());
                        if (pCharacter)
                        {
                            _wstring wsBodyTag = pCharacter->Get_CurBodyTag();
                            if (auto* pBody = dynamic_cast<CBody*>(pOwner->Find_Child(wsBodyTag)))
                            {
                                pSocketEffect->Set_SocketMatrix(pBody->Get_SocketMatrix(wsSocketTag));
                            }
                        }
                    }

                    group.persistent.push_back({ pSocketEffect, false, wsObjTag });
                }
        
                else if (auto* pSocketEffect = dynamic_cast<CEffect_Decal*>(pChild))
                {
                    // 소켓 매트릭스 셋업
                    if (j.contains("SocketTag"))
                    {
                        _wstring wsSocketTag = StringToWstring(j["SocketTag"].get<_string>());
                        if (pCharacter)
                        {
                            _wstring wsBodyTag = pCharacter->Get_CurBodyTag();
                            if (auto* pBody = dynamic_cast<CBody*>(pOwner->Find_Child(wsBodyTag)))
                            {
                                pSocketEffect->Set_SocketMatrix(pBody->Get_SocketMatrix(wsSocketTag));
                            }
                        }
                    }

                    group.persistent.push_back({ pSocketEffect, false, wsObjTag });
                }
            }
            // DestroyOnEnd -> Prototype으로 분류
            else
            {
                // 매 스폰 시 새로 Create
                PROTO_DESC pd;
                pd.iPrototypeLevel = iPrototypeLevel;
                pd.wsPrototypeTag = wsPrototypeTag;
                pd.jSpawnParams = j;

                // 단발성 여부
                pd.bOneShot = j.value("bOneShot", false);
                // 단발성 X ->  생성 주기 필요
                if (!pd.bOneShot)
                {
                    pd.fInterval = j.value("fInterval", j.value("fLiteTime", 1.f));
                }
                else
                    pd.fInterval = 1.f;

                // Trigger 발동 시, 바로 생성되도록
                pd.fElapsed = pd.fInterval;
                pd.bHasSpawned = false;
    
                group.prototypes.push_back(move(pd));
            }
        }
    }
    // SocketParticle
    if (jData.contains("ChildParticle") && jData["ChildParticle"].is_array())
    {
        for (auto& j : jData["ChildParticle"])
        {
            if (!j.contains("iPrototypeLevel") || !j.contains("szPrototypeTag"))
                continue;
    
            _uint iPrototypeLevel = j["iPrototypeLevel"].get<_uint>();
            _wstring wsPrototypeTag = StringToWstring(j["szPrototypeTag"].get<_string>());
    
            _bool bDestroyOnEnd = j.value("bDestroyOnEnd", false);
    
            if (!bDestroyOnEnd)
            {
                CGameObject* pChild = pOwner->Create_Child(iPrototypeLevel, wsPrototypeTag, &j);
                if (!pChild)
                    return E_FAIL;

                _string sObjTag = j.value("szGameObjectTag", j["szPrototypeTag"].get<_string>());
                _wstring wsObjTag = StringToWstring(sObjTag);

                auto* pSocketParticle = dynamic_cast<CSocketParticle_Sprite*>(pChild);
                if (!pSocketParticle)
                    continue;
    
                if (j.contains("SocketTag"))
                {
                    _wstring wsSocketTag = StringToWstring(j["SocketTag"].get<string>());

                    if (pCharacter)
                    {
                        _wstring wsBodyTag = pCharacter->Get_CurBodyTag();
                        if (auto* pBody = dynamic_cast<CBody*>(pOwner->Find_Child(wsBodyTag)))
                            pSocketParticle->Set_SocketMatrix(pBody->Get_SocketMatrix(wsSocketTag));
                    }
                }
    
                group.persistent.push_back({ pSocketParticle, false, wsObjTag });
            }
            else
            {
                PROTO_DESC pd;
                pd.iPrototypeLevel = iPrototypeLevel;
                pd.wsPrototypeTag = wsPrototypeTag;
                pd.jSpawnParams = j;
    
                // 단발성 여부
                pd.bOneShot = j.value("bOneShot", false);
                // 단발성 X ->  생성 주기 필요
                if (!pd.bOneShot)
                {
                    pd.fInterval = j.value("fInterval", j.value("fLiteTime", 1.f));
                }
                else
                    pd.fInterval = 1.f;
    
                pd.fElapsed = pd.fInterval;
                group.prototypes.push_back(move(pd));
            }
        }
    }

    // 일반 Effect/Particle
    for (auto& j : jData["GameObjects"])
    {
        if (!j.contains("iPrototypeLevel") || !j.contains("szPrototypeTag") || !j.contains("szLayerTag"))
            continue;

        _uint iPrototypeLevel = j["iPrototypeLevel"].get<_uint>();
        _wstring wsPrototypeTag = StringToWstring(j["szPrototypeTag"].get<string>());
        _wstring wsLayerTag = StringToWstring(j["szLayerTag"].get<string>());

        _bool bDestroyOnEnd = j.value("bDestroyOnEnd", false);

        if (!bDestroyOnEnd)
        {
            CGameObject* pObj = nullptr;
            if (FAILED(CGameInstance::GetInstance()->Add_GameObject(iPrototypeLevel, wsPrototypeTag, iCurLevel, wsLayerTag, &pObj, &j)))
                return E_FAIL;

            _string sObjTag = j.value("szGameObjectTag",j["szPrototypeTag"].get<_string>());
            _wstring wsObjTag = StringToWstring(sObjTag);

            if (pObj)
                group.persistent.push_back({ pObj, false, wsObjTag });
        }
        else
        {
            PROTO_DESC pd;
            pd.iPrototypeLevel = iPrototypeLevel;
            pd.wsPrototypeTag = wsPrototypeTag;
            pd.wsLayerTag = wsLayerTag;
            pd.jSpawnParams = j;

            pd.bOneShot = j.value("bOneShot", true);
            if (!pd.bOneShot)
            {
                pd.fInterval = j.value("fInterval", j.value("fLiteTime", 1.f));
            }
            else
                pd.fInterval = 1.f;

            pd.fElapsed = pd.fInterval;
            pd.bHasSpawned = false;

            group.prototypes.push_back(move(pd));
        }
    }

	return S_OK;
}

void CEmitter::Trigger(const _wstring& wsTag, const _vector* pPosition, const _vector* pDirection)
{
    // 그룹 활성화
    m_GroupEnabled[wsTag] = true;

    // Persistent 인스턴스 켜기
    auto iter = m_SpawnGroups.find(wsTag);
    if (iter == m_SpawnGroups.end())
        return;
    
    auto& group = m_SpawnGroups[wsTag];
    for (auto& prototype : group.prototypes)
    {
        // Oneshot 이어도, Trigger 시엔 무조건 한 번
        CGameObject* pObject = Spawn_Prototype(wsTag, prototype, pPosition, pDirection);
        prototype.bHasSpawned = true;
        prototype.fElapsed = 0.f; // 재생 시간 초기화
    }

    for (auto& persistent : iter->second.persistent)
    {
        if (!persistent.pEffect)
            continue;

        persistent.pEffect->Set_Active(true);

        if (pPosition)
        {
            if (auto* pTransform = persistent.pEffect->Get_Transform())
            {
                pTransform->Set_State(CTransform::STATE_POSITION, *pPosition);
                pTransform->Update(0.f, persistent.pEffect);
            }
        }

        if (pDirection)
        {
            auto* pTransform = persistent.pEffect->Get_Transform();
            if (pTransform)
            {
                _vector vSrc = XMVectorSet(0.f, 1.f, 0.f, 0.f);
                _vector vDst = XMVector3Normalize(XMLoadFloat4(reinterpret_cast<const _float4*>(pDirection)));

                // 회전축 = cross(src, dst)
                _vector vAxis = XMVector3Cross(vSrc, vDst);
                // 두 벡터 사이 각도
                _float fDot = XMVectorGetX(XMVector3Dot(vSrc, vDst));
                fDot = fDot < -1.f ? -1.f : (fDot > 1.f ? 1.f : fDot);
                _float fAngle = acosf(fDot);

                // 축, 각회전 적용
                pTransform->Rotation(vAxis, fAngle);
                pTransform->Update(0.f, persistent.pEffect);
            }
        }

        if (auto* pParticle = dynamic_cast<CParticleObject*>(persistent.pEffect))
        {
            if (auto* pBuffer = pParticle->Find_Component<CVIBuffer_Point_Instancing>(CLASS_NAME(CVIBuffer)))
            {
                pBuffer->Update_IPBuffer();
            }

            pParticle->Play_Particle(true);
        }

        if (auto* pEffect = dynamic_cast<CEffectObject*>(persistent.pEffect))
        {
            pEffect->Play_Effect(true);
        }
    }
}

void CEmitter::Stop_Trigger(const _wstring& wsTag)
{
    // 그룹 비활성화
    m_GroupEnabled[wsTag] = false;

    // Persistent 인스턴스 끄기
    auto it = m_SpawnGroups.find(wsTag);
    if (it == m_SpawnGroups.end())
        return;

    for (auto& per : it->second.persistent)
    {
        if (per.pEffect)
            per.pEffect->Set_Active(false);
    }
}

void CEmitter::Decal(const _wstring& wsGroupTag, const _vector* pPos, _float fRadius, const _vector* pDir)
{
    // 원 위의 랜덤 위치 스폰
    _vector vSpawnPos = XMVectorZero();
    if (pPos)
    {
        _float2 vUnit = Random_OnCircle();
        _float  fDist = Random(0.f, fRadius);
        _vector vOffset = XMVectorSet(vUnit.x * fDist, 0.f, vUnit.y * fDist, 0.f);
        vSpawnPos = *pPos + vOffset;
    }

    // 풀(persistent) 가져오기
    auto it = m_SpawnGroups.find(wsGroupTag);
    if (it == m_SpawnGroups.end())
        return;

    auto& pool = it->second.persistent;
    for (auto& slot : pool)
    {
        auto* pEffect = slot.pEffect;
        if (!pEffect || pEffect->Is_Active())
            continue;

        pEffect->Set_Active(true);

        if (auto* pTransform = pEffect->Get_Transform())
        {
            // 소켓 데칼
            if (auto* pDecal = dynamic_cast<CEffect_Decal*>(pEffect);
                pDecal && pDecal->Is_UseSocket())
            {
                if (pDir)
                {
                    _vector vDefaultFwd = XMVectorSet(0.f, 0.f, 1.f, 0.f);
                    _vector vDir = XMVector3Normalize(*pDir);
                    _vector vAxis = XMVector3Normalize(XMVector3Cross(vDefaultFwd, vDir));
                    _float fDot = XMVectorGetX(XMVector3Dot(vDefaultFwd, vDir));
                    _float fAngle = acosf(fDot);
                    
                    _float3 vAxis3;
                    XMStoreFloat3(&vAxis3, vAxis);
                    
                    pDecal->Set_OffsetRotationAxisAngle(vAxis3, fAngle);
                }
            }
            // 일반 데칼
            else
            {
                if (pPos)
                    pTransform->Set_State(CTransform::STATE_POSITION, vSpawnPos);

                if (pDir)
                {
                    _vector vTarget = (pPos ? vSpawnPos : XMVectorZero()) + *pDir;
                    pTransform->LookAt(vTarget);
                }
            }

            pTransform->Update(0.f, pEffect);
        }

        if (auto* pParticleObject = dynamic_cast<CParticleObject*>(pEffect))
            pParticleObject->Play_Particle(true);
        else if (auto* pEffectObject = dynamic_cast<CEffectObject*>(pEffect))
            pEffectObject->Play_Effect(true);

        break;
    }
}

void CEmitter::Play_Pool(const _wstring& wsGroupTag, const _vector* pPosition, const _vector* pDirection)
{
    m_GroupEnabled[wsGroupTag] = true;

    auto iter = m_SpawnGroups.find(wsGroupTag);
    if (iter == m_SpawnGroups.end())
        return;

    unordered_set<_wstring> playedTags;

    auto& pool = iter->second.persistent;
    for (auto& slot : pool)
    {
        if (playedTags.count(slot.wsObjectTag))
            continue;

        auto* pEffect = slot.pEffect;
        if (!pEffect || pEffect->Is_Active())
            continue;

        pEffect->Set_Active(true);
        playedTags.insert(slot.wsObjectTag);

        if (pPosition)
        {
            if (auto* pTransform = pEffect->Get_Transform())
            {
                pTransform->Set_State(CTransform::STATE_POSITION, *pPosition);
                pTransform->Update(0.f, pEffect);
            }
        }

        if (pDirection)
        {
            if (auto* pTransform = pEffect->Get_Transform())
            {
                _vector vDir = *pDirection;
                _vector vLen = XMVector3Length(vDir);
                _float fLen = XMVectorGetX(vLen);

                const _float fEpsilon = 1e-6f;
                if (fLen > fEpsilon)
                {
                    _vector vSrc = XMVectorSet(0.f, 1.f, 0.f, 0.f);
                    _vector vDst = XMVector3Normalize(vDir);

                    _float fDot = XMVectorGetX(XMVector3Dot(vSrc, vDst));
                    fDot = clamp(fDot, -1.f, 1.f);

                    _vector vAxis;
                    if (fabsf(fDot) < 1.f - fEpsilon)
                    {
                        vAxis = XMVector3Cross(vSrc, vDst);
                        _vector vAxisLen = XMVector3Length(vAxis);
                        if (XMVectorGetX(vAxisLen) > fEpsilon)
                            vAxis = XMVector3Normalize(vAxis);
                        else
                            vAxis = XMVectorSet(1.f, 0.f, 0.f, 0.f);
                    }

                    else
                    {
                        vAxis = XMVectorSet(1.f, 0.f, 0.f, 0.f);
                    }

                    _float fAngle = acosf(fDot);
                    pTransform->Rotation(vAxis, fAngle);
                    pTransform->Update(0.f, pEffect);
                }
            }
        }

        if (auto* pEObj = dynamic_cast<CEffectObject*>(pEffect))
        {
            pEObj->Play_Effect(true);
        }
        else if (auto* pPObj = dynamic_cast<CParticleObject*>(pEffect))
        {
            if (auto* pBuffer = pPObj->Find_Component<CVIBuffer_Point_Instancing>(CLASS_NAME(CVIBuffer)))
                pBuffer->Update_IPBuffer();
            pPObj->Play_Particle(true);
        }
    }
}

_bool CEmitter::Is_GroupEnabled(const _wstring& wsTag) const
{
    auto iter = m_GroupEnabled.find(wsTag);
    return iter != m_GroupEnabled.end() && iter->second;
}

void CEmitter::Set_SpawnGroupInterval(const _wstring& wsGroupTag, _float fNewInterval)
{
    auto group = m_SpawnGroups.find(wsGroupTag);
    if (group == m_SpawnGroups.end())
        return;

    for (auto& proto : group->second.prototypes)
    {
        proto.fInterval = fNewInterval;
        proto.fElapsed = min(proto.fElapsed, proto.fInterval);
    }
}

CGameObject* CEmitter::Spawn_Prototype(const wstring& wsGroupTag, const PROTO_DESC& pd, const _vector* pPosition, const _vector* pDirection)
{
    CGameObject* pObject = nullptr;

    // SocketEffect / SocketParticle
    if (pd.wsPrototypeTag.rfind(L"CSocket", 0) == 0)
    {
        pObject = Get_Owner()->Create_Child(pd.iPrototypeLevel, pd.wsPrototypeTag, (void*)&pd.jSpawnParams);
        auto* pChararacter = dynamic_cast<CCharacter*>(Get_Owner());

        if (pChararacter)
        {
            // 소켓 세팅
            if (pObject && pd.jSpawnParams.contains("SocketTag"))
            {
                _wstring wsSocketTag = StringToWstring(pd.jSpawnParams["SocketTag"].get<string>());
                _wstring wsCurBody = pChararacter->Get_CurBodyTag();

                if (auto* pBody = dynamic_cast<CBody*>(Get_Owner()->Find_Child(wsCurBody)))
                {
                    const _float4x4* pSocketMatrix = pBody->Get_SocketMatrix(wsSocketTag);

                    if (auto* pSocketEffect = dynamic_cast<CSocketEffect_Sprite*>(pObject))
                        pSocketEffect->Set_SocketMatrix(pSocketMatrix);
                    else if (auto* pSocketParticle = dynamic_cast<CSocketParticle_Sprite*>(pObject))
                        pSocketParticle->Set_SocketMatrix(pSocketMatrix);
                }
            }
        }
    }
    // Effect_Sprite/Particle_Sprite
    else
    {
        LEVEL iCurLevel = CManagers::GetInstance()->Get_CurrentLevel();

        if (FAILED(CGameInstance::GetInstance()->Add_GameObject(pd.iPrototypeLevel, pd.wsPrototypeTag, iCurLevel, pd.wsLayerTag, &pObject, (void*)&pd.jSpawnParams)))
            return nullptr;

        if (pObject)
        {
            auto* pTransform = pObject->Get_Transform();

            if (pTransform)
            {
                if (pPosition)
                {
                    pTransform->Set_State(CTransform::STATE_POSITION, *pPosition);
                }

                if (pDirection)
                {
                    _vector vSrc = XMVectorSet(0.f, 1.f, 0.f, 0.f);
                    _vector vDst = XMVector3Normalize(XMLoadFloat4(reinterpret_cast<const _float4*>(pDirection)));

                    // 회전축 = cross(src, dst)
                    _vector vAxis = XMVector3Cross(vSrc, vDst);
                    // 두 벡터 사이 각도
                    _float fDot = XMVectorGetX(XMVector3Dot(vSrc, vDst));
                    fDot = fDot < -1.f ? -1.f : (fDot > 1.f ? 1.f : fDot);
                    _float fAngle = acosf(fDot);

                    // Transform 에 축, 각회전 적용
                    pTransform->Rotation(vAxis, fAngle);
                }

                pTransform->Update(0.f, pObject);
            }
        }
    }

    if (!pObject)
        return nullptr;

    if (auto* pEffect = dynamic_cast<CEffectObject*>(pObject))
    {
        pEffect->Set_Active(true);
        pEffect->Play_Effect(true);
    }
    else if (auto* pParticle = dynamic_cast<CParticleObject*>(pObject))
    {
        pParticle->Set_Active(true);
        pParticle->Play_Particle(true);
    }

    if (pObject)
    {
        // 동적 그룹에 보관
        auto& group = m_SpawnGroups[wsGroupTag];
        group.dynamic.push_back({ pObject, true });
    }

    return pObject;
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