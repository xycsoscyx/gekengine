#include "GEK\System\AudioSystem.h"
#include "GEK\Context\COM.h"
#include "GEK\Context\UnknownMixin.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Utility\FileSystem.h"
#include "audiere.h"

#include <mmsystem.h>
#define DIRECTSOUND_VERSION 0x0800
#include <dsound.h>

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "audiere.lib")

namespace Gek
{
    template <typename CLASS>
    class SampleMixin : public UnknownMixin
        , public CLASS
    {
    protected:
        CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSoundBuffer;

    public:
        SampleMixin(IDirectSoundBuffer8 *directSoundBuffer)
            : directSoundBuffer(directSoundBuffer)
        {
            setVolume(1.0f);
        }

        virtual ~SampleMixin(void)
        {
        }

        // AudioSample
        STDMETHODIMP_(LPVOID) getBuffer(void)
        {
            GEK_REQUIRE(directSoundBuffer);
            return LPVOID(directSoundBuffer);
        }

        STDMETHODIMP_(void) setFrequency(UINT32 frequency)
        {
            GEK_REQUIRE(directSoundBuffer);
            if (frequency == -1)
            {
                directSoundBuffer->SetFrequency(DSBFREQUENCY_ORIGINAL);
            }
            else
            {
                directSoundBuffer->SetFrequency(frequency);
            }
        }

        STDMETHODIMP_(void) setVolume(float volume)
        {
            GEK_REQUIRE(directSoundBuffer);
            directSoundBuffer->SetVolume(UINT32((DSBVOLUME_MAX - DSBVOLUME_MIN) * volume) + DSBVOLUME_MIN);
        }
    };

    class EffectImplementation : public SampleMixin<AudioEffect>
    {
    public:
        EffectImplementation(IDirectSoundBuffer8 *directSoundBuffer)
            : SampleMixin(directSoundBuffer)
        {
        }

        BEGIN_INTERFACE_LIST(EffectImplementation)
            INTERFACE_LIST_ENTRY_COM(AudioEffect)
        END_INTERFACE_LIST_UNKNOWN

        // AudioEffect
        STDMETHODIMP_(void) setPan(float pan)
        {
            GEK_REQUIRE(directSoundBuffer);
            directSoundBuffer->SetPan(UINT32((DSBPAN_RIGHT - DSBPAN_LEFT) * pan) + DSBPAN_LEFT);
        }

        STDMETHODIMP_(void) play(bool loop)
        {
            GEK_REQUIRE(directSoundBuffer);

            DWORD dwStatus = 0;
            if (SUCCEEDED(directSoundBuffer->GetStatus(&dwStatus)) && !(dwStatus & DSBSTATUS_PLAYING))
            {
                directSoundBuffer->Play(0, 0, (loop ? DSBPLAY_LOOPING : 0));
            }
        }
    };

    class SoundImplementation : public SampleMixin<AudioSound>
    {
    private:
        CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> directSound8Buffer3D;

    public:
        SoundImplementation(IDirectSoundBuffer8 *directSoundBuffer, IDirectSound3DBuffer8 *directSound8Buffer3D)
            : SampleMixin(directSoundBuffer)
            , directSound8Buffer3D(directSound8Buffer3D)
        {
        }

        BEGIN_INTERFACE_LIST(SoundImplementation)
            INTERFACE_LIST_ENTRY_COM(AudioSound)
        END_INTERFACE_LIST_UNKNOWN

        // AudioSound
        STDMETHODIMP_(void) setDistance(float minimum, float maximum)
        {
            GEK_REQUIRE(directSound8Buffer3D);

            directSound8Buffer3D->SetMinDistance(minimum, DS3D_DEFERRED);
            directSound8Buffer3D->SetMaxDistance(maximum, DS3D_DEFERRED);
        }

        STDMETHODIMP_(void) play(const Math::Float3 &origin, bool loop)
        {
            GEK_REQUIRE(directSound8Buffer3D);
            GEK_REQUIRE(directSoundBuffer);

            directSound8Buffer3D->SetPosition(origin.x, origin.y, origin.z, DS3D_DEFERRED);

            DWORD dwStatus = 0;
            if (SUCCEEDED(directSoundBuffer->GetStatus(&dwStatus)) && !(dwStatus & DSBSTATUS_PLAYING))
            {
                directSoundBuffer->Play(0, 0, (loop ? DSBPLAY_LOOPING : 0));
            }
        }
    };

    class AudioSystemImplementation : public ContextUserMixin
        , public AudioSystem
    {
    private:
        CComQIPtr<IDirectSound8, &IID_IDirectSound8> directSound;
        CComQIPtr<IDirectSound3DListener8, &IID_IDirectSound3DListener8> directSoundListener;
        CComQIPtr<IDirectSoundBuffer, &IID_IDirectSoundBuffer> primarySoundBuffer;

    public:
        BEGIN_INTERFACE_LIST(AudioSystemImplementation)
            INTERFACE_LIST_ENTRY_COM(AudioSystem)
        END_INTERFACE_LIST_USER

        // Interface
        STDMETHODIMP initialize(HWND window)
        {
            GEK_REQUIRE(window);

            HRESULT resultValue = E_FAIL;
            resultValue = DirectSoundCreate8(nullptr, &directSound, nullptr);
            if(SUCCEEDED(resultValue))
            {
                resultValue = directSound->SetCooperativeLevel(window, DSSCL_PRIORITY);
                if (SUCCEEDED(resultValue))
                {
                    DSBUFFERDESC primaryBufferDescription = { 0 };
                    primaryBufferDescription.dwSize = sizeof(DSBUFFERDESC);
                    primaryBufferDescription.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;
                    resultValue = directSound->CreateSoundBuffer(&primaryBufferDescription, &primarySoundBuffer, nullptr);
                    if (SUCCEEDED(resultValue))
                    {
                        WAVEFORMATEX primaryBufferFormat;
                        ZeroMemory(&primaryBufferFormat, sizeof(WAVEFORMATEX));
                        primaryBufferFormat.wFormatTag = WAVE_FORMAT_PCM;
                        primaryBufferFormat.nChannels = 2;
                        primaryBufferFormat.wBitsPerSample = 8;
                        primaryBufferFormat.nSamplesPerSec = 48000;
                        primaryBufferFormat.nBlockAlign = (primaryBufferFormat.wBitsPerSample / 8 * primaryBufferFormat.nChannels);
                        primaryBufferFormat.nAvgBytesPerSec = (primaryBufferFormat.nSamplesPerSec * primaryBufferFormat.nBlockAlign);
                        resultValue = primarySoundBuffer->SetFormat(&primaryBufferFormat);
                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = E_FAIL;
                            directSoundListener = primarySoundBuffer;
                            if (directSoundListener)
                            {
                                resultValue = S_OK;
                                setMasterVolume(1.0f);
                                setDistanceFactor(1.0f);
                                setDopplerFactor(0.0f);
                                setRollOffFactor(1.0f);
                            }
                        }
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP_(void) setMasterVolume(float volume)
        {
            GEK_REQUIRE(primarySoundBuffer);
            primarySoundBuffer->SetVolume(UINT32((DSBVOLUME_MAX - DSBVOLUME_MIN) * volume) + DSBVOLUME_MIN);
        }

        STDMETHODIMP_(float) getMasterVolume(void)
        {
            GEK_REQUIRE(primarySoundBuffer);

            long volumeNumber = 0;
            if (FAILED(primarySoundBuffer->GetVolume(&volumeNumber)))
            {
                volumeNumber = 0;
            }

            return (float(volumeNumber - DSBVOLUME_MIN) / float(DSBVOLUME_MAX - DSBVOLUME_MIN));
        }

        STDMETHODIMP_(void) setListener(const Math::Float4x4 &matrix)
        {
            GEK_REQUIRE(directSoundListener);
            directSoundListener->SetPosition(matrix.translation.x, matrix.translation.y, matrix.translation.z, DS3D_DEFERRED);
            directSoundListener->SetOrientation(matrix.rz.x, matrix.rz.y, matrix.rz.z, matrix.ry.x, matrix.ry.y, matrix.ry.z, DS3D_DEFERRED);
            directSoundListener->CommitDeferredSettings();
        }

        STDMETHODIMP_(void) setDistanceFactor(float factor)
        {
            GEK_REQUIRE(directSoundListener);
            directSoundListener->SetDistanceFactor(factor, DS3D_DEFERRED);
        }

        STDMETHODIMP_(void) setDopplerFactor(float factor)
        {
            GEK_REQUIRE(directSoundListener);
            directSoundListener->SetDopplerFactor(factor, DS3D_DEFERRED);
        }

        STDMETHODIMP_(void) setRollOffFactor(float factor)
        {
            GEK_REQUIRE(directSoundListener);
            directSoundListener->SetRolloffFactor(factor, DS3D_DEFERRED);
        }

        STDMETHODIMP copyEffect(AudioEffect **returnObject, AudioEffect *source)
        {
            GEK_REQUIRE(directSound);
            GEK_REQUIRE(returnObject);
            GEK_REQUIRE(source);

            HRESULT resultValue = E_FAIL;
            CComPtr<IDirectSoundBuffer> directSoundBuffer;
            resultValue = directSound->DuplicateSoundBuffer((LPDIRECTSOUNDBUFFER)source->getBuffer(), &directSoundBuffer);
            if (directSoundBuffer)
            {
                resultValue = E_FAIL;
                CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
                if (directSound8Buffer)
                {
                    resultValue = E_OUTOFMEMORY;
                    CComPtr<EffectImplementation> effect = new EffectImplementation(directSound8Buffer);
                    if (effect)
                    {
                        resultValue = effect->QueryInterface(IID_PPV_ARGS(returnObject));
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP copySound(AudioSound **returnObject, AudioSound *source)
        {
            GEK_REQUIRE(directSound);
            GEK_REQUIRE(returnObject);
            GEK_REQUIRE(source);

            HRESULT resultValue = E_FAIL;
            CComPtr<IDirectSoundBuffer> directSoundBuffer;
            resultValue = directSound->DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER(source->getBuffer()), &directSoundBuffer);
            if (directSoundBuffer)
            {
                resultValue = E_FAIL;
                CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
                if (directSound8Buffer)
                {
                    CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> directSound8Buffer3D(directSound8Buffer);
                    if (directSound8Buffer3D)
                    {
                        resultValue = E_OUTOFMEMORY;
                        CComPtr<SoundImplementation> sound = new SoundImplementation(directSound8Buffer, directSound8Buffer3D);
                        if (sound)
                        {
                            resultValue = sound->QueryInterface(IID_PPV_ARGS(returnObject));
                        }
                    }
                }
            }

            return resultValue;
        }

        HRESULT loadFromFile(IDirectSoundBuffer **returnObject, LPCWSTR fileName, DWORD flags, GUID soundAlgorithm)
        {
            GEK_REQUIRE(directSound);
            GEK_REQUIRE(returnObject);
            GEK_REQUIRE(fileName);

            HRESULT resultValue = E_FAIL;
            std::vector<UINT8> fileData;
            resultValue = Gek::FileSystem::load(fileName, fileData);
            if (SUCCEEDED(resultValue))
            {
                resultValue = E_FAIL;
                audiere::RefPtr<audiere::File> audiereFile(audiere::CreateMemoryFile(fileData.data(), fileData.size()));
                if (audiereFile)
                {
                    audiere::RefPtr<audiere::SampleSource> audiereSample(audiere::OpenSampleSource(audiereFile));
                    if (audiereSample)
                    {
                        int channelCount = 0;
                        int samplesPerSecond = 0;
                        audiere::SampleFormat sampleFormat;
                        audiereSample->getFormat(channelCount, samplesPerSecond, sampleFormat);

                        WAVEFORMATEX bufferFormat;
                        bufferFormat.cbSize = 0;
                        bufferFormat.nChannels = channelCount;
                        bufferFormat.wBitsPerSample = (sampleFormat == audiere::SF_U8 ? 8 : 16);
                        bufferFormat.nSamplesPerSec = samplesPerSecond;
                        bufferFormat.nBlockAlign = (bufferFormat.wBitsPerSample / 8 * bufferFormat.nChannels);
                        bufferFormat.nAvgBytesPerSec = (bufferFormat.nSamplesPerSec * bufferFormat.nBlockAlign);
                        bufferFormat.wFormatTag = WAVE_FORMAT_PCM;

                        DWORD sampleLength = audiereSample->getLength();
                        sampleLength *= (bufferFormat.wBitsPerSample / 8);
                        sampleLength *= channelCount;

                        DSBUFFERDESC bufferDescription = { 0 };
                        bufferDescription.dwSize = sizeof(DSBUFFERDESC);
                        bufferDescription.dwBufferBytes = sampleLength;
                        bufferDescription.lpwfxFormat = (WAVEFORMATEX *)&bufferFormat;
                        bufferDescription.guid3DAlgorithm = soundAlgorithm;
                        bufferDescription.dwFlags = flags;

                        CComPtr<IDirectSoundBuffer> directSoundBuffer;
                        resultValue = directSound->CreateSoundBuffer(&bufferDescription, &directSoundBuffer, nullptr);
                        if (directSoundBuffer)
                        {
                            void *sampleData = nullptr;
                            if (SUCCEEDED(directSoundBuffer->Lock(0, sampleLength, &sampleData, &sampleLength, 0, 0, DSBLOCK_ENTIREBUFFER)))
                            {
                                audiereSample->read((sampleLength / bufferFormat.nBlockAlign), sampleData);
                                directSoundBuffer->Unlock(sampleData, sampleLength, 0, 0);
                            }

                            resultValue = directSoundBuffer->QueryInterface(IID_IDirectSoundBuffer, (LPVOID FAR *)returnObject);
                        }
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP loadEffect(AudioEffect **returnObject, LPCWSTR fileName)
        {
            GEK_REQUIRE(directSound);
            GEK_REQUIRE(returnObject);
            GEK_REQUIRE(fileName);

            HRESULT resultValue = E_FAIL;
            CComPtr<IDirectSoundBuffer> directSoundBuffer;
            resultValue = loadFromFile(&directSoundBuffer, fileName, DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY, GUID_NULL);
            if (directSoundBuffer)
            {
                resultValue = E_FAIL;
                CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
                if (directSound8Buffer)
                {
                    CComPtr<EffectImplementation> effect = new EffectImplementation(directSound8Buffer);
                    if (effect)
                    {
                        resultValue = effect->QueryInterface(IID_PPV_ARGS(returnObject));
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP loadSound(AudioSound **returnObject, LPCWSTR fileName)
        {
            GEK_REQUIRE(directSound);
            GEK_REQUIRE(returnObject);
            GEK_REQUIRE(fileName);

            HRESULT resultValue = E_FAIL;
            CComPtr<IDirectSoundBuffer> directSoundBuffer;
            resultValue = loadFromFile(&directSoundBuffer, fileName, DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY, GUID_NULL);
            if (directSoundBuffer)
            {
                resultValue = E_FAIL;
                CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
                if (directSound8Buffer)
                {
                    CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> directSound8Buffer3D(directSound8Buffer);
                    if (directSound8Buffer3D)
                    {
                        CComPtr<SoundImplementation> sound = new SoundImplementation(directSound8Buffer, directSound8Buffer3D);
                        if (sound)
                        {
                            resultValue = sound->QueryInterface(IID_PPV_ARGS(returnObject));
                        }
                    }
                }
            }

            return resultValue;
        }
    };

    REGISTER_CLASS(AudioSystemImplementation)
}; // namespace Gek
