#include "GEK\Utility\Trace.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\AudioDevice.h"
#include <atlbase.h>
#include <mmsystem.h>
#define DIRECTSOUND_VERSION 0x0800
#include <dsound.h>
#include "audiere.h"

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "audiere.lib")

namespace Gek
{
    namespace DirectSound8
    {
        template <typename CLASS>
        class Buffer
            : public CLASS
        {
        protected:
            CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSoundBuffer;

        public:
            Buffer(IDirectSoundBuffer8 *directSoundBuffer)
                : directSoundBuffer(directSoundBuffer)
            {
                setVolume(1.0f);
            }

            virtual ~Buffer(void)
            {
            }

            // AudioSample
            void * const getBuffer(void)
            {
                GEK_REQUIRE(directSoundBuffer);

                return static_cast<void *>(directSoundBuffer.p);
            }

            void setFrequency(uint32_t frequency)
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

            void setVolume(float volume)
            {
                GEK_REQUIRE(directSoundBuffer);

                directSoundBuffer->SetVolume(uint32_t((DSBVOLUME_MAX - DSBVOLUME_MIN) * volume) + DSBVOLUME_MIN);
            }
        };

        class Effect
            : public Buffer<Audio::Effect>
        {
        public:
            Effect(IDirectSoundBuffer8 *directSoundBuffer)
                : Buffer(directSoundBuffer)
            {
            }

            // Audio::Effect
            void setPan(float pan)
            {
                GEK_REQUIRE(directSoundBuffer);
                directSoundBuffer->SetPan(uint32_t((DSBPAN_RIGHT - DSBPAN_LEFT) * pan) + DSBPAN_LEFT);
            }

            void play(bool loop)
            {
                GEK_REQUIRE(directSoundBuffer);

                DWORD dwStatus = 0;
                if (SUCCEEDED(directSoundBuffer->GetStatus(&dwStatus)) && !(dwStatus & DSBSTATUS_PLAYING))
                {
                    directSoundBuffer->Play(0, 0, (loop ? DSBPLAY_LOOPING : 0));
                }
            }
        };

        class Sound
            : public Buffer<Audio::Sound>
        {
        private:
            CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> directSound8Buffer3D;

        public:
            Sound(IDirectSoundBuffer8 *directSoundBuffer, IDirectSound3DBuffer8 *directSound8Buffer3D)
                : Buffer(directSoundBuffer)
                , directSound8Buffer3D(directSound8Buffer3D)
            {
            }

            // Audio::Sound
            void setDistance(float minimum, float maximum)
            {
                GEK_REQUIRE(directSound8Buffer3D);

                directSound8Buffer3D->SetMinDistance(minimum, DS3D_DEFERRED);
                directSound8Buffer3D->SetMaxDistance(maximum, DS3D_DEFERRED);
            }

            void play(const Math::Float3 &origin, bool loop)
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

        GEK_CONTEXT_USER(Device, HWND, const wchar_t *)
            , public Audio::Device
        {
        private:
            CComQIPtr<IDirectSound8, &IID_IDirectSound8> directSound;
            CComQIPtr<IDirectSound3DListener8, &IID_IDirectSound3DListener8> directSoundListener;
            CComQIPtr<IDirectSoundBuffer, &IID_IDirectSoundBuffer> primarySoundBuffer;

        public:
            Device(Context *context, HWND window, const wchar_t *device)
                : ContextRegistration(context)
            {
                GEK_REQUIRE(window);

                GUID deviceGUID = DSDEVID_DefaultPlayback;
                if (device)
                {
                    struct EnumData
                    {
                        String device;
                        GUID *deviceGUID;
                    } enumerationData = { device, &deviceGUID };
                    DirectSoundEnumerateW([](LPGUID deviceGUID, LPCWSTR description, LPCWSTR module, LPVOID context) -> BOOL
                    {
                        EnumData *enumerationData = static_cast<EnumData *>(context);
                        if (enumerationData->device.compareNoCase(description) == 0)
                        {
                            enumerationData->deviceGUID = deviceGUID;
                            return FALSE;
                        }

                        return TRUE;
                    }, &enumerationData);
                }

                HRESULT resultValue = DirectSoundCreate8(&deviceGUID, &directSound, nullptr);
                GEK_CHECK_CONDITION(FAILED(resultValue), Audio::Exception, "Unable to initialize DirectSound8 (error %v)", resultValue);

                resultValue = directSound->SetCooperativeLevel(window, DSSCL_PRIORITY);
                GEK_CHECK_CONDITION(FAILED(resultValue), Audio::Exception, "Unable to set cooperative level (error %v)", resultValue);

                DSBUFFERDESC primaryBufferDescription = { 0 };
                primaryBufferDescription.dwSize = sizeof(DSBUFFERDESC);
                primaryBufferDescription.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;
                resultValue = directSound->CreateSoundBuffer(&primaryBufferDescription, &primarySoundBuffer, nullptr);
                GEK_CHECK_CONDITION(FAILED(resultValue), Audio::Exception, "Unable to create primary sound buffer (error %v)", resultValue);

                WAVEFORMATEX primaryBufferFormat;
                ZeroMemory(&primaryBufferFormat, sizeof(WAVEFORMATEX));
                primaryBufferFormat.wFormatTag = WAVE_FORMAT_PCM;
                primaryBufferFormat.nChannels = 2;
                primaryBufferFormat.wBitsPerSample = 8;
                primaryBufferFormat.nSamplesPerSec = 48000;
                primaryBufferFormat.nBlockAlign = (primaryBufferFormat.wBitsPerSample / 8 * primaryBufferFormat.nChannels);
                primaryBufferFormat.nAvgBytesPerSec = (primaryBufferFormat.nSamplesPerSec * primaryBufferFormat.nBlockAlign);
                resultValue = primarySoundBuffer->SetFormat(&primaryBufferFormat);
                GEK_CHECK_CONDITION(FAILED(resultValue), Audio::Exception, "Unable to set primary sound buffer format (error %v)", resultValue);

                directSoundListener = primarySoundBuffer;
                GEK_CHECK_CONDITION(!directSoundListener, Audio::Exception, "Unable to query for primary sound listener (error %v)", resultValue);

                setVolume(1.0f);
                setDistanceFactor(1.0f);
                setDopplerFactor(0.0f);
                setRollOffFactor(1.0f);
            }

            // Audio::Device
            void setVolume(float volume)
            {
                GEK_REQUIRE(primarySoundBuffer);

                primarySoundBuffer->SetVolume(uint32_t((DSBVOLUME_MAX - DSBVOLUME_MIN) * volume) + DSBVOLUME_MIN);
            }

            float getVolume(void)
            {
                GEK_REQUIRE(primarySoundBuffer);

                long volumeNumber = 0;
                if (FAILED(primarySoundBuffer->GetVolume(&volumeNumber)))
                {
                    volumeNumber = 0;
                }

                return (float(volumeNumber - DSBVOLUME_MIN) / float(DSBVOLUME_MAX - DSBVOLUME_MIN));
            }

            void setListener(const Math::Float4x4 &matrix)
            {
                GEK_REQUIRE(directSoundListener);

                directSoundListener->SetPosition(matrix.translation.x, matrix.translation.y, matrix.translation.z, DS3D_DEFERRED);
                directSoundListener->SetOrientation(matrix.rz.x, matrix.rz.y, matrix.rz.z, matrix.ry.x, matrix.ry.y, matrix.ry.z, DS3D_DEFERRED);
                directSoundListener->CommitDeferredSettings();
            }

            void setDistanceFactor(float factor)
            {
                GEK_REQUIRE(directSoundListener);

                directSoundListener->SetDistanceFactor(factor, DS3D_DEFERRED);
            }

            void setDopplerFactor(float factor)
            {
                GEK_REQUIRE(directSoundListener);

                directSoundListener->SetDopplerFactor(factor, DS3D_DEFERRED);
            }

            void setRollOffFactor(float factor)
            {
                GEK_REQUIRE(directSoundListener);

                directSoundListener->SetRolloffFactor(factor, DS3D_DEFERRED);
            }

            CComPtr<IDirectSoundBuffer> loadFromFile(const wchar_t *fileName, uint32_t flags, GUID soundAlgorithm)
            {
                GEK_REQUIRE(directSound);
                GEK_REQUIRE(fileName);

                std::vector<uint8_t> fileData;
                FileSystem::load(fileName, fileData);

                audiere::RefPtr<audiere::File> audiereFile(audiere::CreateMemoryFile(fileData.data(), fileData.size()));
                GEK_CHECK_CONDITION(!audiereFile, Audio::Exception, "Unable to create memory mapping of audio file");

                audiere::RefPtr<audiere::SampleSource> audiereSample(audiere::OpenSampleSource(audiereFile));
                GEK_CHECK_CONDITION(!audiereSample, Audio::Exception, "YUnable to open audio sample source data");

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
                HRESULT resultValue = directSound->CreateSoundBuffer(&bufferDescription, &directSoundBuffer, nullptr);
                GEK_CHECK_CONDITION(!directSoundBuffer, Audio::Exception, "Unable create sound buffer for audio file (error %v)", resultValue);

                void *sampleData = nullptr;
                resultValue = directSoundBuffer->Lock(0, sampleLength, &sampleData, &sampleLength, 0, 0, DSBLOCK_ENTIREBUFFER);
                GEK_CHECK_CONDITION(!sampleData, Audio::Exception, "Unable to lock sound buffer (error %v)", resultValue);

                audiereSample->read((sampleLength / bufferFormat.nBlockAlign), sampleData);
                directSoundBuffer->Unlock(sampleData, sampleLength, 0, 0);

                return directSoundBuffer;
            }

            Audio::EffectPtr loadEffect(const wchar_t *fileName)
            {
                GEK_REQUIRE(directSound);
                GEK_REQUIRE(fileName);

                CComPtr<IDirectSoundBuffer> directSoundBuffer(loadFromFile(fileName, DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY, GUID_NULL));

                CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
                GEK_CHECK_CONDITION(!directSound8Buffer, Audio::Exception, "Unable to query for advanced sound buffer");

                return makeShared<Effect>(directSound8Buffer.p);
            }

            Audio::SoundPtr loadSound(const wchar_t *fileName)
            {
                GEK_REQUIRE(directSound);
                GEK_REQUIRE(fileName);

                CComPtr<IDirectSoundBuffer> directSoundBuffer(loadFromFile(fileName, DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY, GUID_NULL));

                CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
                GEK_CHECK_CONDITION(!directSound8Buffer, Audio::Exception, "Unable to query for advanced sound buffer");

                CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> directSound8Buffer3D(directSound8Buffer);
                GEK_CHECK_CONDITION(!directSound8Buffer3D, Audio::Exception, "Unable to query for 3D sound buffer");

                return makeShared<Sound>(directSound8Buffer.p, directSound8Buffer3D.p);
            }

            Audio::EffectPtr copyEffect(Audio::Effect *source)
            {
                GEK_REQUIRE(directSound);
                GEK_REQUIRE(source);

                CComPtr<IDirectSoundBuffer> directSoundBuffer;
                HRESULT resultValue = directSound->DuplicateSoundBuffer(static_cast<IDirectSoundBuffer *>(source->getBuffer()), &directSoundBuffer);
                GEK_CHECK_CONDITION(!directSoundBuffer, Audio::Exception, "Unable to duplicate sound buffer (error %v)", resultValue);

                CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
                GEK_CHECK_CONDITION(!directSound8Buffer, Audio::Exception, "Unable to query for advanced sound buffer");

                return makeShared<Effect>(directSound8Buffer.p);
            }

            Audio::SoundPtr copySound(Audio::Sound *source)
            {
                GEK_REQUIRE(directSound);
                GEK_REQUIRE(source);

                CComPtr<IDirectSoundBuffer> directSoundBuffer;
                HRESULT resultValue = directSound->DuplicateSoundBuffer(static_cast<IDirectSoundBuffer *>(source->getBuffer()), &directSoundBuffer);
                GEK_CHECK_CONDITION(!directSoundBuffer, Audio::Exception, "Unable to duplicate sound buffer (error %v)", resultValue);

                CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
                GEK_CHECK_CONDITION(!directSound8Buffer, Audio::Exception, "Unable to query for advanced sound buffer");

                CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> directSound8Buffer3D(directSound8Buffer);
                GEK_CHECK_CONDITION(!directSound8Buffer3D, Audio::Exception, "Unable to query for 3D sound buffer");

                return makeShared<Sound>(directSound8Buffer.p, directSound8Buffer3D.p);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Device);
    }; // namespace DirectSound8
}; // namespace Gek