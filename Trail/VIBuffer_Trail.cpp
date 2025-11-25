#include "VIBuffer_Trail.h"

CVIBuffer_Trail::CVIBuffer_Trail(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CVIBuffer_Instancing(pDevice, pContext)
{
    lstrcpy(m_tComponentDesc.szComponentTag, CLASS_NAME(CVIBuffer_Trail));
}

CVIBuffer_Trail::CVIBuffer_Trail(const CVIBuffer_Trail& Prototype, CGameObject* pOnwer)
    : CVIBuffer_Instancing(Prototype, pOnwer)
{
    lstrcpy(m_tComponentDesc.szComponentTag, CLASS_NAME(CVIBuffer_Trail));
}

HRESULT CVIBuffer_Trail::Initialize_Prototype(CVIBuffer_Instancing::INSTANCE_DESC* pDesc)
{
    // 한 트레일이 갖는 정점 쌍(TRAIL_POINT { vLow, vHigh }) 개수
    m_iMaxTrailPoints = pDesc->iNumInstances;

    m_iVertexStride = sizeof(VTXTRAIL);

    // 총 정점 개수에 따라 세그먼트 결정
    // 포인트가 0개나 1개밖에 없으면 두 점을 잇지도 못 하기 때문에 정점 개수를 0으로 설정
    if (m_iMaxTrailPoints < 2)
    {
        m_iNumVertices = 0;
    }
    // 포인트가 2~ 3개인 경우, Catmull-Rom 커브를 여러 세그먼트로 나누지 않고 단 한 구간만 보간
    else if (m_iMaxTrailPoints == 2 || m_iMaxTrailPoints == 3)
    {
        m_iNumVertices = m_iCatmullCount * 2;
    }
    // 포인트가 4개 이상이면, "P0->P1->P2->P3" 커브를 연속으로 여러 구간 이어 붙일 수 있다
    else
    {
        // 세그먼트 개수: 하나당 정점이 네 개 필요하니까, (트레일에 쓰이는 총 정점 개수 - 3)
        _int iMaxCurveCount = m_iMaxTrailPoints - 3;
        
        // 마지막 세그먼트를 제외한 세그먼트들에 쓰이는 정점의 개수
        // (iCatmullCount - 1) * 2 = 한 세그먼트당 보간 횟수(점 개수) * (상단, 하단 점 쌍)
        _int iVertices = (m_iCatmullCount + (iMaxCurveCount - 1) * (m_iCatmullCount - 1)) * 2;
        
        // 마지막 세그먼트의 정점 개수
        _int iVertices_segment = (m_iCatmullCount - 1) * 2;

        // 총 정점 개수
        m_iNumVertices = iVertices + iVertices_segment;
    }

    m_iNumInstances = m_iMaxTrailPoints;

    m_iInstanceStride = sizeof(VTXINSTANCE);
    m_iIndexStride = sizeof(_ushort);
    m_iNumVertexBuffers = 2;
    m_eIndexFormat = DXGI_FORMAT_R16_UINT;
    m_eTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;


#pragma region Index_Buffer
    _int iTotalVertex = m_iNumVertices / 2;
    if (m_iNumVertices == 0 || iTotalVertex < 1)
        m_iNumIndices = 0;
    else
        m_iNumIndices = (iTotalVertex - 1) * 6;

    D3D11_BUFFER_DESC indexDesc = {};
    indexDesc.ByteWidth = m_iIndexStride * m_iNumIndices;
    indexDesc.Usage = D3D11_USAGE_DYNAMIC;
    indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    vector<_ushort> indices(m_iNumIndices);
    for (_int i = 0; i < iTotalVertex - 1; ++i)
    {
        _int iBase = i * 2;
        indices[i * 6] = iBase;
        indices[i * 6 + 1] = iBase + 2;
        indices[i * 6 + 2] = iBase + 1;
        indices[i * 6 + 3] = iBase + 1;
        indices[i * 6 + 4] = iBase + 2;
        indices[i * 6 + 5] = iBase + 3;
    }

    D3D11_SUBRESOURCE_DATA initialData = {};
    initialData.pSysMem = indices.data();

    m_pDevice->CreateBuffer(&indexDesc, &initialData, &m_pIB);
#pragma endregion

    m_pInstanceVertices = new VTXINSTANCE[m_iNumInstances];
    ZeroMemory(m_pInstanceVertices, sizeof(VTXINSTANCE) * m_iNumInstances);

    return S_OK;
}

HRESULT CVIBuffer_Trail::Initialize(void* pArg)
{
#pragma region Vertex_Buffer
    D3D11_BUFFER_DESC vertexDesc = {};
    vertexDesc.ByteWidth = m_iVertexStride * m_iNumVertices;
    vertexDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    vector<VTXTRAIL> initialVertices(m_iNumVertices);
    for (_int i = 0; i < m_iNumVertices; ++i)
        initialVertices[i].vPosition = _float3(0.f, 0.f, 0.f);

    D3D11_SUBRESOURCE_DATA vertexData = {};
    vertexData.pSysMem = initialVertices.data();

    m_pDevice->CreateBuffer(&vertexDesc, &vertexData, &m_pVB);
#pragma endregion

    return S_OK;
}

HRESULT CVIBuffer_Trail::Render()
{
    if (m_iNumInstances < 2)
        return E_FAIL;

    m_pContext->DrawIndexed(m_iCurIndexCnt, 0, 0);

    return S_OK;
}

void CVIBuffer_Trail::UpdateTrail(_vector vSwordLow, _vector vSwordHigh)
{
    if (!m_TrailPoints.empty())
    {
        _vector vPrevLow = m_TrailPoints.back().vLow;
        _vector vPrevHigh = m_TrailPoints.back().vHight;

        _vector vPrevDir = XMVector3Normalize(vPrevHigh - vPrevLow);
        _vector vCurDir = XMVector3Normalize(vSwordHigh - vSwordLow);
    }

    // 점 쌍 추가
    m_TrailPoints.push_back({ vSwordLow, vSwordHigh });

    // 툴에서 실시간으로 길이 변화를 보기 위해 while
    while (m_TrailPoints.size() > m_iMaxTrailPoints)
        m_TrailPoints.pop_front();

    // Catmull-Rom 보간을 위한 최소한의 점 쌍 개수 확인
    _int iNumActualPoints = (_int)m_TrailPoints.size();
    if (iNumActualPoints < 2) // 최소 2개의 점이 있어야 선이라도 그릴 수 있다
        return;

    // V 좌표 계산을 위한 m_iNumVertices 사전 계산

    // const _int iCatmullCount = 4; // 각 세그먼트당 보간할 점의 수(쌍으로)
    _int iNumVertices_ThisFrame = 0;

    // 점이 2개면 직선 1개
    // 점이 3개면 Catmull-Rom 1개 세그먼트(P0,P1,P2 / P3은 외삽) P0=TP[0], P1=TP[1], P2=TP[2], P3=TP[2]+(TP[2]-TP[1])
    if (iNumActualPoints == 2 || iNumActualPoints == 3) {
        iNumVertices_ThisFrame = m_iCatmullCount * 2;
    }
    else if (iNumActualPoints >= 4) {
        // 기존 루프가 (numActualPoints - 3)개의 Catmull-Rom 세그먼트를 그린다
        // 각 세그먼트는 iCatmullCount개의 버텍스 쌍(Low, High)을 생성
        _int iMainLoopSegments = iNumActualPoints - 3;
        iNumVertices_ThisFrame = iMainLoopSegments * m_iCatmullCount * 2;

        // 추가될 마지막 세그먼트(P0=TP[N-3], P1=TP[N-2], P2=TP[N-1], P3=외삽)
        // 이 세그먼트는 P1(TP[N-2])에서 시작하므로, (iCatmullCount - 1)개의 새 점 쌍을 추가(t=0 제외)
        iNumVertices_ThisFrame += (m_iCatmullCount - 1) * 2;
    }

    // 버텍스 버퍼 업데이트
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    if (FAILED(m_pContext->Map(m_pVB, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource)))
        return;

    VTXTRAIL* pVertices = reinterpret_cast<VTXTRAIL*>(mappedResource.pData);
    _int iVtxIdx = 0;

    // 기존 Catmull-Rom 보간 반복문(numActualPoints >= 4일 때만 조건을 충족하고 실행)
    // 이 루프는 m_TrailPoints[size-2] 까지만 트레일을 그린다
    _int iCurveCount = iNumActualPoints - 3; // 기존과 동일한 CurveCount
    if (iCurveCount > 0) { // NumActualPoints >= 4
        for (_int i = 0; i < iCurveCount; ++i) {
            _vector vP0_low = m_TrailPoints[i + 0].vLow;
            _vector vP1_low = m_TrailPoints[i + 1].vLow;
            _vector vP2_low = m_TrailPoints[i + 2].vLow;
            _vector vP3_low = m_TrailPoints[i + 3].vLow;
            
            _vector vP0_high = m_TrailPoints[i + 0].vHight;
            _vector vP1_high = m_TrailPoints[i + 1].vHight;
            _vector vP2_high = m_TrailPoints[i + 2].vHight;
            _vector vP3_high = m_TrailPoints[i + 3].vHight;

            for (_int j = 0; j < m_iCatmullCount; ++j) {
                _float t = (m_iCatmullCount == 1) ? 0.f : _float(j) / _float(m_iCatmullCount - 1);

                // 중복점 회피: 이전 세그먼트의 마지막 점(t=1)과 현재 세그먼트의 첫 점(t=0)은 동일
                // 첫 번째 세그먼트(i=0)가 아니면서 현재 세그먼트의 첫 점(j=0)이면 스킵
                if (i > 0 && j == 0) {
                    continue;
                }

                _vector vInterpLow = XMVectorCatmullRom(vP0_low, vP1_low, vP2_low, vP3_low, t);
                _vector vInterpHigh = XMVectorCatmullRom(vP0_high, vP1_high, vP2_high, vP3_high, t);

                XMStoreFloat3(&pVertices[iVtxIdx].vPosition, vInterpLow);
                // V좌표 계산시 m_iNumVertices 대신 m_iNumVertices_ThisFrame 사용 또는 m_iNumVertices를 업데이트
                pVertices[iVtxIdx].vTexcoord = _float2(0.f, (iNumVertices_ThisFrame > 1) ? _float(iVtxIdx) / _float(iNumVertices_ThisFrame - 1) : 0.f);
                ++iVtxIdx;

                XMStoreFloat3(&pVertices[iVtxIdx].vPosition, vInterpHigh);
                pVertices[iVtxIdx].vTexcoord = _float2(1.f, (iNumVertices_ThisFrame > 1) ? _float(iVtxIdx - 1) / _float(iNumVertices_ThisFrame - 1) : 0.f); // high와 low는 같은 v값을 가질 수 있음
                ++iVtxIdx;
            }
        }
    }

    // 마지막 세그먼트 추가(검의 현재 위치까지 트레일 연장)
    // NumActualPoints가 최소 2개 이상일 때 작동(P0, P1을 만들 수 있어야 한다)
    // NumActualPoints가 2일 때: P0=TP[0], P1=TP[0], P2=TP[1], P3=TP[1](직선)
    // NumActualPoints가 3일 때: P0=TP[0], P1=TP[1], P2=TP[2], P3=TP[2]+(TP[2]-TP[1])(외삽)
    // NumActualPoints가 4 이상일 때: P0=TP[N-3], P1=TP[N-2], P2=TP[N-1], P3=TP[N-1]+(TP[N-1]-TP[N-2])(외삽)
    if (iNumActualPoints >= 2) {
        _vector vP0_Low_Final, vP1_Low_Final, vP2_Low_Final, vP3_Low_Final;
        _vector vP0_High_Final, vP1_High_Final, vP2_High_Final, vP3_High_Final;

        vP2_Low_Final = m_TrailPoints[iNumActualPoints - 1].vLow; // 현재 검 위치(P2)
        vP2_High_Final = m_TrailPoints[iNumActualPoints - 1].vHight;
        vP1_Low_Final = m_TrailPoints[iNumActualPoints - 2].vLow; // 이전 검 위치(P1)
        vP1_High_Final = m_TrailPoints[iNumActualPoints - 2].vHight;

        if (iNumActualPoints == 2) { // 점이 2개뿐(TP[0], TP[1])
            vP0_Low_Final = vP1_Low_Final;   // P0 = P1(TP[0])
            vP0_High_Final = vP1_High_Final;
            vP3_Low_Final = vP2_Low_Final;   // P3 = P2 (TP[1]): 직선 보간
            vP3_High_Final = vP2_High_Final;
        }
        else { // 점이 3개 이상(TP[N-3], TP[N-2], TP[N-1])
            vP0_Low_Final = m_TrailPoints[iNumActualPoints - 3].vLow; // P0
            vP0_High_Final = m_TrailPoints[iNumActualPoints - 3].vHight;
            // P3 외삽: P3 = P2 + (P2 - P1)
            vP3_Low_Final = XMVectorAdd(vP2_Low_Final, XMVectorSubtract(vP2_Low_Final, vP1_Low_Final));
            vP3_High_Final = XMVectorAdd(vP2_High_Final, XMVectorSubtract(vP2_High_Final, vP1_High_Final));
        }

        // j_start_offset: 메인 루프에서 이미 P1_final에 해당하는 점을 추가했으면 1부터 시작(중복 방지)
        // 메인 루프가 실행되지 않았다면(NumActualPoints < 4), 0부터 시작
        _int j_start_offset = (iCurveCount > 0) ? 1 : 0;
        if (iNumActualPoints == 2) j_start_offset = 0; // 점 2개면 무조건 처음부터 그려야 한다(메인 루프 돌지 않음)


        for (_int j = j_start_offset; j < m_iCatmullCount; ++j) {
            _float t = (m_iCatmullCount == 1) ? 0.f : _float(j) / _float(m_iCatmullCount - 1);

            _vector vInterpLow = XMVectorCatmullRom(vP0_Low_Final, vP1_Low_Final, vP2_Low_Final, vP3_Low_Final, t);
            _vector vInterpHigh = XMVectorCatmullRom(vP0_High_Final, vP1_High_Final, vP2_High_Final, vP3_High_Final, t);

            XMStoreFloat3(&pVertices[iVtxIdx].vPosition, vInterpLow);
            pVertices[iVtxIdx].vTexcoord = _float2(0.f, (iNumVertices_ThisFrame > 1) ? _float(iVtxIdx) / _float(iNumVertices_ThisFrame - 1) : 0.f);
            ++iVtxIdx;

            XMStoreFloat3(&pVertices[iVtxIdx].vPosition, vInterpHigh);
            pVertices[iVtxIdx].vTexcoord = _float2(1.f, (iNumVertices_ThisFrame > 1) ? _float(iVtxIdx - 1) / _float(iNumVertices_ThisFrame - 1) : 0.f);
            ++iVtxIdx;
        }
    }

    m_iCurIndexCnt = ((iVtxIdx / 2) - 1) * 6;
    if (m_iCurIndexCnt < 0) m_iCurIndexCnt = 0; // vtxIdx가 0 또는 2일 때 음수 방지

    m_pContext->Unmap(m_pVB, 0);
}

void CVIBuffer_Trail::ClearTrail(_fvector vSwordLow, _fvector vSwordHigh)
{
    D3D11_MAPPED_SUBRESOURCE mappedResource;

    m_TrailPoints.clear();
    m_iCurIndexCnt = 0;

    if (SUCCEEDED(m_pContext->Map(m_pVB, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource)))
    {
        VTXTRAIL* pVertices = reinterpret_cast<VTXTRAIL*>(mappedResource.pData);

        for (_int i = 0; i < m_iNumVertices; ++i)
        {
            if (i % 2 == 0)
                XMStoreFloat3(&pVertices[i].vPosition, vSwordLow);
            else
                XMStoreFloat3(&pVertices[i].vPosition, vSwordHigh);

            pVertices[i].vTexcoord = _float2((i % 2 == 0) ? 0.f : 1.f, _float(i) / _float(m_iNumVertices - 1));
        }

        m_pContext->Unmap(m_pVB, 0);
    }
}

HRESULT CVIBuffer_Trail::Get_JsonData(json& jData)
{
    m_tComponentDesc.to_json(jData);

    jData["iMaxTrailPoints"] = m_iMaxTrailPoints;
    jData["iCatmullCount"] = m_iCatmullCount;

    return S_OK;
}

HRESULT CVIBuffer_Trail::Set_JsonData(json& jData)
{
    m_tComponentDesc.from_json(jData);

    if (jData.contains("iMaxTrailPoints"))
        m_iMaxTrailPoints = jData["iMaxTrailPoints"];
    if (jData.contains("iCatmullCount"))
        m_iCatmullCount = jData["iCatmullCount"];

    return S_OK;
}

CVIBuffer_Trail* CVIBuffer_Trail::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CVIBuffer_Instancing::INSTANCE_DESC* pDesc)
{
    CVIBuffer_Trail* pInstance = new CVIBuffer_Trail(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype(pDesc)))
    {
        MSG_BOX("Failed to Create: VIBuffer_Trail");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CComponent* CVIBuffer_Trail::Clone(void* pArg, CGameObject* pOnwer)
{
    CVIBuffer_Trail* pInstance = new CVIBuffer_Trail(*this, pOnwer);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Clone: CVIBuffer_Trail");
        Safe_Release(pInstance);
    }
    return pInstance;
}


void CVIBuffer_Trail::Free()
{
    __super::Free();
}
