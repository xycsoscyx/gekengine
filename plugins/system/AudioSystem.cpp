#include "GEK\System\AudioInterface.h"
#include "GEK\Context\Common.h"
#include "GEK\Context\ContextUser.h"
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
        class Sample : public ContextUser
                     , public SampleInterface
        {
        protected:
            CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> buffer;

        public:
            Sample(IDirectSoundBuffer8 *buffer)
                : buffer(buffer)
            {
                setVolume(1.0f);
            }

            virtual ~Sample(void)
            {
            }

            BEGIN_INTERFACE_LIST(Sample)
                INTERFACE_LIST_ENTRY_COM(SampleInterface)
            END_INTERFACE_LIST_UNKNOWN

            // SampleInterface
            STDMETHODIMP_(LPVOID) getBuffer(void)
            {
                REQUIRE_RETURN(buffer, nullptr);
                return LPVOID(buffer);
            }

            STDMETHODIMP_(void) setFrequency(UINT32 frequency)
            {
                REQUIRE_VOID_RETURN(buffer);
                if (frequency == -1)
                {
                    buffer->SetFrequency(DSBFREQUENCY_ORIGINAL);
                }
                else
                {
                    buffer->SetFrequency(frequency);
                }
            }

            STDMETHODIMP_(void) setVolume(float volume)
            {
                REQUIRE_VOID_RETURN(buffer);
                buffer->SetVolume(UINT32((DSBVOLUME_MAX - DSBVOLUME_MIN) * volume) + DSBVOLUME_MIN);
            }
        };

        class Effect : public Sample
                     , public EffectInterface
        {
        public:
            Effect(IDirectSoundBuffer8 *buffer)
                : Sample(buffer)
            {
            }

            BEGIN_INTERFACE_LIST(Effect)
                INTERFACE_LIST_ENTRY_COM(EffectInterface)
            END_INTERFACE_LIST_BASE(Sample)

            // EffectInterface
            STDMETHODIMP_(void) setPan(float pan)
            {
                REQUIRE_VOID_RETURN(buffer);
                buffer->SetPan(UINT32((DSBPAN_RIGHT - DSBPAN_LEFT) * pan) + DSBPAN_LEFT);
            }

            STDMETHODIMP_(void) play(bool loop)
            {
                REQUIRE_VOID_RETURN(buffer);

                DWORD dwStatus = 0;
                if(SUCCEEDED(buffer->GetStatus(&dwStatus)) && !(dwStatus & DSBSTATUS_PLAYING))
                {
                    buffer->Play(0, 0, (loop ? DSBPLAY_LOOPING : 0));
                }
            }

            // SampleInterface
            STDMETHODIMP_(LPVOID) getBuffer(void)
            {
                return Sample::getBuffer();
            }

            STDMETHODIMP_(void) setFrequency(UINT32 frequency)
            {
                Sample::setFrequency(frequency);
            }

            STDMETHODIMP_(void) setVolume(float volume)
            {
                Sample::setVolume(volume);
            }
        };

        class Sound : public Sample
                    , public SoundInterface
        {
        private:
            CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> buffer3D;

        public:
            Sound(IDirectSoundBuffer8 *buffer, IDirectSound3DBuffer8 *buffer3D)
                : Sample(buffer)
                , buffer3D(buffer3D)
            {
            }

            BEGIN_INTERFACE_LIST(Sound)
                INTERFACE_LIST_ENTRY_COM(SoundInterface)
            END_INTERFACE_LIST_BASE(Sample)

            // SoundInterface
            STDMETHODIMP_(void) setDistance(float minimum, float maximum)
            {
                REQUIRE_VOID_RETURN(buffer3D);

                buffer3D->SetMinDistance(minimum, DS3D_DEFERRED);
                buffer3D->SetMaxDistance(maximum, DS3D_DEFERRED);
            }

            STDMETHODIMP_(void) play(const Math::Float3 &origin, bool loop)
            {
                REQUIRE_VOID_RETURN(buffer3D && buffer);

                buffer3D->SetPosition(origin.x, origin.y, origin.z, DS3D_DEFERRED);
    
                DWORD dwStatus = 0;
                if(SUCCEEDED(buffer->GetStatus(&dwStatus)) && !(dwStatus & DSBSTATUS_PLAYING))
                {
                    buffer->Play(0, 0, (loop ? DSBPLAY_LOOPING : 0));
                }
            }

            // SampleInterface
            STDMETHODIMP_(LPVOID) getBuffer(void)
            {
                return Sample::getBuffer();
            }

            STDMETHODIMP_(void) setFrequency(UINT32 frequency)
            {
                Sample::setFrequency(frequency);
            }

            STDMETHODIMP_(void) setVolume(float volume)
            {
                Sample::setVolume(volume);
            }
        };

        class System : public ContextUser
                     , public SystemInterface
        {
        private:
            CComQIPtr<IDirectSound8, &IID_IDirectSound8> directSound;
            CComQIPtr<IDirectSound3DListener8, &IID_IDirectSound3DListener8> listener;
            CComQIPtr<IDirectSoundBuffer, &IID_IDirectSoundBuffer> primaryBuffer;

        public:
            BEGIN_INTERFACE_LIST(System)
                INTERFACE_LIST_ENTRY_COM(SystemInterface)
            END_INTERFACE_LIST_UNKNOWN

            // SystemInterface
            STDMETHODIMP initialize(HWND window)
            {
                REQUIRE_RETURN(window, E_INVALIDARG);

                HRESULT returnValue = E_FAIL;
                if (directSound)
                {
                    returnValue = directSound->SetCooperativeLevel(window, DSSCL_PRIORITY);
                }
                else
                {
                    returnValue = DirectSoundCreate8(nullptr, &directSound, nullptr);
                    if (SUCCEEDED(returnValue))
                    {
                        returnValue = directSound->SetCooperativeLevel(window, DSSCL_PRIORITY);
                        if (SUCCEEDED(returnValue))
                        {
                            DSBUFFERDESC primaryBufferDescription = { 0 };
                            primaryBufferDescription.dwSize = sizeof(DSBUFFERDESC);
                            primaryBufferDescription.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;
                            returnValue = directSound->CreateSoundBuffer(&primaryBufferDescription, &primaryBuffer, nullptr);
                            if (SUCCEEDED(returnValue))
                            {
                                WAVEFORMATEX primaryBufferFormat;
                                ZeroMemory(&primaryBufferFormat, sizeof(WAVEFORMATEX));
                                primaryBufferFormat.wFormatTag = WAVE_FORMAT_PCM;
                                primaryBufferFormat.nChannels = 2;
                                primaryBufferFormat.wBitsPerSample = 8;
                                primaryBufferFormat.nSamplesPerSec = 48000;
                                primaryBufferFormat.nBlockAlign = (primaryBufferFormat.wBitsPerSample / 8 * primaryBufferFormat.nChannels);
                                primaryBufferFormat.nAvgBytesPerSec = (primaryBufferFormat.nSamplesPerSec * primaryBufferFormat.nBlockAlign);
                                returnValue = primaryBuffer->SetFormat(&primaryBufferFormat);
                                if (SUCCEEDED(returnValue))
                                {
                                    returnValue = E_FAIL;
                                    listener = primaryBuffer;
                                    if (listener)
                                    {
                                        returnValue = S_OK;
                                        setMasterVolume(1.0f);
                                        setDistanceFactor(1.0f);
                                        setDopplerFactor(0.0f);
                                        setRollOffFactor(1.0f);
                                    }
                                }
                            }
                        }
                    }
                }

                return returnValue;
            }

            STDMETHODIMP_(void) setMasterVolume(float volume)
            {
                REQUIRE_VOID_RETURN(primaryBuffer);
                primaryBuffer->SetVolume(UINT32((DSBVOLUME_MAX - DSBVOLUME_MIN) * volume) + DSBVOLUME_MIN);
            }

            STDMETHODIMP_(float) getMasterVolume(void)
            {
                REQUIRE_RETURN(primaryBuffer, 0);

                float volumePercent = 0.0f;

                long volumeNumber = 0;
                if (SUCCEEDED(primaryBuffer->GetVolume(&volumeNumber)))
                {
                    volumePercent = (float(volumeNumber - DSBVOLUME_MIN) / float(DSBVOLUME_MAX - DSBVOLUME_MIN));
                }

                return volumePercent;
            }

            STDMETHODIMP_(void) setListener(const Math::Float4x4 &matrix)
            {
                REQUIRE_VOID_RETURN(listener);
                listener->SetPosition(matrix.translation.x, matrix.translation.y, matrix.translation.z, DS3D_DEFERRED);
                listener->SetOrientation(matrix.rz.x, matrix.rz.y, matrix.rz.z, matrix.ry.x, matrix.ry.y, matrix.ry.z, DS3D_DEFERRED);
                listener->CommitDeferredSettings();
            }

            STDMETHODIMP_(void) setDistanceFactor(float factor)
            {
                REQUIRE_VOID_RETURN(listener);
                listener->SetDistanceFactor(factor, DS3D_DEFERRED);
            }

            STDMETHODIMP_(void) setDopplerFactor(float factor)
            {
                REQUIRE_VOID_RETURN(listener);
                listener->SetDopplerFactor(factor, DS3D_DEFERRED);
            }

            STDMETHODIMP_(void) setRollOffFactor(float factor)
            {
                REQUIRE_VOID_RETURN(listener);
                listener->SetRolloffFactor(factor, DS3D_DEFERRED);
            }

            STDMETHODIMP copyEffect(EffectInterface *source, EffectInterface **instance)
            {
                REQUIRE_RETURN(directSound, E_FAIL);
                REQUIRE_RETURN(source && instance, E_INVALIDARG);

                HRESULT returnValue = E_FAIL;
                CComQIPtr<SampleInterface> sourceSample(source);
                if (sourceSample)
                {
                    CComPtr<IDirectSoundBuffer> duplicateBuffer;
                    returnValue = directSound->DuplicateSoundBuffer((LPDIRECTSOUNDBUFFER)sourceSample->getBuffer(), &duplicateBuffer);
                    if (duplicateBuffer)
                    {
                        returnValue = E_FAIL;
                        CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> duplicateBuffer8(duplicateBuffer);
                        if (duplicateBuffer8)
                        {
                            returnValue = E_OUTOFMEMORY;
                            CComPtr<Effect> effect = new Effect(duplicateBuffer8);
                            if (effect)
                            {
                                returnValue = effect->QueryInterface(IID_PPV_ARGS(instance));
                            }
                        }
                    }
                }

                return returnValue;
            }

            STDMETHODIMP copySound(SoundInterface *source, SoundInterface **instance)
            {
                REQUIRE_RETURN(directSound, E_FAIL);
                REQUIRE_RETURN(source && instance, E_INVALIDARG);

                HRESULT returnValue = E_FAIL;
                CComQIPtr<SampleInterface> sourceSample(source);
                if (sourceSample)
                {
                    CComPtr<IDirectSoundBuffer> duplicateBuffer;
                    returnValue = directSound->DuplicateSoundBuffer(LPDIRECTSOUNDBUFFER(sourceSample->getBuffer()), &duplicateBuffer);
                    if (duplicateBuffer)
                    {
                        returnValue = E_FAIL;
                        CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> duplicateBuffer8(duplicateBuffer);
                        if (duplicateBuffer8)
                        {
                            CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> duplicateBuffer3D(duplicateBuffer8);
                            if (duplicateBuffer3D)
                            {
                                returnValue = E_OUTOFMEMORY;
                                CComPtr<Sound> sound = new Sound(duplicateBuffer8, duplicateBuffer3D);
                                if (sound)
                                {
                                    returnValue = sound->QueryInterface(IID_PPV_ARGS(instance));
                                }
                            }
                        }
                    }
                }

                return returnValue;
            }

            HRESULT loadFromFile(LPCWSTR fileName, DWORD nFlags, GUID nAlgorithm, IDirectSoundBuffer **ppBuffer)
            {
                REQUIRE_RETURN(directSound, E_FAIL);
                REQUIRE_RETURN(ppBuffer, E_INVALIDARG);

                std::vector<UINT8> fileData;
                HRESULT returnValue = Gek::FileSystem::load(fileName, fileData);
                if (SUCCEEDED(returnValue))
                {
                    returnValue = E_FAIL;
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
                            bufferDescription.guid3DAlgorithm = nAlgorithm;
                            bufferDescription.dwFlags = nFlags;

                            CComPtr<IDirectSoundBuffer> buffer;
                            returnValue = directSound->CreateSoundBuffer(&bufferDescription, &buffer, nullptr);
                            if (buffer)
                            {
                                void *sampleData = nullptr;
                                if (SUCCEEDED(buffer->Lock(0, sampleLength, &sampleData, &sampleLength, 0, 0, DSBLOCK_ENTIREBUFFER)))
                                {
                                    audiereSample->read((sampleLength / bufferFormat.nBlockAlign), sampleData);
                                    buffer->Unlock(sampleData, sampleLength, 0, 0);
                                }

                                returnValue = buffer->QueryInterface(IID_IDirectSoundBuffer, (LPVOID FAR *)ppBuffer);
                            }
                        }
                    }
                }

                return returnValue;
            }

            STDMETHODIMP loadEffect(LPCWSTR fileName, EffectInterface **instance)
            {
                REQUIRE_RETURN(directSound, E_FAIL);
                REQUIRE_RETURN(instance, E_INVALIDARG);

                CComPtr<IDirectSoundBuffer> buffer;
                HRESULT returnValue = loadFromFile(fileName, DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY, GUID_NULL, &buffer);
                if (buffer)
                {
                    returnValue = E_FAIL;
                    CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> buffer8(buffer);
                    if (buffer8)
                    {
                        CComPtr<Effect> effect = new Effect(buffer8);
                        if (effect)
                        {
                            returnValue = effect->QueryInterface(IID_PPV_ARGS(instance));
                        }
                    }
                }

                return returnValue;
            }

            STDMETHODIMP loadSound(LPCWSTR fileName, SoundInterface **instance)
            {
                REQUIRE_RETURN(directSound, E_FAIL);
                REQUIRE_RETURN(instance, E_INVALIDARG);

                CComPtr<IDirectSoundBuffer> buffer;
                HRESULT returnValue = loadFromFile(fileName, DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY, GUID_NULL, &buffer);
                if (buffer)
                {
                    returnValue = E_FAIL;
                    CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> buffer8(buffer);
                    if (buffer8)
                    {
                        CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> buffer3D(buffer8);
                        if (buffer3D)
                        {
                            CComPtr<Sound> sound = new Sound(buffer8, buffer3D);
                            if (sound)
                            {
                                returnValue = sound->QueryInterface(IID_PPV_ARGS(instance));
                            }
                        }
                    }
                }

                return returnValue;
            }
        };

        REGISTER_CLASS(System)
    }; // namespace Audio
}; // namespace Gek
