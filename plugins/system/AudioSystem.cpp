#include "GEK\System\AudioInterface.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\UserMixin.h"
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
    namespace Audio
    {
        namespace Sample
        {
            class Class : public Context::User::Mixin
                , public Sample::Interface
            {
            protected:
                CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSoundBuffer;

            public:
                Class(IDirectSoundBuffer8 *directSoundBuffer)
                    : directSoundBuffer(directSoundBuffer)
                {
                    setVolume(1.0f);
                }

                virtual ~Class(void)
                {
                }

                BEGIN_INTERFACE_LIST(Class)
                    INTERFACE_LIST_ENTRY_COM(Audio::Sample::Interface)
                END_INTERFACE_LIST_UNKNOWN

                // Sample::Interface
                STDMETHODIMP_(LPVOID) getBuffer(void)
                {
                    REQUIRE_RETURN(directSoundBuffer, nullptr);
                    return LPVOID(directSoundBuffer);
                }

                STDMETHODIMP_(void) setFrequency(UINT32 frequency)
                {
                    REQUIRE_VOID_RETURN(directSoundBuffer);
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
                    REQUIRE_VOID_RETURN(directSoundBuffer);
                    directSoundBuffer->SetVolume(UINT32((DSBVOLUME_MAX - DSBVOLUME_MIN) * volume) + DSBVOLUME_MIN);
                }
            };
        }; // namespace Sample

        namespace Effect
        {
            class Class : public Sample::Class
                , public Effect::Interface
            {
            public:
                Class(IDirectSoundBuffer8 *directSoundBuffer)
                    : Sample::Class(directSoundBuffer)
                {
                }

                BEGIN_INTERFACE_LIST(Class)
                    INTERFACE_LIST_ENTRY_COM(Audio::Effect::Interface)
                END_INTERFACE_LIST_BASE(Sample::Class)

                // Effect::Interface
                STDMETHODIMP_(void) setPan(float pan)
                {
                    REQUIRE_VOID_RETURN(directSoundBuffer);
                    directSoundBuffer->SetPan(UINT32((DSBPAN_RIGHT - DSBPAN_LEFT) * pan) + DSBPAN_LEFT);
                }

                STDMETHODIMP_(void) play(bool loop)
                {
                    REQUIRE_VOID_RETURN(directSoundBuffer);

                    DWORD dwStatus = 0;
                    if (SUCCEEDED(directSoundBuffer->GetStatus(&dwStatus)) && !(dwStatus & DSBSTATUS_PLAYING))
                    {
                        directSoundBuffer->Play(0, 0, (loop ? DSBPLAY_LOOPING : 0));
                    }
                }

                // Sample::Interface
                STDMETHODIMP_(LPVOID) getBuffer(void)
                {
                    return Sample::Class::getBuffer();
                }

                STDMETHODIMP_(void) setFrequency(UINT32 frequency)
                {
                    Sample::Class::setFrequency(frequency);
                }

                STDMETHODIMP_(void) setVolume(float volume)
                {
                    Sample::Class::setVolume(volume);
                }
            };
        }; // namespace Effect

        namespace Sound
        {
            class Class : public Sample::Class
                , public Sound::Interface
            {
            private:
                CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> directSound8Buffer3D;

            public:
                Class(IDirectSoundBuffer8 *directSoundBuffer, IDirectSound3DBuffer8 *directSound8Buffer3D)
                    : Sample::Class(directSoundBuffer)
                    , directSound8Buffer3D(directSound8Buffer3D)
                {
                }

                BEGIN_INTERFACE_LIST(Class)
                    INTERFACE_LIST_ENTRY_COM(Audio::Sound::Interface)
                END_INTERFACE_LIST_BASE(Sample::Class)

                // Sound::Interface
                STDMETHODIMP_(void) setDistance(float minimum, float maximum)
                {
                    REQUIRE_VOID_RETURN(directSound8Buffer3D);

                    directSound8Buffer3D->SetMinDistance(minimum, DS3D_DEFERRED);
                    directSound8Buffer3D->SetMaxDistance(maximum, DS3D_DEFERRED);
                }

                STDMETHODIMP_(void) play(const Math::Float3 &origin, bool loop)
                {
                    REQUIRE_VOID_RETURN(directSound8Buffer3D && directSoundBuffer);

                    directSound8Buffer3D->SetPosition(origin.x, origin.y, origin.z, DS3D_DEFERRED);

                    DWORD dwStatus = 0;
                    if (SUCCEEDED(directSoundBuffer->GetStatus(&dwStatus)) && !(dwStatus & DSBSTATUS_PLAYING))
                    {
                        directSoundBuffer->Play(0, 0, (loop ? DSBPLAY_LOOPING : 0));
                    }
                }

                // Sample::Interface
                STDMETHODIMP_(LPVOID) getBuffer(void)
                {
                    return Sample::Class::getBuffer();
                }

                STDMETHODIMP_(void) setFrequency(UINT32 frequency)
                {
                    Sample::Class::setFrequency(frequency);
                }

                STDMETHODIMP_(void) setVolume(float volume)
                {
                    Sample::Class::setVolume(volume);
                }
            };
        }; // namespace Sound

        class System : public Context::User::Mixin
                     , public Interface
        {
        private:
            CComQIPtr<IDirectSound8, &IID_IDirectSound8> directSound;
            CComQIPtr<IDirectSound3DListener8, &IID_IDirectSound3DListener8> directSoundListener;
            CComQIPtr<IDirectSoundBuffer, &IID_IDirectSoundBuffer> primarySoundBuffer;

        public:
            BEGIN_INTERFACE_LIST(System)
                INTERFACE_LIST_ENTRY_COM(Audio::Interface)
            END_INTERFACE_LIST_USER

            // Interface
            STDMETHODIMP initialize(HWND window)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("0x%p", window);

                REQUIRE_RETURN(window, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                if (SUCCEEDED(gekCheckResult(resultValue = DirectSoundCreate8(nullptr, &directSound, nullptr))))
                {
                    if (SUCCEEDED(gekCheckResult(resultValue = directSound->SetCooperativeLevel(window, DSSCL_PRIORITY))))
                    {
                        DSBUFFERDESC primaryBufferDescription = { 0 };
                        primaryBufferDescription.dwSize = sizeof(DSBUFFERDESC);
                        primaryBufferDescription.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;
                        if (SUCCEEDED(gekCheckResult(resultValue = directSound->CreateSoundBuffer(&primaryBufferDescription, &primarySoundBuffer, nullptr))))
                        {
                            WAVEFORMATEX primaryBufferFormat;
                            ZeroMemory(&primaryBufferFormat, sizeof(WAVEFORMATEX));
                            primaryBufferFormat.wFormatTag = WAVE_FORMAT_PCM;
                            primaryBufferFormat.nChannels = 2;
                            primaryBufferFormat.wBitsPerSample = 8;
                            primaryBufferFormat.nSamplesPerSec = 48000;
                            primaryBufferFormat.nBlockAlign = (primaryBufferFormat.wBitsPerSample / 8 * primaryBufferFormat.nChannels);
                            primaryBufferFormat.nAvgBytesPerSec = (primaryBufferFormat.nSamplesPerSec * primaryBufferFormat.nBlockAlign);
                            if (SUCCEEDED(gekCheckResult(resultValue = primarySoundBuffer->SetFormat(&primaryBufferFormat))))
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
                REQUIRE_VOID_RETURN(primarySoundBuffer);
                primarySoundBuffer->SetVolume(UINT32((DSBVOLUME_MAX - DSBVOLUME_MIN) * volume) + DSBVOLUME_MIN);
            }

            STDMETHODIMP_(float) getMasterVolume(void)
            {
                REQUIRE_RETURN(primarySoundBuffer, 0);

                long volumeNumber = 0;
                if (FAILED(primarySoundBuffer->GetVolume(&volumeNumber)))
                {
                    volumeNumber = 0;
                }

                return (float(volumeNumber - DSBVOLUME_MIN) / float(DSBVOLUME_MAX - DSBVOLUME_MIN));
            }

            STDMETHODIMP_(void) setListener(const Math::Float4x4 &matrix)
            {
                REQUIRE_VOID_RETURN(directSoundListener);
                directSoundListener->SetPosition(matrix.translation.x, matrix.translation.y, matrix.translation.z, DS3D_DEFERRED);
                directSoundListener->SetOrientation(matrix.rz.x, matrix.rz.y, matrix.rz.z, matrix.ry.x, matrix.ry.y, matrix.ry.z, DS3D_DEFERRED);
                directSoundListener->CommitDeferredSettings();
            }

            STDMETHODIMP_(void) setDistanceFactor(float factor)
            {
                REQUIRE_VOID_RETURN(directSoundListener);
                directSoundListener->SetDistanceFactor(factor, DS3D_DEFERRED);
            }

            STDMETHODIMP_(void) setDopplerFactor(float factor)
            {
                REQUIRE_VOID_RETURN(directSoundListener);
                directSoundListener->SetDopplerFactor(factor, DS3D_DEFERRED);
            }

            STDMETHODIMP_(void) setRollOffFactor(float factor)
            {
                REQUIRE_VOID_RETURN(directSoundListener);
                directSoundListener->SetRolloffFactor(factor, DS3D_DEFERRED);
            }

            STDMETHODIMP copyEffect(Effect::Interface **returnObject, Effect::Interface *source)
            {
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(directSound, E_FAIL);
                REQUIRE_RETURN(source, E_INVALIDARG);
                REQUIRE_RETURN(returnObject, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComQIPtr<Sample::Interface> sourceSample(source);
                if (sourceSample)
                {
                    CComPtr<IDirectSoundBuffer> directSoundBuffer;
                    resultValue = directSound->DuplicateSoundBuffer((LPDIRECTSOUNDBUFFER)sourceSample->getBuffer(), &directSoundBuffer);
                    if (directSoundBuffer)
                    {
                        resultValue = E_FAIL;
                        CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
                        if (directSound8Buffer)
                        {
                            resultValue = E_OUTOFMEMORY;
                            CComPtr<Effect::Class> effect = new Effect::Class(directSound8Buffer);
                            if (effect)
                            {
                                resultValue = effect->QueryInterface(IID_PPV_ARGS(returnObject));
                            }
                        }
                    }
                }

                return resultValue;
            }

            STDMETHODIMP copySound(Sound::Interface **returnObject, Sound::Interface *source)
            {
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(directSound, E_FAIL);
                REQUIRE_RETURN(source, E_INVALIDARG);
                REQUIRE_RETURN(returnObject, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComQIPtr<Sample::Interface> sourceSample(source);
                if (sourceSample)
                {
                    CComPtr<IDirectSoundBuffer> directSoundBuffer;
                    resultValue = directSound->DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER(sourceSample->getBuffer()), &directSoundBuffer);
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
                                CComPtr<Sound::Class> sound = new Sound::Class(directSound8Buffer, directSound8Buffer3D);
                                if (sound)
                                {
                                    resultValue = sound->QueryInterface(IID_PPV_ARGS(returnObject));
                                }
                            }
                        }
                    }
                }

                return resultValue;
            }

            HRESULT loadFromFile(IDirectSoundBuffer **returnObject, LPCWSTR fileName, DWORD flags, GUID soundAlgorithm)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%s", fileName);
                gekLogParameter("%d", flags);

                REQUIRE_RETURN(directSound, E_FAIL);
                REQUIRE_RETURN(returnObject, E_INVALIDARG);

                std::vector<UINT8> fileData;
                HRESULT resultValue = Gek::FileSystem::load(fileName, fileData);
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

            STDMETHODIMP loadEffect(Effect::Interface **returnObject, LPCWSTR fileName)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%s", fileName);

                REQUIRE_RETURN(directSound, E_FAIL);
                REQUIRE_RETURN(returnObject, E_INVALIDARG);

                CComPtr<IDirectSoundBuffer> directSoundBuffer;
                HRESULT resultValue = loadFromFile(&directSoundBuffer, fileName, DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY, GUID_NULL);
                if (directSoundBuffer)
                {
                    resultValue = E_FAIL;
                    CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
                    if (directSound8Buffer)
                    {
                        CComPtr<Effect::Class> effect = new Effect::Class(directSound8Buffer);
                        if (effect)
                        {
                            resultValue = effect->QueryInterface(IID_PPV_ARGS(returnObject));
                        }
                    }
                }

                return resultValue;
            }

            STDMETHODIMP loadSound(Sound::Interface **returnObject, LPCWSTR fileName)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%s", fileName);

                REQUIRE_RETURN(directSound, E_FAIL);
                REQUIRE_RETURN(returnObject, E_INVALIDARG);

                CComPtr<IDirectSoundBuffer> directSoundBuffer;
                HRESULT resultValue = loadFromFile(&directSoundBuffer, fileName, DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY, GUID_NULL);
                if (directSoundBuffer)
                {
                    resultValue = E_FAIL;
                    CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
                    if (directSound8Buffer)
                    {
                        CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> directSound8Buffer3D(directSound8Buffer);
                        if (directSound8Buffer3D)
                        {
                            CComPtr<Sound::Class> sound = new Sound::Class(directSound8Buffer, directSound8Buffer3D);
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

        REGISTER_CLASS(System)
    }; // namespace Audio
}; // namespace Gek
