#include "CGEKAudioSystem.h"
#include "IGEKSystem.h"
#include "audiere.h"

#include "GEKSystemCLSIDs.h"

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "audiere.lib")

class CGEKAudioSample : public CGEKUnknown
                      , public IGEKAudioSample
{
protected:
    CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> m_spBuffer;

public:
    DECLARE_UNKNOWN(CGEKAudioSample);
    CGEKAudioSample(IDirectSoundBuffer8 *pBuffer)
        : m_spBuffer(pBuffer)
    {
        SetVolume(1.0f);
    }

    virtual ~CGEKAudioSample(void)
    {
    }

    STDMETHODIMP_(void *) GetBuffer(void)
    {
        REQUIRE_RETURN(m_spBuffer, nullptr);
        return (void *)m_spBuffer;
    }

    STDMETHODIMP_(void) SetFrequency(UINT32 nFrequency)
    {
        REQUIRE_VOID_RETURN(m_spBuffer);
        if (nFrequency == -1)
        {
            m_spBuffer->SetFrequency(DSBFREQUENCY_ORIGINAL);
        }
        else
        {
            m_spBuffer->SetFrequency(nFrequency);
        }
    }

    STDMETHODIMP_(void) SetVolume(float nVolume)
    {
        REQUIRE_VOID_RETURN(m_spBuffer);
        m_spBuffer->SetVolume(UINT32((DSBVOLUME_MAX - DSBVOLUME_MIN) * nVolume) + DSBVOLUME_MIN);
    }
};

class CGEKAudioEffect : public CGEKAudioSample
                      , public IGEKAudioEffect
{
public:
    DECLARE_UNKNOWN(CGEKAudioEffect);
    CGEKAudioEffect(IDirectSoundBuffer8 *pBuffer)
        : CGEKAudioSample(pBuffer)
    {
    }

    ~CGEKAudioEffect(void)
    {
    }

    STDMETHODIMP_(void) SetPan(float fPan)
    {
        REQUIRE_VOID_RETURN(m_spBuffer);

        m_spBuffer->SetPan(UINT32((DSBPAN_RIGHT - DSBPAN_LEFT) * fPan) + DSBPAN_LEFT);
    }

    STDMETHODIMP_(void) Play(bool bLoop)
    {
        REQUIRE_VOID_RETURN(m_spBuffer);

        DWORD dwStatus = 0;
        if(SUCCEEDED(m_spBuffer->GetStatus(&dwStatus)) && !(dwStatus & DSBSTATUS_PLAYING))
        {
            m_spBuffer->Play(0, 0, (bLoop ? DSBPLAY_LOOPING : 0));
        }
    }

    STDMETHODIMP_(void *) GetBuffer(void)
    {
        return CGEKAudioSample::GetBuffer();
    }

    STDMETHODIMP_(void) SetFrequency(UINT32 nFrequency)
    {
        CGEKAudioSample::SetFrequency(nFrequency);
    }

    STDMETHODIMP_(void) SetVolume(float nVolume)
    {
        CGEKAudioSample::SetVolume(nVolume);
    }
};

class CGEKAudioSound : public CGEKAudioSample
                     , public IGEKAudioSound
{
private:
    CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> m_spBuffer3D;

public:
    DECLARE_UNKNOWN(CGEKAudioSound);
    CGEKAudioSound(IDirectSoundBuffer8 *pBuffer, IDirectSound3DBuffer8 *pBuffer3D)
        : CGEKAudioSample(pBuffer)
        , m_spBuffer3D(pBuffer3D)
    {
    }

    ~CGEKAudioSound(void)
    {
    }

    STDMETHODIMP_(void) SetDistance(float nMin, float nMax)
    {
        REQUIRE_VOID_RETURN(m_spBuffer3D);

        m_spBuffer3D->SetMinDistance(nMin, DS3D_DEFERRED);
        m_spBuffer3D->SetMaxDistance(nMax, DS3D_DEFERRED);
    }

    STDMETHODIMP_(void) Play(const float3 &kOrigin, bool bLoop)
    {
        REQUIRE_VOID_RETURN(m_spBuffer3D && m_spBuffer);

        m_spBuffer3D->SetPosition(kOrigin.x, kOrigin.y, kOrigin.z, DS3D_DEFERRED);
    
        DWORD dwStatus = 0;
        if(SUCCEEDED(m_spBuffer->GetStatus(&dwStatus)) && !(dwStatus & DSBSTATUS_PLAYING))
        {
            m_spBuffer->Play(0, 0, (bLoop ? DSBPLAY_LOOPING : 0));
        }
    }

    STDMETHODIMP_(void *) GetBuffer(void)
    {
        return CGEKAudioSample::GetBuffer();
    }

    STDMETHODIMP_(void) SetFrequency(UINT32 nFrequency)
    {
        CGEKAudioSample::SetFrequency(nFrequency);
    }

    STDMETHODIMP_(void) SetVolume(float nVolume)
    {
        CGEKAudioSample::SetVolume(nVolume);
    }
};

BEGIN_INTERFACE_LIST(CGEKAudioSample)
    INTERFACE_LIST_ENTRY_COM(IGEKAudioSample)
END_INTERFACE_LIST_UNKNOWN

BEGIN_INTERFACE_LIST(CGEKAudioEffect)
    INTERFACE_LIST_ENTRY_COM(IGEKAudioEffect)
END_INTERFACE_LIST_BASE(CGEKAudioSample)

BEGIN_INTERFACE_LIST(CGEKAudioSound)
    INTERFACE_LIST_ENTRY_COM(IGEKAudioSound)
END_INTERFACE_LIST_BASE(CGEKAudioSample)

BEGIN_INTERFACE_LIST(CGEKAudioSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKAudioSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKAudioSystem);

CGEKAudioSystem::CGEKAudioSystem(void)
{
}

CGEKAudioSystem::~CGEKAudioSystem(void)
{
}

STDMETHODIMP CGEKAudioSystem::Initialize(void)
{
    return GetContext()->AddCachedClass(CLSID_GEKAudioSystem, GetUnknown());
}

STDMETHODIMP_(void) CGEKAudioSystem::Destroy(void)
{
    GetContext()->RemoveCachedClass(CLSID_GEKAudioSystem);
}

STDMETHODIMP CGEKAudioSystem::Initialize(HWND hWindow)
{
    REQUIRE_RETURN(hWindow, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    if (m_spDirectSound)
    {
        hRetVal = m_spDirectSound->SetCooperativeLevel(hWindow, DSSCL_PRIORITY);
    }
    else
    {
        hRetVal = DirectSoundCreate8(nullptr, &m_spDirectSound, nullptr);
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = m_spDirectSound->SetCooperativeLevel(hWindow, DSSCL_PRIORITY);
            if (SUCCEEDED(hRetVal))
            {
                DSBUFFERDESC kDesc = { 0 };
                kDesc.dwSize = sizeof(DSBUFFERDESC);
                kDesc.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;
                hRetVal = m_spDirectSound->CreateSoundBuffer(&kDesc, &m_spPrimary, nullptr);
                if (SUCCEEDED(hRetVal))
                {
                    WAVEFORMATEX kFormat;
                    ZeroMemory(&kFormat, sizeof(WAVEFORMATEX));
                    kFormat.wFormatTag = WAVE_FORMAT_PCM;
                    kFormat.nChannels = 2;
                    kFormat.wBitsPerSample = 8;
                    kFormat.nSamplesPerSec = 48000;
                    kFormat.nBlockAlign = (kFormat.wBitsPerSample / 8 * kFormat.nChannels);
                    kFormat.nAvgBytesPerSec = (kFormat.nSamplesPerSec * kFormat.nBlockAlign);
                    hRetVal = m_spPrimary->SetFormat(&kFormat);
                    if (SUCCEEDED(hRetVal))
                    {
                        hRetVal = E_FAIL;
                        m_spListener = m_spPrimary;
                        if (m_spListener)
                        {
                            hRetVal = S_OK;
                            SetMasterVolume(1.0f);
                            SetDistanceFactor(1.0f);
                            SetDopplerFactor(0.0f);
                            SetRollOffFactor(1.0f);
                        }
                    }
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKAudioSystem::SetMasterVolume(float nVolume)
{
    REQUIRE_VOID_RETURN(m_spPrimary);
    m_spPrimary->SetVolume(UINT32((DSBVOLUME_MAX - DSBVOLUME_MIN) * nVolume) + DSBVOLUME_MIN);
}

STDMETHODIMP_(float) CGEKAudioSystem::GetMasterVolume(void)
{
    REQUIRE_RETURN(m_spPrimary, 0);

    float nVolumePercent = 0.0f;
    long nVolumeNumber = 0;
    if (SUCCEEDED(m_spPrimary->GetVolume(&nVolumeNumber)))
    {
        nVolumePercent = (float(nVolumeNumber - DSBVOLUME_MIN) / float(DSBVOLUME_MAX - DSBVOLUME_MIN));
    }

    return nVolumePercent;
}

STDMETHODIMP_(void) CGEKAudioSystem::SetListener(const float4x4 &nMatrix)
{
    REQUIRE_VOID_RETURN(m_spListener);
    m_spListener->SetPosition(nMatrix.t.x, nMatrix.t.y, nMatrix.t.z, DS3D_DEFERRED);
    m_spListener->SetOrientation(nMatrix.rz.x, nMatrix.rz.y, nMatrix.rz.z,
                                 nMatrix.ry.x, nMatrix.ry.y, nMatrix.ry.z, DS3D_DEFERRED);
    m_spListener->CommitDeferredSettings();
}

STDMETHODIMP_(void) CGEKAudioSystem::SetDistanceFactor(float nFactor)
{
    REQUIRE_VOID_RETURN(m_spListener);
    m_spListener->SetDistanceFactor(nFactor, DS3D_DEFERRED);
}

STDMETHODIMP_(void) CGEKAudioSystem::SetDopplerFactor(float nFactor)
{
    REQUIRE_VOID_RETURN(m_spListener);
    m_spListener->SetDopplerFactor(nFactor, DS3D_DEFERRED);
}

STDMETHODIMP_(void) CGEKAudioSystem::SetRollOffFactor(float nFactor)
{
    REQUIRE_VOID_RETURN(m_spListener);
    m_spListener->SetRolloffFactor(nFactor, DS3D_DEFERRED);
}

STDMETHODIMP CGEKAudioSystem::CopyEffect(IGEKAudioEffect *pSource, IGEKAudioEffect **ppEffect)
{
    REQUIRE_RETURN(m_spDirectSound, E_FAIL);
    REQUIRE_RETURN(pSource && ppEffect, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    CComQIPtr<IGEKAudioSample> spSample(pSource);
    if (spSample)
    {
        CComPtr<IDirectSoundBuffer> spBuffer;
        hRetVal = m_spDirectSound->DuplicateSoundBuffer((LPDIRECTSOUNDBUFFER)spSample->GetBuffer(), &spBuffer);
        if (spBuffer)
        {
            hRetVal = E_FAIL;
            CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> spBuffer8(spBuffer);
            if (spBuffer8)
            {
                hRetVal = E_OUTOFMEMORY;
                CComPtr<CGEKAudioEffect> spEffect = new CGEKAudioEffect(spBuffer8);
                if (spEffect)
                {
                    hRetVal = spEffect->QueryInterface(IID_PPV_ARGS(ppEffect));
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKAudioSystem::CopySound(IGEKAudioSound *pSource, IGEKAudioSound **ppSound)
{
    REQUIRE_RETURN(m_spDirectSound, E_FAIL);
    REQUIRE_RETURN(pSource && ppSound, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    CComQIPtr<IGEKAudioSample> spSample(pSource);
    if (spSample)
    {
        CComPtr<IDirectSoundBuffer> spBuffer;
        hRetVal = m_spDirectSound->DuplicateSoundBuffer((LPDIRECTSOUNDBUFFER)spSample->GetBuffer(), &spBuffer);
        if (spBuffer)
        {
            hRetVal = E_FAIL;
            CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> spBuffer8(spBuffer);
            if (spBuffer8)
            {
                CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> spBuffer3D(spBuffer8);
                if (spBuffer3D)
                {
                    hRetVal = E_OUTOFMEMORY;
                    CComPtr<CGEKAudioSound> spSound = new CGEKAudioSound(spBuffer8, spBuffer3D);
                    if (spSound)
                    {
                        hRetVal = spSound->QueryInterface(IID_PPV_ARGS(ppSound));
                    }
                }
            }
        }
    }

    return hRetVal;
}

HRESULT CGEKAudioSystem::LoadFromFile(LPCWSTR pFileName, DWORD nFlags, GUID nAlgorithm, IDirectSoundBuffer **ppBuffer)
{
    REQUIRE_RETURN(m_spDirectSound, E_FAIL);
    REQUIRE_RETURN(ppBuffer, E_INVALIDARG);

    std::vector<UINT8> aBuffer;
    HRESULT hRetVal = GEKLoadFromFile(pFileName, aBuffer);
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        audiere::RefPtr<audiere::File> spFile(audiere::CreateMemoryFile(aBuffer.data(), aBuffer.size()));
        if (spFile)
        {
            audiere::RefPtr<audiere::SampleSource> spSource(audiere::OpenSampleSource(spFile));
            if (spSource)
            {
                int nSampleRate = 0;
                int nNumChannels = 0;
                audiere::SampleFormat kType;
                spSource->getFormat(nNumChannels, nSampleRate, kType);

                WAVEFORMATEX kFormat;
                kFormat.cbSize = 0;
                kFormat.nChannels = nNumChannels;
                kFormat.wBitsPerSample = (kType == audiere::SF_U8 ? 8 : 16);
                kFormat.nSamplesPerSec = nSampleRate;
                kFormat.nBlockAlign = (kFormat.wBitsPerSample / 8 * kFormat.nChannels);
                kFormat.nAvgBytesPerSec = (kFormat.nSamplesPerSec * kFormat.nBlockAlign);
                kFormat.wFormatTag = WAVE_FORMAT_PCM;

                DWORD nLength = spSource->getLength();
                nLength *= (kFormat.wBitsPerSample / 8);
                nLength *= nNumChannels;

                DSBUFFERDESC kDesc = { 0 };
                kDesc.dwSize = sizeof(DSBUFFERDESC);
                kDesc.dwBufferBytes = nLength;
                kDesc.lpwfxFormat = (WAVEFORMATEX *)&kFormat;
                kDesc.guid3DAlgorithm = nAlgorithm;
                kDesc.dwFlags = nFlags;

                CComPtr<IDirectSoundBuffer> spBuffer;
                hRetVal = m_spDirectSound->CreateSoundBuffer(&kDesc, &spBuffer, nullptr);
                if (spBuffer)
                {
                    void *pData = nullptr;
                    if (SUCCEEDED(spBuffer->Lock(0, nLength, &pData, &nLength, 0, 0, DSBLOCK_ENTIREBUFFER)))
                    {
                        int iRead = spSource->read((nLength / kFormat.nBlockAlign), pData);
                        spBuffer->Unlock(pData, nLength, 0, 0);
                    }

                    hRetVal = spBuffer->QueryInterface(IID_IDirectSoundBuffer, (LPVOID FAR *)ppBuffer);
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKAudioSystem::LoadEffect(LPCWSTR pFileName, IGEKAudioEffect **ppEffect)
{
    REQUIRE_RETURN(m_spDirectSound, E_FAIL);
    REQUIRE_RETURN(ppEffect, E_INVALIDARG);

    CComPtr<IDirectSoundBuffer> spBuffer;
    HRESULT hRetVal = LoadFromFile(pFileName, DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY, GUID_NULL, &spBuffer);
    if (spBuffer)
    {
        hRetVal = E_FAIL;
        CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> spBuffer8(spBuffer);
        if (spBuffer8)
        {
            CComPtr<CGEKAudioEffect> spEffect = new CGEKAudioEffect(spBuffer8);
            if (spEffect)
            {
                hRetVal = spEffect->QueryInterface(IID_PPV_ARGS(ppEffect));
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKAudioSystem::LoadSound(LPCWSTR pFileName, IGEKAudioSound **ppSound)
{
    REQUIRE_RETURN(m_spDirectSound, E_FAIL);
    REQUIRE_RETURN(ppSound, E_INVALIDARG);

    CComPtr<IDirectSoundBuffer> spBuffer;
    HRESULT hRetVal = LoadFromFile(pFileName, DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY, GUID_NULL, &spBuffer);
    if (spBuffer)
    {
        hRetVal = E_FAIL;
        CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> spBuffer8(spBuffer);
        if (spBuffer8)
        {
            CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> spBuffer3D(spBuffer8);
            if (spBuffer3D)
            {
                CComPtr<CGEKAudioSound> spSound = new CGEKAudioSound(spBuffer8, spBuffer3D);
                if (spSound)
                {
                    hRetVal = spSound->QueryInterface(IID_PPV_ARGS(ppSound));
                }
            }
        }
    }

    return hRetVal;
}
